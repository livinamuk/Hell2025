#version 460
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_ray_query : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "../common/constants.glsl"
#include "../common/ddgi.glsl"
#include "../common/intersect.glsl"
#include "../common/lighting.glsl"
#include "../common/normal_encoding.glsl"
#include "../common/util.glsl"
#include "../common/reconstruction.glsl"
#include "../common/viewport.glsl"
#include "../common/flags.glsl"
#include "../common/Vulkan/VK_binding_indices.glsl"
#include "../common/Vulkan/VK_push_constants.glsl"

layout(set = 0, binding = DESC_IDX_SAMPLERS) uniform sampler samplers[];
layout(set = 0, binding = DESC_IDX_TEXTURES) uniform texture2D textures[];
layout(set = 0, binding = DESC_IDX_TEXTURE_SAMPLERS) uniform sampler textureSamplers[];
layout(set = 0, binding = DESC_IDX_TEXTURE_ARRAYS_RG16F) uniform texture2DArray textureArraysRG16F[];
layout(set = 0, binding = DESC_IDX_TEXTURE_ARRAYS_RGBA16F) uniform texture2DArray textureArraysRGBA16F[];
layout(set = 1, binding = 0) uniform accelerationStructureEXT u_RayQueryAccelerationStructure;

#include "../common/Vulkan/VK_ray_query_scene.glsl"
#include "VK_point_shadows.glsl"

#include "VK_indirect_specular_amd_apply.glsl"

layout(early_fragment_tests) in;
layout(location = 0) out vec4 out_color;

#define LIGHT_COUNT 9             // TODO "tile based deferred" me the fuck outta here
#define DEBUG_REFLECTION_RAY_CLASSIFICATION 0
#define DEBUG_REFLECTED_HIT_LIGHT_COUNT 1
#define DEBUG_COUNT_LIGHTS_AT_PRIMARY_POINT 1
#define DEBUG_REFLECTED_TILE_LIGHT_COUNT 0
#define DEBUG_REFLECTED_TILE_BOUNDS 0
#define DEBUG_REJECT_RAYS_BELOW_UNPERTURBED_NORMAL 1

const float AMD_REFLECTION_MISS_DISTANCE = 100.0;
const float AMD_REFLECTION_RAY_T_MIN = 3.0e-3;
const float AMD_REFLECTION_RAY_ORIGIN_BIAS = 3.0e-3;
const float AMD_REFLECTION_MAX_DISTANCE = 8.0;
const float AMD_REFLECTION_RAY_LENGTH_EXP_FACTOR = 1.386; // Casts 5m rays for roughness 0.5
const float AMD_GOLDEN_RATIO = 1.61803398875;
const uint AMD_BLUE_NOISE_TILE_SIZE = 128u;
const uint AMD_BLUE_NOISE_SAMPLE_COUNT = 32u;
const float DDGI_REFLECTION_ENCODING_GAMMA = 5.0;
const float DDGI_REFLECTION_TWO_PI = 6.2831853071795864;

struct Surface {
    vec3 worldPos;
    vec3 normal;
    vec3 linearBaseColor;
    float roughness;
    float metallic;
};

layout(buffer_reference, scalar) readonly buffer ProbeStatesBuffer {
    ProbeState probeStates[];
};

layout(buffer_reference, scalar) readonly buffer DDGIReflectionVolumeDataBuffer {
    uint64_t probeStatesDeviceAddress;
    uint volumeCount;
    uint padding0;
    DDGIReflectionVolume volumes[];
};

layout(push_constant, scalar) uniform PushConstants {
    PushConstantsIndirectSpecularAMDInput data;
} pc;

vec2 GetBlueNoiseSample(ivec2 pixelCoord, uint sampleIndex) {
    uint textureIndex = uint(pc.data.blueNoiseTextureIndex);
    uvec2 pixel = uvec2(pixelCoord);
    uint atlasFrame = sampleIndex % AMD_BLUE_NOISE_SAMPLE_COUNT;
    ivec2 atlasCoord = ivec2(pixel.x & (AMD_BLUE_NOISE_TILE_SIZE - 1u), (pixel.y & (AMD_BLUE_NOISE_TILE_SIZE - 1u)) + atlasFrame * AMD_BLUE_NOISE_TILE_SIZE);
    vec2 xi = texelFetch(sampler2D(textures[nonuniformEXT(textureIndex)], textureSamplers[nonuniformEXT(textureIndex)]), atlasCoord, 0).rg;

    // Match AMD's SampleRandomVector2DBaked spatial decorrelation when the reflection target is larger than the native 128x128 blue-noise tile.
    return fract(xi + vec2(float((pixel.x / AMD_BLUE_NOISE_TILE_SIZE) & 0xffu), float((pixel.y / AMD_BLUE_NOISE_TILE_SIZE) & 0xffu)) * AMD_GOLDEN_RATIO);
}

