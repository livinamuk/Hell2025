#version 460 core

layout (location = 0) out vec4 FragOut;
layout(binding = 1) uniform sampler2D outlineMaskTexture;

uniform vec2 u_offsets[256];

flat in int offsetIndex;

void main() {
    // Get the size of the texture to convert pixel coords to UVs
    ivec2 texSize = textureSize(outlineMaskTexture, 0);
    vec2 invTexSize = 1.0 / vec2(texSize);

    // Center coordinate in UV space [0.0, 1.0]
    vec2 centerUV = gl_FragCoord.xy * invTexSize;
    
    // Offset coordinate in pixel space, then convert to UV space
    vec2 offsetCoords = gl_FragCoord.xy + u_offsets[offsetIndex];
    vec2 offsetUV = offsetCoords * invTexSize;

    // Sample both locations using floating-point UVs
    float centerMask = texture(outlineMaskTexture, centerUV).r;
    float offsetMask = texture(outlineMaskTexture, offsetUV).r;

    if (centerMask == offsetMask) {
        discard;
    }
    else {
        FragOut = vec4(offsetMask, 0.0, 0.0, 0.0);
    }
}
