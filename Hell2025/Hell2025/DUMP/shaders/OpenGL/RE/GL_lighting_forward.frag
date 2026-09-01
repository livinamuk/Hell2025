#version 460
#include "../../common/OpenGL/GL_binding_indices.glsl"

#extension GL_ARB_bindless_texture : enable
readonly restrict layout(std430, binding = SSBO_IDX_SAMPLERS) buffer textureSamplersBuffer { uvec2 textureSamplers[]; };

layout (binding = TEX_IDX_SHADOW_MAP_FLASHLIGHT) uniform sampler2DArray flashlightShadowMapArray;
layout (binding = TEX_IDX_SHADOW_MAP_HI_RES)     uniform samplerCubeArrayShadow hiResShadowMapArray;
layout (binding = TEX_IDX_SHADOW_MAP_LOW_RES)    uniform samplerCubeArrayShadow lowResShadowMapArray;

layout (binding = 5) uniform sampler2D u_indirectDiffuseTexture;
layout (binding = 7) uniform sampler2DArray woundMaskTextureArray;
layout (binding = 10) uniform sampler2D u_indirectDiffuseSurfaceTexture;
layout (binding = 11) uniform sampler2D hairFlowMap;
layout (binding = 12) uniform sampler2D hairIDMap;
layout (binding = 13) uniform sampler2D hairRootMap;

#include "../../common/lighting.glsl"
#include "../../common/normal_encoding.glsl"
#include "../../common/post_processing.glsl"
#include "../../common/viewport.glsl"
#include "../../common/ddgi_upsample.glsl"

readonly restrict layout(std430, binding = SSBO_IDX_MATERIALS) buffer materialsBuffer { Material materials[]; };
readonly restrict layout(std430, binding = SSBO_IDX_RENDERER_DATA) buffer rendererDataBuffer { RendererData rendererData; };
readonly restrict layout(std430, binding = SSBO_IDX_VIEWPORT_DATA) buffer viewportDataBuffer { ViewportData viewportData[]; };
readonly restrict layout(std430, binding = SSBO_IDX_SCENE_RENDER_ITEMS) buffer renderItemsBuffer { RenderItem renderItems[]; };
readonly restrict layout(std430, binding = SSBO_IDX_LIGHTS) buffer lightsBuffer { Light lights[]; };
readonly restrict layout(std430, binding = SSBO_IDX_LIGHTING_TILE_LIGHTS) buffer tileLightsBuffer   { TileLights tileLights[];   };
readonly restrict layout(std430, binding = SSBO_IDX_LIGHTING_TILE_SPOT_LIGHTS) buffer tileSpotLightsBuffer { TileSpotLights tileSpotLights[]; };
readonly restrict layout(std430, binding = SSBO_IDX_SPOT_LIGHTS) buffer spotLightsBuffer { SpotLight spotLights[]; };
readonly restrict layout(std430, binding = SSBO_IDX_LIGHTING_TILE_CHRISTMAS_LIGHTS) buffer tileChristmasLightsBuffer  { TileInstanceData tileChristmasLights[]; };
readonly restrict layout(std430, binding = SSBO_IDX_LIGHTING_CHRISTMAS_LIGHTS) buffer ChristmasLightsBuffer      { ChristmasLight christmasLights[]; };
readonly restrict layout(std430, binding = SSBO_IDX_LIGHTING_CHRISTMAS_INDEX_POOL) buffer ChristmasLightsIndexBuffer { uint globalChristmasLightIndices[]; };

layout (location = 0) out vec4 LightingOut;
layout (location = 1) out vec4 BaseColorOut;
layout (location = 2) out vec4 NormalOut;
layout (location = 3) out vec4 MaterialOut;
layout (location = 4) out vec4 RENormalOut;

centroid in vec2 TexCoord;
centroid in vec3 Normal;
centroid in vec3 Tangent;
centroid in vec4 WorldPos;
centroid in vec3 ViewPos;

in flat int v_globalInstanceIndex;
in flat int v_viewportIndex;