vec3 EvaluatePointLight(vec3 lightPos, vec3 lightColor, float lightRadius, float lightStrength, vec3 surfacePos, vec3 normal, vec3 baseColor, float roughness, float metallic, vec3 viewPos) {
    // Build the light and view directions
    vec3 toLight = lightPos - surfacePos;
    float dist = length(toLight);
    vec3 lightDir = toLight / max(dist, 0.0001);
    vec3 viewDir = normalize(viewPos - surfacePos);
    float attenuation = smoothstep(lightRadius, 0.0, dist) * lightStrength;
    float ndotl = max(dot(normal, lightDir), 0.0);

    // Reject surfaces this light cannot reach
    if (ndotl <= 0.0 || attenuation <= 0.0) {
        return vec3(0.0);
    }

    // Match the wrapped diffuse response used by the main direct-light helper
    float wrap = 0.125;
    ndotl = clamp((ndotl + wrap) / (1.0 + wrap), 0.0, 1.0);

    vec3 brdf = microfacetBRDF(lightDir, viewDir, normal, baseColor, metallic, 1.0, roughness);

    return brdf * ndotl * attenuation * clamp(lightColor, 0.0, 1.0);
}

Surface SurfaceFromRayHit(RayQueryContext context, RayQueryHit rayhit, float materialLod, float normalLod) {
    RayQueryMaterialSample materialSample = EvaluateRayHitMaterial(context, rayhit, materialLod, normalLod);

    Surface surface;
    surface.worldPos = rayhit.hitPos;
    surface.normal = materialSample.normal;
    surface.linearBaseColor = materialSample.linearBaseColor;
    surface.roughness = materialSample.roughness;
    surface.metallic = materialSample.metallic;

    return surface;
}

bool IsFinite(vec3 value) {
    return !any(isnan(value)) && !any(isinf(value));
}

bool IsFinite(vec4 value) {
    return !any(isnan(value)) && !any(isinf(value));
}

bool IsFinite(float value) {
    return !isnan(value) && !isinf(value);
}

vec2 EncodeAMDDeferredHitNormal(vec3 normal) {
    normal.xy /= dot(vec3(1.0), abs(normal));

    if (normal.z <= 0.0) {
        vec2 signNotZero = mix(vec2(-1.0), vec2(1.0), greaterThanEqual(normal.xy, vec2(0.0)));
        normal.xy = (vec2(1.0) - abs(normal.yx)) * signNotZero;
    }

    return normal.xy * 0.5 + 0.5;
}

vec3 DecodeAMDDeferredHitNormal(vec2 encoded) {
    encoded = 2.0 * (encoded - 0.5);
    vec3 normal = vec3(encoded, 1.0 - dot(vec2(1.0), abs(encoded)));
    float fold = max(-normal.z, 0.0);
    normal.xy += mix(vec2(fold), vec2(-fold), greaterThanEqual(normal.xy, vec2(0.0)));

    return normalize(normal);
}

void BuildAMDOrthonormalBasis(vec3 normal, out vec3 tangent, out vec3 bitangent) {
    // AMD's CreateTBN
    if (abs(normal.z) > 0.0) {
        float k = sqrt(normal.y * normal.y + normal.z * normal.z);
        tangent = vec3(0.0, -normal.z / k, normal.y / k);
    }
    else {
        float k = sqrt(normal.x * normal.x + normal.y * normal.y);
        tangent = vec3(normal.y / k, -normal.x / k, 0.0);
    }
    bitangent = cross(normal, tangent);
}

