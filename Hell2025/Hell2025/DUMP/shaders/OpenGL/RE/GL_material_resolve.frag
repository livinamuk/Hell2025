#version 460
#extension GL_ARB_bindless_texture : require

layout(early_fragment_tests) in;

#include "../../common/constants.glsl"
#include "../../common/OpenGL/GL_binding_indices.glsl"
#include "../../common/lighting.glsl"
#include "../../common/flags.glsl"
#include "../../common/normal_encoding.glsl"
#include "../../common/post_processing.glsl"
#include "../../common/reconstruction.glsl"
#include "../../common/types.glsl"
#include "../../common/util.glsl"
#include "../../common/viewport.glsl"

layout (location = 0) out vec4 BaseColorMetallicOut;
layout (location = 1) out vec4 NormalXYRoughnessMiscOut;
layout (location = 2) out vec4 VelocityXYOcclusionSubSurfaceOut;

layout(rg32ui, binding = 0) uniform readonly uimage2D u_VisibilityBuffer;
layout(binding = 1) uniform sampler2D u_DepthTexture;
layout(binding = 2) uniform sampler2DArray u_WoundMaskTexture;
uniform bool u_hasPreviousSkinnedPositions;
uniform bool u_woundMaskEnabled;

struct PackedVertex {
    float vx, vy, vz;
    float nx, ny, nz;
    float u, v;
    float tx, ty, tz;
};

struct PackedPosition {
    float x, y, z;
};

vec3 UnpackPosition(PackedPosition position) {
    return vec3(position.x, position.y, position.z);
}

readonly restrict layout(std430, binding = SSBO_IDX_SAMPLERS) buffer textureSamplersBuffer { uvec2 textureSamplers[]; };
readonly restrict layout(std430, binding = SSBO_IDX_MATERIALS) buffer materialsBuffer { Material materials[]; };
readonly restrict layout(std430, binding = SSBO_IDX_RENDERER_DATA) buffer rendererDataBuffer { RendererData rendererData; };
readonly restrict layout(std430, binding = SSBO_IDX_VIEWPORT_DATA) buffer viewportDataBuffer { ViewportData viewportDataArr[]; };
readonly restrict layout(std430, binding = SSBO_IDX_SCENE_RENDER_ITEMS) buffer sceneRenderItemsBuffer { RenderItem sceneRenderItems[]; };
readonly restrict layout(std430, binding = SSBO_IDX_MATERIAL_RESOLVE_VERTICES) buffer vertexBuffer { PackedVertex vertices[]; };
readonly restrict layout(std430, binding = SSBO_IDX_MATERIAL_RESOLVE_INDICES) buffer indexBuffer { uint indices[]; };
readonly restrict layout(std430, binding = SSBO_IDX_MATERIAL_RESOLVE_PREVIOUS_POSITIONS) buffer previousPositionBuffer { PackedPosition previousSkinnedPositions[]; };

float Cross2D(vec2 a, vec2 b) {
    return a.x * b.y - a.y * b.x;
}

vec3 ComputeScreenBarycentrics(vec2 p, vec2 s0, vec2 s1, vec2 s2, vec3 invW) {
    vec2 a = s0 - p;
    vec2 b = s1 - p;
    vec2 c = s2 - p;

    float area = Cross2D(b - a, c - a);
    if (abs(area) < 1e-20) {
        return vec3(1.0, 0.0, 0.0);
    }

    float u = Cross2D(b, c) / area;
    float v = Cross2D(c, a) / area;
    float w = Cross2D(a, b) / area;

    float interpW = u * invW.x + v * invW.y + w * invW.z;
    if (abs(interpW) < 1e-20) {
        return vec3(1.0, 0.0, 0.0);
    }

    return vec3(u * invW.x, v * invW.y, w * invW.z) / interpW;
}

