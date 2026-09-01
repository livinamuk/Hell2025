#version 460
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "../common/Vulkan/VK_push_constants.glsl"

layout(push_constant, scalar) uniform PushConstants {
    PushConstantsDebug2D data;
} pc;

layout(location = 0) in ivec2 a_position;
layout(location = 1) in vec3 a_color;

layout(location = 0) out vec3 v_color;

void main() {
    vec2 pos = vec2(a_position);
    float x = (pos.x / pc.data.renderTargetWidth) * 2.0 - 1.0;
    float y = 1.0 - (pos.y / pc.data.renderTargetHeight) * 2.0;

    v_color = a_color;
    gl_PointSize = 8.0;
    gl_Position = vec4(x, y, 0.0, 1.0);
}
