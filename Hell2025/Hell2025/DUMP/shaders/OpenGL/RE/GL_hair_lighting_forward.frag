#version 460 core

#extension GL_ARB_bindless_texture : enable
#include "../../common/OpenGL/GL_binding_indices.glsl"

readonly restrict layout(std430, binding = SSBO_IDX_SAMPLERS) buffer textureSamplersBuffer { uvec2 textureSamplers[]; };

layout (binding = TEX_IDX_SHADOW_MAP_FLASHLIGHT) uniform sampler2DArray u_flashlighShadowMapArrayTexture;
layout (binding = TEX_IDX_SHADOW_MAP_HI_RES)     uniform samplerCubeArrayShadow hiResShadowMapArray;
layout (binding = TEX_IDX_SHADOW_MAP_LOW_RES)    uniform samplerCubeArrayShadow lowResShadowMapArray;

layout (binding = 5) uniform sampler2D u_indirectDiffuseTexture;
layout (binding = 7) uniform sampler2DArray woundMaskTextureArray;
layout (binding = 10) uniform sampler2D u_indirectDiffuseSurfaceTexture;
layout (binding = 11) uniform sampler2D hairFlowMap;
layout (binding = 12) uniform sampler2D hairIdMap;
layout (binding = 13) uniform sampler2D hairRootMap;

layout(early_fragment_tests) in;

#include "../../common/hair.glsl"
#include "../../common/lighting.glsl"
#include "../../common/post_processing.glsl"
#include "../../common/util.glsl"
#include "../../common/viewport.glsl"
#include "../../common/ddgi_upsample.glsl"

readonly restrict layout(std430, binding = SSBO_IDX_MATERIALS) buffer materialsBuffer { Material materials[]; };
readonly restrict layout(std430, binding = SSBO_IDX_RENDERER_DATA) buffer rendererDataBuffer { RendererData rendererData; };
readonly restrict layout(std430, binding = SSBO_IDX_VIEWPORT_DATA) buffer viewportDataBuffer { ViewportData viewportData[]; };
readonly restrict layout(std430, binding = SSBO_IDX_SCENE_RENDER_ITEMS) buffer renderItemsBuffer { RenderItem renderItems[]; };
readonly restrict layout(std430, binding = SSBO_IDX_LIGHTS) buffer lightsBuffer       { Light lights[]; };
readonly restrict layout(std430, binding = SSBO_IDX_LIGHTING_TILE_LIGHTS) buffer tileLightsBuffer   { TileLights tileLights[]; };
readonly restrict layout(std430, binding = SSBO_IDX_LIGHTING_TILE_SPOT_LIGHTS) buffer tileSpotLightsBuffer { TileSpotLights tileSpotLights[]; };
readonly restrict layout(std430, binding = SSBO_IDX_SPOT_LIGHTS) buffer spotLightsBuffer { SpotLight spotLights[]; };

layout (location = 0) out vec4 LightingOut;

centroid in vec2 v_texCoord;
centroid in vec3 v_normal;
centroid in vec3 v_tangent;
centroid in vec4 v_worldPos;

in flat int v_globalInstanceIndex;
in flat int v_viewportIndex;

uniform bool u_alphaDiscard;
uniform bool u_flipv_normalMapY;
uniform float u_renderResolutionScale;

const float u_spec1Intensity   = 0.25;
const float u_spec2Intensity   = 0.1;
const float u_scatterPower     = 12.0;
const float u_scatterIntensity = 0.1;
const float u_rootColorFloor   = 0.2;
const float u_rootAOFloor      = 0.7;
const float u_tipColorFloor    = 0.45;
const float u_tipAOFloor       = 0.7;

float HairSpecular(vec3 t, vec3 h, float roughness) {
    float alpha = max(roughness * roughness, 0.001);
    float n = 0.36 / alpha;
    float dotTH = dot(t, h);
    float sinTH = sqrt(max(0.0, 1.0 - dotTH * dotTH));
    float dirAtten = smoothstep(-1.0, 0.0, dotTH);
    return dirAtten * pow(sinTH, n) * (n + 2.0) / (2.0 * 3.14159);
}

