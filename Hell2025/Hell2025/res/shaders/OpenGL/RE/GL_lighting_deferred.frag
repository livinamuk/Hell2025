#version 460

#extension GL_ARB_bindless_texture : enable
#include "../../common/OpenGL/GL_binding_indices.glsl"
#include "../../common/constants.glsl"
#include "../../common/lighting.glsl"
#include "../../common/normal_encoding.glsl"
#include "../../common/post_processing.glsl"
#include "../../common/types.glsl"
#include "../../common/reconstruction.glsl"
#include "../../common/util.glsl"
#include "../../common/viewport.glsl"
#include "../../common/ddgi_upsample.glsl"

layout (location = 0) out vec4 LightingOut;

layout (binding = TEX_IDX_SHADOW_MAP_FLASHLIGHT) uniform sampler2DArray u_flashlighShadowMapArrayTexture;
layout (binding = TEX_IDX_SHADOW_MAP_HI_RES)     uniform samplerCubeArrayShadow u_hiResShadowMapArray;
layout (binding = TEX_IDX_SHADOW_MAP_LOW_RES)    uniform samplerCubeArrayShadow u_lowResShadowMapArray;
layout (binding = TEX_IDX_SHADOW_MAP_CSM)        uniform sampler2DArray u_shadowMapCascadeArray;

layout (binding = 4) uniform sampler2D u_baseColorMetallicTexture;
layout (binding = 5) uniform sampler2D u_normalXYRoughnessMiscTexture;
layout (binding = 6) uniform sampler2D u_velocityXYOcclusionSubSurfaceTexture;
layout (binding = 7) uniform sampler2D u_depthTexture;
layout (binding = 8) uniform sampler2D u_indirectDiffuseTexture;
layout (binding = 10) uniform sampler2D u_indirectDiffuseSurfaceTexture;

readonly restrict layout(std430, binding = SSBO_IDX_SAMPLERS)                       buffer textureSamplersBuffer { uvec2 textureSamplers[]; };
readonly restrict layout(std430, binding = SSBO_IDX_RENDERER_DATA)                  buffer rendererDataBuffer { RendererData rendererData; };
readonly restrict layout(std430, binding = SSBO_IDX_VIEWPORT_DATA)                  buffer viewportDataBuffer { ViewportData viewportDataArr[]; };
readonly restrict layout(std430, binding = SSBO_IDX_SCENE_RENDER_ITEMS)             buffer renderItemsBuffer { RenderItem renderItems[]; };
readonly restrict layout(std430, binding = SSBO_IDX_LIGHTS)                         buffer lightsBuffer { Light lights[]; };
readonly restrict layout(std430, binding = SSBO_IDX_LIGHTING_TILE_LIGHTS)           buffer tileLightsBuffer   { TileLights tileLights[];   };
readonly restrict layout(std430, binding = SSBO_IDX_LIGHTING_TILE_SPOT_LIGHTS)      buffer tileSpotLightsBuffer { TileSpotLights tileSpotLights[]; };
readonly restrict layout(std430, binding = SSBO_IDX_SPOT_LIGHTS)                    buffer spotLightsBuffer { SpotLight spotLights[]; };
readonly restrict layout(std430, binding = SSBO_IDX_LIGHTING_TILE_CHRISTMAS_LIGHTS) buffer tileChristmasLightsBuffer  { TileInstanceData tileChristmasLights[]; };
readonly restrict layout(std430, binding = SSBO_IDX_LIGHTING_CHRISTMAS_LIGHTS)      buffer ChristmasLightsBuffer { ChristmasLight christmasLights[]; };
readonly restrict layout(std430, binding = SSBO_IDX_LIGHTING_CHRISTMAS_INDEX_POOL)  buffer ChristmasLightsIndexBuffer { uint globalChristmasLightIndices[]; };

// Moon lighting
uniform float u_cascadeFarPlane = 256.0;
uniform float u_cascadePlaneDistances[16];
#include "../../common/moon_lighting.glsl"

// TODO: dont hardcode
uniform float u_oceanHeight = 30;

// Grass uses a dedicated draw, so this branch is uniform across each draw.
uniform int u_grassLightingPass = 0;
uniform float u_grassNormalUpBlend = 0.35;
uniform float u_grassNormalBlendStartDistance = 3.0;
uniform float u_grassNormalBlendEndDistance = 10.0;
uniform float u_grassDiffuseWrap = 0.35;
uniform float u_grassTransmissionPower = 4.0;
uniform float u_grassSpecularStrength = 0.25;