vec3 SampleAMDGGXVNDF(vec3 viewDirection, float alphaX, float alphaY, vec2 randomSample) {
    vec3 stretchedView = normalize(vec3(alphaX * viewDirection.x, alphaY * viewDirection.y, viewDirection.z));
    float lensq = stretchedView.x * stretchedView.x + stretchedView.y * stretchedView.y;

    vec3 tangent1 = lensq > 0.0
        ? vec3(-stretchedView.y, stretchedView.x, 0.0) * inversesqrt(lensq)
        : vec3(1.0, 0.0, 0.0);

    vec3 tangent2 = cross(stretchedView, tangent1);

    float radius = sqrt(randomSample.x);
    float phi = 2.0 * PI * randomSample.y;
    float t1 = radius * cos(phi);
    float t2 = radius * sin(phi);
    float s = 0.5 * (1.0 + stretchedView.z);
    t2 = (1.0 - s) * sqrt(1.0 - t1 * t1) + s * t2;

    vec3 hemisphereNormal = t1 * tangent1 + t2 * tangent2 + sqrt(max(0.0, 1.0 - t1 * t1 - t2 * t2)) * stretchedView;

    return normalize(vec3(
        alphaX * hemisphereNormal.x,
        alphaY * hemisphereNormal.y,
        max(0.0, hemisphereNormal.z)));
}

vec3 SampleAMDReflectionVector(vec3 viewDirToCamera, vec3 normal, float roughness, vec2 randomSample) {
    if (roughness < 0.001) {
        return normalize(reflect(-viewDirToCamera, normal));
    }

    vec3 tangent;
    vec3 bitangent;
    BuildAMDOrthonormalBasis(normal, tangent, bitangent);
    mat3 tangentToWorld = mat3(tangent, bitangent, normal);
    vec3 localView = transpose(tangentToWorld) * viewDirToCamera;
    vec3 localHalfVector = SampleAMDGGXVNDF(localView, roughness, roughness, randomSample);
    vec3 localReflection = reflect(-localView, localHalfVector);
    return normalize(tangentToWorld * localReflection);
}

