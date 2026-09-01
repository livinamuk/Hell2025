#version 460
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "../common/types.glsl"
#include "../common/Vulkan/VK_push_constants.glsl"

layout(push_constant, scalar) uniform PushConstants {
    PushConstantsPointShadow data;
} pc;

layout(location = 0) in vec3 v_worldPosition;
layout(location = 1) flat in vec4 v_lightPositionRadius;

void main() {
    vec3 lightPosition = v_lightPositionRadius.xyz;
    float lightRadius = v_lightPositionRadius.w;
    gl_FragDepth = length(v_worldPosition - lightPosition) / lightRadius;
}
