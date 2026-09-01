#version 460 core
layout (location = 0) out vec4 FragOut;

in vec3 Color;

uniform bool useUniformColor;
uniform vec4 uniformColor;

void main() {
    FragOut.rgb = Color;
	FragOut.a = 1.0;

	if (useUniformColor) {
		FragOut = uniformColor;
	}
}
