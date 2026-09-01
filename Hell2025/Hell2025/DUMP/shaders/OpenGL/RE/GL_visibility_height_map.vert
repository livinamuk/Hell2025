#version 460

#include "../../common/types.glsl"
#include "../../common/OpenGL/GL_binding_indices.glsl"

layout(location = 0) in vec3 a_position;

layout(location = 0) out vec3 v_controlPointPosition;
layout(location = 1) flat out int v_sceneRenderItemIndex;

readonly restrict layout(std430, binding = SSBO_IDX_DRAW_RENDER_ITEM_INDICES) buffer drawRenderItemIndicesBuffer { uint drawRenderItemIndices[]; };

void main() {
    uint drawIndex = uint(gl_BaseInstance + gl_InstanceID);
    v_sceneRenderItemIndex = int(drawRenderItemIndices[drawIndex]);
    v_controlPointPosition = a_position;
    gl_Position = vec4(a_position, 1.0);
}
