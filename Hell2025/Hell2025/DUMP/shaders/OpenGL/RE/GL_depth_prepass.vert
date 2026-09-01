#version 460
#include "../../common/OpenGL/GL_binding_indices.glsl"

#include "../../common/util.glsl"
#include "../../common/types.glsl"
#include "../../common/constants.glsl"

layout (location = 0) in vec3 a_position;

readonly restrict layout(std430, binding = SSBO_IDX_VIEWPORT_DATA) buffer viewportDataBuffer { ViewportData viewportData[]; };
readonly restrict layout(std430, binding = SSBO_IDX_SCENE_RENDER_ITEMS) buffer sceneRenderItemsBuffer { RenderItem sceneRenderItems[]; };
readonly restrict layout(std430, binding = SSBO_IDX_DRAW_RENDER_ITEM_INDICES) buffer drawRenderItemIndicesBuffer { uint drawRenderItemIndices[]; };

uniform int u_viewportIndex;

void main() {
    int viewportIndex = u_viewportIndex;
    int globalInstanceIndex = int(drawRenderItemIndices[gl_BaseInstance + gl_InstanceID]);

    RenderItem renderItem = sceneRenderItems[globalInstanceIndex];
    mat4 projectionView = viewportData[viewportIndex].jitteredProjectionViewReverseZ;
    mat4 modelMatrix = renderItem.modelMatrix;

    vec4 worldPos = modelMatrix * vec4(a_position, 1.0);
    gl_Position = projectionView * worldPos;
}
