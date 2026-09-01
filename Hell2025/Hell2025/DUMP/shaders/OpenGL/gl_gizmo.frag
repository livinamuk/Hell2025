#version 460 core
layout (location = 0) out vec4 FragOut;

uniform vec3 color;

void main() {
    FragOut.rgb = color;
	FragOut.a = 1.0;    
}
