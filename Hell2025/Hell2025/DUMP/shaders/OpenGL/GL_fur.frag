#version 460 core
#extension GL_ARB_bindless_texture : enable
#extension GL_EXT_nonuniform_qualifier : enable
#include "../common/OpenGL/GL_binding_indices.glsl"
#include "../common/lighting.glsl"
#include "../common/post_processing.glsl"
#include "../common/types.glsl"
#include "../common/util.glsl"

layout (binding = TEX_IDX_SHADOW_MAP_FLASHLIGHT) uniform sampler2DArray FlashlighShadowMapArrayTexture;
layout (binding = TEX_IDX_SHADOW_MAP_HI_RES)     uniform samplerCubeArrayShadow highResShadowCubeMapArray;
layout (binding = TEX_IDX_SHADOW_MAP_LOW_RES)    uniform samplerCubeArrayShadow lowResShadowCubeMapArray;

layout (binding = 4) uniform sampler2D BaseColorTexture;
layout (binding = 5) uniform sampler2D NormalTexture;
layout (binding = 6) uniform sampler2D RMATexture;
layout (binding = 7) uniform sampler2D BlueNoiseTexture;
layout (binding = 9) uniform sampler2D FurMaskTexture;

layout (location = 0) out vec4 FinalLightingOut;

readonly restrict layout(std430, binding = SSBO_IDX_SAMPLERS) buffer textureSamplersBuffer { uvec2 textureSamplers[]; };
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
uniform int u_viewportIndex;
in float H0;

void main() {
    vec2 uv = TexCoord * 20.0; // bunny
    const float MAX_LOD = 3.0;

    float furMask = texture(BaseColorTexture, TexCoord).r;

    float wantLod = textureQueryLod(BlueNoiseTexture, uv).x;
    float lod = clamp(wantLod, 0.0, MAX_LOD);
    float blueNoise = textureLod(BlueNoiseTexture, uv, lod).r;

    if (blueNoise < 0.25) {
       discard;
    }

    float taper = pow(1.0 - H0, 2.0); 
    float alpha = taper * blueNoise;
    
    alpha *= 2;
    alpha = clamp(alpha, 0, 1);

    if (alpha < 0.05) {
       discard;
    }
    
    vec3 viewPos = viewportData[u_viewportIndex].viewPos.xyz;
    vec4 baseColor = texture(BaseColorTexture, TexCoord);
    vec3 gammaBaseColor = pow(baseColor.rgb, vec3(2.2));

    vec4 normalMapData = texture(NormalTexture, TexCoord) * 2.0 - 1.0;
    
    float normalMapStrength = 1.0 - (H0 * 0.8); 
    vec3 blendedMap = mix(vec3(0.0, 0.0, 1.0), normalMapData.rgb, normalMapStrength);
    
    mat3 TBN = mat3(normalize(Tangent), normalize(BiTangent), normalize(Normal));
    vec3 normal = normalize(TBN * blendedMap);

    if (!gl_FrontFacing) {
        normal = -normal;
    }

    vec4 rma = texture(RMATexture, TexCoord);
    float roughness = rma.r;
    float metallic = rma.g;

    ivec2 tile = ivec2(gl_FragCoord.xy) / TILE_SIZE;
    uint tileIndex = uint(tile.y) * rendererData.tileCountX + uint(tile.x);
    uint lightCount = tileLights[tileIndex].lightCount;

    vec3 directLighting = vec3(0.0);
    for (uint i = 0; i < lightCount; ++i) {
        uint lightIndex = tileLights[tileIndex].lightIndices[i];
        Light light = lights[lightIndex];
        vec3 lightPosition = vec3(light.posX, light.posY, light.posZ);

        vec3 L = lightPosition - WorldPos.xyz;
        float distSq = dot(L, L);
        //float radiusSq = light.radius * light.radius;
        float radiusSq = 3.5 * 3.5;
        
        if (distSq < radiusSq) {
            float nDotL = dot(normal, normalize(L));

            // Only bother lighting if it's hitting the front face
            if (nDotL > 0.0) { 
                vec3 lightColor = vec3(light.colorR, light.colorG, light.colorB);
                
                float shadow = 1.0;
                if (light.hiResShadowMapIndex != -1) {
                    shadow = ShadowCalculationNEW(light.hiResShadowMapIndex, lightPosition, light.radius, WorldPos.xyz, viewPos, normal, highResShadowCubeMapArray);
                }
                else if (light.lowResShadowMapIndex != -1) {
                    shadow = ShadowCalculationNEW(light.lowResShadowMapIndex, lightPosition, light.radius, WorldPos.xyz, viewPos, normal, lowResShadowCubeMapArray);
                }
                
                if (shadow > 0.01) {
                    vec3 directLight = GetDirectLighting(lightPosition, lightColor, light.radius, light.strength, normal, WorldPos.xyz, gammaBaseColor, roughness, metallic, viewPos) * shadow;
                    
                    if (light.iesTextureIndex != 0) {
                        sampler2D iesSampler = sampler2D(textureSamplers[(light.iesTextureIndex)]);
                        directLight *= ApplyIESProfile(WorldPos.xyz, light, iesSampler);
                    }
                    directLighting += directLight;
                }
            }
        }
    }

    float fragDistance = distance(WorldPos.xyz, viewPos);
    uint spotLightCount = tileSpotLights[tileIndex].lightCount;
    for (uint i = 0u; i < spotLightCount; i++) {
        SpotLight spotLight = spotLights[tileSpotLights[tileIndex].lightIndices[i]];
        sampler2D iesTexture = sampler2D(textureSamplers[max(rendererData.flashlightIESTextureIndex, 0)]);
        directLighting += GetSpotLightContribution(spotLight, rendererData, uint(u_viewportIndex), viewPos, normal, WorldPos.xyz, gammaBaseColor, roughness, metallic, fragDistance, -1000.0, iesTexture, FlashlighShadowMapArrayTexture);
    }

    directLighting = clamp(directLighting, 0, 1);

    // Ambient light
    vec3 amibentLightColor = vec3(1, 0.98, 0.94);
    float ambientIntensity = 0.0025;
    vec3 ambientColor = baseColor.rgb * amibentLightColor;
    vec3 ambientLighting = ambientColor * ambientIntensity;

    // Ambient hack
    float factor = min(1, 1 - metallic * 1.0);
    ambientLighting *= (1.0) * vec3(factor);

    // composite PBR
    vec3 finalColor = directLighting + ambientLighting;
    finalColor = clamp(finalColor, 0, 1);

    // silhouette boost logic (keeping your structure)
    vec3 V = normalize(viewPos - WorldPos.xyz);
    float ndotv = dot(normal, V);
    
    FinalLightingOut = vec4(finalColor, alpha);
}
