#ifndef RECONSTRUCTION_GLSL
#define RECONSTRUCTION_GLSL

#include "viewport.glsl"

vec3 WorldPosFromDepth(vec2 viewportUV, float depth, mat4 inverseProjectionView) {
    vec2 clipXY = ViewportNDCFromViewportUV(viewportUV);
    vec4 clip = vec4(clipXY, depth, 1.0);
    vec4 worldH = inverseProjectionView * clip;
    return worldH.xyz / worldH.w;
}

float LinearViewDepthFromNDC(vec2 viewportNDC, float depth, mat4 inverseProjection) {
    vec4 viewPosition = inverseProjection * vec4(viewportNDC, depth, 1.0);
    if (abs(viewPosition.w) < 1.0e-6) return 0.0;
    return abs(viewPosition.z / viewPosition.w);
}

#endif
