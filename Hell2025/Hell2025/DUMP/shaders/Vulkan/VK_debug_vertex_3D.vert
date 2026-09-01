#version 460
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "../common/types.glsl"
#include "../common/Vulkan/VK_push_constants.glsl"

layout(push_constant, scalar) uniform PushConstants {
    PushConstantsDebug3D data;
} pc;

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_color;
layout(location = 2) in ivec2 a_pixelOffset;
layout(location = 3) in int a_depthEnabled;
layout(location = 4) in int a_exclusiveViewportIndex;
layout(location = 5) in int a_ignoredViewportIndex;

layout(location = 0) out vec3 v_color;

void main() {
    ViewportDataBuffer viewportData = pc.data.frame.viewportDataBuffer;
    mat4 projectionView = viewportData.viewportData[pc.data.viewportIndex].projectionViewReverseZ;
    vec2 viewportSize = vec2(viewportData.viewportData[pc.data.viewportIndex].width, viewportData.viewportData[pc.data.viewportIndex].height);

    v_color = a_color;
    gl_Position = projectionView * vec4(a_position, 1.0);

    // Convert the pixel offset to clip space
    gl_Position.xy += vec2(a_pixelOffset) * (2.0 / viewportSize) * gl_Position.w;
    gl_PointSize = 8.0;

    if (a_exclusiveViewportIndex != -1 && a_exclusiveViewportIndex != int(pc.data.viewportIndex)) {
        gl_Position = vec4(0, 0, 0, 0);
    }
    if (a_ignoredViewportIndex != -1 && a_ignoredViewportIndex == int(pc.data.viewportIndex)) {
        gl_Position = vec4(0, 0, 0, 0);
    }
    if (a_depthEnabled != 1) {
        gl_Position.z = 0.99999 * gl_Position.w;
    }
}
