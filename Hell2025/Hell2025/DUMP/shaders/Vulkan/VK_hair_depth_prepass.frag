#version 460
#extension GL_EXT_nonuniform_qualifier : require

#include "../common/Vulkan/VK_binding_indices.glsl"

layout(set = 0, binding = DESC_IDX_TEXTURES) uniform texture2D textures[];
layout(set = 0, binding = DESC_IDX_TEXTURE_SAMPLERS) uniform sampler textureSamplers[];

layout(location = 1) in vec2 v_uv;
layout(location = 2) flat in int v_baseColorTextureIndex;

void main() {
    uint textureIndex = uint(v_baseColorTextureIndex);
    float alpha = texture(sampler2D(textures[nonuniformEXT(textureIndex)], textureSamplers[nonuniformEXT(textureIndex)]), v_uv).a;
    float mipLevel = textureQueryLod(sampler2D(textures[nonuniformEXT(textureIndex)], textureSamplers[nonuniformEXT(textureIndex)]), v_uv).y;

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
