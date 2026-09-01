#version 460
#extension GL_EXT_nonuniform_qualifier : require

#include "../common/Vulkan/VK_binding_indices.glsl"

layout(set = 0, binding = DESC_IDX_SAMPLERS) uniform sampler samplers[];
layout(set = 0, binding = DESC_IDX_TEXTURES) uniform texture2D textures[];
layout(set = 0, binding = DESC_IDX_TEXTURE_SAMPLERS) uniform sampler textureSamplers[];

layout(location = 0) flat in uint v_sceneRenderItemIndex;
layout(location = 1) in vec2 v_uv;
layout(location = 2) flat in int v_baseColorTextureIndex;
layout(location = 0) out uvec2 out_visibility;

void main() {
    if (v_baseColorTextureIndex >= 0) {
        uint textureIndex = uint(v_baseColorTextureIndex);
        float alpha = texture(sampler2D(textures[nonuniformEXT(textureIndex)], textureSamplers[nonuniformEXT(textureIndex)]), v_uv).a;
        if (alpha < 0.25) discard;
    }

    out_visibility = uvec2(v_sceneRenderItemIndex, uint(gl_PrimitiveID));
}
