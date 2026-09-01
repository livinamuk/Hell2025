#version 460 core
#include "../common/OpenGL/GL_binding_indices.glsl"
#extension GL_ARB_bindless_texture : enable

#include "../common/types.glsl"

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_uv;
layout (location = 3) in vec3 a_tangent;

uniform int u_viewportIndex;
uniform mat4 u_modelMatrix;
uniform mat4 u_inverseModelMatrix;
uniform float u_time;
uniform float u_fps;
uniform int u_frameCount;
uniform vec3 u_boundsMin;
uniform vec3 u_boundsMax;
uniform int u_positionTextureIndex;
uniform int u_rotationTextureIndex;
uniform int u_lookupTextureIndex;
uniform bool u_mirror;

readonly restrict layout(std430, binding = SSBO_IDX_SAMPLERS) buffer textureSamplersBuffer {
    uvec2 textureSamplers[];
};

readonly restrict layout(std430, binding = SSBO_IDX_VIEWPORT_DATA) buffer viewportDataBuffer {
	ViewportData viewportData[];
};

out vec3 v_normal;

vec2 ToVATTextureUV(vec2 unityUV) {
    // VAT textures are authored for Unity; this engine uploads image rows unflipped to OpenGL.
    return vec2(unityUV.x, 1.0 - unityUV.y);
}

float GetLookupRowScale() {
    return 1.0 - fract(-u_boundsMax.x * 10.0);
}

float GetLookupUScale() {
    float packedValue = u_boundsMin.z * 10.0;
    return 1.0 - (ceil(packedValue) - packedValue);
}

float GetLookupUnpackDivisor() {
    return fract(-u_boundsMin.x * 10.0) >= 0.5 ? 2048.0 : 255.0;
}

bool UsesDirectHDRData() {
    return fract(u_boundsMax.z * 10.0) >= 0.5;
}

float GetAnimationProgress() {
    float frameCount = max(float(u_frameCount), 1.0);
    return fract(u_time * (u_fps / max(frameCount - 0.01, 0.01)));
}

float GetFrameLookupV(float frameIndex) {
    float frameCount = max(float(u_frameCount), 1.0);
    float rowScale = GetLookupRowScale();
    return ((1.0 - a_uv.y) * rowScale) + ((mod(frameIndex, frameCount) / frameCount) * rowScale);
}

vec2 DecodeVATUV(float frameIndex) {
    vec2 lookupUV = vec2(a_uv.x * GetLookupUScale(), 1.0 - GetFrameLookupV(frameIndex));
    vec4 lookup = textureLod(sampler2D(textureSamplers[u_lookupTextureIndex]), ToVATTextureUV(lookupUV), 0.0);
    float unpackDivisor = GetLookupUnpackDivisor();

    return vec2(
        lookup.r + (lookup.g / unpackDivisor),
        1.0 - (lookup.b + (lookup.a / unpackDivisor))
    );
}

vec3 DecodePosition(vec4 positionSample) {
    if (UsesDirectHDRData()) {
        return positionSample.xyz;
    }

    return (positionSample.xyz * (u_boundsMax - u_boundsMin)) + u_boundsMin;
}

vec4 DecodeRotation(vec4 rotationSample) {
    if (UsesDirectHDRData()) {
        return rotationSample;
    }

    return (rotationSample * 2.0) - 1.0;
}

vec3 RotateUpVectorByQuaternion(vec4 quaternion) {
    vec3 up = vec3(0.0, 1.0, 0.0);
    return normalize(up + (2.0 * cross(quaternion.xyz, (quaternion.w * up) + cross(quaternion.xyz, up))));
}

void main() {

	mat4 projectionView = viewportData[u_viewportIndex].jitteredProjectionViewReverseZ;
    mat4 normalMatrix = transpose(u_inverseModelMatrix);

    float frameCount = max(float(u_frameCount), 1.0);
    float frameIndex = floor(GetAnimationProgress() * frameCount);
    vec2 vatUV = DecodeVATUV(frameIndex);

    vec4 positionSample = textureLod(sampler2D(textureSamplers[u_positionTextureIndex]), ToVATTextureUV(vatUV), 0.0);
    vec4 rotationSample = textureLod(sampler2D(textureSamplers[u_rotationTextureIndex]), ToVATTextureUV(vatUV), 0.0);

    vec3 animatedPosition = DecodePosition(positionSample);
    vec3 animatedNormal = RotateUpVectorByQuaternion(DecodeRotation(rotationSample));

    if (u_mirror) {
        animatedPosition.z = -animatedPosition.z;
        animatedNormal.z = -animatedNormal.z;
    }

    v_normal = normalize((normalMatrix * vec4(animatedNormal, 0.0)).xyz);

    if (a_uv.y <= 0.1) {
        animatedPosition = vec3(0.0);
    }

	gl_Position = projectionView * u_modelMatrix * vec4(animatedPosition, 1.0);
}
