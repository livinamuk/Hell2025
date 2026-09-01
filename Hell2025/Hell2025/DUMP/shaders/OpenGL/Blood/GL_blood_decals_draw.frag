#version 450
#include "../../common/OpenGL/GL_binding_indices.glsl"
#extension GL_ARB_bindless_texture : enable

#include "../../common/constants.glsl"
#include "../../common/flags.glsl"
#include "../../common/reconstruction.glsl"
#include "../../common/util.glsl"
#include "../../common/viewport.glsl"

layout (location = 0) out vec4 DecalMaskOut;

layout(binding = 1) uniform sampler2D GBufferNormalXYRoughnessMiscTexture;
layout(binding = 2) uniform sampler2D u_depthTexture;

layout(std430, binding = SSBO_IDX_SAMPLERS)  readonly restrict buffer textureSamplersBuffer { uvec2 textureSamplers[]; };
layout(std430, binding = SSBO_IDX_RENDERER_DATA)  readonly restrict buffer rendererDataBuffer    { RendererData rendererData; };
layout(std430, binding = SSBO_IDX_VIEWPORT_DATA)  readonly restrict buffer viewportDataBuffer    { ViewportData viewportDataArr[]; };
layout(std430, binding = SSBO_IDX_BLOOD_DRAW_TILE_DECALS)  restrict          buffer tileBloodDecalsBuffer { TileInstanceData tileBloodDecals[]; };
layout(std430, binding = SSBO_IDX_BLOOD_DRAW_DECALS)  readonly restrict buffer BloodDecalBuffer      { BloodDecal bloodDecals[]; };
layout(std430, binding = SSBO_IDX_BLOOD_DRAW_INDEX_POOL) restrict          buffer DecalIndexPool        { uint globalBloodDecalIndices[]; };

uniform int u_tileXCount;
uniform int u_tileYCount;

void main() {
	ivec2 px = ivec2(gl_FragCoord.xy);
    ivec2 outputImageSize = ivec2(rendererData.gBufferWidth, rendererData.gBufferHeight);

    uvec2 tileCoords = uvec2(px) / TILE_SIZE;
    uint tileIndex = tileCoords.y * u_tileXCount + tileCoords.x;
    uint count = tileBloodDecals[tileIndex].count;

    // Skip if this tile has no decals
    if (count == 0) discard;

    vec4 gBufferNormalXYRoughnessMisc = texelFetch(GBufferNormalXYRoughnessMiscTexture, px, 0);
    uint miscFlags = DecodeMiscFlags(gBufferNormalXYRoughnessMisc.a);
    if ((miscFlags & MISC_FLAG_DYNAMIC_OBJECT) != 0u) discard;

    uint viewportIndex = ViewportIndexFromPixel(px, outputImageSize, rendererData.viewportLayout, vec2(rendererData.viewportSplitX, rendererData.viewportSplitY));
    ivec4 viewportRect = ivec4(viewportDataArr[viewportIndex].xOffset, viewportDataArr[viewportIndex].yOffset, viewportDataArr[viewportIndex].width, viewportDataArr[viewportIndex].height);
    vec2 viewportUV = ViewportUVFromPixel(px, outputImageSize, viewportRect);
    mat4 inverseProjectionView = viewportDataArr[viewportIndex].inverseJitteredProjectionViewReverseZ;

    float depth = texelFetch(u_depthTexture, px, 0).r;
    if (depth <= 0.0) discard;

    vec3 worldPos = WorldPosFromDepth(viewportUV, depth, inverseProjectionView);

    vec3 positionDx = dFdx(worldPos);
    vec3 positionDy = dFdy(worldPos);
    vec3 receiverNormalUnnormalized = cross(positionDx, positionDy);
    float receiverNormalLengthSquared = dot(receiverNormalUnnormalized, receiverNormalUnnormalized);

    // Derivative magnitude changes with distance, so reject only a genuinely
    // degenerate or non-finite surface differential.
    if (!(receiverNormalLengthSquared > 0.0) || isinf(receiverNormalLengthSquared)) discard;

    vec3 receiverNormal = receiverNormalUnnormalized * inversesqrt(receiverNormalLengthSquared);

    float bestMask = 0.0;

    uint tileOffset = tileBloodDecals[tileIndex].offset;

    for (uint i = 0; i < count; ++i) {
        uint decalIdx = globalBloodDecalIndices[tileOffset + i];
        mat4 inverseModelMatrix = bloodDecals[decalIdx].inverseModelMatrix;
        vec3 localPos = (inverseModelMatrix * vec4(worldPos, 1.0)).xyz;
        vec2 aspectScale = vec2(bloodDecals[decalIdx].aspectScaleX, bloodDecals[decalIdx].aspectScaleY);
        vec3 decalHalfExtents = vec3(0.5 * aspectScale.x, 0.5 * BLOOD_DECAL_DEPTH_SCALE, 0.5 * aspectScale.y);

        if (any(greaterThan(abs(localPos), decalHalfExtents))) {
            continue;
        }

        // These decals lie in local XZ, with local Y as the projector normal
        vec3 decalNormal = normalize(
            transpose(mat3(inverseModelMatrix)) * vec3(0.0, 1.0, 0.0)
        );
        if (abs(dot(receiverNormal, decalNormal)) < BLOOD_DECAL_MIN_NORMAL_ALIGNMENT) {
            continue;
        }

        vec2 texCoords = (localPos.xz / aspectScale) + 0.5;
        float a = 0.0;

        int textureIndex = bloodDecals[decalIdx].textureIndex;
        if (textureIndex < 0) continue;
        a = texture(sampler2D(textureSamplers[textureIndex]), texCoords).a;
     
        bestMask = max(bestMask, a);

        if (bestMask >= 0.990) {
            break;
        }
    }

    DecalMaskOut = vec4(vec3(bestMask), 1.0);
}
