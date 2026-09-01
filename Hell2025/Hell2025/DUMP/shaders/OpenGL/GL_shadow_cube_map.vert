#version 460 core
#extension GL_ARB_shader_viewport_layer_array : require

#include "../common/types.glsl"
#include "../common/constants.glsl"
#include "../common/OpenGL/GL_binding_indices.glsl"

layout(location = 0) in vec3 vPosition;
layout(location = 2) in vec2 vTexCoord;

layout(std430, binding = SSBO_IDX_SCENE_RENDER_ITEMS) readonly buffer sceneRenderItemsBuffer { RenderItem sceneRenderItems[]; };
layout(std430, binding = SSBO_IDX_DRAW_RENDER_ITEM_INDICES) readonly buffer drawRenderItemIndicesBuffer { uint drawRenderItemIndices[]; };

struct PointShadowFaceData {
    mat4 projectionView;
    vec4 lightPositionRadius;
    uvec4 arrayLayer;
};

layout(std430, binding = SSBO_IDX_POINT_SHADOW_FACE_DATA) readonly buffer pointShadowFaceDataBuffer {
    PointShadowFaceData pointShadowFaceData[];
};

layout(std430, binding = SSBO_IDX_POINT_SHADOW_DRAW_FACE_INDICES) readonly buffer pointShadowDrawFaceIndexBuffer {
    uint pointShadowDrawFaceIndices[];
};

uniform int u_drawDataOffset;

layout(location = 0) out vec3 v_worldPosition;
layout(location = 1) flat out vec4 v_lightPositionRadius;
layout(location = 2) out vec2 v_uv;
layout(location = 3) flat out int v_sceneRenderItemIndex;

void main() {
    uint drawIndex = uint(gl_BaseInstance + gl_InstanceID);
    uint sceneRenderItemIndex = drawRenderItemIndices[drawIndex];
    uint faceDataIndex = pointShadowDrawFaceIndices[uint(u_drawDataOffset) + uint(gl_DrawID)];
    RenderItem renderItem = sceneRenderItems[sceneRenderItemIndex];
    PointShadowFaceData faceData = pointShadowFaceData[faceDataIndex];
    vec4 worldPosition = renderItem.modelMatrix * vec4(vPosition, 1.0);

    v_worldPosition = worldPosition.xyz;
    v_lightPositionRadius = faceData.lightPositionRadius;
    v_uv = vTexCoord;
    v_sceneRenderItemIndex = int(sceneRenderItemIndex);
    gl_Position = faceData.projectionView * worldPosition;
    gl_Layer = int(faceData.arrayLayer.x);
}