vec3 EvaluateHairLight(vec3 hairBaseColor, vec3 finalTangent, vec3 V, vec3 L, vec3 t1, vec3 t2, float alpha1, float alpha2, vec3 lightColor, float visibility, float primarySpecularScale) {
    vec3 H = normalize(L + V);

    float dotTL = dot(finalTangent, L);
    float sinTL = sqrt(max(0.0, 1.0 - dotTL * dotTL));
    vec3 diffuse = hairBaseColor * sinTL;

    float D1 = HairSpecular(t1, H, alpha1);
    float D2 = HairSpecular(t2, H, alpha2);

    float dotVH = clamp(dot(V, H), 0.0, 1.0);
    float fresnel = pow(1.0 - dotVH, 5.0);

    vec3 F1 = vec3(0.04) + vec3(0.96) * fresnel;
    vec3 F2 = hairBaseColor + (vec3(1.0) - hairBaseColor) * fresnel;

    vec3 spec1 = D1 * F1 * u_spec1Intensity * primarySpecularScale;
    vec3 spec2 = D2 * F2 * hairBaseColor * u_spec2Intensity;

    float scatterProp = pow(max(dot(V, -L), 0.0), u_scatterPower);
    vec3 scattering = hairBaseColor * scatterProp * u_scatterIntensity;

    return (diffuse + spec1 + spec2 + scattering) * clamp(lightColor, 0.0, 1.0) * visibility;
}

void ComputeCCNormalAndTangents(vec3 vertexNormal, vec3 vertexTangent, vec3 flowMap, float hairID, float flipTangentGreen, out vec3 finalNormal, out vec3 finalTangent) {
    vec3 meshTangent = normalize(vertexTangent);
    vec3 meshNormalUnflipped = normalize(vertexNormal);
    vec3 meshNormal = gl_FrontFacing ? -meshNormalUnflipped : meshNormalUnflipped;

    vec3 meshBitangent = normalize(cross(meshNormalUnflipped, meshTangent));
    
    flowMap = flowMap * 2.0 - 1.0;

    vec3 tangentSpaceShift;
    tangentSpaceShift.x = flowMap.x;
    tangentSpaceShift.y = flowMap.y * flipTangentGreen;
    tangentSpaceShift.z = 0;//flowMap.z * 0.03;

    vec3 blackOffset = vec3(-0.206, -0.687, -0.338);
    vec3 whiteOffset = vec3(-0.148, 0.0, 0.370);
    vec3 idOffset = mix(blackOffset, whiteOffset, hairID);

    tangentSpaceShift = normalize(tangentSpaceShift + idOffset);

    finalTangent = vec3(
        tangentSpaceShift.x * meshTangent +
        tangentSpaceShift.y * meshBitangent +
        tangentSpaceShift.z * meshNormal
    );

    finalNormal = meshNormal;
}

void ComputeGhettoNormalAndTangents(vec3 vertexNormal, vec3 vertexTangent, vec3 flowMap, float hairID, float flipTangentGreen, out vec3 finalNormal, out vec3 finalTangent) {
    vec3 meshTangent = normalize(vertexTangent);
    vec3 meshNormalUnflipped = normalize(vertexNormal); // unused???
    vec3 meshNormal = gl_FrontFacing ? -meshNormalUnflipped : meshNormalUnflipped;

    meshTangent = normalize(meshTangent - dot(meshTangent, meshNormal) * meshNormal);
    vec3 meshBitangent = cross(meshNormalUnflipped, meshTangent);

    float kTangentMapFlipGreen = 1.0;
    
    flowMap = flowMap * 2.0 - 1.0;

    float flowLen2 = dot(flowMap.rg, flowMap.rg);

    vec3 t;
    if (flowLen2 < 0.0001) {
        t = normalize(meshTangent);
    }
    else {
        vec2 flowDir = flowMap.rg * inversesqrt(flowLen2);
        t = normalize(meshTangent * flowDir.x + meshBitangent * flowDir.y);
    }
    
    const float u_specularJitter = 0.5;

    finalTangent = normalize(t + meshNormal * (hairID - 0.5) * u_specularJitter);

    vec3 bumpNormal = vec3(0.0, 0.0, 1.0); // There is no normal map for this hair

    finalNormal = normalize(
        bumpNormal.x * meshTangent +
        bumpNormal.y * meshBitangent +
        bumpNormal.z * meshNormal
    );
}

