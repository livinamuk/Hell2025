#version 460
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_ARB_shader_draw_parameters : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "../common/constants.glsl"
#include "../common/types.glsl"
#include "../common/Vulkan/VK_push_constants.glsl"

layout(push_constant, scalar) uniform PushConstants {
    PushConstantsEmissive data;
} pc;

layout(location = 0) in vec3 a_position;
layout(location = 2) in vec2 a_uv;

layout(location = 0) out vec2 v_uv;
layout(location = 1) flat out uint v_globalInstanceIndex;

void main() {
    RenderItemBuffer sceneRenderItems = pc.data.frame.sceneRenderItemBuffer;
    DrawRenderItemIndexBuffer drawRenderItemIndices = pc.data.frame.drawRenderItemIndexBuffer;
    ViewportDataBuffer viewportData = pc.data.frame.viewportDataBuffer;

    uint sceneRenderItemIndex = drawRenderItemIndices.renderItemIndices[uint(gl_InstanceIndex)];
    uint viewportIndex = pc.data.viewportIndex;

    RenderItem renderItem = sceneRenderItems.renderItems[sceneRenderItemIndex];
    mat4 projectionView = viewportData.viewportData[viewportIndex].projectionViewReverseZ;
    vec4 worldPosition = renderItem.modelMatrix * vec4(a_position, 1.0);

    v_uv = a_uv;
    v_globalInstanceIndex = sceneRenderItemIndex;
    gl_Position = projectionView * worldPosition;
}
