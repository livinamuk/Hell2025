#version 460
#include "../../common/OpenGL/GL_binding_indices.glsl"

#extension GL_ARB_bindless_texture : enable
readonly restrict layout(std430, binding = SSBO_IDX_SAMPLERS) buffer textureSamplersBuffer { uvec2 textureSamplers[]; };
in flat int OpacityTextureIndex;

in vec2 v_uv;

void main() {
    sampler2D opacitySampler = sampler2D(textureSamplers[OpacityTextureIndex]);
    float alpha = texture(opacitySampler, v_uv).a;
    vec2 textureSizePixels = vec2(textureSize(opacitySampler, 0));

    //vec2 dx = dFdx(v_uv) * textureSizePixels;
    //vec2 dy = dFdy(v_uv) * textureSizePixels;
    //float mipLevel = 0.5 * log2(max(dot(dx, dx), dot(dy, dy)));
    float mipLevel = textureQueryLod(opacitySampler, v_uv).y;

    float alphaPivot = 0.025;
    float alphaSharpness = 0.75;
    float alphaBaseBoost = 1.5;

    float boost = max(alphaBaseBoost, mipLevel * alphaSharpness);
    alpha = clamp((alpha - alphaPivot) * boost + alphaPivot, 0.0, 1.0);

    uint mask =
        (uint(alpha > 0.10) << 0) |
        (uint(alpha > 0.35) << 1) |
        (uint(alpha > 0.65) << 2) |
        (uint(alpha > 0.90) << 3);

    gl_SampleMask[0] = int(mask);
}
