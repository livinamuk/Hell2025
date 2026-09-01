#version 460
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_ARB_shader_draw_parameters : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "../common/constants.glsl"
#include "../common/types.glsl"
#include "../common/Vulkan/VK_push_constants.glsl"

layout(push_constant, scalar) uniform PushConstants {
    PushConstantsHair data;
} pc;

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_uv;
layout(location = 3) in vec3 a_tangent;

layout(location = 0) centroid out vec2 v_texCoord;
layout(location = 1) centroid out vec3 v_normal;
layout(location = 2) centroid out vec3 v_tangent;
layout(location = 3) centroid out vec4 v_worldPos;
layout(location = 4) flat out uint v_globalInstanceIndex;
layout(location = 5) flat out uint v_viewportIndex;

void main() {
    RenderItemBuffer sceneRenderItems = pc.data.frame.sceneRenderItemBuffer;
    DrawRenderItemIndexBuffer drawRenderItemIndices = pc.data.frame.drawRenderItemIndexBuffer;
    ViewportDataBuffer viewportData = pc.data.frame.viewportDataBuffer;

    uint sceneRenderItemIndex = drawRenderItemIndices.renderItemIndices[uint(gl_InstanceIndex)];
    uint viewportIndex = pc.data.viewportIndex;

    RenderItem renderItem = sceneRenderItems.renderItems[sceneRenderItemIndex];
    mat4 modelMatrix = renderItem.modelMatrix;
    mat4 normalMatrix = transpose(renderItem.inverseModelMatrix);
    mat4 projectionView = viewportData.viewportData[viewportIndex].jitteredProjectionViewReverseZ;

    v_normal = normalize((normalMatrix * vec4(a_normal, 0.0)).xyz);
    v_tangent = normalize((modelMatrix * vec4(a_tangent, 0.0)).xyz);
    v_texCoord = a_uv;
    v_worldPos = modelMatrix * vec4(a_position, 1.0);
    v_globalInstanceIndex = sceneRenderItemIndex;
    v_viewportIndex = viewportIndex;

    gl_Position = projectionView * v_worldPos;
}
