#version 460
#extension GL_EXT_nonuniform_qualifier : require

#include "../common/Vulkan/VK_binding_indices.glsl"

layout(set = 0, binding = DESC_IDX_SAMPLERS) uniform sampler samplers[];
layout(set = 0, binding = DESC_IDX_TEXTURES) uniform texture2D textures[];

layout(location = 0) in vec2 v_uv;
layout(location = 0) out vec4 out_color;

void main() {
    out_color = texture(sampler2D(textures[VULKAN_TEXTURE_IDX_PRESENT], samplers[VULKAN_SAMPLER_IDX_LINEAR]), vec2(v_uv.x, 1.0 - v_uv.y));
    //out_color = vec4(gl_FragCoord.x / 960, gl_FragCoord.y / 540, 0.0, 1.0);
}
