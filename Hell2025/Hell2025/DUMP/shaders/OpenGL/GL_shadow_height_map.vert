#version 460

layout(location = 0) in vec3 a_position;
layout(location = 0) out vec3 v_controlPointPosition;

void main() {
    v_controlPointPosition = a_position;
    gl_Position = vec4(a_position, 1.0);
}
