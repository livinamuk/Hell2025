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
uniform int u_editorRenderMode;

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
            shadow = ShadowCalculationSkin(light.hiResShadowMapIndex, lightPosition, lightRadius, worldPos.xyz, viewPos, normal.xyz, u_hiResShadowMapArray);
        }
        else if (light.lowResShadowMapIndex != -1) {
            shadow = ShadowCalculationSkin(light.lowResShadowMapIndex, lightPosition, lightRadius, worldPos.xyz, viewPos, normal.xyz, u_lowResShadowMapArray);
        }

        vec3 directLight = GetDirectLighting(lightPosition, lightColor, lightRadius, lightStrength, normal.xyz, worldPos.xyz, linearBaseColor.rgb, roughness, metallic, viewPos) * shadow;

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

        directLighting += GetDirectLighting(lightPosition, lightColor, lightRadius, lightStrength, normal.xyz, worldPos.xyz, linearBaseColor.rgb, roughness, metallic, viewPos);
    }

    // Flash lights
    uint spotLightCount = tileSpotLights[tileIndex].lightCount;
    for (uint i = 0u; i < spotLightCount; i++) {
        SpotLight spotLight = spotLights[tileSpotLights[tileIndex].lightIndices[i]];
        sampler2D iesTexture = sampler2D(textureSamplers[max(rendererData.flashlightIESTextureIndex, 0)]);
        directLighting += GetSpotLightContribution(spotLight, rendererData, viewportIndex, viewPos, normal.xyz, worldPos.xyz, linearBaseColor.rgb, roughness, metallic, fragDistance, u_oceanHeight, iesTexture, u_flashlighShadowMapArrayTexture);
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
    float moonNdotL = dot(normal.xyz, moonLightDir);
    bool hasSubSurface = subSurface > 0.0 && subSurface < 0.99;

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

    vec3 finalColor = (directLighting + indirectDiffuse + moonLighting) * ao;


    // SOLID COLOR
    if (u_editorRenderMode == 1) {
        vec3 fakeProbeIrradiance = vec3(1.00, 0.7799999713897705, 0.5289999842643738) * 0.65;
        vec3 diffuseAlbedo = linearBaseColor.rgb * (1.0 - metallic);
        finalColor = fakeProbeIrradiance * diffuseAlbedo;
    }

    // NORMALS
    if (u_editorRenderMode == 2) {
        finalColor = vec3(normal * 0.5) + 0.5;
        finalColor = pow(finalColor, vec3(2.2));
    }

    LightingOut = vec4(finalColor, 1);
}
