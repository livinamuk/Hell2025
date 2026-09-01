#version 450

#include "../../common/normal_encoding.glsl"

layout (location = 0) out vec4 BaseColorMetallicOut;
layout (location = 1) out vec4 NormalXYRoughnessMiscOut;
layout (location = 2) out vec4 VelocityXYOcclusionSubSurfaceOut;

in vec4 WorldPos;
in vec3 Normal;

void main() {
    vec3 baseColor = vec3(0.5, 0.0, 0.0);
    vec3 normal = normalize(Normal);

    float roughness = 0.015;
    float metallic = 0.54;
    float ao = 1.0;
    vec2 velocity = vec2(0.0);

    // Basecolor / Metallic out
    BaseColorMetallicOut.rgb = baseColor;
    BaseColorMetallicOut.a = metallic;

    // NormalXY / Roughness out
    NormalXYRoughnessMiscOut.rg = EncodeOct(normal);
    NormalXYRoughnessMiscOut.b = roughness;
    NormalXYRoughnessMiscOut.a = 0.0; // Misc 4 bit value

    // Velocity / Occlusion / Subsurface out
    VelocityXYOcclusionSubSurfaceOut.rg = velocity;
    VelocityXYOcclusionSubSurfaceOut.b = ao;
    VelocityXYOcclusionSubSurfaceOut.a = 0.0; // Subsurface. Not quite sure what this is yet
}
