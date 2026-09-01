#version 460 core
#include "../common/flags.glsl"
#include "../common/normal_encoding.glsl"

layout (location = 0) out vec4 BaseColorMetallicOut;
layout (location = 1) out vec4 NormalXYRoughnessMiscOut;
layout (location = 2) out vec4 VelocityXYOcclusionSubSurfaceOut;

in vec3 v_normal;
in flat vec3 v_color;

void main() {
    float roughness = 0.8;
    float metallic = 0.1;
    float ao = 1.0;

    // Basecolor / Metallic out
    BaseColorMetallicOut.rgb = v_color;
    BaseColorMetallicOut.a = metallic;

    // NormalXY / Roughness out
    NormalXYRoughnessMiscOut.rg = EncodeOct(v_normal);
    NormalXYRoughnessMiscOut.b = roughness;
    NormalXYRoughnessMiscOut.a = EncodeMiscFlags(MISC_FLAG_DYNAMIC_OBJECT);

    // Velocity
    vec2 velocity = vec2(0, 0);
    VelocityXYOcclusionSubSurfaceOut = vec4(velocity, ao, 1.0);
}