uniform bool u_alphaDiscard;
uniform bool u_flipNormalMapY;
uniform bool u_solidAlpha;

void main() {
    // grab the item once to avoid multiple buffer lookups
    RenderItem item = renderItems[v_globalInstanceIndex];

    Material material = materials[item.materialIndex];
    vec4 baseColor = texture(sampler2D(textureSamplers[material.basecolor]), TexCoord);
    vec3 normalMap = texture(sampler2D(textureSamplers[material.normal]), TexCoord).rgb;
    vec4 rma = texture(sampler2D(textureSamplers[material.rma]), TexCoord).rgba;
    vec3 emissiveMapColor = texture(sampler2D(textureSamplers[material.emissive]), TexCoord).rgb;

    vec3 viewPos = viewportData[v_viewportIndex].inverseView[3].xyz;

    float roughness = clamp(rma.r * item.roughnessFactor, 0.0, 1.0);
    float metallic = clamp(rma.g * item.metallicFactor, 0.0, 1.0);
    float ao = rma.b;

    vec3 gammaBaseColor = pow(baseColor.rgb, vec3(2.2));

    ViewportData vd = viewportData[v_viewportIndex];
    mat4 inverseProjection = vd.inverseProjection;
    mat4 inverseView = vd.inverseView;
    mat4 viewMatrix = vd.view;
    bool thisViewportIsInShop = bool(vd.isInShop);

    // normal mapping
    normalMap = normalMap * 2.0 - 1.0;
    vec3 n = normalize(Normal);
    vec3 t = normalize(Tangent);
    if (!gl_FrontFacing) n = -n;
    t = normalize(t - dot(t, n) * n);
    vec3 b = cross(n, t);
    mat3 tbn = mat3(t, b, n);
    vec3 normal = normalize(tbn * normalMap);

    // Material out
    //vec3 linearBaseColor = pow(baseColor.rgb, vec3(2.2));
    vec3 linearBaseColor = baseColor.rgb * baseColor.rgb;
    vec3 F0 = mix(vec3(0.04), linearBaseColor, metallic);
    float f0_lum = dot(F0, vec3(0.2126, 0.7152, 0.0722));
    MaterialOut = vec4(roughness, metallic, ao, f0_lum);

    // Fix fireflies
    //float geometricRoughness = length(fwidth(normal));
    //roughness = clamp(roughness + geometricRoughness, 0.0, 1.0);
    //
    //float filterRadius = 0.1;
    //float geometricRoughness = length(fwidth(normal));
    //roughness = max(roughness, geometricRoughness * filterRadius);

    float variation = length(fwidth(normal));
    float smoothnessFactor = 0.5;
    roughness = clamp(roughness + (variation * smoothnessFactor), 0.0, 1.0);

    //float normalMapVariation = length(fwidth(normalMap.rgb));
    //roughness = clamp(roughness + (normalMapVariation * 0.5), 0.0, 1.0);

    vec3 directLighting = vec3(0.0);
    for (int i = 2; i < 4; i++) {
        int lightIndex = i; //int(tileLights[tileIndex].lightIndices[i]);

        Light light = lights[lightIndex];
        vec3 lightPosition = vec3(light.posX, light.posY, light.posZ);
        vec3 lightColor =  vec3(light.colorR, light.colorG, light.colorB);
        float lightStrength = light.strength;
        float lightRadius = light.radius;

        float shadow = 1.0;
        if (light.hiResShadowMapIndex != -1) {
            shadow = ShadowCalculationNEW(light.hiResShadowMapIndex, lightPosition, lightRadius, WorldPos.xyz, viewPos, normal.xyz, hiResShadowMapArray);
        }
        else if (light.lowResShadowMapIndex != -1) {
            shadow = ShadowCalculationNEW(light.lowResShadowMapIndex, lightPosition, lightRadius, WorldPos.xyz, viewPos, normal.xyz, lowResShadowMapArray);
        }

        vec3 directLight = GetDirectLighting(lightPosition, lightColor, lightRadius, lightStrength, normal.xyz, WorldPos.xyz, gammaBaseColor.rgb, roughness, metallic, viewPos) * shadow;

        if (light.iesTextureIndex != 0) {
            sampler2D iesSampler = sampler2D(textureSamplers[(light.iesTextureIndex)]);
            float candelas = ApplyIESProfile(WorldPos.xyz, light, iesSampler);
            directLight *= candelas;
        }

        directLighting += directLight;
    }

    // Tiele index
    ivec2 tile = ivec2(gl_FragCoord.xy) / TILE_SIZE;
    uint tileIndex = uint(tile.y) * rendererData.tileCountX + uint(tile.x);

    // Direct light (Christmas lights)
    uint christmasLightCount = tileChristmasLights[tileIndex].count; // Num of Chrissy lights in this tile
    uint christmasLightOffset = tileChristmasLights[tileIndex].offset;

    for (uint i = 0; i < christmasLightCount; ++i) {
        uint idx = globalChristmasLightIndices[christmasLightOffset + i];
        vec3 lightPosition = christmasLights[idx].position.xyz;
        vec3 lightColor = christmasLights[idx].color.rgb;
        float lightRadius = rendererData.christmasLightRadius;
        float lightStrength = rendererData.christmasLightStrength;

        directLighting += GetDirectLighting(lightPosition, lightColor, lightRadius, lightStrength, normal.xyz, WorldPos.xyz, gammaBaseColor.rgb, roughness, metallic, viewPos);
    }

    // Direct lighting (point lights)
    float fragDistance = distance(WorldPos.xyz, viewPos);
    uint spotLightCount = tileSpotLights[tileIndex].lightCount;
    for (uint i = 0u; i < spotLightCount; i++) {
        SpotLight spotLight = spotLights[tileSpotLights[tileIndex].lightIndices[i]];
        sampler2D iesTexture = sampler2D(textureSamplers[max(rendererData.flashlightIESTextureIndex, 0)]);
        directLighting += GetSpotLightContribution(spotLight, rendererData, uint(v_viewportIndex), viewPos, normal.xyz, WorldPos.xyz, gammaBaseColor.rgb, roughness, metallic, fragDistance, -1000.0, iesTexture, flashlightShadowMapArray);
    }

    vec3 indirectDiffuse = vec3(0.0);

    // Indirect diffuse
    if (rendererData.enableIrradianceProbeSampling) {
        ivec2 outputImageSize = ivec2(rendererData.gBufferWidth, rendererData.gBufferHeight);
        vec2 screenUV = ScreenUVFromFragCoord(gl_FragCoord.xy, outputImageSize);
        ivec4 viewportRect = ivec4(vd.xOffset, vd.yOffset, vd.width, vd.height);
        vec3 probeIrradiance = SampleDDGIIndirectDiffuseBilateral(u_indirectDiffuseTexture, u_indirectDiffuseSurfaceTexture, screenUV, normal, distance(WorldPos.xyz, viewPos), outputImageSize, viewportRect);
        vec3 diffuseAlbedo = gammaBaseColor.rgb * (1.0 - metallic);
        float indirectDiffuseScale = 1.0;
        indirectDiffuse = probeIrradiance * diffuseAlbedo * indirectDiffuseScale;
    }

    
    vec3 finalLitColor = (directLighting + indirectDiffuse) * ao;


    LightingOut.rgb = finalLitColor;
    LightingOut.a = baseColor.a;

    if (u_solidAlpha) {
        LightingOut.a = 1.0;
    }

    NormalOut = vec4(normalize(normal) * 0.5 + 0.5, 1.0);

    RENormalOut.rg = EncodeOct(normal);
    RENormalOut.b = metallic;
    RENormalOut.a = 0.0; // Misc 4 bit value

    //RENormalOut = vec4(1,0,0,1);



    //LightingOut.rgba = vec4(baseColor.rgb, 0);
}
