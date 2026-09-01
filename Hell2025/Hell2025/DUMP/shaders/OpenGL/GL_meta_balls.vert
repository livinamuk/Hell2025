#version 460 

layout (location = 0) in vec3 v_position;

uniform mat4 u_projectionView;
uniform mat4 u_model;

void main() {
	gl_Position = u_projectionView * u_model * vec4(v_position, 1.0);
}