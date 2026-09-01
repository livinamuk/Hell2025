#version 460
#include "../../common/OpenGL/GL_binding_indices.glsl"
#include "../../common/ddgi.glsl"
#include "../../common/types.glsl"

layout (location = 0) out vec4 FragOut;

layout(binding = 0) uniform sampler2DArray u_distanceAtlas;
layout(binding = 1) uniform sampler2DArray u_irradianceAtlas;

layout(std430, binding = SSBO_IDX_DEBUG_PROBES_VOLUME) readonly buffer DDGIVolumeBuffer { DDGIVolume volume; };
layout(std430, binding = SSBO_IDX_DEBUG_PROBES_STATES) readonly buffer ProbeStatesBuffer { ProbeState probeStates[]; };

flat in int v_probeIndex;
flat in ivec3 v_voxelCoord;
in vec3 v_worldPos;
in vec3 v_normal;

uniform int u_probeDebugState;

vec3 GetColor(int probeIdx) {
    const float encodingGamma = 5.0; // Must match the gather shader

    vec2 oct = DDGIGetOctahedralCoordinates(normalize(v_normal));
    vec3 probeUVW = DDGIGetProbeUV(probeIdx, oct, 6, volume);
    vec3 sampledValue = texture(u_irradianceAtlas, probeUVW).rgb;
    vec3 linearIrradiance = pow(max(vec3(0.0), sampledValue), vec3(encodingGamma));

    // Might need to multipy by 2 here to match gather shader
    return linearIrradiance;
}

vec3 GetDistance(int probeIdx) {
    vec2 oct = DDGIGetOctahedralCoordinates(normalize(v_normal));
    vec3 probeUVW = DDGIGetProbeUV(probeIdx, oct, 14, volume);
    vec2 moments = texture(u_distanceAtlas, probeUVW).rg;
    float maxRange = volume.probeSpacing * PROBE_MAX_RAY_DISTANCE;
    float grayscale = clamp(moments.x / maxRange, 0.0, 1.0);

    return vec3(grayscale);
}

vec3 GetRelevance(int probeIdx) {
    if (probeStates[volume.probeOffset + uint(probeIdx)].isRelevant) {
        return vec3(0, 1, 0);
    }
    else {
        return vec3(1, 0, 0);
    }
}

vec3 GetActiveState(int probeIdx) {
    if (probeStates[volume.probeOffset + uint(probeIdx)].isActive) {
        return vec3(0, 1, 1);
    }
    else {
        return vec3(1, 1, 0);
    }
}

vec3 GetDistanceCooldown(int probeIdx) {
    uint cooldown = probeStates[volume.probeOffset + uint(probeIdx)].distanceCooldown;
    float value = float(cooldown) / float(PROBE_MAX_DISTANCE_COOLDOWN);
    return vec3(value, 0.0, 0.0);
}


vec3 GetDisttanceWithCooldown(int probeIdx) {
    vec3 dist = GetDistance(probeIdx);
    vec3 distanceCooldown = GetDistanceCooldown(probeIdx);

    if (distanceCooldown.x > 0) {
        return vec3(dist.x, 0, 0);
    }
    else {
        return dist;
    }
}

vec3 GetIrradianceCoolDown(int probeIdx) {
    uint cooldown = probeStates[volume.probeOffset + uint(probeIdx)].irradianceCooldown;

    //if (cooldown == PROBE_MAX_IRRADIANCE_COOLDOWN) {
    //    return vec3(0.0); // This is a hack to match your other hack, that probes at max cooldown actually aren't updated
    //}

    float value = float(cooldown) / float(PROBE_MAX_IRRADIANCE_COOLDOWN);
    return vec3(value, 0.0, value);
}

void main() {
    int probeIdx = v_probeIndex;

    vec3 color = GetColor(probeIdx) * 0.5; // this 0.5 is a bit of a hack, find out why u need it!!!!
    vec3 dist = GetDistance(probeIdx);
    vec3 relevance = GetRelevance(probeIdx);
    vec3 activeState = GetActiveState(probeIdx);
    vec3 distanceCooldown = GetDistanceCooldown(probeIdx);
    vec3 distanceWithCoolDown = GetDisttanceWithCooldown(probeIdx);
    vec3 irradianceCoolDown = GetIrradianceCoolDown(probeIdx);

    if (u_probeDebugState == 1) FragOut = vec4(color, 1.0);
    if (u_probeDebugState == 2) FragOut = vec4(dist, 1.0);
    if (u_probeDebugState == 3) FragOut = vec4(distanceCooldown, 1.0);
    if (u_probeDebugState == 4) FragOut = vec4(irradianceCoolDown, 1.0);
    if (u_probeDebugState == 5) FragOut = vec4(relevance, 1.0);
    if (u_probeDebugState == 6) FragOut = vec4(activeState, 1.0);
}
