#version 460
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_ray_query : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "../common/constants.glsl"
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
layout(set = 1, binding = 0) uniform accelerationStructureEXT u_RayQueryAccelerationStructure;

#include "../common/Vulkan/VK_ray_query_scene.glsl"
#include "VK_point_shadows.glsl"

#include "VK_ddgi_upsample.glsl"
#include "VK_indirect_specular_amd_apply.glsl"

layout(early_fragment_tests) in;
layout(location = 0) out vec4 out_color;

#define LIGHT_COUNT 9             // TODO "tile based deferred" me the fuck outta here

struct Surface {
    vec3 worldPos;
    vec3 normal;
    vec3 linearBaseColor;
    float roughness;
    float metallic;
};

layout(push_constant, scalar) uniform PushConstants {
    PushConstantsDeferredLighting data;
} pc;

//vec3 FresnelSchlick(float cosTheta, vec3 f0) {
//    return f0 + (1.0 - f0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
//}

float MaxComponent(vec3 v) {
    return max(max(v.x, v.y), v.z);
}

float Hash12(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

vec2 Hash22(vec2 p) {
    float x = Hash12(p);
    float y = Hash12(p + 17.17);
    return vec2(x, y);
}

vec3 GetJitterRay(vec3 dir, float lightSize, float sampleIndex) {
    vec3 n = normalize(dir);

    vec3 up = abs(n.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, n));
    vec3 bitangent = cross(n, tangent);

    vec2 seed = gl_FragCoord.xy + n.xy * 113.1 + vec2(sampleIndex * 37.17, sampleIndex * 91.73);
    vec2 r = Hash22(seed);
    float angle = r.x * 6.28318530718;
    float radius = sqrt(r.y) * lightSize;

    vec2 disk = vec2(cos(angle), sin(angle)) * radius;

    return normalize(n + tangent * disk.x + bitangent * disk.y);
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

    // Evaluate the direct BRDF
    vec3 brdf = microfacetBRDF(lightDir, viewDir, normal, baseColor, metallic, 1.0, roughness);
    return brdf * ndotl * attenuation * clamp(lightColor, 0.0, 1.0);
}

bool AnyHit(RayQueryContext rayQueryContext, vec3 rayOrigin, vec3 rayDir, float maxDistance) {
    if (maxDistance <= 0.001) {
        return false;
    }

    // Return true when anything blocks this shadow ray
    rayQueryEXT rayQuery;
    // Shadow flags, blending modes, and alpha masks must be checked for every triangle.
    rayQueryInitializeEXT(rayQuery, u_RayQueryAccelerationStructure, gl_RayFlagsNoOpaqueEXT | gl_RayFlagsTerminateOnFirstHitEXT, 0xff, rayOrigin, 0.001, rayDir, maxDistance);

    // Walk candidates until a blocker is accepted or the BVH ends
    while (rayQueryProceedEXT(rayQuery)) {
        uint candidateType = rayQueryGetIntersectionTypeEXT(rayQuery, false);
        if (candidateType != gl_RayQueryCandidateIntersectionTriangleEXT) {
            continue;
        }

        // instanceCustomIndex points to the BLAS instance table
        uint instanceIndex = rayQueryGetIntersectionInstanceCustomIndexEXT(rayQuery, false);
        RayQueryBLASData blasData = rayQueryContext.blasDataBuffer.blasData[instanceIndex];

        uint geometryIndex = rayQueryGetIntersectionGeometryIndexEXT(rayQuery, false);
        uint sceneRenderItemIndex;
        RenderItem renderItem;
        if (!ResolveRayQueryRenderItem(rayQueryContext, blasData, geometryIndex, sceneRenderItemIndex, renderItem)) {
            rayQueryConfirmIntersectionEXT(rayQuery);
            return true;
        }

        // Skip meshes that do not cast point-light shadows
        if ((renderItem.shadowFlags & SHADOW_FLAG_POINT_LIGHT) == 0u) {
            continue;
        }

        // Skip blended materials (aka eyebrows)
        if (renderItem.blendingMode == BLENDING_MODE_BLENDED) {
            continue;
        }

        // Skip alpha-tested materials where the sampled hit pixel is transparent
        if (RayQueryBlendingModeUsesAlphaMask(renderItem.blendingMode)) {

            uint primitiveIndex = rayQueryGetIntersectionPrimitiveIndexEXT(rayQuery, false);
            vec2 barycentrics = rayQueryGetIntersectionBarycentricsEXT(rayQuery, false);

            if (!RayQueryCandidatePassesAlphaTest(rayQueryContext, blasData, renderItem, primitiveIndex, barycentrics)) {
                continue;
            }
        }

        // This hit blocks the light
        rayQueryConfirmIntersectionEXT(rayQuery);
        return true;
    }

    // A committed hit means the ray is blocked
    uint intersectionType = rayQueryGetIntersectionTypeEXT(rayQuery, true);
    return intersectionType != gl_RayQueryCommittedIntersectionNoneEXT;
}

