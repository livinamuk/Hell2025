#version 460 core
#include "../common/OpenGL/GL_binding_indices.glsl"
#extension GL_ARB_bindless_texture : enable
layout (location = 0) out vec4 ColorOut;

in vec4 v_color;
in vec2 v_uv;
in flat uint v_filterMode;
in flat uint v_textureIndex;

readonly restrict layout(std430, binding = SSBO_IDX_SAMPLERS) buffer textureSamplersBuffer {
	uvec2 textureSamplers[];
};

void main() {
    sampler2D textureSampler = sampler2D(textureSamplers[v_textureIndex]);
    vec4 color;

    if (v_filterMode == 1) {
        ivec2 size = textureSize(textureSampler, 0);
        ivec2 texel = ivec2(v_uv * vec2(size));
        texel = clamp(texel, ivec2(0), size - 1);
        color = texelFetch(textureSampler, texel, 0);
    }
    else {
        color = texture(textureSampler, v_uv);
    }

    ColorOut = color;
    ColorOut.rgba *= v_color;
}
