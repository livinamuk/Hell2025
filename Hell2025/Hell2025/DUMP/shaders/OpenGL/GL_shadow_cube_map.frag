#version 460 core

layout(location = 0) in vec3 v_worldPosition;
layout(location = 1) flat in vec4 v_lightPositionRadius;

void main() {
    gl_FragDepth = length(v_worldPosition - v_lightPositionRadius.xyz) / v_lightPositionRadius.w;
}
