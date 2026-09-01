#version 460

#include "../common/constants.glsl"
#include "../common/types.glsl"
#include "../common/OpenGL/GL_binding_indices.glsl"

layout(vertices = 3) out;
layout(location = 0) in vec3 v_controlPointPosition[];
layout(location = 0) out vec3 tc_controlPointPosition[];

readonly restrict layout(std430, binding = SSBO_IDX_VIEWPORT_DATA) buffer viewportDataBuffer { ViewportData viewportDataArr[]; };

uniform int u_viewportIndex;

float TessellationFactorAt(vec2 controlPosition) {
#if TERRAIN_DISPLACEMENT_ENABLED
    vec2 target = viewportDataArr[u_viewportIndex].viewPos.xz / HEIGHTMAP_SCALE_XZ;
    float distanceToTarget = max(abs(controlPosition.x - target.x), abs(controlPosition.y - target.y));
    if (distanceToTarget <= TERRAIN_DISPLACEMENT_MESH_SIZE / 8.0) return 16.0;
    if (distanceToTarget <= TERRAIN_DISPLACEMENT_MESH_SIZE / 4.0) return 8.0;
    if (distanceToTarget <= TERRAIN_DISPLACEMENT_MESH_SIZE / 2.0) return 4.0;
    if (distanceToTarget <= TERRAIN_DISPLACEMENT_MESH_SIZE) return 2.0;
#endif
    return 1.0;
}

float EdgeTessellationFactor(vec2 a, vec2 b) {
    return max(TessellationFactorAt(a), max(TessellationFactorAt(b), TessellationFactorAt((a + b) * 0.5)));
}

void main() {
    tc_controlPointPosition[gl_InvocationID] = v_controlPointPosition[gl_InvocationID];
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    barrier();
    if (gl_InvocationID != 0) return;

    vec2 p0 = v_controlPointPosition[0].xz;
    vec2 p1 = v_controlPointPosition[1].xz;
    vec2 p2 = v_controlPointPosition[2].xz;
    gl_TessLevelOuter[0] = EdgeTessellationFactor(p1, p2);
    gl_TessLevelOuter[1] = EdgeTessellationFactor(p2, p0);
    gl_TessLevelOuter[2] = EdgeTessellationFactor(p0, p1);
    gl_TessLevelInner[0] = max(
        max(gl_TessLevelOuter[0], gl_TessLevelOuter[1]),
        max(gl_TessLevelOuter[2], TessellationFactorAt((p0 + p1 + p2) / 3.0)));
}
