#version 460
#extension GL_ARB_bindless_texture : require

layout(early_fragment_tests) in;

#include "../../common/constants.glsl"
#include "../../common/terrain_projection.glsl"
#include "../../common/OpenGL/GL_binding_indices.glsl"
#include "../../common/normal_encoding.glsl"
#include "../../common/reconstruction.glsl"
#include "../../common/types.glsl"
#include "../../common/viewport.glsl"

layout(location = 0) out vec4 BaseColorMetallicOut;
layout(location = 1) out vec4 NormalXYRoughnessMiscOut;
layout(location = 2) out vec4 VelocityXYOcclusionSubSurfaceOut;

layout(rg32ui, binding = 0) uniform readonly uimage2D u_VisibilityBuffer;
layout(binding = 1) uniform sampler2D u_DepthTexture;
layout(binding = 4) uniform usampler2D u_TerrainControlTexture;
layout(binding = 5) uniform sampler2D u_HeightMapTexture;

readonly restrict layout(std430, binding = SSBO_IDX_SAMPLERS) buffer textureSamplersBuffer { uvec2 textureSamplers[]; };
readonly restrict layout(std430, binding = SSBO_IDX_MATERIALS) buffer materialsBuffer { Material materials[]; };
readonly restrict layout(std430, binding = SSBO_IDX_RENDERER_DATA) buffer rendererDataBuffer { RendererData rendererData; };
readonly restrict layout(std430, binding = SSBO_IDX_VIEWPORT_DATA) buffer viewportDataBuffer { ViewportData viewportDataArr[]; };

uniform int u_terrainMaterial0;
uniform int u_terrainMaterial1;
uniform int u_terrainMaterial2;
uniform int u_terrainMaterial3;
uniform float u_textureScaling;
uniform float blend_sharpness = 0.5;

struct TerrainMaterialAccumulation {
    vec4 baseColor;
    vec3 normalMap;
    vec4 rma;
    float totalWeight;
};

int GetTerrainMaterialIndex(uint materialId) {
    if (materialId == 1u) return u_terrainMaterial1;
    if (materialId == 2u) return u_terrainMaterial2;
    if (materialId == 3u) return u_terrainMaterial3;
    return u_terrainMaterial0;
}

float GetTerrainUvScale(uint materialId) {
    return materialId == 1u ? 2.0 : 1.0;
}

void SampleTerrainMaterial(
    uint materialId,
    vec3 terrainTexturePosition,
    vec3 terrainTextureDPdx,
    vec3 terrainTextureDPdy,
    vec3 terrainNormal,
    out vec4 baseColor,
    out vec3 normalMap,
    out vec4 rma,
    out float height) {

    Material material = materials[GetTerrainMaterialIndex(materialId)];
    float textureScale = GetTerrainUvScale(materialId);
    vec2 projectedUv;
    vec2 projectedDPdx;
    vec2 projectedDPdy;
    mat2 projectedNormalAlignment;
    bool verticallyProjected = GetTerrainTextureProjection(
        materialId,
        terrainTexturePosition,
        terrainNormal,
        terrainTextureDPdx,
        terrainTextureDPdy,
        projectedUv,
        projectedDPdx,
        projectedDPdy,
        projectedNormalAlignment);

    vec2 scaledUv = projectedUv * textureScale;
    vec2 scaledDPdx = projectedDPdx * textureScale;
    vec2 scaledDPdy = projectedDPdy * textureScale;
    baseColor = textureGrad(sampler2D(textureSamplers[material.basecolor]), scaledUv, scaledDPdx, scaledDPdy);
    normalMap = textureGrad(sampler2D(textureSamplers[material.normal]), scaledUv, scaledDPdx, scaledDPdy).rgb;
    rma = textureGrad(sampler2D(textureSamplers[material.rma]), scaledUv, scaledDPdx, scaledDPdy);
    height = textureGrad(sampler2D(textureSamplers[material.disp]), scaledUv, scaledDPdx, scaledDPdy).r;

    if (verticallyProjected) {
        // Terrain3D rotates the two tangent-plane components into the selected
        // vertical projection plane before transforming the normal to world space.
        vec3 unpackedNormal = normalize(fma(normalMap, vec3(2.0), vec3(-1.0)));
        unpackedNormal.xy = unpackedNormal.xy * projectedNormalAlignment;
        normalMap = fma(unpackedNormal, vec3(0.5), vec3(0.5));
    }
}

