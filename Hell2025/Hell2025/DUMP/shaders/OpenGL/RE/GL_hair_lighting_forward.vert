#version 460
#include "../../common/OpenGL/GL_binding_indices.glsl"

#include "../../common/util.glsl"
#include "../../common/types.glsl"
#include "../../common/constants.glsl"

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_uv;
layout (location = 3) in vec3 a_tangent;

layout(std430, binding = SSBO_IDX_VIEWPORT_DATA) readonly restrict buffer viewportDataBuffer { ViewportData viewportData[]; };
layout(std430, binding = SSBO_IDX_SCENE_RENDER_ITEMS) readonly restrict buffer sceneRenderItemsBuffer { RenderItem sceneRenderItems[]; };
layout(std430, binding = SSBO_IDX_DRAW_RENDER_ITEM_INDICES) readonly restrict buffer drawRenderItemIndicesBuffer { uint drawRenderItemIndices[]; };

centroid out vec2 v_texCoord;
centroid out vec4 v_worldPos;
centroid out vec3 v_normal;
centroid out vec3 v_tangent;

out flat int v_globalInstanceIndex;
out flat int v_viewportIndex;

uniform int u_viewportIndex;

void main() {
    v_globalInstanceIndex = int(drawRenderItemIndices[gl_BaseInstance + gl_InstanceID]);
    v_viewportIndex = u_viewportIndex;

    RenderItem renderItem = sceneRenderItems[v_globalInstanceIndex];
    mat4 modelMatrix = renderItem.modelMatrix;
    mat4 inverseModelMatrix = renderItem.inverseModelMatrix;
    mat4 normalMatrix = transpose(inverseModelMatrix);

    ViewportData viewportData = viewportData[v_viewportIndex];
    mat4 projectionView = viewportData.jitteredProjectionViewReverseZ;

    v_normal = normalize(normalMatrix * vec4(a_normal, 0.0)).xyz;
    v_tangent = normalize(modelMatrix * vec4(a_tangent, 0.0)).xyz;

    v_texCoord = a_uv;

    v_worldPos = modelMatrix * vec4(a_position, 1.0);
    gl_Position = projectionView * v_worldPos;
}
