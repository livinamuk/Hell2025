#version 460
layout (location = 0) out float ThicknessOut;

in vec4 v_viewSpacePosition;
in vec4 v_viewSpaceCenter;
in float v_radius;

in vec4 v_worldPos;
in vec3 v_sphereCenter;
in float v_sphereScale;

void main() {
float dist = distance(v_viewSpacePosition.xyz, v_viewSpaceCenter.xyz);

    // Normalize by radius
    float nDist = dist / (v_radius * 1.1);
    if (nDist > 1.0) discard;

    // This makes the center solid and the edges fall off quickly
    float t = 1.0 - nDist;
    t = t * t * t;

    // 0.1 means 10 particles must overlap to hit pure white 1.0
    ThicknessOut = t * 0.1;
    ThicknessOut = 1.0;
}