uint ResolveTerrainControl(uint control, float autoBlend) {
    if ((control & 0x1u) != 0u) {
        control =
            ((0u & 0x1Fu) << 27u) |
            ((2u & 0x1Fu) << 22u) |
            ((uint(fma(autoBlend, 255.0, 0.5)) & 0xFFu) << 14u);
    }
    return control;
}

void AccumulateTerrainMaterial(
    float weight,
    uint control,
    vec2 textureWeight,
    ivec2 textureId,
    vec3 terrainTexturePosition,
    vec3 terrainTextureDPdx,
    vec3 terrainTextureDPdy,
    vec3 terrainNormal,
    mat3 tbn,
    inout TerrainMaterialAccumulation accumulation) {

    float worldNormal = 1.0;
    float blend = float((control >> 14u) & 0xFFu) / 255.0;
    float sharpness = fma(60.0, blend_sharpness, 4.0);

    if (blend < 1.0) {
        uint id = uint(textureId.x);
        vec4 materialBaseColor;
        vec3 materialNormalMap;
        vec4 materialRma;
        float materialHeight;
        SampleTerrainMaterial(id, terrainTexturePosition, terrainTextureDPdx, terrainTextureDPdy, terrainNormal,
            materialBaseColor, materialNormalMap, materialRma, materialHeight);

        vec3 unpackedNormal = normalize(fma(materialNormalMap, vec3(2.0), vec3(-1.0)));
        worldNormal = normalize(tbn * unpackedNormal).y;
        float idWeight = exp2(sharpness * log2(weight + textureWeight.x + materialHeight)) * weight;
        accumulation.baseColor = fma(materialBaseColor, vec4(idWeight), accumulation.baseColor);
        accumulation.normalMap = fma(materialNormalMap, vec3(idWeight), accumulation.normalMap);
        accumulation.rma = fma(materialRma, vec4(idWeight), accumulation.rma);
        accumulation.totalWeight += idWeight;
    }

    if (blend > 0.0 && textureId.y != textureId.x) {
        uint id = uint(textureId.y);
        vec4 materialBaseColor;
        vec3 materialNormalMap;
        vec4 materialRma;
        float materialHeight;
        SampleTerrainMaterial(id, terrainTexturePosition, terrainTextureDPdx, terrainTextureDPdy, terrainNormal,
            materialBaseColor, materialNormalMap, materialRma, materialHeight);

        float idWeight = exp2(sharpness * log2(weight + textureWeight.y + materialHeight * clamp(worldNormal, 0.0, 1.0))) * weight;
        accumulation.baseColor = fma(materialBaseColor, vec4(idWeight), accumulation.baseColor);
        accumulation.normalMap = fma(materialNormalMap, vec3(idWeight), accumulation.normalMap);
        accumulation.rma = fma(materialRma, vec4(idWeight), accumulation.rma);
        accumulation.totalWeight += idWeight;
    }
}