vec3 SampleDDGIReflectionVolumeIrradiance(DDGIReflectionVolume reflectionVolume, uint64_t probeStatesDeviceAddress, vec3 worldPosition, vec3 normal, vec3 viewPosition) {
    DDGIVolume volume = reflectionVolume.volume;
    ProbeStatesBuffer probeStates = ProbeStatesBuffer(probeStatesDeviceAddress);
    vec3 irradiance = vec3(0.0);
    float accumulatedWeights = 0.0;

    vec3 surfaceBias = DDGIGetSurfaceBias(worldPosition, normal, viewPosition);
    vec3 biasedWorldPosition = worldPosition + surfaceBias;

    ivec3 baseProbeCoords = DDGIGetBaseProbeGridCoords(biasedWorldPosition, volume);
    vec3 baseProbeWorldPosition = DDGIGetProbeBaseWorldPosition(baseProbeCoords, volume);
    vec3 gridSpaceDistance = biasedWorldPosition - baseProbeWorldPosition;
    vec3 alpha = clamp(gridSpaceDistance / volume.probeSpacing, vec3(0.0), vec3(1.0));

    for (int probeIndex = 0; probeIndex < 8; probeIndex++) {
        ivec3 adjacentProbeOffset = ivec3(probeIndex, probeIndex >> 1, probeIndex >> 2) & ivec3(1);
        ivec3 adjacentProbeCoords = clamp(baseProbeCoords + adjacentProbeOffset, ivec3(0), volume.probeCounts - ivec3(1));
        int adjacentProbeIndex = DDGIGetProbeIndex(adjacentProbeCoords, volume.probeCounts);
        uint globalProbeIndex = volume.probeOffset + uint(adjacentProbeIndex);

        if (!probeStates.probeStates[globalProbeIndex].isActive) continue;

        vec3 adjacentProbeWorldPosition = DDGIGetProbeWorldPosition(adjacentProbeCoords,volume,probeStates.probeStates[globalProbeIndex]);
        vec3 worldPosToAdjProbe = normalize(adjacentProbeWorldPosition - worldPosition);
        vec3 biasedPosToAdjProbe = normalize(adjacentProbeWorldPosition - biasedWorldPosition);
        float biasedPosToAdjProbeDist = length(adjacentProbeWorldPosition - biasedWorldPosition);

        vec3 trilinear = max(vec3(0.001), mix(1.0 - alpha, alpha, vec3(adjacentProbeOffset)));
        float trilinearWeight = trilinear.x * trilinear.y * trilinear.z;

        float weight = 1.0;
        float wrapShading = (dot(worldPosToAdjProbe, normal) + 1.0) * 0.5;
        weight *= (wrapShading * wrapShading) + 0.2;

        vec2 octantCoordsDist = DDGIGetOctahedralCoordinates(-biasedPosToAdjProbe);
        vec3 probeTextureUVDist = DDGIGetProbeUV(adjacentProbeIndex, octantCoordsDist, 14, volume);
        uint probeAtlasImageIndex = reflectionVolume.probeAtlasImageIndex;
        vec2 filteredDistance = 2.0 * texture(sampler2DArray(textureArraysRG16F[nonuniformEXT(probeAtlasImageIndex)], samplers[VULKAN_SAMPLER_IDX_LINEAR]), probeTextureUVDist).rg;

        float variance = abs((filteredDistance.x * filteredDistance.x) - filteredDistance.y);
        float chebyshevWeight = 1.0;

        if (biasedPosToAdjProbeDist > filteredDistance.x) {
            float v = biasedPosToAdjProbeDist - filteredDistance.x;
            chebyshevWeight = variance / (variance + (v * v) + 1e-5);
            chebyshevWeight = max(chebyshevWeight * chebyshevWeight * chebyshevWeight, 0.0);
        }

        weight *= max(0.05, chebyshevWeight);
        weight = max(0.000001, weight);

        const float crushThreshold = 0.2;
        if (weight < crushThreshold) {
            weight *= (weight * weight) * (1.0 / (crushThreshold * crushThreshold));
        }

        weight *= trilinearWeight;
        if (weight < 0.001) continue;

        vec2 octantCoordsIrrad = DDGIGetOctahedralCoordinates(normal);
        vec3 probeTextureUVIrrad = DDGIGetProbeUV(adjacentProbeIndex, octantCoordsIrrad, 6, volume);
        vec3 probeIrradiance = texture(sampler2DArray(textureArraysRGBA16F[nonuniformEXT(probeAtlasImageIndex)], samplers[VULKAN_SAMPLER_IDX_LINEAR]), probeTextureUVIrrad).rgb; probeIrradiance = pow(max(vec3(0.0), probeIrradiance),vec3(DDGI_REFLECTION_ENCODING_GAMMA * 0.5));

        irradiance += weight * probeIrradiance;
        accumulatedWeights += weight;
    }

    if (accumulatedWeights == 0.0) return vec3(0.0);

    irradiance /= accumulatedWeights;
    irradiance *= irradiance;
    irradiance *= DDGI_REFLECTION_TWO_PI;
    return irradiance;
}

vec3 SampleDDGIReflectionIrradiance(vec3 worldPosition, vec3 normal, vec3 viewPosition) {
    DDGIReflectionVolumeDataBuffer reflectionVolumeData = DDGIReflectionVolumeDataBuffer(pc.data.ddgiReflectionVolumeDataDeviceAddress);
    vec3 irradiance = vec3(0.0);

    // DDGIIrradianceTexturePass processes volumes in this same order
    // Keeping the last containing volume matches its overlap behavior

    for (uint volumeIndex = 0u; volumeIndex < reflectionVolumeData.volumeCount; volumeIndex++) {
        DDGIReflectionVolume reflectionVolume = reflectionVolumeData.volumes[volumeIndex];
        DDGIVolume volume = reflectionVolume.volume;

        if (any(lessThan(worldPosition, volume.worldBoundsMin)) || any(greaterThan(worldPosition, volume.worldBoundsMax))) {
            continue;
        }

        irradiance = SampleDDGIReflectionVolumeIrradiance(reflectionVolume, reflectionVolumeData.probeStatesDeviceAddress, worldPosition, normal, viewPosition);
    }

    return irradiance;
}

