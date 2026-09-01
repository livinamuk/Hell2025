#version 460 core
#include "../common/OpenGL/GL_binding_indices.glsl"

#extension GL_ARB_bindless_texture : enable
readonly restrict layout(std430, binding = SSBO_IDX_SAMPLERS) buffer textureSamplersBuffer {
	uvec2 textureSamplers[];
};
in flat int MaterialIndex;
in flat float RoughnessFactor;
in flat float MetallicFactor;

layout (binding = TEX_IDX_SHADOW_MAP_HI_RES)     uniform samplerCubeArrayShadow hiResShadowMapArray;
layout (binding = TEX_IDX_SHADOW_MAP_LOW_RES)    uniform samplerCubeArrayShadow lowResShadowMapArray;
layout (binding = TEX_IDX_SHADOW_MAP_FLASHLIGHT) uniform sampler2DArray flashlightShadowMapArray;

#include "../common/lighting.glsl"
#include "../common/post_processing.glsl"
#include "../common/types.glsl"
#include "../common/util.glsl"

readonly restrict layout(std430, binding = SSBO_IDX_MATERIALS) buffer materialsBuffer { Material materials[]; };

layout (location = 0) out vec4 FragOut;
layout (location = 1) out vec4 ViewSpaceDepthPreviousOut;
readonly restrict layout(std430, binding = SSBO_IDX_RENDERER_DATA) buffer rendererDataBuffer { RendererData  rendererData;   };
readonly restrict layout(std430, binding = SSBO_IDX_VIEWPORT_DATA) buffer viewportDataBuffer { ViewportData  viewportData[]; };
readonly restrict layout(std430, binding = SSBO_IDX_LIGHTS) buffer lightsBuffer       { Light         lights[];       };
readonly restrict layout(std430, binding = SSBO_IDX_LIGHTING_TILE_LIGHTS) buffer tileLightsBuffer   { TileLights    tileLights[];   };
readonly restrict layout(std430, binding = SSBO_IDX_LIGHTING_TILE_SPOT_LIGHTS) buffer tileSpotLightsBuffer { TileSpotLights tileSpotLights[]; };
readonly restrict layout(std430, binding = SSBO_IDX_SPOT_LIGHTS) buffer spotLightsBuffer { SpotLight spotLights[]; };

in vec2 TexCoord;
in vec3 Normal;
in vec3 Tangent;
in vec3 BiTangent;
in vec4 WorldPos;
in vec3 ViewPos;
in flat int ViewportIndex;

uniform float u_alphaBoost = 1.0;
uniform vec3 u_moonlightDir;

void main() {
    Material material = materials[MaterialIndex];
    vec4 baseColor = texture(sampler2D(textureSamplers[material.basecolor]), TexCoord);
    vec3 normalMap = texture(sampler2D(textureSamplers[material.normal]), TexCoord).rgb;   
    vec3 rma = texture(sampler2D(textureSamplers[material.rma]), TexCoord).rgb;  

	baseColor.rgb = pow(baseColor.rgb, vec3(2.2));
    float finalAlpha = baseColor.a;// * u_alphaBoost;


    mat3 tbn = mat3(Tangent, BiTangent, Normal);
    vec3 normal = normalize(tbn * (normalMap.rgb * 2.0 - 1.0));

    // Flip backfacing
    if (!gl_FrontFacing) {
        normal = -normal; 
    }

    finalAlpha = clamp(finalAlpha, 0, 1);
    
    float roughness = clamp(rma.r * RoughnessFactor, 0.0, 1.0);
    float metallic = clamp(rma.g * MetallicFactor, 0.0, 1.0);
    float ao = rma.b;

    // Tiled lights
    ivec2 tile = ivec2(gl_FragCoord.xy) / TILE_SIZE;
    uint tileIndex  = uint(tile.y) * rendererData.tileCountX + uint(tile.x);
    uint lightCount = tileLights[tileIndex].lightCount;

    vec3 directLighting = vec3(0.0);
    
    // Point lights
  for(uint i = 2; i < 4; ++i) {
      uint lightIndex = i;//tileData[tileIndex].lightIndices[i];
      Light light = lights[lightIndex];
      vec3 lightPosition = vec3(light.posX, light.posY, light.posZ);
      vec3 lightColor = vec3(light.colorR, light.colorG, light.colorB);
      float shadow = 1.0;
      if (light.hiResShadowMapIndex != -1) {
          shadow = ShadowCalculationNEW(light.hiResShadowMapIndex, lightPosition, light.radius, WorldPos.xyz, ViewPos, normal, hiResShadowMapArray);
      }
      else if (light.lowResShadowMapIndex != -1) {
          shadow = ShadowCalculationNEW(light.lowResShadowMapIndex, lightPosition, light.radius, WorldPos.xyz, ViewPos, normal, lowResShadowMapArray);
      }
      vec3 directLight = GetDirectLighting(lightPosition, lightColor, light.radius, light.strength, normal, WorldPos.xyz, baseColor.rgb, roughness, metallic, ViewPos) * shadow;
      //vec3 directLight = GetDirectLightingHair(lightPosition, lightColor, light.radius, light.strength, normal, Tangent, WorldPos.xyz, baseColor.rgb, roughness, metallic, ViewPos) * shadow;
      
      if (light.iesTextureIndex != 0) {
          sampler2D iesSampler = sampler2D(textureSamplers[(light.iesTextureIndex)]);
          float candelas = ApplyIESProfile(WorldPos.xyz, light, iesSampler);
          directLight *= candelas;
      }

      directLighting += directLight;
  }

    
    float sssRadius = 0.02;
    float sssStrength = 5.0;
    float fragDistance = distance(WorldPos.xyz, ViewPos); // is this right?

    uint spotLightCount = tileSpotLights[tileIndex].lightCount;
    for (uint i = 0u; i < spotLightCount; i++) {
        SpotLight spotLight = spotLights[tileSpotLights[tileIndex].lightIndices[i]];
        sampler2D iesTexture = sampler2D(textureSamplers[max(rendererData.flashlightIESTextureIndex, 0)]);
        directLighting += GetSpotLightContribution(spotLight, rendererData, uint(ViewportIndex), ViewPos, normal.xyz, WorldPos.xyz, baseColor.rgb, roughness, metallic, fragDistance, -1000.0, iesTexture, flashlightShadowMapArray);
    }
   
    vec3 moonColor = rendererData.moonLightColorStrength.rgb;
    float moonLightStrength = rendererData.moonLightColorStrength.a;
    vec3 moonLighting = GetDirectionalLighting(u_moonlightDir, moonColor, moonLightStrength, normal.xyz, WorldPos.xyz, baseColor.rgb, roughness, metallic, ViewPos);
    
    vec3 radius = vec3(sssRadius);
    vec3 subColor = Saturate(baseColor.rgb, 1.5);
    vec3 L = u_moonlightDir;
    float NdotL = max(dot(normal.xyz, L), 0.0);
    vec3 sss = 0.2 * exp(-3.0 * abs(NdotL) / (radius + 0.001)); 
    vec3 sssColor = subColor * radius * sss * sssStrength;
    float csmShadow = 1.0;
    //moonLighting += sssColor * csmShadow * 1.0; // OG

    vec3 finalColor = directLighting.rgb + moonLighting;
    finalColor *= ao;
    
    finalColor.rgb = finalColor.rgb * finalAlpha;
    FragOut = vec4(finalColor, finalAlpha * 1.0);

    // Write current depth to be usefd as previous depth for next layer
    ViewSpaceDepthPreviousOut = vec4(gl_FragCoord.z, 0.0, 0.0, 0.0);
}
