#version 460  core
#include "../common/OpenGL/GL_binding_indices.glsl"
#include "../common/types.glsl"
#include "../common/constants.glsl"

layout (location = 0) in vec3 vPosition;

layout(std430, binding = SSBO_IDX_SCENE_RENDER_ITEMS) readonly buffer sceneRenderItemsBuffer { RenderItem sceneRenderItems[]; };
layout(std430, binding = SSBO_IDX_DRAW_RENDER_ITEM_INDICES) readonly buffer drawRenderItemIndicesBuffer { uint drawRenderItemIndices[]; };

uniform mat4 u_projectionView;
uniform mat4 u_modelMatrix;
uniform bool u_useDrawRenderItemIndices = false;

void main() {

    // Regular render items
    if (u_useDrawRenderItemIndices) {
        uint drawIndex = uint(gl_BaseInstance + gl_InstanceID);
        uint sceneRenderItemIndex = drawRenderItemIndices[drawIndex];
        RenderItem renderItem = sceneRenderItems[sceneRenderItemIndex];
        mat4 modelMatrix = renderItem.modelMatrix;

        gl_Position = u_projectionView * modelMatrix * vec4(vPosition, 1.0);
    }

    // Height maps, and house render items
    else {
        gl_Position = u_projectionView * u_modelMatrix * vec4(vPosition, 1.0);
    }
}
