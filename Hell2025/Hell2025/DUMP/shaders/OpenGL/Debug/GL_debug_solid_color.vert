#version 460 core

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vUV;
layout (location = 3) in vec3 vTangent;

uniform mat4 u_projectionView;
uniform mat4 u_model;

out vec2 TexCoords;
out vec4 WorldPos;

void main() {
    TexCoords = vUV;
    WorldPos = u_model * vec4(vPosition, 1.0);
	gl_Position = u_projectionView * WorldPos;
}