#version 460 core
layout (location = 0) out vec4 FragOut;
in vec3 Color;

void main() {
    FragOut.rgb = Color;
	FragOut.a = 1.0;
}
