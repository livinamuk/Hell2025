#version 460
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

layout(early_fragment_tests) in;

#include "../common/types.glsl"
#include "../common/Vulkan/VK_binding_indices.glsl"
#include "../common/Vulkan/VK_push_constants.glsl"

layout(set = 0, binding = DESC_IDX_TEXTURES) uniform texture2D textures[];
layout(set = 0, binding = DESC_IDX_TEXTURE_SAMPLERS) uniform sampler textureSamplers[];

layout(push_constant, scalar) uniform PushConstants {
    PushConstantsEmissive data;
} pc;

layout(location = 0) out vec4 EmissiveOut;

layout(location = 0) in vec2 v_uv;
layout(location = 1) flat in uint v_globalInstanceIndex;

void main() {
    RenderItem renderItem = pc.data.frame.sceneRenderItemBuffer.renderItems[v_globalInstanceIndex];
    Material material = pc.data.frame.materialBuffer.materials[renderItem.materialIndex];
    vec3 emissiveColor = vec3(renderItem.emissiveR, renderItem.emissiveG, renderItem.emissiveB);

    if (material.emissive != -1) {
        int emissiveTextureIndex = material.emissive;
        emissiveColor *= texture(sampler2D(textures[nonuniformEXT(emissiveTextureIndex)], textureSamplers[nonuniformEXT(emissiveTextureIndex)]), v_uv).rgb;
    }

    EmissiveOut = vec4(emissiveColor, 1.0);
}
