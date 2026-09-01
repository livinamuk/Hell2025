#version 460 core
#include "../common/OpenGL/GL_binding_indices.glsl"

#include "../common/util.glsl"
#include "../common/types.glsl"
#include "../common/constants.glsl"

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vUV;
layout (location = 3) in vec3 vTangent;

readonly restrict layout(std430, binding = SSBO_IDX_VIEWPORT_DATA) buffer viewportDataBuffer {
	ViewportData viewportData[];
};

readonly restrict layout(std430, binding = SSBO_IDX_SCENE_RENDER_ITEMS) buffer sceneRenderItemsBuffer { RenderItem sceneRenderItems[]; };
readonly restrict layout(std430, binding = SSBO_IDX_DRAW_RENDER_ITEM_INDICES) buffer drawRenderItemIndicesBuffer { uint drawRenderItemIndices[]; };

out vec2 TexCoord;
out vec4 WorldPos;
out vec3 Normal;
out vec3 Tangent;
out vec3 BiTangent;
out vec3 ViewPos;
out flat int ViewportIndex;

out flat int MaterialIndex;
out flat float RoughnessFactor;
out flat float MetallicFactor;
uniform int u_viewportIndex;

void main() {
    int viewportIndex = u_viewportIndex;
    int globalInstanceIndex = int(drawRenderItemIndices[gl_BaseInstance + gl_InstanceID]);

    RenderItem renderItem = sceneRenderItems[globalInstanceIndex];
    MaterialIndex = renderItem.materialIndex;
    RoughnessFactor = renderItem.roughnessFactor;
    MetallicFactor = renderItem.metallicFactor;
    
    mat4 jitterMatrix = viewportData[viewportIndex].jitteredProjectionViewReverseZ *
                        viewportData[viewportIndex].inverseProjectionViewReverseZ;
    mat4 projectionView = jitterMatrix * viewportData[viewportIndex].projectionView;
    mat4 inverseView = viewportData[viewportIndex].inverseView;
    mat4 inverseModelMatrix = renderItem.inverseModelMatrix;
    mat4 modelMatrix = renderItem.modelMatrix;
         
    mat4 normalMatrix = transpose(inverseModelMatrix);
    Normal = normalize(normalMatrix * vec4(vNormal, 0)).xyz;
    Tangent = normalize(normalMatrix * vec4(vTangent, 0)).xyz;
    BiTangent = normalize(cross(Normal, Tangent));
    
	TexCoord = vUV;
    WorldPos = modelMatrix * vec4(vPosition, 1.0);
    ViewPos = viewportData[viewportIndex].inverseView[3].xyz;
    ViewportIndex = viewportIndex;

	gl_Position = projectionView * WorldPos;
}
