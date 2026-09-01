#version 460 core
layout (binding = 0) uniform sampler2D u_gBufferDepthTexture;
layout (location = 0) out vec4 DebugOut;

void main() {
    ivec2 px = ivec2(gl_FragCoord.xy);
    gl_FragDepth = texelFetch(u_gBufferDepthTexture, px, 0).r;
    // gl_FragDepth = 1; // render no hair
    // gl_FragDepth = 0; // Copy no depth
}
