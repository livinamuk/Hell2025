#version 460
#include "../../common/OpenGL/GL_binding_indices.glsl"
#extension GL_ARB_bindless_texture : require
#include "../../common/types.glsl"

layout(location = 0) out uvec2 FragmentOutput;

layout(location = 0) flat in int v_sceneRenderItemIndex;
layout(location = 1) in vec2 v_uv;

readonly restrict layout(std430, binding = SSBO_IDX_SAMPLERS) buffer textureSamplersBuffer { uvec2 textureSamplers[]; };
readonly restrict layout(std430, binding = SSBO_IDX_MATERIALS) buffer materialsBuffer { Material materials[]; };
readonly restrict layout(std430, binding = SSBO_IDX_SCENE_RENDER_ITEMS) buffer sceneRenderItemsBuffer { RenderItem sceneRenderItems[]; };

const float bayerMatrix[16] = float[16](
    0.0,    0.5,    0.125,  0.625,
    0.75,   0.25,   0.875,  0.375,
    0.1875, 0.6875, 0.0625, 0.5625,
    0.9375, 0.4375, 0.8125, 0.3125
);

uniform uint u_frameCount;

void main() {

    RenderItem renderItem = sceneRenderItems[v_sceneRenderItemIndex];
    Material material = materials[renderItem.materialIndex];
    float alpha = texture(sampler2D(textureSamplers[material.basecolor]), v_uv).a;
    //float alpha = texture(sampler2D(textureSamplers[material.opacity]), v_uv).r;

    bool useStochasticDiscard = false;
    float hardAlphaCutoff = 0.25;

    if (useStochasticDiscard) {
        ivec2 pixelCoords = ivec2(gl_FragCoord.xy);

        // Offset jitter coords over time
        ivec2 temporalOffset = ivec2(
            int(u_frameCount % 4),
            int((u_frameCount / 4) % 4)
        );

        ivec2 jitteredCoords = pixelCoords + temporalOffset;
        uint bayerIndex = ((jitteredCoords.y & 3) << 2) | (jitteredCoords.x & 3);
        float ditherThreshold = bayerMatrix[bayerIndex];

        // Stochastic Discard
        float baseCutoff = 0.001;
        if (alpha - baseCutoff < ditherThreshold) {
            discard;
        }
    }
    else {
        // Hard Discard
        if (alpha < hardAlphaCutoff) {
            discard;
        }
    }

    FragmentOutput.x = uint(v_sceneRenderItemIndex);
    FragmentOutput.y = uint(gl_PrimitiveID);
}
