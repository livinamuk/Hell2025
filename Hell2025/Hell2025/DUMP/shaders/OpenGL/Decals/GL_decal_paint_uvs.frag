#version 460 core
layout (binding = 0) uniform sampler2D baseColorTexture;
layout (binding = 1) uniform sampler2D normalTexture;
layout (binding = 2) uniform sampler2D rmaTexture;

layout (location = 0) out vec4 ColorOut;

in vec2 TexCoord;
in vec4 WorldPos;

void main() {
    ColorOut = vec4(TexCoord, 0, 0);
}
