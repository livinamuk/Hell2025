#version 460 core

#include "../common/normal_encoding.glsl"

layout (location = 0) out vec4 BaseColorMetallicOut;
layout (location = 1) out vec4 NormalXYRoughnessMiscOut;
layout (location = 2) out vec4 VelocityXYOcclusionSubSurfaceOut;

in vec3 v_normal;

void main() {
    vec3 baseColor = vec3(0.15, 0.004, 0.001); 
    float roughness = 0.1;
    float metallic = 0.0;
    vec3 normal = normalize(v_normal);

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
    VelocityXYOcclusionSubSurfaceOut.a = 1.0; // Subsurface. Not quite sure what this is yet
}
