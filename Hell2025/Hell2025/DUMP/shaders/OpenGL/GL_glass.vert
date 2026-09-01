#version 460 core

#include "../common/util.glsl"
#include "../common/types.glsl"
#include "../common/constants.glsl"
#include "../common/OpenGL/GL_binding_indices.glsl"

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_uv;
layout (location = 3) in vec3 a_tangent;

uniform int u_viewportIndex;

out vec2 v_uv;
out vec3 v_normal;
out vec3 v_tangent;
out vec3 v_bitangent;
out vec4 v_worldPos;
out vec3 v_viewPos;
out vec3 v_tint;

out flat int v_materialIndex;
out flat uint v_instanceIndex;
out flat float v_roughnessFactor;
out flat float v_metallicFactor;

readonly restrict layout(std430, binding = SSBO_IDX_VIEWPORT_DATA) buffer viewportDataBuffer {
	ViewportData viewportData[];
};

readonly restrict layout(std430, binding = SSBO_IDX_SCENE_RENDER_ITEMS) buffer sceneRenderItemsBuffer { RenderItem sceneRenderItems[]; };
readonly restrict layout(std430, binding = SSBO_IDX_DRAW_RENDER_ITEM_INDICES) buffer drawRenderItemIndicesBuffer { uint drawRenderItemIndices[]; };

void main() {
    int globalInstanceIndex = gl_BaseInstance + gl_InstanceID;
    uint sceneRenderItemIndex = drawRenderItemIndices[globalInstanceIndex];
    RenderItem renderItem = sceneRenderItems[sceneRenderItemIndex];

	mat4 projectionView = viewportData[u_viewportIndex].jitteredProjectionViewReverseZ;
	mat4 inverseView = viewportData[u_viewportIndex].inverseView;

    mat4 modelMatrix = renderItem.modelMatrix;
    mat4 inverseModelMatrix = inverse(modelMatrix);
    mat4 normalMatrix = transpose(inverseModelMatrix);

    v_normal = normalize(normalMatrix * vec4(a_normal, 0)).xyz;
    v_tangent = normalize(normalMatrix * vec4(a_tangent, 0)).xyz;
    v_bitangent = normalize(cross(a_normal, a_tangent));
    v_uv = a_uv;
    v_materialIndex = renderItem.materialIndex;
    v_instanceIndex = uint(globalInstanceIndex);
    v_roughnessFactor = renderItem.roughnessFactor;
    v_metallicFactor = renderItem.metallicFactor;

    v_worldPos = modelMatrix * vec4(a_position, 1.0);
    v_viewPos = inverseView[3].xyz;

    v_tint.r = renderItem.tintColorR;
    v_tint.g = renderItem.tintColorG;
    v_tint.b = renderItem.tintColorB;

	gl_Position = projectionView * v_worldPos;
}
