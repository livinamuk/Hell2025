#version 460
#extension GL_ARB_bindless_texture : enable
#include "../../common/OpenGL/GL_binding_indices.glsl"
#include "../../common/lighting.glsl"
#include "../../common/types.glsl"

layout (location = 0) out vec4 ColorOut;

in vec3 v_worldPos;
in vec2 v_uv;

layout (binding = 0) uniform sampler2D u_texture;

readonly restrict layout(std430, binding = SSBO_IDX_SAMPLERS) buffer textureSamplersBuffer { uvec2 textureSamplers[]; };
readonly restrict layout(std430, binding = SSBO_IDX_RENDERER_DATA) buffer rendererDataBuffer { RendererData rendererData; };
readonly restrict layout(std430, binding = SSBO_IDX_VIEWPORT_DATA) buffer viewportDataBuffer { ViewportData viewportDataArr[]; };
readonly restrict layout(std430, binding = SSBO_IDX_SPOT_LIGHTS) buffer spotLightsBuffer { SpotLight spotLights[]; };

void main() {
    // Sample the flip book texture
    vec4 spriteSheetColor = texture(u_texture, v_uv);
    if (spriteSheetColor.a < 0.01) discard;

    spriteSheetColor.rgb = pow(spriteSheetColor.rgb, vec3(2.2));
    spriteSheetColor.rgb = pow(spriteSheetColor.rgb, vec3(2.2)); // Stack another POW for good measure
    float a = spriteSheetColor.a;

    // Base moonlight contribution
    vec3 finalLight = rendererData.moonLightColorStrength.rgb * rendererData.moonLightColorStrength.a;

    sampler2D flashlightIES = sampler2D(textureSamplers[max(rendererData.flashlightIESTextureIndex, 0)]);
    for (uint i = 0u; i < rendererData.spotLightCount; i++) {
        float attenuation = GetSpotLightAttenuation(spotLights[i], rendererData, v_worldPos, flashlightIES);
        finalLight += rendererData.flashlightColor.rgb * attenuation;
    }

    // Final color
    vec3 finalColor = spriteSheetColor.rgb * finalLight;

    const vec3 UNDER_WATER_TINT = mix(vec3(0.4, 0.8, 0.6) * 1.75, vec3(0.01, 0.03, 0.04), 0.25);
    finalColor *= UNDER_WATER_TINT;

    // Dampen
    finalColor *= 0.5;
    a *= 0.5;

    ColorOut = vec4(finalColor, a);
}
