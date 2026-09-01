#version 460

void main() {
    vec2 uvs = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    gl_Position = vec4(uvs * 2.0 - 1.0, 0.0, 1.0);
}