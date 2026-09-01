#version 460

#include "../common/Vulkan/VK_binding_indices.glsl"

layout(set = 0, binding = DESC_IDX_SAMPLERS) uniform sampler samplers[];
layout(set = 0, binding = DESC_IDX_TEXTURES) uniform texture2D textures[];

layout(location = 0) out vec4 out_color;

void main() {
    ivec2 px = ivec2(gl_FragCoord.xy);
    gl_FragDepth = texelFetch(sampler2D(textures[VULKAN_TEXTURE_IDX_GBUFFER_DEPTH], samplers[VULKAN_SAMPLER_IDX_NEAREST]), px, 0).r;
    out_color = vec4(0.0);
}
