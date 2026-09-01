#version 460
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

layout(early_fragment_tests) in;

#include "../common/hair.glsl"
#include "../common/Vulkan/VK_binding_indices.glsl"
#include "../common/constants.glsl"
#include "../common/lighting.glsl"
#include "../common/types.glsl"
#include "../common/util.glsl"
#include "../common/viewport.glsl"
#include "../common/Vulkan/VK_push_constants.glsl"

layout(set = 0, binding = DESC_IDX_TEXTURES) uniform texture2D textures[];
layout(set = 0, binding = DESC_IDX_SAMPLERS) uniform sampler samplers[];
layout(set = 0, binding = DESC_IDX_TEXTURE_SAMPLERS) uniform sampler textureSamplers[];

#include "VK_point_shadows.glsl"
#include "VK_ddgi_upsample.glsl"

layout(location = 0) out vec4 LightingOut;

layout(location = 0) centroid in vec2 v_texCoord;
layout(location = 1) centroid in vec3 v_normal;
layout(location = 2) centroid in vec3 v_tangent;
layout(location = 3) centroid in vec4 v_worldPos;
layout(location = 4) flat in uint v_globalInstanceIndex;
layout(location = 5) flat in uint v_viewportIndex;

layout(push_constant, scalar) uniform PushConstants {
    PushConstantsHair data;
} pc;

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

vec3 EvaluateHairLight(vec3 hairBaseColor, vec3 finalTangent, vec3 V, vec3 L, vec3 t1, vec3 t2, float alpha1, float alpha2, vec3 lightColor, float visibility) {
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

    vec3 spec1 = D1 * F1 * u_spec1Intensity;
    vec3 spec2 = D2 * F2 * hairBaseColor * u_spec2Intensity;

    float scatterProp = pow(max(dot(V, -L), 0.0), u_scatterPower);
    vec3 scattering = hairBaseColor * scatterProp * u_scatterIntensity;

    return (diffuse + spec1 + spec2 + scattering) * clamp(lightColor, 0.0, 1.0) * visibility;
}

