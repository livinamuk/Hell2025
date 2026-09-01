#version 460
#extension GL_EXT_nonuniform_qualifier : require

#include "../common/Vulkan/VK_binding_indices.glsl"

layout(set = 0, binding = DESC_IDX_TEXTURES) uniform texture2D textures[];
layout(set = 0, binding = DESC_IDX_TEXTURE_SAMPLERS) uniform sampler textureSamplers[];

layout(location = 0) in vec2 v_texCoord;
layout(location = 1) in vec2 v_texCoordNext;
layout(location = 2) flat in int v_textureIndex;
layout(location = 3) flat in float v_mixFactor;

layout(location = 0) out vec4 LightingOut;

void main() {
    if (v_textureIndex < 0) {
        discard;
    }

    uint textureIndex = uint(v_textureIndex);
    vec4 color = texture(sampler2D(textures[nonuniformEXT(textureIndex)], textureSamplers[nonuniformEXT(textureIndex)]), v_texCoord);
    vec4 colorNext = texture(sampler2D(textures[nonuniformEXT(textureIndex)], textureSamplers[nonuniformEXT(textureIndex)]), v_texCoordNext);
    LightingOut = mix(color, colorNext, v_mixFactor);
}
