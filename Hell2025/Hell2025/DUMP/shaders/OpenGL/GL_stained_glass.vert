#version 460 core
#include "../common/OpenGL/GL_binding_indices.glsl"

#include "../common/util.glsl"
#include "../common/types.glsl"
#include "../common/constants.glsl"

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vUV;
layout (location = 3) in vec3 vTangent;

uniform int u_viewportIndex; // TODO: Don't upload per mesh uniforms
uniform mat4 u_modelMatrix;  // TODO: Don't upload per mesh uniforms
uniform vec3 u_tintColor;    // TODO: Don't upload per mesh uniforms

out vec2 TexCoord;
out vec4 WorldPos;
out vec3 Normal;
out vec3 Tangent;
out vec3 BiTangent;
out vec3 ViewPos;
out vec3 TintColor;

readonly restrict layout(std430, binding = SSBO_IDX_VIEWPORT_DATA) buffer viewportDataBuffer {
	ViewportData viewportData[];
};

void main() {

	mat4 projectionView = viewportData[u_viewportIndex].jitteredProjectionViewReverseZ;
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

    TintColor = u_tintColor;

	gl_Position = projectionView * WorldPos;
}