TerrainMaterialAccumulation GetTerrainMaterial(
    vec2 controlPosition,
    float autoBlend,
    vec3 terrainTexturePosition,
    vec3 terrainTextureDPdx,
    vec3 terrainTextureDPdy,
    vec3 terrainNormals[4],
    mat3 tbn) {

    ivec2 maximumCoord = textureSize(u_TerrainControlTexture, 0) - 1;
    ivec2 baseCoord = ivec2(floor(controlPosition));
    vec2 blend = fract(controlPosition);
    vec2 inverseBlend = 1.0 - blend;

    // Terrain3D's spatial ordering is TL, TR, BR, BL.
    vec4 spatialWeights = vec4(
        inverseBlend.x * blend.y,
        blend.x * blend.y,
        blend.x * inverseBlend.y,
        inverseBlend.x * inverseBlend.y);
    ivec2 controlCoords[4] = ivec2[4](
        baseCoord + ivec2(0, 1),
        baseCoord + ivec2(1, 1),
        baseCoord + ivec2(1, 0),
        baseCoord);

    uint controls[4];
    ivec2 textureIds[4];
    vec2 textureWeights[4];
    vec4 weightsId1;
    vec4 weightsId0;
    TerrainMaterialAccumulation accumulation = TerrainMaterialAccumulation(vec4(0.0), vec3(0.0), vec4(0.0), 0.0);

    for (int i = 0; i < 4; i++) {
        controls[i] = texelFetch(u_TerrainControlTexture, clamp(controlCoords[i], ivec2(0), maximumCoord), 0).r;
        controls[i] = ResolveTerrainControl(controls[i], autoBlend);

        uint baseMaterialId = (controls[i] >> 27u) & 0x1Fu;
        uint overlayMaterialId = (controls[i] >> 22u) & 0x1Fu;
        if (baseMaterialId > 3u) baseMaterialId = 0u;
        if (overlayMaterialId > 3u) overlayMaterialId = 0u;
        textureIds[i] = ivec2(baseMaterialId, overlayMaterialId);
        weightsId1[i] = float((controls[i] >> 14u) & 0xFFu) / 255.0;
        weightsId0[i] = 1.0 - weightsId1[i];
        textureWeights[i] = vec2(weightsId0[i], weightsId1[i]);
    }

    weightsId0 *= spatialWeights;
    weightsId1 *= spatialWeights;
    for (int target = 0; target < 4; target++) {
        textureWeights[target] = vec2(0.0);
        for (int source = 0; source < 4; source++) {
            textureWeights[target] += fma(
                vec2(weightsId0[source]),
                vec2(equal(textureIds[target], textureIds[source].xx)),
                vec2(weightsId1[source]) * vec2(equal(textureIds[target], textureIds[source].yy)));
        }
    }

    AccumulateTerrainMaterial(spatialWeights[3], controls[3], textureWeights[3], textureIds[3], terrainTexturePosition,
        terrainTextureDPdx, terrainTextureDPdy, terrainNormals[3], tbn, accumulation);
    AccumulateTerrainMaterial(spatialWeights[2], controls[2], textureWeights[2], textureIds[2], terrainTexturePosition,
        terrainTextureDPdx, terrainTextureDPdy, terrainNormals[2], tbn, accumulation);
    AccumulateTerrainMaterial(spatialWeights[1], controls[1], textureWeights[1], textureIds[1], terrainTexturePosition,
        terrainTextureDPdx, terrainTextureDPdy, terrainNormals[1], tbn, accumulation);
    AccumulateTerrainMaterial(spatialWeights[0], controls[0], textureWeights[0], textureIds[0], terrainTexturePosition,
        terrainTextureDPdx, terrainTextureDPdy, terrainNormals[0], tbn, accumulation);

    float inverseWeight = 1.0 / max(accumulation.totalWeight, 1e-8);
    accumulation.baseColor *= inverseWeight;
    accumulation.normalMap *= inverseWeight;
    accumulation.rma *= inverseWeight;
    return accumulation;
}

float GetTerrainHeight(ivec2 coord) {
    ivec2 maximumCoord = textureSize(u_HeightMapTexture, 0) - 1;
    return texelFetch(u_HeightMapTexture, clamp(coord, ivec2(0), maximumCoord), 0).r * HEIGHTMAP_SCALE_Y;
}

void GetTerrainGeometry(
    vec2 controlPosition,
    out vec3 indexNormal[4],
    out vec3 smoothNormal,
    out float terrainHeight) {

    ivec2 baseCoord = ivec2(floor(controlPosition));
    vec2 fractional = fract(controlPosition);
    vec2 inverseFractional = 1.0 - fractional;
    vec4 weights = vec4(
        inverseFractional.x * fractional.y,
        fractional.x * fractional.y,
        fractional.x * inverseFractional.y,
        inverseFractional.x * inverseFractional.y);

    float h[4];
    h[3] = GetTerrainHeight(baseCoord);
    h[2] = GetTerrainHeight(baseCoord + ivec2(1, 0));
    h[0] = GetTerrainHeight(baseCoord + ivec2(0, 1));
    h[1] = GetTerrainHeight(baseCoord + ivec2(1, 1));
    float h4 = GetTerrainHeight(baseCoord + ivec2(1, 2));
    float h5 = GetTerrainHeight(baseCoord + ivec2(2, 1));
    float h6 = GetTerrainHeight(baseCoord + ivec2(2, 0));
    float h7 = GetTerrainHeight(baseCoord + ivec2(0, 2));

    indexNormal[3] = normalize(vec3(h[3] - h[2], HEIGHTMAP_SCALE_XZ, h[3] - h[0]));
    indexNormal[0] = normalize(vec3(h[0] - h[1], HEIGHTMAP_SCALE_XZ, h[0] - h7));
    indexNormal[1] = normalize(vec3(h[1] - h5, HEIGHTMAP_SCALE_XZ, h[1] - h4));
    indexNormal[2] = normalize(vec3(h[2] - h6, HEIGHTMAP_SCALE_XZ, h[2] - h[1]));

    smoothNormal = normalize(
        indexNormal[0] * weights[0] +
        indexNormal[1] * weights[1] +
        indexNormal[2] * weights[2] +
        indexNormal[3] * weights[3]);
    terrainHeight =
        h[0] * weights[0] +
        h[1] * weights[1] +
        h[2] * weights[2] +
        h[3] * weights[3];
}

