#version 460 core

#include "../common/util.glsl"
#include "../common/types.glsl"
#include "../common/constants.glsl"
#include "../common/OpenGL/GL_binding_indices.glsl"

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vUV;
layout (location = 3) in vec3 vTangent;

readonly restrict layout(std430, binding = SSBO_IDX_VIEWPORT_DATA) buffer viewportDataBuffer { ViewportData viewportData[]; };
readonly restrict layout(std430, binding = SSBO_IDX_SCENE_RENDER_ITEMS) buffer sceneRenderItemsBuffer { RenderItem sceneRenderItems[]; };
readonly restrict layout(std430, binding = SSBO_IDX_DRAW_RENDER_ITEM_INDICES) buffer drawRenderItemIndicesBuffer { uint drawRenderItemIndices[]; };

out vec2 TexCoord;
out vec3 WorldPos;
out vec3 Normal;
out vec3 Tangent;
out vec3 BiTangent;

out flat int MaterialIndex;

uniform float u_textureScaling;
uniform int u_viewportIndex;

void main() {
    TexCoord = vUV * 50.0 * u_textureScaling;

    int viewportIndex = u_viewportIndex;
    int globalInstanceIndex = int(drawRenderItemIndices[gl_BaseInstance + gl_InstanceID]);

    RenderItem renderItem = sceneRenderItems[globalInstanceIndex];
    mat4 modelMatrix = renderItem.modelMatrix;
    mat4 inverseModelMatrix = renderItem.inverseModelMatrix;
    mat4 projectionView = viewportData[viewportIndex].jitteredProjectionViewReverseZ;

    MaterialIndex = renderItem.materialIndex;

    vec4 worldPos4 = modelMatrix * vec4(vPosition, 1.0);
    WorldPos = worldPos4.xyz;

    mat3 normalMatrix = transpose(mat3(inverseModelMatrix));
    Normal = normalize(normalMatrix * vNormal);
    Tangent = normalize(normalMatrix * vTangent);
    BiTangent = normalize(cross(Normal, Tangent));
    gl_Position = projectionView * worldPos4;
}