vec3 GetIndirectSpecularSample(Surface surface, vec3 viewDirToCamera, vec3 cameraWorldPos, mat4 viewMatrix, mat4 inverseViewMatrix, vec3 reflectionOrigin, vec2 randomSample, float roughnessDampening, bool useDDGIReflections, out float reflectedHitDistance) {

    reflectedHitDistance = -1.0;

    // AMD samples the GGX VNDF in view space, then transforms the sampled reflection direction back to world space for ray traversal.
    vec3 viewSpaceViewDirToCamera = normalize(mat3(viewMatrix) * viewDirToCamera);
    vec3 viewSpaceNormal = normalize(mat3(viewMatrix) * surface.normal);
    vec3 viewSpaceReflectionRayDir = SampleAMDReflectionVector(viewSpaceViewDirToCamera, viewSpaceNormal, surface.roughness * roughnessDampening, randomSample);
    vec3 reflectionRayDir = normalize(mat3(inverseViewMatrix) * viewSpaceReflectionRayDir);
    //float reflectionNoL = dot(surface.normal, reflectionRayDir);

    if (!IsFinite(reflectionRayDir)) { //if (!IsFinite(reflectionRayDir) || !IsFinite(reflectionNoL)) {
        return vec3(0.0);
    }

    // Keep AMD miss samples valid but write black instead of skybox value, that's what AMD do, but that won't work inside your houses m8
    vec3 incidentRadiance = vec3(0.0);
    reflectedHitDistance = AMD_REFLECTION_MISS_DISTANCE;

    float rayMaxDistance = AMD_REFLECTION_MAX_DISTANCE * exp(-surface.roughness * AMD_REFLECTION_RAY_LENGTH_EXP_FACTOR);

    RayQueryContext rayQueryContext = CreateRayQueryContext(pc.data.rayQueryBLASDataDeviceAddress, pc.data.rayQuerySceneRenderItemIndicesDeviceAddress, uint64_t(pc.data.frame.sceneRenderItemBuffer), uint64_t(pc.data.frame.materialBuffer));
    RayQueryHit reflectedHit = TraceClosestReflectionHit(u_RayQueryAccelerationStructure, rayQueryContext, reflectionOrigin, reflectionRayDir, AMD_REFLECTION_RAY_T_MIN, rayMaxDistance);

    if (reflectedHit.found) {
        // AMD traces from the biased origin, but reconstructs the position used
        // for deferred hit shading from the unbiased primary surface.
        reflectedHit.hitPos = surface.worldPos + reflectionRayDir * reflectedHit.rayT;
        reflectedHitDistance = reflectedHit.rayT;
        Surface reflectedSurface = SurfaceFromRayHit(rayQueryContext, reflectedHit, 0.0, 2.0);

        if (useDDGIReflections) {
            vec3 probeIrradiance = SampleDDGIReflectionIrradiance(reflectedSurface.worldPos, reflectedSurface.normal, surface.worldPos) * pc.data.frame.rendererDataBuffer.rendererData.irradianceDampening;
            vec3 diffuseAlbedo = reflectedSurface.linearBaseColor * (1.0 - reflectedSurface.metallic);
            incidentRadiance += probeIrradiance * diffuseAlbedo;
        }

        LightBuffer reflectionLightBuffer = pc.data.frame.lightBuffer;
        for (int i = 0; i < LIGHT_COUNT; i++) {
            Light light = reflectionLightBuffer.lights[i];
            if (light.radius <= 0.0 || light.strength <= 0.0) {
                continue;
            }

            vec3 lightPosition = vec3(light.posX, light.posY, light.posZ);
            vec3 lightColor = vec3(light.colorR, light.colorG, light.colorB);
            vec3 lightBoundsMin = light.worldBoundsMin.xyz;
            vec3 lightBoundsMax = light.worldBoundsMax.xyz;

            if (!PointInAABB(reflectedSurface.worldPos, lightBoundsMin, lightBoundsMax)) {
                continue;
            }

            vec3 toLight = lightPosition - reflectedSurface.worldPos;
            float dist = length(toLight);
            vec3 lightDir = toLight / max(dist, 0.0001);
            float attenuation = smoothstep(light.radius, 0.0, dist) * light.strength;
            float ndotl = max(dot(reflectedSurface.normal, lightDir), 0.0);

            if (ndotl <= 0.0 || attenuation <= 0.0) {
                continue;
            }

            float candelas = 1.0;

            if (light.iesTextureIndex != 0) {
                uint iesTextureIndex = uint(light.iesTextureIndex);
                candelas = ApplyIESProfile(reflectedSurface.worldPos, light, textures[nonuniformEXT(iesTextureIndex)], textureSamplers[nonuniformEXT(iesTextureIndex)]);
            }

            if (candelas == 0.0) {
                continue;
            }

            float visibility = GetPointShadowMapVisibility(light, reflectedSurface.worldPos, reflectedSurface.normal, cameraWorldPos);

            if (visibility <= 0.0) {
                continue;
            }

            vec3 directLight = EvaluatePointLight(lightPosition, lightColor, light.radius, light.strength, reflectedSurface.worldPos, reflectedSurface.normal, reflectedSurface.linearBaseColor, reflectedSurface.roughness, reflectedSurface.metallic, cameraWorldPos);

            incidentRadiance += directLight * visibility * candelas;
        }
    }

    // DNSR's temporal stages operate on AMD's unmodified HDR intersection radiance, without primary-surface BRDF/PDF or firefly weighting
    return incidentRadiance;
}

