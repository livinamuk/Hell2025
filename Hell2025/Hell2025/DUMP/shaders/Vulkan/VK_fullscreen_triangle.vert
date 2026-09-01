#version 460

layout(location = 0) out vec2 v_uv;

void main() {
    vec2 position = vec2(-1.0, -1.0);
    if (gl_VertexIndex == 1) position = vec2(3.0, -1.0);
    if (gl_VertexIndex == 2) position = vec2(-1.0, 3.0);

    v_uv = position * 0.5 + 0.5;
    gl_Position = vec4(position, 0.0, 1.0);
}