void main() {
    ivec2 px =  ivec2(gl_FragCoord.xy);;
    ivec2 outputImageSize = imageSize(u_VisibilityBuffer);

    uint viewportIndex = ViewportIndexFromPixel(px, outputImageSize, rendererData.viewportLayout, vec2(rendererData.viewportSplitX, rendererData.viewportSplitY));
    ivec4 viewportRect = ivec4(viewportDataArr[viewportIndex].xOffset, viewportDataArr[viewportIndex].yOffset, viewportDataArr[viewportIndex].width, viewportDataArr[viewportIndex].height);
    vec2 viewportSize = vec2(viewportRect.zw);
    mat4 inverseProjectionView = viewportDataArr[viewportIndex].inverseJitteredProjectionViewReverseZ;
    mat4 jitteredProjectionView = viewportDataArr[viewportIndex].jitteredProjectionViewReverseZ;
    mat4 projectionView = viewportDataArr[viewportIndex].projectionViewReverseZ;
    mat4 prevProjectionView = viewportDataArr[viewportIndex].prevProjectionViewReverseZ;
    vec3 viewPosition = viewportDataArr[viewportIndex].viewPos.xyz;
    vec2 viewportUV = ViewportUVFromPixel(px, outputImageSize, viewportRect);

    uvec4 visibilityData = imageLoad(u_VisibilityBuffer, px);
    uint sceneRenderItemIndex = visibilityData.x;
    uint primitiveID = visibilityData.y;

    RenderItem renderItem = sceneRenderItems[sceneRenderItemIndex];
    Material material = materials[renderItem.materialIndex];
    uint triangleIndexOffset = renderItem.baseIndex + (primitiveID * 3);

    uint i0 = indices[triangleIndexOffset + 0] + renderItem.baseVertex;
    uint i1 = indices[triangleIndexOffset + 1] + renderItem.baseVertex;
    uint i2 = indices[triangleIndexOffset + 2] + renderItem.baseVertex;

    PackedVertex v0 = vertices[i0];
    PackedVertex v1 = vertices[i1];
    PackedVertex v2 = vertices[i2];

    // Position from depth reconstruction
    float depth = texelFetch(u_DepthTexture, px, 0).r;
    vec3 worldPos = WorldPosFromDepth(viewportUV, depth, inverseProjectionView);

    // Transform vertices to world space
    mat4 modelMatrix = renderItem.modelMatrix;
    vec3 ws0 = (modelMatrix * vec4(v0.vx, v0.vy, v0.vz, 1.0)).xyz;
    vec3 ws1 = (modelMatrix * vec4(v1.vx, v1.vy, v1.vz, 1.0)).xyz;
    vec3 ws2 = (modelMatrix * vec4(v2.vx, v2.vy, v2.vz, 1.0)).xyz;

    // Calculate world space edges
    vec3 viewDir = normalize(worldPos - viewPosition);
    vec3 e1 = ws1 - ws0;
    vec3 e2 = ws2 - ws0;
    vec3 geoNormal = normalize(cross(e1, e2));
    bool isFrontFacing = dot(geoNormal, viewDir) <= 0.0;

    // Match the jittered raster positions that produced gl_FragCoord.
    vec4 clip0 = jitteredProjectionView * vec4(ws0, 1.0);
    vec4 clip1 = jitteredProjectionView * vec4(ws1, 1.0);
    vec4 clip2 = jitteredProjectionView * vec4(ws2, 1.0);

    vec2 p = ViewportNDCFromPixel(px, outputImageSize, viewportRect);

    vec2 s0 = clip0.xy / clip0.w;
    vec2 s1 = clip1.xy / clip1.w;
    vec2 s2 = clip2.xy / clip2.w;

    vec3 invW = vec3(1.0 / clip0.w, 1.0 / clip1.w, 1.0 / clip2.w);

    vec2 pixelStep = 2.0 / viewportSize;

    vec3 bary  = ComputeScreenBarycentrics(p, s0, s1, s2, invW);
    vec3 baryX = ComputeScreenBarycentrics(p + vec2(pixelStep.x, 0.0), s0, s1, s2, invW);
    vec3 baryY = ComputeScreenBarycentrics(p + vec2(0.0, -pixelStep.y), s0, s1, s2, invW);

    vec2 uv0 = vec2(v0.u, v0.v);
    vec2 uv1 = vec2(v1.u, v1.v);
    vec2 uv2 = vec2(v2.u, v2.v);

    vec2 uv  = uv0 * bary.x  + uv1 * bary.y  + uv2 * bary.z;
    vec2 uvX = uv0 * baryX.x + uv1 * baryX.y + uv2 * baryX.z;
    vec2 uvY = uv0 * baryY.x + uv1 * baryY.y + uv2 * baryY.z;

    vec2 dPdx = uvX - uv;
    vec2 dPdy = uvY - uv;
    dPdx = clamp(dPdx, vec2(-1.0), vec2(1.0));
    dPdy = clamp(dPdy, vec2(-1.0), vec2(1.0));

    mat4 normalMatrix = transpose(renderItem.inverseModelMatrix);

    vec3 n0 = normalize(normalMatrix * vec4(v0.nx, v0.ny, v0.nz, 0.0)).xyz;
    vec3 n1 = normalize(normalMatrix * vec4(v1.nx, v1.ny, v1.nz, 0.0)).xyz;
    vec3 n2 = normalize(normalMatrix * vec4(v2.nx, v2.ny, v2.nz, 0.0)).xyz;
    vec3 n  = normalize(n0 * bary.x  + n1 * bary.y  + n2 * bary.z);

    vec3 nX = normalize(n0 * baryX.x + n1 * baryX.y + n2 * baryX.z);
    vec3 nY = normalize(n0 * baryY.x + n1 * baryY.y + n2 * baryY.z);

    vec3 t0 = normalize(modelMatrix * vec4(v0.tx, v0.ty, v0.tz, 0.0)).xyz;
    vec3 t1 = normalize(modelMatrix * vec4(v1.tx, v1.ty, v1.tz, 0.0)).xyz;
    vec3 t2 = normalize(modelMatrix * vec4(v2.tx, v2.ty, v2.tz, 0.0)).xyz;
    vec3 t = normalize(t0 * bary.x + t1 * bary.y + t2 * bary.z);

    if (!isFrontFacing) {
        n = -n;
        nX = -nX;
        nY = -nY;
    }

    t = normalize(t - dot(t, n) * n);
    vec3 b = cross(n, t);
    mat3 tbn = mat3(t, b, n);

    vec4 baseColor;
    vec3 normalMap;
    vec4 rma;

    baseColor = textureGrad(sampler2D(textureSamplers[material.basecolor]), uv, dPdx, dPdy);
    normalMap = textureGrad(sampler2D(textureSamplers[material.normal]), uv, dPdx, dPdy).rgb;
    rma = textureGrad(sampler2D(textureSamplers[material.rma]), uv, dPdx, dPdy).rgba;

    if (u_woundMaskEnabled && renderItem.woundMaskTextureIndex != -1 && renderItem.woundMaterialIndex != -1) {
        Material woundMaterial = materials[renderItem.woundMaterialIndex];
        vec4 woundBaseColor = textureGrad(sampler2D(textureSamplers[woundMaterial.basecolor]), uv, dPdx, dPdy);
        vec3 woundNormalMap = textureGrad(sampler2D(textureSamplers[woundMaterial.normal]), uv, dPdx, dPdy).rgb;
        vec3 woundRma = textureGrad(sampler2D(textureSamplers[woundMaterial.rma]), uv, dPdx, dPdy).rgb;
        float woundMask = textureGrad(u_WoundMaskTexture, vec3(uv, float(renderItem.woundMaskTextureIndex)), dPdx, dPdy).r;

        woundMask = clamp(woundMask * 1.25, 0.0, 1.0);
        float woundDarken = clamp(pow(woundMask, 0.1) * 0.3, 0.0, 1.0);
        woundBaseColor.rgb = mix(woundBaseColor.rgb, vec3(0.0), woundDarken);
        woundRma.r = mix(woundRma.r, 0.0, woundDarken * 2.0);
        woundRma.b = mix(woundRma.b, 0.0, woundDarken * 2.0);

        baseColor = mix(baseColor, woundBaseColor, woundMask);
        normalMap = mix(normalMap, woundNormalMap, woundMask);
        rma.rgb = mix(rma.rgb, woundRma, woundMask);
    }

    float roughness = clamp(rma.r * renderItem.roughnessFactor, 0.0, 1.0);
    float metallic  = clamp(rma.g * renderItem.metallicFactor, 0.0, 1.0);
    float ao = rma.b;

    normalMap = normalMap * 2.0 - 1.0;
    vec3 normal = normalize(tbn * normalMap);

    vec3 dNdx = nX - n;
    vec3 dNdy = nY - n;
    float variance = (dot(dNdx, dNdx) + dot(dNdy, dNdy)) * 0.1591549;
    roughness = sqrt(clamp(roughness * roughness + min(variance, 0.18), 0.0, 1.0));

    vec4 currPos;
    vec4 prevPos;

    if (u_hasPreviousSkinnedPositions) {
        vec3 currentLocalPos =
            vec3(v0.vx, v0.vy, v0.vz) * bary.x +
            vec3(v1.vx, v1.vy, v1.vz) * bary.y +
            vec3(v2.vx, v2.vy, v2.vz) * bary.z;

        vec3 previousLocalPos =
            UnpackPosition(previousSkinnedPositions[i0]) * bary.x +
            UnpackPosition(previousSkinnedPositions[i1]) * bary.y +
            UnpackPosition(previousSkinnedPositions[i2]) * bary.z;

        currPos = projectionView * renderItem.modelMatrix * vec4(currentLocalPos, 1.0);
        prevPos = prevProjectionView * renderItem.prevModelMatrix * vec4(previousLocalPos, 1.0);
    }
    else {
        vec4 localPos = renderItem.inverseModelMatrix * vec4(worldPos, 1.0);
        currPos = projectionView * vec4(worldPos, 1.0);
        prevPos = prevProjectionView * renderItem.prevModelMatrix * localPos;
    }

    vec2 currNDC = currPos.xy / currPos.w;
    vec2 prevNDC = prevPos.xy / prevPos.w;
    // Match FidelityFX's motion-vector input contract: store the raw
    // current-minus-previous displacement in clip-space NDC. Consumers are
    // responsible for converting this to texture-UV displacement.
    vec2 velocityNDC = currNDC - prevNDC;

    BaseColorMetallicOut.rgb = baseColor.rgb;
    BaseColorMetallicOut.a = metallic;

    NormalXYRoughnessMiscOut.rg = EncodeOct(normal);
    NormalXYRoughnessMiscOut.b = roughness;
    NormalXYRoughnessMiscOut.a = EncodeMiscFlags(renderItem.miscFlags);

    VelocityXYOcclusionSubSurfaceOut.rg = velocityNDC;
    VelocityXYOcclusionSubSurfaceOut.b = ao;
    VelocityXYOcclusionSubSurfaceOut.a = 0.0;
}
