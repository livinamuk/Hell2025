#version 460

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vUV;
layout (location = 3) in vec3 vTangent;

uniform mat4 u_projectionView;
uniform mat4 u_inverseModel;
uniform mat4 u_model;

out vec2 TexCoord;
out vec4 WorldPos;
out vec3 Normal;
out vec3 Tangent;
out vec3 BiTangent;
out vec3 ViewPos;

void main() {
    mat4 modelMatrix = u_model;
    mat4 inverseModelMatrix = u_inverseModel;
    mat4 normalMatrix = transpose(inverseModelMatrix);

    Normal = normalize(normalMatrix * vec4(vNormal, 0)).xyz;
    Tangent = normalize(normalMatrix * vec4(vTangent, 0)).xyz;
    BiTangent = normalize(cross(Normal, Tangent));
    TexCoord = vUV;

    WorldPos = u_model * vec4(vPosition, 1.0);
    ViewPos = inverseModelMatrix[3].xyz;

    vec4 worldPos = u_model * vec4(vPosition, 1.0);
	gl_Position = u_projectionView * worldPos;
}