void WriteEmptyIndirectSpecularPixel() {
    out_color = vec4(0.0);
}

bool IsAMDBaseRay(uvec2 pixel, uint samplesPerQuad) {
    uvec2 quadLane = pixel & uvec2(1u);

    if (samplesPerQuad == 1u) {
        return (quadLane.x | quadLane.y) == 0u;
    }

    if (samplesPerQuad == 2u) {
        return quadLane.x == quadLane.y;
    }

    return true;
}

void WriteAMDRayCopyMarker(uvec2 pixel, uint samplesPerQuad) {
    uvec2 quadLane = pixel & uvec2(1u);

    // Matching FidelityFX SDK 1.1.4 SSSR's copy flags
    // Their resolve pass flips the corresponding coordinate bit to find the ray that owns this pixel

    if (samplesPerQuad == 2u) {
        out_color = vec4(0.0, 0.0, 0.0, VK_INDIRECT_SPECULAR_AMD_COPY_HORIZONTAL);
        return;
    }

    if (quadLane.x != 0u && quadLane.y == 0u) {
        out_color = vec4(0.0, 0.0, 0.0, VK_INDIRECT_SPECULAR_AMD_COPY_HORIZONTAL);
        return;
    }

    if (quadLane.x == 0u && quadLane.y != 0u) {
        out_color = vec4(0.0, 0.0, 0.0, VK_INDIRECT_SPECULAR_AMD_COPY_VERTICAL);
        return;
    }

    out_color = vec4(0.0, 0.0, 0.0, VK_INDIRECT_SPECULAR_AMD_COPY_DIAGONAL);
}

