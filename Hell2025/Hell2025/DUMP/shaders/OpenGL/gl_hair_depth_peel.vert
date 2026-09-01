#version 460
#include "../common/OpenGL/GL_binding_indices.glsl"
#include "../common/util.glsl"
#include "../common/types.glsl"
#include "../common/constants.glsl"

layout (location = 0) in vec3 a_position;
layout (location = 2) in vec2 a_uv;

readonly restrict layout(std430, binding = SSBO_IDX_VIEWPORT_DATA) buffer viewportDataBuffer {
	ViewportData viewportData[];
};

readonly restrict layout(std430, binding = SSBO_IDX_SCENE_RENDER_ITEMS) buffer sceneRenderItemsBuffer { RenderItem sceneRenderItems[]; };
readonly restrict layout(std430, binding = SSBO_IDX_DRAW_RENDER_ITEM_INDICES) buffer drawRenderItemIndicesBuffer { uint drawRenderItemIndices[]; };

uniform int u_viewportIndex;

out vec2 v_uv;

void main() {
    int viewportIndex = u_viewportIndex;
    int globalInstanceIndex = int(drawRenderItemIndices[gl_BaseInstance + gl_InstanceID]);
    RenderItem renderItem = sceneRenderItems[globalInstanceIndex];
    
    mat4 jitterMatrix = viewportData[viewportIndex].jitteredProjectionViewReverseZ *
                        viewportData[viewportIndex].inverseProjectionViewReverseZ;
    mat4 projectionView = jitterMatrix * viewportData[viewportIndex].projectionView;
    mat4 modelMatrix = renderItem.modelMatrix;

    vec4 WorldPos = modelMatrix * vec4(a_position, 1.0);
    v_uv = a_uv;

	gl_Position = projectionView * WorldPos;
}
