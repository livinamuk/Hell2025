#version 460 core
layout(location = 0) in vec3 v_color;
layout(location = 0) out vec4 fragColor;

void main() {
    fragColor = vec4(v_color, 1.0);
    gl_FragDepth = 0.0;
}
