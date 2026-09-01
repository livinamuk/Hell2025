#version 460 core

layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vNormal;

uniform mat4 u_projectionView;

out vec3 Normal;

void main() {
	Normal = vNormal;
	gl_Position = u_projectionView * vec4(vPos, 1.0);

}