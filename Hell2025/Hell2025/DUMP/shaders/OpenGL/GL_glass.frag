#version 460 core
#extension GL_ARB_bindless_texture : enable

layout(early_fragment_tests) in;

#include "../common/OpenGL/GL_binding_indices.glsl"
#include "../common/lighting.glsl"
#include "../common/post_processing.glsl"
#include "../common/types.glsl"

layout(location = 0, index = 0) out vec4 LightingOut;
layout(location = 0, index = 1) out vec4 TintOut;

readonly restrict layout(std430, binding = SSBO_IDX_SAMPLERS)                 buffer textureSamplersBuffer       { uvec2 textureSamplers[]; };
readonly restrict layout(std430, binding = SSBO_IDX_MATERIALS)                buffer materialsBuffer             { Material materials[]; };
readonly restrict layout(std430, binding = SSBO_IDX_RENDERER_DATA)            buffer rendererDataBuffer          { RendererData rendererData;   };
readonly restrict layout(std430, binding = SSBO_IDX_VIEWPORT_DATA)            buffer viewportDataBuffer          { ViewportData viewportData[]; };
readonly restrict layout(std430, binding = SSBO_IDX_LIGHTS)                   buffer lightsBuffer                { Light lights[]; };
readonly restrict layout(std430, binding = SSBO_IDX_SPOT_LIGHTS)              buffer spotLightsBuffer            { SpotLight spotLights[]; };
readonly restrict layout(std430, binding = SSBO_IDX_GLASS_LIGHT_RANGES)       buffer glassLightRangesBuffer      { uvec2 glassLightRanges[]; };
readonly restrict layout(std430, binding = SSBO_IDX_GLASS_LIGHT_INDICES)      buffer glassLightIndicesBuffer     { uint glassLightIndices[]; };
readonly restrict layout(std430, binding = SSBO_IDX_GLASS_SPOT_LIGHT_RANGES)  buffer glassSpotLightRangesBuffer  { uvec2 glassSpotLightRanges[]; };
readonly restrict layout(std430, binding = SSBO_IDX_GLASS_SPOT_LIGHT_INDICES) buffer glassSpotLightIndicesBuffer { uint glassSpotLightIndices[]; };

layout(binding = TEX_IDX_SHADOW_MAP_FLASHLIGHT) uniform sampler2DArray          FlashlighShadowMapTextureArray;
layout(binding = TEX_IDX_SHADOW_MAP_HI_RES)     uniform samplerCubeArrayShadow  HiResShadowMapArray;
layout(binding = TEX_IDX_SHADOW_MAP_LOW_RES)    uniform samplerCubeArrayShadow  LowResShadowMapArray;

in vec2 v_uv;
in vec3 v_normal;
in vec3 v_tangent;
in vec3 v_bitangent;
in vec4 v_worldPos;
in vec3 v_viewPos;
in vec3 v_tint;

in flat int v_materialIndex;
in flat uint v_instanceIndex;
in flat float v_roughnessFactor;
in flat float v_metallicFactor;

uniform int u_viewportIndex;
uniform bool u_flipNormalMapY;
uniform bool u_pointShadowsEnabled;

float SamplePointShadowSingle(int shadowMapIndex, vec3 lightToFrag, float currentDepth, float lightRadius, float nDotL, samplerCubeArrayShadow shadowMap) {
    float bias = max(0.05 * (1.0 - clamp(nDotL, 0.0, 1.0)), 0.005);
    float compareDepth = clamp((currentDepth - bias) / lightRadius, 0.0, 1.0);
    return texture(shadowMap, vec4(lightToFrag, float(shadowMapIndex)), compareDepth);
}

