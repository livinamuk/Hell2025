#version 460
layout (location = 0) out float ViewspaceDepthOut;

layout(binding = 0, r32f) uniform image2D u_viewspaceDepthPrevious;

in vec4 v_worldPosition;
uniform mat4 u_view;

void main() {
    ivec2 px = ivec2(gl_FragCoord.xy);
    float viewspaceDepthPrevious = imageLoad(u_viewspaceDepthPrevious, px).r;

    float viewspaceDepth = (u_view * vec4(v_worldPosition)).z;

    if (viewspaceDepth < viewspaceDepthPrevious) {
        discard;
    }

    ViewspaceDepthOut = viewspaceDepth;
}