void main() {
    RenderItem renderItem = renderItems[v_globalInstanceIndex];
    Material material = materials[renderItem.materialIndex];

    sampler2D baseColorSampler = sampler2D(textureSamplers[material.basecolor]);
    vec2 baseTextureSizePixels = vec2(textureSize(baseColorSampler, 0));

    vec4 baseColor =   texture(baseColorSampler, v_texCoord);
    vec4 rma =         texture(sampler2D(textureSamplers[material.rma]), v_texCoord).rgba;
    vec4 hairTexture = texture(sampler2D(textureSamplers[material.hairMaps]), v_texCoord);

    vec3 flowMap = vec3(hairTexture.rg, 0);
    float hairID = hairTexture.b;
    float rootFactor = hairTexture.a;

    float hairMipLevelRaw = ComputeHairMipLevel(v_texCoord, baseTextureSizePixels);

    vec3 viewPos = viewportData[v_viewportIndex].viewPos.xyz;

    float roughness = clamp(rma.r * renderItem.roughnessFactor, 0.0, 1.0);
    float metallic = clamp(renderItem.metallicFactor, 0.0, 1.0);
    float ao = rma.b;
    vec3 linearBaseColor = DarkenHairBaseColor(pow(baseColor.rgb, vec3(2.2)));

    vec3 V = normalize(viewPos - v_worldPos.xyz);

    vec3 finalNormal;
    vec3 finalTangent;

    ComputeCCNormalAndTangents(v_normal, v_tangent, flowMap, hairID, 1.0, finalNormal, finalTangent);
    //ComputeGhettoNormalAndTangents(v_normal, v_tangent, flowMap, hairID, 1.0, finalNormal, finalTangent);
    
    vec3 hairBaseColor = linearBaseColor * mix(u_rootColorFloor, u_tipColorFloor, rootFactor);
    hairBaseColor *= 0.8;

    const float u_specularAARoughnessPerMip = 0.5;
    const float u_specularMipFadeStrength = 0.2;
    const float u_specularMipStart = 0.9;

    float mipLevelRaw = max(0.0, hairMipLevelRaw + log2(u_renderResolutionScale));
    float mipLevel = max(0.0, mipLevelRaw - u_specularMipStart);

    float roughnessAA = clamp(roughness + mipLevel * u_specularAARoughnessPerMip, 0.0, 1.0);
    float specularMipFade = 1.0 / (1.0 + mipLevel * mipLevel * u_specularMipFadeStrength);

    const float kHairRoughnessMapStrength = 1.00;
    const float kRoughnessGamma = 1.0;
    const float kRoughnessWeight = 1.0;

    float ue4Roughness = pow(abs(roughness), kRoughnessGamma) * kRoughnessWeight * kHairRoughnessMapStrength;

    const float u_specularAlpha1Min = 0.055;
    const float u_specularAlpha2Min = 0.070;

    float alpha1 = clamp(ue4Roughness * ue4Roughness, u_specularAlpha1Min, 1.0);
    float alpha2 = clamp(ue4Roughness * ue4Roughness * 1.5, u_specularAlpha2Min, 1.0);

    vec3 t1 = normalize(finalTangent + finalNormal * 0.035);
    vec3 t2 = normalize(finalTangent - finalNormal * 0.052);

    uvec2 tileCoord = uvec2(gl_FragCoord.xy) / uint(TILE_SIZE);
    uint tileIndex = tileCoord.y * rendererData.tileCountX + tileCoord.x;
	uint lightCount = tileLights[tileIndex].lightCount;

    vec3 directLighting = vec3(0.0);

    // Direct lighting
    for (int i = 0; i < lightCount; i++) {
        int lightIndex = int(tileLights[tileIndex].lightIndices[i]);

        Light light = lights[lightIndex];

        vec3 lightPos = vec3(light.posX, light.posY, light.posZ);
        vec3 lightCol = vec3(light.colorR, light.colorG, light.colorB);

        vec3 lightVector = lightPos - v_worldPos.xyz;
        float distanceSquared = max(dot(lightVector, lightVector), 0.0001);
        float lightDistance = sqrt(distanceSquared);
        float attenuation = smoothstep(light.radius, 0.0, lightDistance) * light.strength;
        if (attenuation <= 0.0) {
            continue;
        }
        vec3 L = lightVector * inversesqrt(distanceSquared);

        float shadow = 1.0;
        if (light.hiResShadowMapIndex != -1) {
            shadow = ShadowCalculationMedium(light.hiResShadowMapIndex, lightPos, light.radius, v_worldPos.xyz, viewPos, finalNormal, hiResShadowMapArray);
        }
        else if (light.lowResShadowMapIndex != -1) {
            shadow = ShadowCalculationMedium(light.lowResShadowMapIndex, lightPos, light.radius, v_worldPos.xyz, viewPos, finalNormal, lowResShadowMapArray);
        }

        vec3 lightContribution = EvaluateHairLight(hairBaseColor, finalTangent, V, L, t1, t2, alpha1, alpha2, lightCol, shadow, 1.0);

        if (light.iesTextureIndex != 0) {
            sampler2D iesSampler = sampler2D(textureSamplers[light.iesTextureIndex]);
            lightContribution *= ApplyIESProfile(v_worldPos.xyz, light, iesSampler);
        }
        directLighting += lightContribution * attenuation;
    }

    float fragDistance = distance(v_worldPos.xyz, viewPos);

    sampler2D flashlightIES = sampler2D(textureSamplers[max(rendererData.flashlightIESTextureIndex, 0)]);
    uint spotLightCount = tileSpotLights[tileIndex].lightCount;
    for (uint i = 0u; i < spotLightCount; i++) {
            SpotLight spotLight = spotLights[tileSpotLights[tileIndex].lightIndices[i]];
            float modifier = spotLight.positionModifier.w;
            if (modifier <= 0.05) continue;

            vec3 spotLightPos;
            vec3 spotLightDir;
            vec3 spotLightColor;
            GetSpotLightShadingInputs(spotLight, rendererData, uint(v_viewportIndex), spotLightPos, spotLightDir, spotLightColor);

            vec3 lightVector = spotLightPos - v_worldPos.xyz;
            float distanceSquared = max(dot(lightVector, lightVector), 0.0001);
            vec3 L = lightVector * inversesqrt(distanceSquared);
            float attenuation = GetFlashlightIESAttenuation(v_worldPos.xyz, spotLightPos, spotLightDir, rendererData, flashlightIES);
            uint flags = uint(spotLight.metadata.z);
            if ((flags & (1u << 2)) != 0u) attenuation *= GetFlashlightViewDistanceScale(fragDistance);
            if (attenuation <= 0.0) continue;

            float shadow = 0.0;
            int shadowLayer = spotLight.metadata.x;
            int ownerViewportIndex = spotLight.metadata.y;
            bool skipOwnerShadow = ownerViewportIndex == v_viewportIndex && (flags & (1u << 1)) != 0u;
            if (shadowLayer >= 0 && (flags & (1u << 0)) != 0u && !skipOwnerShadow) {
                vec4 fragPosLightSpace = spotLight.projectionView * vec4(v_worldPos.xyz, 1.0);
                shadow = SpotlightShadowCalculation(fragPosLightSpace, finalNormal, spotLightDir, v_worldPos.xyz, spotLightPos, viewPos, u_flashlighShadowMapArrayTexture, shadowLayer);
            }

            float visibility = 1.0 - shadow;
            if (visibility <= 0.0) continue;

            float viewLightAlignment = clamp(dot(V, L), 0.0, 1.0);
            float headOnSpecularFactor = smoothstep(0.8, 0.98, viewLightAlignment);
            float primarySpecularScale = mix(1.0, 0.25, headOnSpecularFactor);
            vec3 lightContribution = EvaluateHairLight(hairBaseColor, finalTangent, V, L, t1, t2, alpha1, alpha2, spotLightColor, visibility, primarySpecularScale);
            float macroNoL = clamp(abs(dot(normalize(finalNormal), L)), 0.0, 1.0);
            float macroShapeFalloff = mix(0.35, 1.0, pow(macroNoL, 2.25));
            lightContribution *= macroShapeFalloff;
            directLighting += lightContribution * attenuation * modifier;
    }

    vec3 indirectDiffuse = vec3(0.0);

    // Indirect diffuse
    if (rendererData.enableIrradianceProbeSampling) {
        ViewportData vd = viewportData[v_viewportIndex];
        ivec2 outputImageSize = ivec2(rendererData.gBufferWidth, rendererData.gBufferHeight);
        vec2 screenUV = ScreenUVFromFragCoord(gl_FragCoord.xy, outputImageSize);
        ivec4 viewportRect = ivec4(vd.xOffset, vd.yOffset, vd.width, vd.height);
        vec3 probeIrradiance = SampleDDGIIndirectDiffuseBilateral(u_indirectDiffuseTexture, u_indirectDiffuseSurfaceTexture, screenUV, finalNormal, fragDistance, outputImageSize, viewportRect);
        vec3 diffuseAlbedo = hairBaseColor.rgb * (1.0 - metallic);
        float indirectDiffuseScale = 1.0;
        indirectDiffuse = probeIrradiance * diffuseAlbedo * indirectDiffuseScale;
    }

    vec3 color = (directLighting + indirectDiffuse) * ao;

    color += vec3(0.00001);

    LightingOut = vec4(color, 1.0);

    
   // LightingOut = vec4(debugColor, 1.0);
    
    //LightingOut = vec4(directLighting, 1.0);

}
