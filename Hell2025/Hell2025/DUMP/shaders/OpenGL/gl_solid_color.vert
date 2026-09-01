#version 460 core

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

out vec3 Color;

void main() {
	Color = vNormal;
	gl_Position = projection * view * model * vec4(vPosition, 1.0);
}