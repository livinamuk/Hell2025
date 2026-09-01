#version 460
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "../common/types.glsl"
#include "../common/Vulkan/VK_push_constants.glsl"

layout(push_constant, scalar) uniform PushConstants {
    PushConstantsDDGIPointCloudDebug data;
} pc;

layout(buffer_reference, scalar) readonly buffer PointCloudBuffer {
    CloudPoint points[];
};

layout(buffer_reference, scalar) readonly buffer PointCloudDirtyFlagsBuffer {
    uint pointCloudDirtyFlags[];
};

layout(location = 0) out vec3 v_color;

void main() {
    if (uint(gl_VertexIndex) >= pc.data.pointCount) {
        v_color = vec3(0.0);
        gl_Position = vec4(0.0);
        return;
    }

    ViewportDataBuffer viewportData = pc.data.frame.viewportDataBuffer;
    PointCloudBuffer pointCloud = PointCloudBuffer(pc.data.pointCloudDeviceAddress);
    PointCloudDirtyFlagsBuffer dirtyFlags = PointCloudDirtyFlagsBuffer(pc.data.pointCloudDirtyFlagsDeviceAddress);

    CloudPoint point = pointCloud.points[gl_VertexIndex];
    vec3 position = point.position.xyz + point.normal.xyz * 0.01;

    mat4 projectionView = viewportData.viewportData[pc.data.viewportIndex].projectionViewReverseZ;
    gl_Position = projectionView * vec4(position, 1.0);
    gl_PointSize = 8.0;

    v_color = point.directLightingRGB_dirty.rgb;
    if (dirtyFlags.pointCloudDirtyFlags[gl_VertexIndex] == 1u) {
        v_color.r += 0.5;
    }
}
