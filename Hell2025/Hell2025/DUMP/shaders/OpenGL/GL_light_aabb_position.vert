#version 460
layout (location = 0) in vec3 a_position;

uniform mat4 u_model;
uniform mat4 u_shadowMatrix;

out vec3 v_worldPos;

void main() {
    vec4 worldPos = u_model * vec4(a_position, 1.0);
    v_worldPos = worldPos.xyz;
    gl_Position = u_shadowMatrix * worldPos;
}