float GetShadowVisibility(RayQueryContext rayQueryContext, vec3 rayOrigin, vec3 target) {

    // Build a ray that stops before the light
    vec3 rayVector = target - rayOrigin;
    float rayLength = length(rayVector);

    const float rayTMin = 0.001;
    const float targetBias = 0.01;

    float rayTMax = rayLength - targetBias;

    // Treat very short rays as visible
    if (rayTMax <= rayTMin) {
        return 1.0;
    }

    vec3 rayDir = rayVector / rayLength;

    const int shadowSampleCount = 1;
    const float shadowLightSize = 0.0;

    // Jitter and average the shadow samples
    float visibility = 0.0;
    for (int i = 0; i < shadowSampleCount; i++) {
        vec3 jitteredRayDir = GetJitterRay(rayDir, shadowLightSize, float(i));
        visibility += AnyHit(rayQueryContext, rayOrigin, jitteredRayDir, rayTMax) ? 0.0 : 1.0;
    }

    return visibility / float(shadowSampleCount);
}

void main() {
    ViewportDataBuffer viewportDataBuffer = pc.data.frame.viewportDataBuffer;
    RendererDataBuffer rendererDataBuffer = pc.data.frame.rendererDataBuffer;
    TileLightsBuffer tileLights = pc.data.frame.tileLightBuffer;
    RayQueryContext rayQueryContext = CreateRayQueryContext(pc.data.rayQueryBLASDataDeviceAddress, pc.data.rayQuerySceneRenderItemIndicesDeviceAddress, uint64_t(pc.data.frame.sceneRenderItemBuffer), uint64_t(pc.data.frame.materialBuffer));

    RendererData rendererData = rendererDataBuffer.rendererData;

    // Get viewport data
    ivec2 px = ivec2(gl_FragCoord.xy);
    ivec2 outputImageSize = textureSize(sampler2D(textures[VULKAN_TEXTURE_IDX_BASE_COLOR_METALLIC], samplers[VULKAN_SAMPLER_IDX_NEAREST]), 0);
    vec2 screenUV = ScreenUVFromPixel(px, outputImageSize);
    uint viewportIndex = ViewportIndexFromPixel(px, outputImageSize, rendererDataBuffer.rendererData.viewportLayout, vec2(rendererDataBuffer.rendererData.viewportSplitX, rendererDataBuffer.rendererData.viewportSplitY));
    ivec4 viewportRect = ivec4(viewportDataBuffer.viewportData[viewportIndex].xOffset, viewportDataBuffer.viewportData[viewportIndex].yOffset, viewportDataBuffer.viewportData[viewportIndex].width, viewportDataBuffer.viewportData[viewportIndex].height);
    mat4 inverseProjectionView = viewportDataBuffer.viewportData[viewportIndex].inverseProjectionViewReverseZ;
    vec3 viewPos = viewportDataBuffer.viewportData[viewportIndex].viewPos.xyz;

    // Fetch GBuffer
    vec4 baseColorMetallic = texelFetch(sampler2D(textures[VULKAN_TEXTURE_IDX_BASE_COLOR_METALLIC], samplers[VULKAN_SAMPLER_IDX_NEAREST]), px, 0);
    vec4 normalXYRoughnessMisc = texelFetch(sampler2D(textures[VULKAN_TEXTURE_IDX_NORMAL_XY_ROUGHNESS_MISC], samplers[VULKAN_SAMPLER_IDX_NEAREST]), px, 0);
    float depth = texelFetch(sampler2D(textures[VULKAN_TEXTURE_IDX_GBUFFER_DEPTH], samplers[VULKAN_SAMPLER_IDX_NEAREST]), px, 0).r;

    // Reconstruct position from depth
    vec2 viewportUV = ViewportUVFromPixel(px, outputImageSize, viewportRect);
    vec3 worldPos = WorldPosFromDepth(viewportUV, depth, inverseProjectionView);
    float fragDistance = distance(worldPos, viewPos);

    // Reconstruct materials
    vec3 baseColor = baseColorMetallic.rgb;
    vec3 normal = DecodeOct(normalXYRoughnessMisc.rg);
    float metallic = baseColorMetallic.a;
    float roughness = normalXYRoughnessMisc.b;

    vec3 linearBaseColor = pow(baseColor, vec3(2.2));

    Surface surface;
    surface.worldPos = worldPos;
    surface.normal = normal;
    surface.linearBaseColor = linearBaseColor;
    surface.roughness = roughness;
    surface.metallic = metallic;

    // Decode flags
    uint miscFlags = DecodeMiscFlags(normalXYRoughnessMisc.a);
    bool isMirrorSurface = (miscFlags & MISC_FLAG_MIRROR_SURFACE) != 0u;

    // Direct light
    vec3 directLighting = vec3(0.0);


    LightBuffer lightBuffer = pc.data.frame.lightBuffer;

    // Tile data
    uvec2 tileCoord = uvec2(px) / uint(TILE_SIZE);
    uint tileIndex = tileCoord.y * rendererData.tileCountX + tileCoord.x;
    uint tileLightCount = tileLights.tileLights[tileIndex].lightCount;

    // Direct lighting
    for (int i = 0; i < tileLightCount; i++) {

        int lightIndex = int(tileLights.tileLights[tileIndex].lightIndices[i]);
        Light light = lightBuffer.lights[lightIndex];

        vec3 lightPosition = vec3(light.posX, light.posY, light.posZ);
        vec3 lightColor =  vec3(light.colorR, light.colorG, light.colorB);

        vec3 lightBoundsMin = light.worldBoundsMin.xyz;
        vec3 lightBoundsMax = light.worldBoundsMax.xyz;

        float candelas = 1.0;

        if (light.iesTextureIndex != 0) {
            uint iesTextureIndex = uint(light.iesTextureIndex);
            candelas = ApplyIESProfile(worldPos, light, textures[nonuniformEXT(iesTextureIndex)], textureSamplers[nonuniformEXT(iesTextureIndex)]);
        }

        if (candelas == 0) {
            continue;
        }

        float visibility = rendererData.directPointShadowMode == POINT_SHADOW_MODE_RAY_QUERY
            ? GetShadowVisibility(rayQueryContext, worldPos + normal * 0.001, lightPosition)
            : GetPointShadowMapVisibilitySkin(light, worldPos, normal, viewPos);
        visibility *= candelas;

        if (visibility <= 0.0) {
            continue;
        }

        vec3 directLight = GetDirectLighting(lightPosition, lightColor, light.radius, light.strength, normal.xyz, worldPos.xyz, linearBaseColor.rgb, roughness, metallic, viewPos) * visibility;
        directLighting += directLight;
    }

    // Indirect specular
    vec3 indirectSpecular = vec3(0, 0, 0);

    if (rendererData.enableIndirectSpecular) {
        indirectSpecular = texelFetch(sampler2D(textures[VULKAN_TEXTURE_IDX_INDIRECT_SPECULAR_AMD_TEMPORAL], samplers[VULKAN_SAMPLER_IDX_NEAREST]), px, 0).rgb;
        float amdAlphaRoughness = texelFetch(sampler2D(textures[VULKAN_TEXTURE_IDX_INDIRECT_SPECULAR_AMD_MATERIAL_ROUGHNESS], samplers[VULKAN_SAMPLER_IDX_NEAREST]), px, 0).r;
        float amdPerceptualRoughness = sqrt(clamp(amdAlphaRoughness, 0.0, 1.0));
        vec3 viewDirToCamera = normalize(viewPos - worldPos);

        vec3 primaryBaseColor = isMirrorSurface ? vec3(1.0) : linearBaseColor;
        float primaryMetallic = isMirrorSurface ? 1.0 : metallic;
        vec3 primaryResponse = GetAMDIndirectSpecularPrimaryResponse(primaryBaseColor, primaryMetallic, amdPerceptualRoughness, normal, viewDirToCamera, pc.data.brdfLutTextureIndex);
        float indirectSpecularFactor = isMirrorSurface ? 1.0 : rendererData.indirectSpecularFactor; // Mirrors always get a boost factor of 1.0, any other reflective surface uses the RendererData value

        indirectSpecular = ApplyAMDIndirectSpecularBRDF(indirectSpecular, primaryResponse) * indirectSpecularFactor;
    }

    // Indirect diffuse
    vec3 indirectDiffuse = vec3(0.0);

    if (rendererData.enableIrradianceProbeSampling) {
        if (!isMirrorSurface) {
            vec3 probeIrradiance = SampleDDGIIndirectDiffuseBilateral_VK(screenUV, normal, fragDistance, outputImageSize, viewportRect);
            vec3 diffuseAlbedo = linearBaseColor.rgb * (1.0 - metallic);
            indirectDiffuse = probeIrradiance * diffuseAlbedo;
        }
    }


    // Final composite
    vec3 finalLighting = directLighting + indirectDiffuse + indirectSpecular;

    out_color = vec4(finalLighting, 1.0);


   // out_color = vec4(indirectSpecular, 1.0);


    // Normals test
    if (false) {
        vec3 debugColor = vec3(0, 0, 0); // No reflected surface is black

        vec3 testViewDirToCamera = normalize(viewPos - surface.worldPos);
        float testNoV = clamp(dot(surface.normal, testViewDirToCamera), 0.0, 1.0);

        if (testNoV > 0.0) {
            vec3 testRayDir = normalize(reflect(-testViewDirToCamera, surface.normal));
            vec3 testRayOrigin = surface.worldPos + surface.normal * 0.01;
            RayQueryHit reflectedHit = TraceClosestReflectionHit(u_RayQueryAccelerationStructure, rayQueryContext, testRayOrigin, testRayDir, 0.01, 80.0);

            if (reflectedHit.found) {
                RayQueryMaterialSample reflectedMaterial = EvaluateRayHitMaterial(rayQueryContext, reflectedHit, 0.0, 0.0);
                vec2 normalOct = EncodeOct(reflectedMaterial.normal); // write/read this
                vec3 reconstructedNormal = DecodeOct(normalOct);
                debugColor = clamp(reconstructedNormal, 0, 1);
            }
        }

        out_color = vec4(debugColor, 1.0);
    }

    // World pos test
    if (false) {
        vec3 debugColor = vec3(0, 0, 0); // No reflected surface is black

        vec3 testViewDirToCamera = normalize(viewPos - surface.worldPos);
        float testNoV = clamp(dot(surface.normal, testViewDirToCamera), 0.0, 1.0);

        if (testNoV > 0.0) {
            vec3 testRayDir = normalize(reflect(-testViewDirToCamera, surface.normal));
            vec3 testRayOrigin = surface.worldPos + surface.normal * 0.01;
            RayQueryHit reflectedHit = TraceClosestReflectionHit(u_RayQueryAccelerationStructure, rayQueryContext, testRayOrigin, testRayDir, 0.01, 80.0);

            if (reflectedHit.found) {
                vec2 rayDirOct = EncodeOct(testRayDir) * 2.0 - 1.0; // write/read this
                float rayT = reflectedHit.rayT; // write/read this
                vec3 reconstructedRayDir = DecodeOct(rayDirOct * 0.5 + 0.5);
                vec3 reconstructedWorldPos = testRayOrigin + reconstructedRayDir * rayT;
                debugColor = reconstructedWorldPos;
            }
        }

        out_color = vec4(debugColor, 1.0);
    }

    // Material test
    if (false) {
        vec3 debugColor = vec3(0, 0, 0); // No reflected surface is black

        vec3 testViewDirToCamera = normalize(viewPos - surface.worldPos);
        float testNoV = clamp(dot(surface.normal, testViewDirToCamera), 0.0, 1.0);

        if (testNoV > 0.0) {
            vec3 testRayDir = normalize(reflect(-testViewDirToCamera, surface.normal));
            vec3 testRayOrigin = surface.worldPos + surface.normal * 0.01;
            RayQueryHit reflectedHit = TraceClosestReflectionHit(u_RayQueryAccelerationStructure, rayQueryContext, testRayOrigin, testRayDir, 0.01, 80.0);

            if (reflectedHit.found) {
                vec2 uv = reflectedHit.uv; // write/read this
                uint materialIndex = uint(reflectedHit.materialIndex); // write/read this

                MaterialBuffer materialBuffer = pc.data.frame.materialBuffer;
                Material material = materialBuffer.materials[int(materialIndex)];
                if (material.basecolor >= 0) {
                    uint textureIndex = uint(material.basecolor);
                    vec3 baseColor = textureLod(sampler2D(textures[nonuniformEXT(textureIndex)], textureSamplers[nonuniformEXT(textureIndex)]), uv, 0.0).rgb;
                    debugColor = pow(baseColor, vec3(2.2));
                }
            }
        }

        out_color = vec4(debugColor, 1.0);
    }

}