float GetGrassWrappedNdotL(vec3 normal, vec3 lightDirection) {
    float nDotL = dot(normal, lightDirection);
    return clamp((nDotL + u_grassDiffuseWrap) / (1.0 + u_grassDiffuseWrap), 0.0, 1.0);
}

vec3 GetGrassDirectResponse(vec3 lightDirection, vec3 lightColor, float lightScale, vec3 detailNormal, vec3 macroNormal, vec3 baseColor, float roughness, float metallic, vec3 viewDirection, float normalLod) {
    vec3 halfVector = normalize(lightDirection + viewDirection);
    vec3 F0 = mix(vec3(0.04), baseColor, metallic);
    vec3 fresnel = FresnelSchlick(max(dot(halfVector, viewDirection), 0.0), F0);
    vec3 diffuseBrdf = (vec3(1.0) - fresnel) * (1.0 - metallic) * baseColor / PI;
    float wrappedNdotL = GetGrassWrappedNdotL(macroNormal, lightDirection);

    float detailNdotL = max(dot(detailNormal, lightDirection), 0.0);
    float specularRoughness = mix(roughness, 1.0, normalLod);
    float specularScale = u_grassSpecularStrength * (1.0 - normalLod);
    vec3 specularBrdf = microfacetBRDFSpecularOnly(lightDirection, viewDirection, detailNormal, baseColor, metallic, 1.0, specularRoughness);

    vec3 diffuse = diffuseBrdf * wrappedNdotL;
    vec3 specular = specularBrdf * detailNdotL * specularScale;
    return (diffuse + specular) * lightScale * clamp(lightColor, 0.0, 1.0);
}

vec3 GetGrassPointLighting(vec3 lightPosition, vec3 lightColor, float radius, float strength, vec3 detailNormal, vec3 macroNormal, vec3 worldPosition, vec3 baseColor, float roughness, float metallic, vec3 viewPosition, float normalLod) {
    vec3 toLight = lightPosition - worldPosition;
    float lightDistance = max(length(toLight), 0.000001);
    vec3 lightDirection = toLight / lightDistance;
    vec3 viewDirection = normalize(viewPosition - worldPosition);
    float attenuation = smoothstep(radius, 0.0, lightDistance) * strength;
    return GetGrassDirectResponse(lightDirection, lightColor, attenuation, detailNormal, macroNormal, baseColor, roughness, metallic, viewDirection, normalLod);
}

vec3 GetGrassSpotLightContribution(SpotLight light, RendererData settings, uint viewportIndex, vec3 viewPosition, vec3 detailNormal, vec3 macroNormal, vec3 worldPosition, vec3 baseColor, float roughness, float metallic, float fragDistance, float normalLod, float oceanHeight, sampler2D iesTexture, sampler2DArray shadowMapArray) {
    float modifier = light.positionModifier.w;
    if (modifier <= 0.05) return vec3(0.0);

    vec3 lightPosition;
    vec3 lightDirection;
    vec3 lightColor;
    GetSpotLightShadingInputs(light, settings, viewportIndex, lightPosition, lightDirection, lightColor);

    vec3 toLight = lightPosition - worldPosition;
    float lightDistance = length(toLight);
    if (lightDistance >= settings.flashlightRange) return vec3(0.0);

    vec3 surfaceToLight = toLight / max(lightDistance, 0.000001);
    float attenuation = GetFlashlightIESAttenuation(worldPosition, lightPosition, lightDirection, settings, iesTexture);
    if (worldPosition.y < oceanHeight - 0.1) attenuation *= 2.0;
    if (attenuation <= 0.0) return vec3(0.0);

    vec3 viewDirection = normalize(viewPosition - worldPosition);
    vec3 lighting = GetGrassDirectResponse(surfaceToLight, lightColor, attenuation, detailNormal, macroNormal, baseColor, roughness, metallic, viewDirection, normalLod);

    const uint castShadowsFlag = 1u << 0;
    const uint skipOwnerShadowFlag = 1u << 1;
    uint flags = uint(light.metadata.z);
    int shadowLayer = light.metadata.x;
    int ownerViewportIndex = light.metadata.y;
    bool skipOwnerShadow = ownerViewportIndex == int(viewportIndex) && (flags & skipOwnerShadowFlag) != 0u;
    if (shadowLayer >= 0 && (flags & castShadowsFlag) != 0u && !skipOwnerShadow) {
        vec4 fragPosLightSpace = light.projectionView * vec4(worldPosition, 1.0);
        float shadow = SpotlightShadowCalculation(fragPosLightSpace, macroNormal, lightDirection, worldPosition, lightPosition, viewPosition, shadowMapArray, shadowLayer);
        lighting *= 1.0 - shadow;
    }

    if ((flags & (1u << 2)) != 0u) lighting *= GetFlashlightViewDistanceScale(fragDistance);
    return lighting * modifier;
}

