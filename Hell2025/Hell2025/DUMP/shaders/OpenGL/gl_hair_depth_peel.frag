#version 460
layout (location = 0) out float ViewspaceDepthOut;
layout (binding = 0, r32f) uniform image2D u_viewspaceDepthPrevious;
layout (binding = 1) uniform sampler2D u_gBufferDepth;

in vec2 v_uv;

void main() {
    ivec2 px = ivec2(gl_FragCoord.xy);
    
    // Calculate normalized UVs based on the half-res viewport size
    ivec2 halfResSize = imageSize(u_viewspaceDepthPrevious);
    vec2 uv = gl_FragCoord.xy / vec2(halfResSize);

    // Fetch the full-res scene depth using UVs
    float sceneDepth = texture(u_gBufferDepth, uv).r;
    
    // Early out if behind scene depth
    if (gl_FragCoord.z >= sceneDepth+ 0.00005) {
        discard;
    }

    // Compare against the previous peel layer
    float currentDepth = gl_FragCoord.z;
    float previousDepth = imageLoad(u_viewspaceDepthPrevious, px).r;

    if (currentDepth <= previousDepth) {
        discard;
    }
}