void main() {
    ivec2 px = ivec2(gl_FragCoord.xy);
    ivec2 outputImageSize = imageSize(u_VisibilityBuffer);
    uint viewportIndex = ViewportIndexFromPixel(px, outputImageSize, rendererData.viewportLayout, vec2(rendererData.viewportSplitX, rendererData.viewportSplitY));
    ivec4 viewportRect = ivec4(
        viewportDataArr[viewportIndex].xOffset,
        viewportDataArr[viewportIndex].yOffset,
        viewportDataArr[viewportIndex].width,
        viewportDataArr[viewportIndex].height);
    vec2 viewportUV = ViewportUVFromPixel(px, outputImageSize, viewportRect);

    uvec4 visibilityData = imageLoad(u_VisibilityBuffer, px);
    vec2 baseUv = vec2(uintBitsToFloat(visibilityData.x), uintBitsToFloat(visibilityData.y));
    float depth = texelFetch(u_DepthTexture, px, 0).r;
    vec3 worldPos = WorldPosFromDepth(viewportUV, depth, viewportDataArr[viewportIndex].inverseJitteredProjectionViewReverseZ);

    vec2 controlPosition = baseUv * vec2(textureSize(u_TerrainControlTexture, 0) - 1);
    vec3 terrainNormals[4];
    vec3 baseTerrainNormal;
    float terrainHeight;
    GetTerrainGeometry(controlPosition, terrainNormals, baseTerrainNormal, terrainHeight);

    float textureRepeat = 50.0 * u_textureScaling;
    float textureWorldScale = textureRepeat /
        (float(max(textureSize(u_TerrainControlTexture, 0).x - 1, 1)) * HEIGHTMAP_SCALE_XZ);
    vec3 terrainTexturePosition = vec3(baseUv.x * textureRepeat, terrainHeight * textureWorldScale, baseUv.y * textureRepeat);
    vec3 terrainTextureDPdx = clamp(dFdx(terrainTexturePosition), vec3(-1.0), vec3(1.0));
    vec3 terrainTextureDPdy = clamp(dFdy(terrainTexturePosition), vec3(-1.0), vec3(1.0));

    vec3 tangent = normalize(cross(baseTerrainNormal, vec3(0.0, 0.0, 1.0)));
    vec3 binormal = normalize(cross(baseTerrainNormal, tangent));
    mat3 tbn = mat3(tangent, binormal, baseTerrainNormal);

    float terrainSlope = 1.0 - clamp(baseTerrainNormal.y, 0.0, 1.0);
    float autoBlend = smoothstep(0.15, 0.45, terrainSlope);
    TerrainMaterialAccumulation terrainMaterial = GetTerrainMaterial(
        controlPosition,
        autoBlend,
        terrainTexturePosition,
        terrainTextureDPdx,
        terrainTextureDPdy,
        terrainNormals,
        tbn);

    vec3 normalMap = fma(terrainMaterial.normalMap, vec3(2.0), vec3(-1.0));
    vec3 normal = normalize(tbn * normalMap);
    float roughness = terrainMaterial.rma.r;
    float metallic = terrainMaterial.rma.g;
    float ao = terrainMaterial.rma.b;

    vec3 dNdx = dFdx(baseTerrainNormal);
    vec3 dNdy = dFdy(baseTerrainNormal);
    float variance = (dot(dNdx, dNdx) + dot(dNdy, dNdy)) * 0.1591549;
    roughness = sqrt(clamp(roughness * roughness + min(variance, 0.18), 0.0, 1.0));

    vec4 currentClip = viewportDataArr[viewportIndex].projectionViewReverseZ * vec4(worldPos, 1.0);
    vec4 previousClip = viewportDataArr[viewportIndex].prevProjectionViewReverseZ * vec4(worldPos, 1.0);
    vec2 velocityNdc = currentClip.xy / currentClip.w - previousClip.xy / previousClip.w;

    BaseColorMetallicOut = vec4(terrainMaterial.baseColor.rgb, metallic);
    NormalXYRoughnessMiscOut = vec4(EncodeOct(normal), roughness, 0.0);
    VelocityXYOcclusionSubSurfaceOut = vec4(velocityNdc, ao, 0.0);
}