void ComputeCCNormalAndTangents(vec3 vertexNormal, vec3 vertexTangent, vec3 flowMap, float hairID, float flipTangentGreen, out vec3 finalNormal, out vec3 finalTangent) {
    vec3 meshTangent = normalize(vertexTangent);
    vec3 meshNormalUnflipped = normalize(vertexNormal);
    vec3 meshNormal = gl_FrontFacing ? meshNormalUnflipped : -meshNormalUnflipped;
    vec3 meshBitangent = normalize(cross(meshNormalUnflipped, meshTangent));

    flowMap = flowMap * 2.0 - 1.0;

    vec3 tangentSpaceShift;
    tangentSpaceShift.x = flowMap.x;
    tangentSpaceShift.y = flowMap.y * flipTangentGreen;
    tangentSpaceShift.z = 0.0;

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

void main() {
    RenderItemBuffer renderItemBuffer = pc.data.frame.sceneRenderItemBuffer;
    MaterialBuffer materialBuffer = pc.data.frame.materialBuffer;
    RendererDataBuffer rendererDataBuffer = pc.data.frame.rendererDataBuffer;
    ViewportDataBuffer viewportDataBuffer = pc.data.frame.viewportDataBuffer;
    LightBuffer lightBuffer = pc.data.frame.lightBuffer;
    TileLightsBuffer tileLights = pc.data.frame.tileLightBuffer;

    RenderItem renderItem = renderItemBuffer.renderItems[v_globalInstanceIndex];
    Material material = materialBuffer.materials[renderItem.materialIndex];

    uint baseColorTextureIndex = uint(material.basecolor);
    uint rmaTextureIndex = uint(material.rma);
    uint hairTextureIndex = uint(material.hairMaps);

    vec2 baseTextureSizePixels = vec2(textureSize(sampler2D(textures[nonuniformEXT(baseColorTextureIndex)], textureSamplers[nonuniformEXT(baseColorTextureIndex)]), 0));

    vec4 baseColor = texture(sampler2D(textures[nonuniformEXT(baseColorTextureIndex)], textureSamplers[nonuniformEXT(baseColorTextureIndex)]), v_texCoord);
    vec4 rma = texture(sampler2D(textures[nonuniformEXT(rmaTextureIndex)], textureSamplers[nonuniformEXT(rmaTextureIndex)]), v_texCoord).rgba;
    vec4 hairTexture = texture(sampler2D(textures[nonuniformEXT(hairTextureIndex)], textureSamplers[nonuniformEXT(hairTextureIndex)]), v_texCoord);

    vec3 flowMap = vec3(hairTexture.rg, 0.0);
    float hairID = hairTexture.b;
    float rootFactor = hairTexture.a;

    float hairMipLevelRaw = ComputeHairMipLevel(v_texCoord, baseTextureSizePixels);
    float roughness = clamp(rma.r * renderItem.roughnessFactor, 0.0, 1.0);
    float metallic = clamp(renderItem.metallicFactor, 0.0, 1.0);
    float ao = rma.b;
    vec3 linearBaseColor = DarkenHairBaseColor(pow(baseColor.rgb, vec3(2.2)));

    vec3 viewPos = viewportDataBuffer.viewportData[v_viewportIndex].viewPos.xyz;
    vec3 V = normalize(viewPos - v_worldPos.xyz);

    vec3 finalNormal;
    vec3 finalTangent;
    ComputeCCNormalAndTangents(v_normal, v_tangent, flowMap, hairID, 1.0, finalNormal, finalTangent);

    vec3 hairBaseColor = linearBaseColor * mix(u_rootColorFloor, u_tipColorFloor, rootFactor);
    hairBaseColor *= 0.8;

    const float u_specularAARoughnessPerMip = 0.5;
    const float u_specularMipFadeStrength = 0.2;
    const float u_specularMipStart = 0.9;
    float u_renderResolutionScale = 1.0;
    float mipLevelRaw = max(0.0, hairMipLevelRaw + log2(u_renderResolutionScale));
    float mipLevel = max(0.0, mipLevelRaw - u_specularMipStart);
    float roughnessAA = clamp(roughness + mipLevel * u_specularAARoughnessPerMip, 0.0, 1.0);
    float specularMipFade = 1.0 / (1.0 + mipLevel * mipLevel * u_specularMipFadeStrength);

    const float kHairRoughnessMapStrength = 1.0;
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
    uint tileIndex = tileCoord.y * rendererDataBuffer.rendererData.tileCountX + tileCoord.x;
    uint lightCount = tileLights.tileLights[tileIndex].lightCount;

    vec3 directLighting = vec3(0.0);

    for (int i = 0; i < lightCount; i++) {
        int lightIndex = int(tileLights.tileLights[tileIndex].lightIndices[i]);
        Light light = lightBuffer.lights[lightIndex];

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
            shadow = ShadowCalculationMediumBindless(light.hiResShadowMapIndex, lightPos, light.radius, v_worldPos.xyz, viewPos, finalNormal, VULKAN_POINT_SHADOW_IDX_HIGH_RES);
        }
        else if (light.lowResShadowMapIndex != -1) {
            shadow = ShadowCalculationMediumBindless(light.lowResShadowMapIndex, lightPos, light.radius, v_worldPos.xyz, viewPos, finalNormal, VULKAN_POINT_SHADOW_IDX_LOW_RES);
        }

        vec3 lightContribution = EvaluateHairLight(hairBaseColor, finalTangent, V, L, t1, t2, alpha1, alpha2, lightCol, shadow);

        if (light.iesTextureIndex != 0) {
            uint iesTextureIndex = uint(light.iesTextureIndex);
            lightContribution *= ApplyIESProfile(v_worldPos.xyz, light, textures[nonuniformEXT(iesTextureIndex)], textureSamplers[nonuniformEXT(iesTextureIndex)]);
        }
        directLighting += lightContribution * attenuation;
    }

    float fragDistance = distance(v_worldPos.xyz, viewPos);
    vec3 indirectDiffuse = vec3(0.0);

    if (rendererDataBuffer.rendererData.enableIrradianceProbeSampling) {
        ViewportData vd = viewportDataBuffer.viewportData[v_viewportIndex];
        ivec2 outputImageSize = ivec2(rendererDataBuffer.rendererData.gBufferWidth, rendererDataBuffer.rendererData.gBufferHeight);
        vec2 screenUV = ScreenUVFromFragCoord(gl_FragCoord.xy, outputImageSize);
        ivec4 viewportRect = ivec4(vd.xOffset, vd.yOffset, vd.width, vd.height);
        vec3 probeIrradiance = SampleDDGIIndirectDiffuseBilateral_VK(screenUV, finalNormal, fragDistance, outputImageSize, viewportRect);
        vec3 diffuseAlbedo = hairBaseColor.rgb * (1.0 - metallic);
        float indirectDiffuseScale = 1.0;
        indirectDiffuse = probeIrradiance * diffuseAlbedo * indirectDiffuseScale;
    }

    vec3 color = (directLighting + indirectDiffuse) * ao;
    color += vec3(0.00001);

    LightingOut = vec4(color, 1.0);
}
