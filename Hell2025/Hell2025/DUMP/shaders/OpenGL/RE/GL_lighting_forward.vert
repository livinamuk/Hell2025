#version 460
#include "../../common/OpenGL/GL_binding_indices.glsl"

#include "../../common/util.glsl"
#include "../../common/types.glsl"
#include "../../common/constants.glsl"

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vUV;
layout (location = 3) in vec3 vTangent;

layout(std430, binding = SSBO_IDX_VIEWPORT_DATA) readonly restrict buffer viewportDataBuffer { ViewportData viewportData[]; };
layout(std430, binding = SSBO_IDX_SCENE_RENDER_ITEMS) readonly restrict buffer sceneRenderItemsBuffer { RenderItem sceneRenderItems[]; };
layout(std430, binding = SSBO_IDX_DRAW_RENDER_ITEM_INDICES) readonly restrict buffer drawRenderItemIndicesBuffer { uint drawRenderItemIndices[]; };

centroid out vec2 TexCoord;
centroid out vec4 WorldPos;
centroid out vec3 Normal;
centroid out vec3 Tangent;
centroid out vec3 ViewPos;

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

    Normal = normalize(normalMatrix * vec4(vNormal, 0.0)).xyz;
    Tangent = normalize(modelMatrix * vec4(vTangent, 0.0)).xyz;
    
    TexCoord = vUV;
    
    WorldPos = modelMatrix * vec4(vPosition, 1.0);
    gl_Position = projectionView * WorldPos;
}
