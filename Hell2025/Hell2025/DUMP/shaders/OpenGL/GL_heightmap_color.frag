#version 460 core

#extension GL_ARB_bindless_texture : enable

#include "../common/constants.glsl"
#include "../common/normal_encoding.glsl"
#include "../common/post_processing.glsl"
#include "../common/types.glsl"
#include "../common/OpenGL/GL_binding_indices.glsl"

layout (location = 0) out vec4 BaseColorMetallicOut;
layout (location = 1) out vec4 NormalXYRoughnessMiscOut;
layout (location = 2) out vec4 EmissiveOut;
layout (location = 3) out vec4 VelocityXYOcclusionSubSurfaceOut;

readonly restrict layout(std430, binding = SSBO_IDX_SAMPLERS) buffer textureSamplersBuffer { uvec2 textureSamplers[]; };
readonly restrict layout(std430, binding = SSBO_IDX_MATERIALS) buffer materialsBuffer { Material materials[]; };
in flat int MaterialIndex;

layout (binding = 6) uniform sampler2D RoadMaskTexture;
layout (binding = 7) uniform usampler2D TerrainControlTexture;

in vec2 TexCoord;
in vec3 Normal;
in vec3 Tangent;
in vec3 BiTangent;
in vec3 WorldPos;

uniform float u_discardHeight = 0.01;
uniform int u_terrainMaterial0;
uniform int u_terrainMaterial1;
uniform int u_terrainMaterial2;
uniform int u_terrainMaterial3;

int GetTerrainMaterialIndex(uint materialId) {
    if (materialId == 1u) return u_terrainMaterial1;
    if (materialId == 2u) return u_terrainMaterial2;
    if (materialId == 3u) return u_terrainMaterial3;
    return u_terrainMaterial0;
}

void SampleTerrainMaterial(uint materialId, vec2 uv, vec2 dPdx, vec2 dPdy, out vec4 baseColor, out vec3 normalMap, out vec4 rma) {
    Material material = materials[GetTerrainMaterialIndex(materialId)];
    float textureScale = materialId == 1u ? 2.0 : 1.0;
    baseColor = textureGrad(sampler2D(textureSamplers[material.basecolor]), uv * textureScale, dPdx * textureScale, dPdy * textureScale);
    normalMap = textureGrad(sampler2D(textureSamplers[material.normal]), uv * textureScale, dPdx * textureScale, dPdy * textureScale).rgb;
    rma = textureGrad(sampler2D(textureSamplers[material.rma]), uv * textureScale, dPdx * textureScale, dPdy * textureScale);
}

struct TerrainMaterialAccumulation {
    vec4 baseColor;
    vec3 normalMap;
    vec4 rma;
    float totalWeight;
};

void AccumulateTerrainControlWeights(uint control, float spatialWeight, float autoBlend, inout vec4 materialWeights) {
    uint baseMaterialId = (control >> 27u) & 0x1Fu;
    uint overlayMaterialId = (control >> 22u) & 0x1Fu;
    float terrainBlend = float((control >> 14u) & 0xFFu) / 255.0;

    if ((control & 0x1u) != 0u) {
        baseMaterialId = 0u;
        overlayMaterialId = 2u;
        terrainBlend = autoBlend;
    }

    if (baseMaterialId > 3u) baseMaterialId = 0u;
    if (overlayMaterialId > 3u) overlayMaterialId = 0u;
    if (baseMaterialId == overlayMaterialId) {
        materialWeights[int(baseMaterialId)] += spatialWeight;
        return;
    }
    materialWeights[int(baseMaterialId)] += spatialWeight * (1.0 - terrainBlend);
    materialWeights[int(overlayMaterialId)] += spatialWeight * terrainBlend;
}