void main() {
	ivec2 px = ivec2(gl_FragCoord.xy);
    ivec2 outputImageSize = ivec2(rendererData.gBufferWidth, rendererData.gBufferHeight);

    uint viewportIndex = ViewportIndexFromPixel(px, outputImageSize, rendererData.viewportLayout, vec2(rendererData.viewportSplitX, rendererData.viewportSplitY));
    vec2 screenUV = ScreenUVFromPixel(px, outputImageSize);
    ivec4 viewportRect = ivec4(viewportDataArr[viewportIndex].xOffset, viewportDataArr[viewportIndex].yOffset, viewportDataArr[viewportIndex].width, viewportDataArr[viewportIndex].height);
    vec2 viewportUV = ViewportUVFromPixel(px, outputImageSize, viewportRect);

    vec4 normalXYRoughnessMisc = texelFetch(u_normalXYRoughnessMiscTexture, px, 0);
    vec3 normal = DecodeOct(normalXYRoughnessMisc.rg);
    float roughness = normalXYRoughnessMisc.b;
    float misc = normalXYRoughnessMisc.a;

    vec4 baseColorMetallic = texelFetch(u_baseColorMetallicTexture, px, 0);
    vec3 baseColor = baseColorMetallic.rgb;
    float metallic = baseColorMetallic.a;
    vec3 linearBaseColor = pow(baseColor.rgb, vec3(2.2)); // baseColor.rgb * baseColor.rgb;

    vec4 velocityXYOcclusionSubSurface = texelFetch(u_velocityXYOcclusionSubSurfaceTexture, px, 0).rgba;
    float ao =velocityXYOcclusionSubSurface.b;
    float subSurface = velocityXYOcclusionSubSurface.a;
    vec2 velocity = velocityXYOcclusionSubSurface.rg;

    mat4 inverseProjection = viewportDataArr[viewportIndex].inverseProjection;
    mat4 inverseProjectionViewReverseZ = viewportDataArr[viewportIndex].inverseJitteredProjectionViewReverseZ;
    mat4 inverseView = viewportDataArr[viewportIndex].inverseView;
    mat4 viewMatrix = viewportDataArr[viewportIndex].view;
    vec3 viewPos = viewportDataArr[viewportIndex].viewPos.xyz;
    bool thisViewportIsInShop = bool(viewportDataArr[viewportIndex].isInShop);

    // Tile data
    uvec2 tileCoord = uvec2(px) / uint(TILE_SIZE);
    uint tileIndex = tileCoord.y * rendererData.tileCountX + tileCoord.x;
	uint lightCount = tileLights[tileIndex].lightCount;

    // Depth reconstruction
    float depth = texelFetch(u_depthTexture, px, 0).r;
    vec3 worldPos = WorldPosFromDepth(viewportUV, depth, inverseProjectionViewReverseZ);

    float fragDistance = distance(worldPos, viewPos);
    vec3 detailNormal = normal;
    float grassNormalLod = 0.0;
    vec3 macroNormal = normal;
    if (u_grassLightingPass != 0) {
        grassNormalLod = smoothstep(u_grassNormalBlendStartDistance, u_grassNormalBlendEndDistance, fragDistance);
        macroNormal = normalize(mix(detailNormal, vec3(0.0, 1.0, 0.0), u_grassNormalUpBlend * grassNormalLod));
    }

    vec3 F0 = mix(vec3(0.04), linearBaseColor, metallic);



    vec3 directLighting = vec3(0.0);

    // Direct light (point lights)
    for (int i = 0; i < lightCount; i++) {
        int lightIndex = int(tileLights[tileIndex].lightIndices[i]);

        Light light = lights[lightIndex];
        vec3 lightPosition = vec3(light.posX, light.posY, light.posZ);
        vec3 lightColor =  vec3(light.colorR, light.colorG, light.colorB);
        float lightStrength = light.strength;
        float lightRadius = light.radius;

        float shadow = 1.0;
        if (light.hiResShadowMapIndex != -1) {
            shadow = ShadowCalculationSkin(light.hiResShadowMapIndex, lightPosition, lightRadius, worldPos.xyz, viewPos, macroNormal, u_hiResShadowMapArray);
        }
        else if (light.lowResShadowMapIndex != -1) {
            shadow = ShadowCalculationSkin(light.lowResShadowMapIndex, lightPosition, lightRadius, worldPos.xyz, viewPos, macroNormal, u_lowResShadowMapArray);
        }

        vec3 directLight;
        if (u_grassLightingPass != 0) {
            directLight = GetGrassPointLighting(lightPosition, lightColor, lightRadius, lightStrength, detailNormal, macroNormal, worldPos.xyz, linearBaseColor.rgb, roughness, metallic, viewPos, grassNormalLod) * shadow;
        }
        else {
            directLight = GetDirectLighting(lightPosition, lightColor, lightRadius, lightStrength, normal.xyz, worldPos.xyz, linearBaseColor.rgb, roughness, metallic, viewPos) * shadow;
        }

        if (light.iesTextureIndex != 0) {
            float candelas = ApplyIESProfile(worldPos.xyz, light, sampler2D(textureSamplers[light.iesTextureIndex]));
            directLight *= candelas;
        }

        directLighting += directLight;
    }

    // Direct light (Christmas lights)
    uint christmasLightCount = tileChristmasLights[tileIndex].count; // Num of Chrissy lights in this tile
    uint christmasLightOffset = tileChristmasLights[tileIndex].offset;

    for (uint i = 0; i < christmasLightCount; ++i) {
        uint idx = globalChristmasLightIndices[christmasLightOffset + i];
        vec3 lightPosition = christmasLights[idx].position.xyz;
        vec3 lightColor = christmasLights[idx].color.rgb;
        float lightRadius = rendererData.christmasLightRadius;
        float lightStrength = rendererData.christmasLightStrength;

        if (u_grassLightingPass != 0) {
            directLighting += GetGrassPointLighting(lightPosition, lightColor, lightRadius, lightStrength, detailNormal, macroNormal, worldPos.xyz, linearBaseColor.rgb, roughness, metallic, viewPos, grassNormalLod);
        }
        else {
            directLighting += GetDirectLighting(lightPosition, lightColor, lightRadius, lightStrength, normal.xyz, worldPos.xyz, linearBaseColor.rgb, roughness, metallic, viewPos);
        }
    }

    // Flash lights
    uint spotLightCount = tileSpotLights[tileIndex].lightCount;
    for (uint i = 0u; i < spotLightCount; i++) {
        SpotLight spotLight = spotLights[tileSpotLights[tileIndex].lightIndices[i]];
        sampler2D iesTexture = sampler2D(textureSamplers[max(rendererData.flashlightIESTextureIndex, 0)]);
        if (u_grassLightingPass != 0) {
            directLighting += GetGrassSpotLightContribution(spotLight, rendererData, viewportIndex, viewPos, detailNormal, macroNormal, worldPos.xyz, linearBaseColor.rgb, roughness, metallic, fragDistance, grassNormalLod, u_oceanHeight, iesTexture, u_flashlighShadowMapArrayTexture);
        }
        else {
            directLighting += GetSpotLightContribution(spotLight, rendererData, viewportIndex, viewPos, normal.xyz, worldPos.xyz, linearBaseColor.rgb, roughness, metallic, fragDistance, u_oceanHeight, iesTexture, u_flashlighShadowMapArrayTexture);
        }
    }


    // Mermaid shop point light
    if (viewportDataArr[viewportIndex].isInShop == 1) {

        vec3 lightPosition = viewPos;
        //vec3 lightColor = vec3(1.00, 0.7799999713897705, 0.5289999842643738);
        vec3 lightColor = rendererData.moonLightColorStrength.xyz;
        float lightRadius = 1.0;
        float lightStrength = 3.0;

        directLighting += GetDirectLighting(lightPosition, lightColor, lightRadius, lightStrength, normal.xyz, worldPos.xyz, linearBaseColor.rgb, roughness, metallic, viewPos);
    }

    // Indirect diffuse
    vec3 indirectDiffuse = vec3(0);
    if (rendererData.enableIrradianceProbeSampling) {
        vec3 probeIrradiance = SampleDDGIIndirectDiffuseBilateral(u_indirectDiffuseTexture, u_indirectDiffuseSurfaceTexture, screenUV, normal, fragDistance, outputImageSize, viewportRect);
        vec3 diffuseAlbedo = linearBaseColor.rgb * (1.0 - metallic);
        indirectDiffuse = probeIrradiance * diffuseAlbedo;
    }

    // Extra near-camera light for the first-person view weapon. This is
    // deliberately separate from the world-space IES flashlight beam.
    /*
    if (viewportDataArr[viewportIndex].flashlightModifer > 0.1) {
        vec3 offset = viewportDataArr[viewportIndex].cameraForward.xyz * 0.001;
        vec3 spotLightPos = viewPos + offset;
        vec3 spotLightDir = viewportDataArr[viewportIndex].flashlightDir.xyz;
        vec3 spotLightColor = vec3(0.9, 0.95, 1.1);

        spotLightColor = mix(spotLightColor, vec3(1, 0.7799999713897705, 0.5289999842643738), 0.5);

        float spotLightRadius = 0.165;
        float spotLightStregth = 10.0;
        float innerAngle = cos(radians(0.0 * viewportDataArr[viewportIndex].flashlightModifer));
        float outerAngle = cos(radians(40.0));

        mat4 lightProjectionView = viewportDataArr[viewportIndex].flashlightProjectionView;
        vec3 flashlightViewPos = viewportDataArr[viewportIndex].inverseView[3].xyz;

        vec3 re7Lighting = GetSpotlightLighting(spotLightPos, spotLightDir, spotLightColor, spotLightRadius, spotLightStregth, innerAngle, outerAngle, normal.xyz, worldPos, linearBaseColor.rgb, roughness, metallic, flashlightViewPos, lightProjectionView);
        directLighting += re7Lighting;
    }
    */

    // Moon light
    vec3 moonLighting = vec3(0.0);
    vec3 moonLightDir = rendererData.moonLightDir.xyz;
    bool hasSubSurface = subSurface > 0.0 && subSurface < 0.99;

    if (u_grassLightingPass != 0) {
        float macroMoonNdotL = dot(macroNormal, moonLightDir);
        float wrappedMoonNdotL = GetGrassWrappedNdotL(macroNormal, moonLightDir);
        float detailMoonNdotL = max(dot(detailNormal, moonLightDir), 0.0);

        if (wrappedMoonNdotL > 0.0 || detailMoonNdotL > 0.0 || hasSubSurface) {
            vec3 shadowNormal = macroMoonNdotL >= 0.0 ? macroNormal : -macroNormal;
            vec3 shadow = ShadowCalculationCSM(worldPos, shadowNormal, moonLightDir, viewMatrix, viewportIndex);

            if (any(greaterThan(shadow, vec3(0.0)))) {
                vec3 viewDirection = normalize(viewPos - worldPos);
                moonLighting = GetGrassDirectResponse(moonLightDir, rendererData.moonLightColorStrength.rgb, rendererData.moonLightColorStrength.a, detailNormal, macroNormal, linearBaseColor.rgb, roughness, metallic, viewDirection, grassNormalLod) * shadow;

                if (hasSubSurface) {
                    float backNdotL = max(-macroMoonNdotL, 0.0);
                    float forwardScatter = pow(max(dot(viewDirection, -moonLightDir), 0.0), u_grassTransmissionPower);
                    float transmission = subSurface * backNdotL * mix(0.25, 1.0, forwardScatter);
                    vec3 transmissionColor = linearBaseColor.rgb * rendererData.moonLightColorStrength.rgb;
                    moonLighting += transmissionColor * rendererData.moonLightColorStrength.a * transmission * shadow;
                }
            }
        }
    }
    else {
        float moonNdotL = dot(normal.xyz, moonLightDir);
        if (moonNdotL > 0.0 || hasSubSurface) {
            vec3 shadowNormal = moonNdotL >= 0.0 ? normal.xyz : -normal.xyz;
            vec3 shadow = ShadowCalculationCSM(worldPos, shadowNormal, moonLightDir, viewMatrix, viewportIndex);

            if (any(greaterThan(shadow, vec3(0.0)))) {
                if (moonNdotL > 0.0) {
                    moonLighting = GetDirectionalLighting(moonLightDir, rendererData.moonLightColorStrength.rgb, rendererData.moonLightColorStrength.a, normal.xyz, worldPos, linearBaseColor.rgb, roughness, metallic, viewPos);
                    moonLighting *= shadow;
                }

                if (hasSubSurface) {
                    vec3 viewDir = normalize(viewPos - worldPos);
                    float backNdotL = max(-moonNdotL, 0.0);
                    float forwardScatter = pow(max(dot(viewDir, -moonLightDir), 0.0), 4.0);
                    float transmission = subSurface * backNdotL * mix(0.25, 1.0, forwardScatter);
                    vec3 transmissionColor = linearBaseColor.rgb * rendererData.moonLightColorStrength.rgb;
                    moonLighting += transmissionColor * rendererData.moonLightColorStrength.a * transmission * shadow;
                }
            }
        }
    }

    vec3 finalColor = (directLighting + indirectDiffuse + moonLighting) * ao;

    //#ifdef VIEW_WEAPON
    //    finalColor *= vec3(1, 0, 0);
    //#endif


    LightingOut = vec4(finalColor, 1);
}
