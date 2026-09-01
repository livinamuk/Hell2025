#version 460 core
#include "../../common/OpenGL/GL_binding_indices.glsl"

#include "../../common/util.glsl"
#include "../../common/types.glsl"
#include "../../common/constants.glsl"

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vUV;
layout (location = 3) in vec3 vTangent;

uniform int u_viewportIndex;
uniform mat4 u_modelMatrix;

out vec2 TexCoord;
out vec4 WorldPos;
out vec3 Normal;
out vec3 Tangent;
out vec3 BiTangent;
out vec3 ViewPos;

readonly restrict layout(std430, binding = SSBO_IDX_VIEWPORT_DATA) buffer viewportDataBuffer {
	ViewportData viewportData[];
};

void main() {

	mat4 jitterMatrix = viewportData[u_viewportIndex].jitteredProjectionViewReverseZ *
	                    viewportData[u_viewportIndex].inverseProjectionViewReverseZ;
	mat4 projectionView = jitterMatrix * viewportData[u_viewportIndex].projectionView;
	mat4 inverseView = viewportData[u_viewportIndex].inverseView;

    mat4 modelMatrix = u_modelMatrix;
    mat4 inverseModelMatrix = inverse(modelMatrix);
    mat4 normalMatrix = transpose(inverseModelMatrix);

    Normal = normalize(normalMatrix * vec4(vNormal, 0)).xyz;
    Tangent = normalize(normalMatrix * vec4(vTangent, 0)).xyz;
    BiTangent = normalize(cross(Normal, Tangent));
    TexCoord = vUV;

    WorldPos = u_modelMatrix * vec4(vPosition, 1.0);
    ViewPos = inverseView[3].xyz;

	gl_Position = projectionView * WorldPos;
}
