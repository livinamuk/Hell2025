#version 460
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_ARB_shader_draw_parameters : require
#extension GL_ARB_shader_viewport_layer_array : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "../common/constants.glsl"
#include "../common/types.glsl"
#include "../common/Vulkan/VK_push_constants.glsl"

layout(push_constant, scalar) uniform PushConstants {
    PushConstantsPointShadow data;
} pc;

layout(buffer_reference, scalar) readonly buffer PointShadowFaceDataBuffer {
    PointShadowFaceData faceData[];
};

layout(buffer_reference, scalar) readonly buffer PointShadowDrawFaceIndexBuffer {
    uint faceDataIndices[];
};

layout(location = 0) in vec3 a_position;
layout(location = 2) in vec2 a_uv;

layout(location = 0) out vec3 v_worldPosition;
layout(location = 1) flat out vec4 v_lightPositionRadius;
layout(location = 2) out vec2 v_uv;
layout(location = 3) flat out int v_baseColorTextureIndex;

void main() {
    RenderItemBuffer sceneRenderItems = pc.data.frame.sceneRenderItemBuffer;
    DrawRenderItemIndexBuffer drawRenderItemIndices = pc.data.frame.drawRenderItemIndexBuffer;
    MaterialBuffer materials = pc.data.frame.materialBuffer;
    PointShadowDrawFaceIndexBuffer drawFaceDataIndices = PointShadowDrawFaceIndexBuffer(pc.data.drawFaceDataIndicesDeviceAddress);
    uint sceneRenderItemIndex = drawRenderItemIndices.renderItemIndices[gl_InstanceIndex];
    uint faceDataIndex = drawFaceDataIndices.faceDataIndices[gl_DrawIDARB];
    RenderItem renderItem = sceneRenderItems.renderItems[sceneRenderItemIndex];
    Material material = materials.materials[renderItem.materialIndex];
    PointShadowFaceDataBuffer faceDataBuffer = PointShadowFaceDataBuffer(pc.data.faceDataDeviceAddress);
    PointShadowFaceData faceData = faceDataBuffer.faceData[faceDataIndex];
    vec4 worldPosition = renderItem.modelMatrix * vec4(a_position, 1.0);

    v_worldPosition = worldPosition.xyz;
    v_lightPositionRadius = faceData.lightPositionRadius;
    v_uv = a_uv;
    v_baseColorTextureIndex = material.basecolor;
    gl_Position = faceData.projectionView * worldPosition;
    gl_Layer = int(faceData.arrayLayer);
}
