#version 460
#extension GL_EXT_nonuniform_qualifier : require

#include "../common/Vulkan/VK_binding_indices.glsl"

layout(set = 0, binding = DESC_IDX_SAMPLERS) uniform sampler samplers[];
layout(set = 0, binding = DESC_IDX_TEXTURES) uniform texture2D textures[];

layout(location = 0) in vec2 v_uv;
layout(location = 1) in vec4 v_color;
layout(location = 2) flat in uint v_textureIndex;
layout(location = 3) flat in uint v_filterMode;
layout(location = 0) out vec4 out_color;

void main() {
    uint samplerIndex = min(v_filterMode, 1);
    out_color = texture(sampler2D(textures[nonuniformEXT(v_textureIndex)], samplers[nonuniformEXT(samplerIndex)]), v_uv) * v_color;
}