TerrainMaterialAccumulation GetTerrainMaterial(vec2 controlPosition, float autoBlend, vec2 uv, vec2 dPdx, vec2 dPdy) {
    ivec2 maximumCoord = textureSize(TerrainControlTexture, 0) - 1;
    ivec2 baseCoord = ivec2(floor(controlPosition));
    vec2 blend = fract(controlPosition);
    vec2 inverseBlend = 1.0 - blend;
    vec4 spatialWeights = vec4(inverseBlend.x * inverseBlend.y, blend.x * inverseBlend.y, inverseBlend.x * blend.y, blend.x * blend.y);
    ivec2 controlCoords[4] = ivec2[4](baseCoord, baseCoord + ivec2(1, 0), baseCoord + ivec2(0, 1), baseCoord + ivec2(1, 1));
    vec4 materialWeights = vec4(0.0);
    TerrainMaterialAccumulation accumulation = TerrainMaterialAccumulation(vec4(0.0), vec3(0.0), vec4(0.0), 0.0);

    for (int i = 0; i < 4; i++) {
        uint control = texelFetch(TerrainControlTexture, clamp(controlCoords[i], ivec2(0), maximumCoord), 0).r;
        AccumulateTerrainControlWeights(control, spatialWeights[i], autoBlend, materialWeights);
    }

    for (uint materialId = 0u; materialId < 4u; materialId++) {
        float materialWeight = materialWeights[int(materialId)];
        if (materialWeight <= 0.0) continue;
        vec4 materialBaseColor;
        vec3 materialNormalMap;
        vec4 materialRma;
        SampleTerrainMaterial(materialId, uv, dPdx, dPdy, materialBaseColor, materialNormalMap, materialRma);
        accumulation.baseColor += materialBaseColor * materialWeight;
        accumulation.normalMap += materialNormalMap * materialWeight;
        accumulation.rma += materialRma * materialWeight;
        accumulation.totalWeight += materialWeight;
    }

    float inverseWeight = 1.0 / max(accumulation.totalWeight, 1e-8);
    accumulation.baseColor *= inverseWeight;
    accumulation.normalMap *= inverseWeight;
    accumulation.rma *= inverseWeight;
    return accumulation;
}

void main() {
    if (WorldPos.y < u_discardHeight) {
        discard;
    }

    mat3 tbn = mat3(normalize(Tangent), normalize(BiTangent), normalize(Normal));
    float terrainSlope = 1.0 - clamp(normalize(Normal).y, 0.0, 1.0);
    float autoBlend = smoothstep(0.15, 0.45, terrainSlope);
    vec2 terrainUvDdx = dFdx(TexCoord);
    vec2 terrainUvDdy = dFdy(TexCoord);
    TerrainMaterialAccumulation terrainMaterial = GetTerrainMaterial(WorldPos.xz / HEIGHTMAP_SCALE_XZ, autoBlend, TexCoord, terrainUvDdx, terrainUvDdy);

    vec2 roadMaskWorldSize = textureSize(RoadMaskTexture, 0) * HEIGHTMAP_SCALE_XZ / 4;
    vec2 roadMaskUV = vec2(WorldPos.x / roadMaskWorldSize.x, WorldPos.z / roadMaskWorldSize.y);
    float roadMask = texture(RoadMaskTexture, roadMaskUV).r;

    vec3 baseColor = terrainMaterial.baseColor.rgb;
    vec3 normalMap = terrainMaterial.normalMap;
    vec4 rma = terrainMaterial.rma;
    if (roadMask > 0.0) {
        vec4 dirtRoadBaseColor;
        vec3 dirtRoadNormalMap;
        vec4 dirtRoadRma;
        SampleTerrainMaterial(1u, TexCoord, terrainUvDdx, terrainUvDdy, dirtRoadBaseColor, dirtRoadNormalMap, dirtRoadRma);
        baseColor = mix(baseColor, dirtRoadBaseColor.rgb, roadMask);
        normalMap = mix(normalMap, dirtRoadNormalMap, roadMask);
        rma = mix(rma, dirtRoadRma, roadMask);
    }

    //vec3 normalMap = texture2D(normalTexture, TexCoord).rgb;
    normalMap.rgb = normalMap.rgb * 2.0 - 1.0;
    normalMap = normalize(normalMap);
    vec3 normal = normalize(tbn * (normalMap));

    float roughness = rma.r;
    float metallic = rma.g;
    float ao = rma.b;


    vec2 velocity = vec2(0.0);

    // Basecolor / Metallic out
    BaseColorMetallicOut.rgb = baseColor.rgb;
    BaseColorMetallicOut.a = metallic;

    // NormalXY / Roughness out
    NormalXYRoughnessMiscOut.rg = EncodeOct(normal);
    NormalXYRoughnessMiscOut.b = roughness;
    NormalXYRoughnessMiscOut.a = 0.0; // Misc 4 bit value

    // Velocity / Occlusion / Subsurface out
    VelocityXYOcclusionSubSurfaceOut.rg = velocity;
    VelocityXYOcclusionSubSurfaceOut.b = ao;
    VelocityXYOcclusionSubSurfaceOut.a = 0.0; // Subsurface. Not quite sure what this is yet

    EmissiveOut = vec4(0.0, 0.0, 0.0, 0.45);
}
