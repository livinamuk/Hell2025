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
out vec3 ViewPos;
out vec3 EmissiveColor;

out flat int MaterialIndex;
out flat int WoundMaterialIndex;
out flat float RoughnessFactor;
out flat float MetallicFactor;

out flat int WoundMaskTextureIndex;
out flat uint MiscFlags;

out vec4 v_currPos;
out vec4 v_prevPos;

// temporarily here
uniform bool u_useMirrorMatrix;
uniform mat4 u_mirrorViewMatrix;
uniform vec4 u_mirrorClipPlane;
uniform int u_viewportIndex;

void main() {
    int viewportIndex = u_viewportIndex;
    int globalInstanceIndex = int(drawRenderItemIndices[gl_BaseInstance + gl_InstanceID]);

    RenderItem renderItem = sceneRenderItems[globalInstanceIndex];

    MaterialIndex = renderItem.materialIndex;
    WoundMaterialIndex = renderItem.woundMaterialIndex;
    RoughnessFactor = renderItem.roughnessFactor;
    MetallicFactor = renderItem.metallicFactor;

    mat4 modelMatrix = renderItem.modelMatrix;
    mat4 prevModelMatrix = renderItem.prevModelMatrix;
    mat4 inverseModelMatrix = renderItem.inverseModelMatrix;
	mat4 projectionView = viewportData[viewportIndex].projectionViewReverseZ;
	mat4 rasterProjectionView = viewportData[viewportIndex].jitteredProjectionViewReverseZ;
	mat4 prevProjectionView = viewportData[viewportIndex].prevProjectionViewReverseZ;
	mat4 projection = viewportData[viewportIndex].projection;
	mat4 view = viewportData[viewportIndex].view;
    mat4 normalMatrix = transpose(inverseModelMatrix);

    WoundMaskTextureIndex = renderItem.woundMaskTextureIndex;

    Normal = normalize(normalMatrix * vec4(vNormal, 0)).xyz;
    Tangent = normalize(normalMatrix * vec4(vTangent, 0)).xyz;

	TexCoord = vUV;
    ViewPos = viewportData[viewportIndex].inverseView[3].xyz;
    EmissiveColor = vec3(renderItem.emissiveR, renderItem.emissiveG, renderItem.emissiveB);

    WorldPos = modelMatrix * vec4(vPosition, 1.0);
    vec4 prevWorldPos = prevModelMatrix * vec4(vPosition, 1.0);

    v_currPos = projectionView * WorldPos;
    v_prevPos = prevProjectionView * prevWorldPos;

    // Planar reflections
    if (u_useMirrorMatrix) {
        mat4 projection = viewportData[viewportIndex].projectionReverseZ;
        projection[0][0] *= -1.0;
        projectionView = projection * u_mirrorViewMatrix;
        mat4 jitterMatrix = viewportData[viewportIndex].jitteredProjectionViewReverseZ *
                            viewportData[viewportIndex].inverseProjectionViewReverseZ;
        rasterProjectionView = jitterMatrix * projectionView;
        gl_ClipDistance[0] = dot(WorldPos, u_mirrorClipPlane);
        //projection[0][0] *= -1.0;
        //view = u_mirrorViewMatrix;
        //gl_ClipDistance[0] = dot(WorldPos, u_mirrorClipPlane);
    }

    // Old
    gl_Position = rasterProjectionView * WorldPos;

    // Camera relative position for depth precision
    //vec4 camRelativeWorldPos = vec4(WorldPos.xyz - ViewPos, 1.0);
    //gl_Position = projection * mat4(mat3(view)) * camRelativeWorldPos;

    MiscFlags = renderItem.miscFlags;
}
