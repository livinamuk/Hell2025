#version 460 core
#extension GL_ARB_bindless_texture : require
#include "../common/OpenGL/GL_binding_indices.glsl"

layout (location = 0) out vec4 FragOut;
in vec2 TexCoord;
in vec2 TexCoordNext;
flat in int TextureIndex;
flat in float MixFactor;

readonly restrict layout(std430, binding = SSBO_IDX_SAMPLERS) buffer textureSamplersBuffer { uvec2 textureSamplers[]; };

void main() {
    sampler2D spriteSheetTexture = sampler2D(textureSamplers[TextureIndex]);
    vec4 color = texture(spriteSheetTexture, TexCoord);
    vec4 colorNext = texture(spriteSheetTexture, TexCoordNext);
    FragOut = mix(color, colorNext, MixFactor);
}