void main() {
    ViewportDataBuffer viewportDataBuffer = pc.data.frame.viewportDataBuffer;
    RendererDataBuffer rendererDataBuffer = pc.data.frame.rendererDataBuffer;

    ivec2 px = ivec2(gl_FragCoord.xy);

    ivec2 fullSize = textureSize(sampler2D(textures[VULKAN_TEXTURE_IDX_GBUFFER_DEPTH], samplers[VULKAN_SAMPLER_IDX_NEAREST]), 0);
    float amdRoughness = texelFetch(sampler2D(textures[VULKAN_TEXTURE_IDX_INDIRECT_SPECULAR_AMD_EXTRACTED_ROUGHNESS], samplers[VULKAN_SAMPLER_IDX_NEAREST]), px, 0).r;

    // Bail if above roughness threshold
    if (amdRoughness >= VK_INDIRECT_SPECULAR_AMD_ROUGHNESS_THRESHOLD) {
        WriteEmptyIndirectSpecularPixel();
        return;
    }

    uint viewportIndex = ViewportIndexFromPixel(px, fullSize, rendererDataBuffer.rendererData.viewportLayout, vec2(rendererDataBuffer.rendererData.viewportSplitX, rendererDataBuffer.rendererData.viewportSplitY));
    ivec4 viewportRect = ivec4(viewportDataBuffer.viewportData[viewportIndex].xOffset, viewportDataBuffer.viewportData[viewportIndex].yOffset, viewportDataBuffer.viewportData[viewportIndex].width, viewportDataBuffer.viewportData[viewportIndex].height);
    mat4 inverseProjectionView = viewportDataBuffer.viewportData[viewportIndex].inverseProjectionViewReverseZ;
    mat4 viewMatrix = viewportDataBuffer.viewportData[viewportIndex].view;
    mat4 inverseViewMatrix = viewportDataBuffer.viewportData[viewportIndex].inverseView;
    vec3 viewPos = viewportDataBuffer.viewportData[viewportIndex].viewPos.xyz;

    // Match AMD's classifier input and reject background before reconstruction or ray traversal
    float depth = texelFetch(sampler2D(textures[VULKAN_TEXTURE_IDX_GBUFFER_DEPTH], samplers[VULKAN_SAMPLER_IDX_NEAREST]), px, 0).r;

    if (!IsFinite(depth) || depth <= 0.000001) {
        WriteEmptyIndirectSpecularPixel();
        return;
    }

    // AMD keeps every mirror ray, but schedules only one or two base rays denoised glossy 2x2 quads
    // Non-base pixels are filled after this pass
    bool needsDenoiser = amdRoughness >= VK_INDIRECT_SPECULAR_AMD_MIRROR_ROUGHNESS_THRESHOLD && amdRoughness < VK_INDIRECT_SPECULAR_AMD_ROUGHNESS_THRESHOLD;

    if (needsDenoiser && !IsAMDBaseRay(uvec2(px), pc.data.samplesPerQuad)) {
        WriteAMDRayCopyMarker(uvec2(px), pc.data.samplesPerQuad);
        return;
    }

    vec4 normalXYRoughnessMisc = texelFetch(sampler2D(textures[VULKAN_TEXTURE_IDX_NORMAL_XY_ROUGHNESS_MISC], samplers[VULKAN_SAMPLER_IDX_NEAREST]), px, 0);

    uint miscFlags = DecodeMiscFlags(normalXYRoughnessMisc.a);
    bool isMirrorSurface = (miscFlags & MISC_FLAG_MIRROR_SURFACE) != 0u;

    if (!IsFinite(normalXYRoughnessMisc)) {
        WriteEmptyIndirectSpecularPixel();
        return;
    }

    bool isDeltaSample = amdRoughness < 0.001;

    vec2 viewportUV = ViewportUVFromPixel(px, fullSize, viewportRect);
    vec3 worldPos = WorldPosFromDepth(viewportUV, depth, inverseProjectionView);
    float fragDistance = distance(worldPos, viewPos);

    if (!IsFinite(worldPos) || !IsFinite(fragDistance) || fragDistance <= 0.0) {
        WriteEmptyIndirectSpecularPixel();
        return;
    }

    vec3 normal = DecodeOct(normalXYRoughnessMisc.rg);

    if (!IsFinite(normal)) {
        WriteEmptyIndirectSpecularPixel();
        return;
    }

    vec3 viewDirToCamera = normalize(viewPos - worldPos);

    if (!IsFinite(viewDirToCamera)) {
        WriteEmptyIndirectSpecularPixel();
        return;
    }

    Surface surface;
    surface.worldPos = worldPos;
    surface.normal = normal;
    surface.linearBaseColor = vec3(0.0);
    surface.roughness = amdRoughness;
    surface.metallic = 0.0;

    vec3 reflectionOrigin = surface.worldPos + surface.normal * (AMD_REFLECTION_RAY_ORIGIN_BIAS * fragDistance);

    // Delta rays are deterministic and never touch the blue-noise texture
    vec2 randomSample = isDeltaSample ? vec2(0.0) : GetBlueNoiseSample(px, pc.data.frameIndex);

    // Mirrors have roughness reduced to zero, otherwise use apply the RendererData dampening factor to the material roughness
    float roughnessDampening = isMirrorSurface ? 0.0 : rendererDataBuffer.rendererData.indirectSpecularRoughnessDampening;

    float reflectedHitDistance;

    bool useDDGIReflections = rendererDataBuffer.rendererData.enableDDGIReflections;

    // not sure about mirrors yet
    // || isMirrorSurface

 //   pc.data.enableDDGIReflections != 0u;//pc.data.enableDDGIReflections != 0u || isMirrorSurface;

    vec3 incidentRadiance = GetIndirectSpecularSample(surface, viewDirToCamera, viewPos, viewMatrix, inverseViewMatrix, reflectionOrigin, randomSample, roughnessDampening, useDDGIReflections, reflectedHitDistance);
    vec4 amdRadianceAndDistance = vec4(incidentRadiance, reflectedHitDistance);

    if (any(isnan(amdRadianceAndDistance))) {
        // AMD's WriteRadiance returns without touching its UAV for NaN input
        discard;
    }

    out_color = amdRadianceAndDistance;
}
