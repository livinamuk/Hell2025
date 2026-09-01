#version 460
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "../common/types.glsl"
#include "../common/Vulkan/VK_push_constants.glsl"

layout(push_constant, scalar) uniform PushConstants {
    PushConstantsDDGIProbeDebug data;
} pc;

layout(buffer_reference, scalar) readonly buffer ProbeStatesBuffer {
    ProbeState probeStates[];
};

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;

layout(location = 0) out vec3 v_color;

ivec3 GetProbeDebugCoords(int probeIndex, ivec3 probeCounts) {
    ivec3 probeCoords;
    probeCoords.x = probeIndex % probeCounts.x;
    probeCoords.z = (probeIndex / probeCounts.x) % probeCounts.z;
    probeCoords.y = probeIndex / (probeCounts.x * probeCounts.z);
    return probeCoords;
}

vec3 GetProbeDebugBaseWorldPosition(ivec3 probeCoords, vec3 origin, float probeSpacing, ivec3 probeCounts) {
    vec3 counts = vec3(probeCounts);
    vec3 coords = vec3(probeCoords);
    return origin + (coords - (counts - 1.0) * 0.5) * probeSpacing;
}

vec3 GetProbeDebugColor(uint debugState, vec3 normal, ivec3 probeCoords, ivec3 probeCounts, ProbeState probeState) {
    if (debugState == 2) return vec3(0.1, 0.45, 1.0);
    if (debugState == 3) {
        float value = float(probeState.distanceCooldown) / float(PROBE_MAX_DISTANCE_COOLDOWN);
        return vec3(value, 0.0, 0.0);
    }
    if (debugState == 4) {
        float value = float(probeState.irradianceCooldown) / float(PROBE_MAX_IRRADIANCE_COOLDOWN);
        return vec3(value, 0.0, value);
    }
    if (debugState == 5) return probeState.isRelevant ? vec3(0.1, 0.9, 0.25) : vec3(1.0, 0.0, 0.0);
    if (debugState == 6) return probeState.isActive ? vec3(0.0, 0.85, 1.0) : vec3(1.0, 1.0, 0.0);

    vec3 probeGrid = vec3(probeCoords) / max(vec3(probeCounts - ivec3(1)), vec3(1.0));
    vec3 normalColor = normal * 0.25 + 0.75;
    return mix(probeGrid, normalColor, 0.35);
}

void main() {
    ViewportDataBuffer viewportData = pc.data.frame.viewportDataBuffer;
    ProbeStatesBuffer probeStates = ProbeStatesBuffer(pc.data.probeStatesDeviceAddress);

    int probeIndex = gl_InstanceIndex;
    ivec3 probeCoords = GetProbeDebugCoords(probeIndex, pc.data.probeCounts);
    uint globalProbeIndex = pc.data.probeOffset + uint(probeIndex);
    ProbeState probeState = probeStates.probeStates[globalProbeIndex];
    vec3 probePos = GetProbeDebugBaseWorldPosition(probeCoords, pc.data.volumeOrigin, pc.data.probeSpacing, pc.data.probeCounts);
    probePos += probeState.relocationOffset * pc.data.probeSpacing;
    vec3 worldPos = probePos + a_position * 0.0625;
    mat4 projectionView = viewportData.viewportData[pc.data.viewportIndex].projectionViewReverseZ;

    v_color = GetProbeDebugColor(pc.data.probeDebugState, normalize(a_normal), probeCoords, pc.data.probeCounts, probeState);
    gl_Position = projectionView * vec4(worldPos, 1.0);
}
