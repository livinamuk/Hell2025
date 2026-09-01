#version 460 core
layout (location = 0) in vec3 vPosition;

flat out int offsetIndex;

void main() {
    offsetIndex = gl_InstanceID;;

    // x2 becoz you are render with a quad ranging from -0.5 to 0.5
    gl_Position = vec4(vPosition * 2, 1.0);
}