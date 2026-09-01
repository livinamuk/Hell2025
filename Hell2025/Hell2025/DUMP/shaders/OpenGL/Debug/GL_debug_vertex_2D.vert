#version 450
layout (location = 0) in ivec2 vPosition;
layout (location = 1) in vec3 vColor;

uniform int u_viewportWidth;
uniform int u_viewportHeight;
out vec3 Color;

void main() {
 vec2 pos = vec2(vPosition);
    float x = (pos.x / float(u_viewportWidth)) * 2.0 - 1.0;
    float y = 1.0 - (pos.y / float(u_viewportHeight)) * 2.0;
    Color = vColor;
    gl_Position = vec4(x, y, 0.0, 1.0);
}
