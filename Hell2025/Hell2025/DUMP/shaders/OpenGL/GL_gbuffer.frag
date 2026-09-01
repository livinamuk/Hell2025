#version 460 core
#include "../common/OpenGL/GL_binding_indices.glsl"
#extension GL_ARB_bindless_texture : enable
readonly restrict layout(std430, binding = SSBO_IDX_SAMPLERS) buffer textureSamplersBuffer { uvec2 textureSamplers[]; };
in flat int MaterialIndex;
in flat int WoundMaterialIndex;
in flat float RoughnessFactor;
in flat float MetallicFactor;

layout (binding = 7) uniform sampler2DArray woundMaskTextureArray;

#include "../common/lighting.glsl"
#include "../common/flags.glsl"
#include "../common/normal_encoding.glsl"
#include "../common/post_processing.glsl"

readonly restrict layout(std430, binding = SSBO_IDX_MATERIALS) buffer materialsBuffer { Material materials[]; };

layout (location = 0) out vec4 BaseColorMetallicOut;
layout (location = 1) out vec4 NormalXYRoughnessMiscOut;
layout (location = 2) out vec4 EmissiveOut;
layout (location = 3) out vec4 VelocityXYOcclusionSubSurfaceOut;

in vec2 TexCoord;
in vec3 Normal;
in vec3 Tangent;
in vec4 WorldPos;
in vec3 ViewPos;
in vec3 EmissiveColor;
in vec4 v_currPos;
in vec4 v_prevPos;

in flat int WoundMaskTextureIndex;
in flat uint MiscFlags;

uniform bool u_alphaDiscard;
uniform bool u_flipNormalMapY;

void main() {
    vec3 emissiveColor = EmissiveColor;

    Material material = materials[MaterialIndex];
    vec4 baseColor = texture(sampler2D(textureSamplers[material.basecolor]), TexCoord);
    vec3 normalMap = texture(sampler2D(textureSamplers[material.normal]), TexCoord).rgb;
    vec4 rmat = texture(sampler2D(textureSamplers[material.rma]), TexCoord).rgba;
    vec3 emissiveMapColor = texture(sampler2D(textureSamplers[material.emissive]), TexCoord).rgb;

    // Emissive
    if (material.emissive != -1) {
        emissiveColor *= emissiveMapColor;
    }
    EmissiveOut = vec4(emissiveColor, 0);

    if (u_alphaDiscard) {
        if (baseColor.a < 0.5) {
            discard;
        }
    }


    // Sensible defaults for wound texture
    vec4 woundBaseColor = vec4(0,0,0,0);
    vec3 woundNormalMap = vec3(0,0,0);
    vec3 woundRma = vec3(0,0,0);

    // If this mesh has a wound mask, then sample it
    float woundMask = 0;
    bool hasWoundMaterial = WoundMaterialIndex != -1;
    if (WoundMaskTextureIndex != -1 && hasWoundMaterial) {
        Material woundMaterial = materials[WoundMaterialIndex];
        woundBaseColor = texture(sampler2D(textureSamplers[woundMaterial.basecolor]), TexCoord);
        woundNormalMap = texture(sampler2D(textureSamplers[woundMaterial.normal]), TexCoord).rgb;
        woundRma = texture(sampler2D(textureSamplers[woundMaterial.rma]), TexCoord).rgb;
        woundMask  = texture(woundMaskTextureArray, vec3(TexCoord, WoundMaskTextureIndex)).r;

        // Hack to make the center of wounds black
        const float woundK = 0.1;
        const float woundDarkenStrength = 0.3;
        const float woundGamma = 0.1;
        woundMask = clamp(woundMask * 1.25, 0, 1);
        float t = clamp(pow(woundMask, woundGamma) * woundDarkenStrength, 0.0, 1.0);
        woundBaseColor.rgb = mix(woundBaseColor.rgb, vec3(0.0), t);
        woundRma.r = mix(woundRma.r, 0.0, t * 2);
        woundRma.b = mix(woundRma.b, 0.0, t * 2);
    }

    baseColor = mix(baseColor, woundBaseColor, woundMask);
    normalMap = mix(normalMap, woundNormalMap, woundMask);
    rmat.rgb = mix(rmat.rgb, woundRma, woundMask);

    vec3 n = normalize(Normal);
    vec3 t = normalize(Tangent - dot(Tangent, n) * n);
    vec3 b = cross(n, t);
    mat3 tbn = mat3(t, b, n);

    normalMap.rgb = normalMap.rgb * 2.0 - 1.0;
    normalMap = normalize(normalMap);

    if (u_flipNormalMapY) {
        normalMap.y *= -1.0;
    };

    vec3 normal = normalize(tbn * (normalMap));

    if (!gl_FrontFacing) {
        normal = -normal;
    }

    float roughness = clamp(rmat.r * RoughnessFactor, 0.0, 1.0);
    float metallic = clamp(rmat.g * MetallicFactor, 0.0, 1.0);
    float ao = rmat.b;




    // Basecolor / Metallic out
    BaseColorMetallicOut.rgb = baseColor.rgb;
    BaseColorMetallicOut.a = metallic;

    // NormalXY / Roughness out
    NormalXYRoughnessMiscOut.rg = EncodeOct(normal);
    NormalXYRoughnessMiscOut.b = roughness;
    NormalXYRoughnessMiscOut.a = EncodeMiscFlags(MiscFlags);

    //RMAOut.rgb = rmat.rgb;

    // Thickness
    float thickness = rmat.a;

    // Emissive
    EmissiveOut.a = thickness;

    // Velocity
    vec2 currNDC = v_currPos.xy / v_currPos.w;
    vec2 prevNDC = v_prevPos.xy / v_prevPos.w;
    vec2 velocityNDC = currNDC - prevNDC;
    VelocityXYOcclusionSubSurfaceOut = vec4(velocityNDC, ao, 1.0);


    // BaseColorMetallicOut.rgb = vec3(woundMask);


}
