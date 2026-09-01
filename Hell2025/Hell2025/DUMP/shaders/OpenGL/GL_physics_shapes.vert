#version 460 core
#include "../common/OpenGL/GL_binding_indices.glsl"
#include "../common/types.glsl"

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;

readonly restrict layout(std430, binding = SSBO_IDX_SCENE_RENDER_ITEMS) buffer sceneRenderItemsBuffer { RenderItem sceneRenderItems[]; };
readonly restrict layout(std430, binding = SSBO_IDX_DRAW_RENDER_ITEM_INDICES) buffer drawRenderItemIndicesBuffer { uint drawRenderItemIndices[]; };

uniform mat4 u_projectionView;

out vec3 v_normal;
out flat vec3 v_color;

void main() {
    int globalInstanceIndex = int(drawRenderItemIndices[gl_BaseInstance + gl_InstanceID]);
    RenderItem renderItem = sceneRenderItems[globalInstanceIndex];
    mat4 modelMatrix = renderItem.modelMatrix;

    mat3 normalMatrix = transpose(mat3(renderItem.inverseModelMatrix));
    v_normal = normalize(normalMatrix * a_normal);
    v_color = vec3(renderItem.tintColorR, renderItem.tintColorG, renderItem.tintColorB);

    gl_Position = u_projectionView * modelMatrix * vec4(a_position, 1.0);
}
