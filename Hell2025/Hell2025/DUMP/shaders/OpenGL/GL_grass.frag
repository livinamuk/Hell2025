#version 460 core
#include "../common/normal_encoding.glsl"

#ifndef GRASS_CURVED_NORMALS
#define GRASS_CURVED_NORMALS 1
#endif

layout (location = 0) out vec4 BaseColorMetallicOut;
layout (location = 1) out vec4 NormalXYRoughnessMiscOut;
layout (location = 2) out vec4 EmissiveOut;
layout (location = 3) out vec4 VelocityXYOcclusionSubSurfaceOut;

 //"BaseColorMetallic", "NormalXYRoughnessMisc", "Emissive", "VelocityXYOcclusionSubSurface" });

//layout (location = 0) out vec4 BaseColorOut;
//layout (location = 1) out vec4 NormalOut;
//layout (location = 2) out vec4 RMAOut;


layout (binding = 2) uniform sampler2D NoiseTexture;

in vec3 Normal;
in vec3 WorldPos;
in vec3 BladeTangent;
in float BladeSide;
in vec4 v_currPos;
in vec4 v_prevPos;

uniform vec3 u_viewPosition;
uniform vec3 u_grassColor1;
uniform vec3 u_grassColor2;
uniform float u_grassColor1Darkness;
uniform float u_grassColor2Darkness;
uniform float u_noiseSquareMultiplier;
uniform float u_noiseMixMultiplier;
uniform float u_grassRoughness;
uniform float u_grassSubSurfaceFactor;

void main() {

    vec2 noiseUV = WorldPos.xz * 0.25;
    float noiseValue = texture(NoiseTexture, noiseUV).r;

    float noiseSq = noiseValue * noiseValue * u_noiseSquareMultiplier;

    vec3 color = mix(
        u_grassColor1 * u_grassColor1Darkness,
        u_grassColor2 * u_grassColor2Darkness,
        noiseValue);
    color = mix(color, vec3(noiseSq), u_noiseMixMultiplier);

    vec3 baseColor = color * 0.6;
    vec3 normal = gl_FrontFacing ? Normal : -Normal;

#if GRASS_CURVED_NORMALS
    const float normalCurveRadians = 1.0;

    vec3 V = normalize(u_viewPosition - WorldPos);
    vec3 T = normalize(BladeTangent);
    vec3 centerN = V - T * dot(V, T);

    // Looking almost directly along the blade makes the camera-facing
    // projection degenerate, so retain the geometric normal in that case.
    if (dot(centerN, centerN) > 1e-6) {
        centerN = normalize(centerN);
        vec3 sideAxis = normalize(cross(T, centerN));
        float angle = clamp(BladeSide, -1.0, 1.0) * normalCurveRadians;
        normal = normalize(centerN * cos(angle) + sideAxis * sin(angle));
    }
#endif


    vec2 currNDC = v_currPos.xy / v_currPos.w;
    vec2 prevNDC = v_prevPos.xy / v_prevPos.w;
    vec2 velocityNDC = currNDC - prevNDC;

    float roughness = u_grassRoughness;
    float metallic = 0.0;
    float ao = 1.0;

    // Basecolor / Metallic out
    BaseColorMetallicOut.rgb = baseColor.rgb;
    BaseColorMetallicOut.a = metallic;

    // NormalXY / Roughness out
    NormalXYRoughnessMiscOut.rg = EncodeOct(normal);
    NormalXYRoughnessMiscOut.b = roughness;
    NormalXYRoughnessMiscOut.a = 0.0; // Misc 4 bit value

    // Velocity / Occlusion / Subsurface out
    VelocityXYOcclusionSubSurfaceOut.rg = velocityNDC;
    VelocityXYOcclusionSubSurfaceOut.b = ao;
    VelocityXYOcclusionSubSurfaceOut.a = u_grassSubSurfaceFactor; // Thin-blade moonlight transmission strength

    //BaseColorOut = vec4(color * 0.6, 1.0);
    //RMAOut = vec4(0.9, 0.5, 1.0, 1.0);
    //NormalOut = vec4(Normal, 0.0);
    EmissiveOut = vec4(0.0, 0.0, 0.0, u_grassSubSurfaceFactor);
}