void main() {
    TintOut = vec4(v_tint, 1.0);

    uvec2 range = glassLightRanges[v_instanceIndex];
    uvec2 spotLightRange = glassSpotLightRanges[v_instanceIndex];

    if (range.y == 0u && spotLightRange.y == 0u) {
        LightingOut = vec4(0.0);
        return;
    }

    Material material = materials[v_materialIndex];
    vec4 baseColor = texture(sampler2D(textureSamplers[material.basecolor]), v_uv);
    vec3 normalMap = texture(sampler2D(textureSamplers[material.normal]), v_uv).rgb;
    vec3 rma = texture(sampler2D(textureSamplers[material.rma]), v_uv).rgb;

    normalMap = mix(normalMap, vec3(0.5, 0.5, 1), 0.7);

    mat3 tbn = mat3(normalize(v_tangent), normalize(v_bitangent), normalize(v_normal));
    normalMap.rgb = normalMap.rgb * 2.0 - 1.0;
    normalMap = normalize(normalMap);

    if (u_flipNormalMapY) {
        normalMap.y *= -1;
    }

    vec3 normal = normalize(tbn * normalMap);

    vec3 linearBaseColor = pow(baseColor.rgb, vec3(2.2));
    float roughness = clamp(rma.r * v_roughnessFactor, 0.0, 1.0);
    float metallic = clamp(rma.g * v_metallicFactor, 0.0, 1.0);

    vec3 directLighting = vec3(0);
    vec3 viewDirection = normalize(v_viewPos - v_worldPos.xyz);

    for (uint i = 0; i < range.y; i++) {
        uint lightIndex = glassLightIndices[range.x + i];
        Light light = lights[lightIndex];
        vec3 lightPos = vec3(light.posX, light.posY, light.posZ);
        float lightRadius = light.radius;
        float lightStrength = light.strength;

        if (lightRadius <= 0.0 || lightStrength <= 0.0) {
            continue;
        }

        if (any(lessThan(v_worldPos.xyz, light.worldBoundsMin.xyz)) ||
            any(greaterThan(v_worldPos.xyz, light.worldBoundsMax.xyz))) {
            continue;
        }

        vec3 lightDelta = lightPos - v_worldPos.xyz;
        float distanceSquared = dot(lightDelta, lightDelta);

        if (distanceSquared > lightRadius * lightRadius) {
            continue;
        }

        float lightDistance = sqrt(distanceSquared);
        vec3 L = lightDelta / max(lightDistance, 0.000001);
        float ndotl = dot(normal, L);

        if (ndotl <= 0.0) {
            continue;
        }

        float iesVisibility = 1.0;
        if (light.iesTextureIndex != 0) {
            sampler2D iesSampler = sampler2D(textureSamplers[light.iesTextureIndex]);
            iesVisibility = ApplyIESProfilePrecomputed(-L, lightDistance, light, iesSampler);
            if (iesVisibility <= 0.0) {
                continue;
            }
        }

        float shadow = 1.0;

        if (u_pointShadowsEnabled) {
            vec3 lightToFrag = -lightDelta;

            if (light.hiResShadowMapIndex != -1) {
                shadow = SamplePointShadowSingle(light.hiResShadowMapIndex, lightToFrag, lightDistance, lightRadius, ndotl, HiResShadowMapArray);
            }
            else if (light.lowResShadowMapIndex != -1) {
                shadow = SamplePointShadowSingle(light.lowResShadowMapIndex, lightToFrag, lightDistance, lightRadius, ndotl, LowResShadowMapArray);
            }

            if (shadow <= 0.0) {
                continue;
            }
        }

        float attenuation = smoothstep(lightRadius, 0.0, lightDistance) * lightStrength;
        vec3 lightColor = clamp(vec3(light.colorR, light.colorG, light.colorB), 0.0, 1.0);
        vec3 brdf = microfacetBRDFSpecularOnly(L, viewDirection, normal, linearBaseColor.rgb, metallic, 1.0, roughness);
        vec3 lightContribution = brdf * (ndotl * attenuation * shadow * iesVisibility) * lightColor;

        directLighting += lightContribution;
    }

    float fragDistance = distance(v_viewPos, v_worldPos.xyz);

    for (uint i = 0u; i < spotLightRange.y; i++) {
        SpotLight spotLight = spotLights[glassSpotLightIndices[spotLightRange.x + i]];
        if (!u_pointShadowsEnabled) spotLight.metadata.x = -1;
        sampler2D iesTexture = sampler2D(textureSamplers[max(rendererData.flashlightIESTextureIndex, 0)]);
        directLighting += GetSpotLightContributionSingleSample(spotLight, rendererData, uint(u_viewportIndex), v_viewPos, normal.xyz, v_worldPos.xyz, linearBaseColor.rgb, roughness, metallic, fragDistance, -1000.0, iesTexture, FlashlighShadowMapTextureArray);
    }

    LightingOut = vec4(directLighting, 0.0);
}
