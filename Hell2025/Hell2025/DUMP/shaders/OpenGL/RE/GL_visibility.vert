#version 460

#include "../../common/util.glsl"
#include "../../common/types.glsl"
#include "../../common/constants.glsl"
#include "../../common/OpenGL/GL_binding_indices.glsl"

layout(location = 0) in vec3 a_position;
layout(location = 2) in vec2 a_uv;

layout(location = 0) flat out int v_sceneRenderItemIndex;
layout(location = 1) out vec2 v_uv;

readonly restrict layout(std430, binding = SSBO_IDX_SCENE_RENDER_ITEMS) buffer sceneRenderItemsBuffer { RenderItem sceneRenderItems[]; };
readonly restrict layout(std430, binding = SSBO_IDX_DRAW_RENDER_ITEM_INDICES) buffer drawRenderItemIndicesBuffer { uint drawRenderItemIndices[]; };
readonly restrict layout(std430, binding = SSBO_IDX_VIEWPORT_DATA) buffer viewportDataBuffer { ViewportData viewportDataArr[]; };

uniform bool u_depthOffset;
uniform int u_viewportIndex;

void main() {
    uint drawIndex = uint(gl_BaseInstance + gl_InstanceID);
    v_sceneRenderItemIndex = int(drawRenderItemIndices[drawIndex]);

    RenderItem renderItem = sceneRenderItems[v_sceneRenderItemIndex];
    mat4 projectionView = viewportDataArr[u_viewportIndex].jitteredProjectionViewReverseZ;
    mat4 modelMatrix = renderItem.modelMatrix;
    vec4 worldPos = modelMatrix * vec4(a_position, 1.0);

    if (u_depthOffset) {
        vec3 cameraPos = viewportDataArr[u_viewportIndex].viewPos.xyz;
        vec3 awayFromCamera = normalize(worldPos.xyz - cameraPos);
        float depthBiasMeters = 0.01;
        worldPos.xyz += awayFromCamera * depthBiasMeters;
    }

    gl_Position = projectionView * worldPos;

    v_uv = a_uv;
}
