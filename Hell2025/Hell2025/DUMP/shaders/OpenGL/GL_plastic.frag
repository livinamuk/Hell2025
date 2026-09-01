#version 460 core

#extension GL_ARB_bindless_texture : enable

#include "../common/OpenGL/GL_binding_indices.glsl"

readonly restrict layout(std430, binding = SSBO_IDX_SAMPLERS) buffer textureSamplersBuffer { uvec2 textureSamplers[]; };

#include "../common/lighting.glsl"
#include "../common/post_processing.glsl"
#include "../common/types.glsl"
#include "../common/util.glsl"

readonly restrict layout(std430, binding = SSBO_IDX_LIGHTING_TILE_LIGHTS) buffer tileLightsBuffer { TileLights tileLights[]; };

layout (location = 0) out vec4 ColorOut;
layout (location = 1) out float ViewSpaceDepthPreviousOut;

layout (binding = TEX_IDX_SHADOW_MAP_HI_RES)     uniform samplerCubeArrayShadow hiResShadowMapArray;
layout (binding = TEX_IDX_SHADOW_MAP_LOW_RES)    uniform samplerCubeArrayShadow lowResShadowMapArray;

layout (binding = 4) uniform sampler2D baseColorTexture;
layout (binding = 5) uniform sampler2D normalTexture;
layout (binding = 6) uniform sampler2D rmaTexture;

layout (binding = 7) uniform sampler2D plasticBaseColorTexture;
layout (binding = 8) uniform sampler2D plasticNormalTexture;
layout (binding = 9) uniform sampler2D plasticRmaTexture;

layout (binding = 10) uniform sampler2D u_SceneColorTexture;
layout (binding = 11) uniform sampler2D u_SceneDepthTexture;

in vec2 TexCoord;
in vec3 Normal;
in vec3 Tangent;
in vec3 BiTangent;
in vec4 v_worldPos;

uniform mat4 u_view;
uniform vec3 u_viewPos;
uniform int u_tileXCount;

readonly restrict layout(std430, binding = SSBO_IDX_LIGHTS) buffer lightsBuffer { Light lights[]; };

float InterleavedGradientNoise(vec2 uv) {
    vec3 magic = vec3(0.06711056, 0.00583715, 52.9829189);
    return fract(magic.z * fract(dot(uv, magic.xy)));
}

vec3 GetDirectLightingDiffuseOnly(vec3 lightPos, vec3 lightCol, float lightRadius, float lightStrength, vec3 normal, vec3 v_worldPos, vec3 gammaBaseColor, float roughness, float transparency, vec3 camerav_worldPos) {
    vec3 L = normalize(lightPos - v_worldPos);
    vec3 V = normalize(camerav_worldPos - v_worldPos);
    vec3 H = normalize(L + V);

    float nDl = max(dot(normal, L), 0.0);
    float nDv = max(dot(normal, V), 0.0);
    float hDv = max(dot(H, V), 0.0);

    float f0 = 0.04;
    float F = f0 + (1.0 - f0) * pow(1.0 - hDv, 5.0);
    float kD = 1.0 - F;

    // Disney diffuse factor for rough plastic edges
    float fd90 = 0.5 + 2.0 * roughness * hDv * hDv;
    float fresnelL = 1.0 + (fd90 - 1.0) * pow(1.0 - nDl, 5.0);
    float fresnelV = 1.0 + (fd90 - 1.0) * pow(1.0 - nDv, 5.0);
    float disneyDiffuse = fresnelL * fresnelV;

    // Attenuation
    float dist = length(lightPos - v_worldPos);
    float att = smoothstep(lightRadius, 0.0, dist) * lightStrength;
    vec3 radiance = lightCol * att * nDl;

    // Back scattering
    float backScatter = pow(max(0.0, dot(-L, V)), 2.0) * transparency;

    // Energy conservation from composite
    vec3 diffuseResult = (gammaBaseColor * (disneyDiffuse / PI)) + (gammaBaseColor * backScatter);
    return diffuseResult * kD * radiance;
}

vec3 GetDirectLightingDiffuseOnly2(vec3 lightPos, vec3 lightCol, float lightRadius, float lightStrength, vec3 normal, vec3 v_worldPos, vec3 gammaBaseColor, float roughness, float transparency, vec3 camerav_worldPos) {
    float smokeDensity = 1.0;

    vec3 L = normalize(lightPos - v_worldPos);
    vec3 V = normalize(camerav_worldPos - v_worldPos);
    vec3 H = normalize(L + V);

    float nDl = max(dot(normal, L), 0.0);
    float nDv = max(dot(normal, V), 0.0);
    float hDv = max(dot(H, V), 0.0);

    // Attenuation
    float dist = length(lightPos - v_worldPos);
    float att = smoothstep(lightRadius, 0.0, dist) * lightStrength;

    vec3 diffuse = vec3(0);

    // Front scattering
    float wrap = 0.5; // Higher for softer light wrap, lower for sharper shadows
    float wrappedNdotL = max(0.0, (nDl + wrap) / (1.0 + wrap));
    diffuse += (gammaBaseColor / PI) * wrappedNdotL * att * lightCol;

    // Back scattering
    float backSpread = 4.0; // Lower for wider glow, higher for sharper light spot
    float backlight = max(0.0, dot(-L, V));

    // Thickness
    float localThickness = 0.1;
    float translucencyGlow = pow(backlight, backSpread) * (localThickness * smokeDensity);

    // Hack to tint the backlight so it looks like it passed through the material
    diffuse += (gammaBaseColor * lightCol) * translucencyGlow * att;

    return diffuse;
}


vec3 refraction(vec3 normal) {
    float distortionStrength = 0.02;
    float thicknessSpread = 0.004;
    int samples = 24;

    ivec2 texSize = textureSize(u_SceneColorTexture, 0);
    vec3 viewNormal = normalize(mat3(u_view) * normal);
    vec3 accumulatedRefraction = vec3(0.0);
    for (int i = 0; i < samples; i++) {
        float angle = float(i) * (6.28318 / float(samples));
        vec2 jitter = vec2(cos(angle), sin(angle)) * thicknessSpread;
        vec2 refractedUV = (gl_FragCoord.xy / texSize.xy) + (viewNormal.xy * distortionStrength) + jitter;

        float backgroundDepth = texture2D(u_SceneDepthTexture, refractedUV).r;
        if (backgroundDepth < gl_FragCoord.z) {
            accumulatedRefraction += texture2D(u_SceneColorTexture, gl_FragCoord.xy / texSize.xy).rgb;
        } else {
            accumulatedRefraction += texture2D(u_SceneColorTexture, refractedUV).rgb;
        }
    }
    vec3 refractionColor = accumulatedRefraction / float(samples);
    return refractionColor;
}

vec3 refraction2(vec3 normal) {
    float baseDistortionStrength = 0.02;
    float baseThicknessSpread = 0.004;
    const int samples = 8;

    vec2 texSize = vec2(textureSize(u_SceneColorTexture, 0));
    vec2 screenUV = gl_FragCoord.xy / texSize;

    // Get how much the mesh uvs change over one screen pixel
    float uvRate = max(length(fwidth(TexCoord)), 0.00001);

    // Convert the mesh uv distance to screen uv distance
    float meshToScreenUV = (1.0 / uvRate) / texSize.x;

    // Scale parameters to lock them to the mesh surface
    float distortionStrength = baseDistortionStrength * meshToScreenUV;
    float thicknessSpread = baseThicknessSpread * meshToScreenUV;

    // Base refraction
    vec3 viewNormal = normalize(mat3(u_view) * normal);
    vec2 baseRefractUV = screenUV + (viewNormal.xy * distortionStrength);

    // Unrefracted color for depth fail
    vec3 unrefractedColor = texture2D(u_SceneColorTexture, screenUV).rgb;

    // Precompute rotation
    float angleStep = 6.28318 / float(samples);
    float c = cos(angleStep);
    float s = sin(angleStep);
    mat2 rot = mat2(c, s, -s, c);

    // Initial jitter
    vec2 jitter = vec2(thicknessSpread, 0.0);

    vec3 accumulatedRefraction = vec3(0.0);

    for (int i = 0; i < samples; i++) {
        vec2 sampleUV = baseRefractUV + jitter;
        float backgroundDepth = texture2D(u_SceneDepthTexture, sampleUV).r;

        // Fallback if sample is behind
        if (backgroundDepth < gl_FragCoord.z) {
            accumulatedRefraction += unrefractedColor;
        } else {
            accumulatedRefraction += texture2D(u_SceneColorTexture, sampleUV).rgb;
        }

        // Rotate jitter
        jitter = rot * jitter;
    }

    vec3 finalRefraction = accumulatedRefraction / float(samples);
    return finalRefraction;
}

const vec3 W = vec3(0.2126, 0.7152, 0.0722);

float getBrightness(vec3 color) {
    return dot(color, W);
}

float exponentialRemap(float x, float exponent) {
    return pow(x, exponent);
}

void main() {
    ivec2 px = ivec2(gl_FragCoord.xy);

    // Blending masks
    float baseColorMask = 0.25;
    float rmaMask = 0.00;
    float plasticStrength = 1.0;
    float plasticNormalWeight = 0.00;

    // Smoke and volume tuning
    float transThicknessMaster = 1.5;        // Higher is denser overall     Lower for thinner overall
    float thicknessFloor       = 0.5;        // Higher is smokier center     Lower for clearer center
    float thicknessCeiling     = 2.0;        // Higher is darker edges       Lower for thinner edges
    float noiseJitterStrength  = 2.5;        // Higher is more grain         Lower for smoother smoke
    float smokeDensity         = 1.0;        // Higher for opaque smoke,     Lower for subtle haze
    float absorptionFalloff    = 0.25;       // Higher for more tint/brown,  Lower for less tint and clearer
    vec3 smokeInternalTint     = vec3(0.08); // Higher to fuck up your image even more

    // texture samples
    vec3 materialbaseColor = texture2D(baseColorTexture, TexCoord).rgb;
    vec3 materialNormal = texture2D(normalTexture, TexCoord).rgb;
    vec3 materialRma = texture2D(rmaTexture, TexCoord).rgb;

    vec2 plasticUV = TexCoord * 1.5;
    vec3 plasticBaseColor = texture2D(plasticBaseColorTexture, plasticUV).rgb;
    vec3 plasticNormal = texture2D(plasticNormalTexture, plasticUV).rgb;
    vec3 plasticRma = texture2D(plasticRmaTexture, plasticUV).rgb;

    vec3 baseColor = mix(materialbaseColor, plasticBaseColor, baseColorMask);
    vec3 rma = mix(materialRma, plasticRma, rmaMask);

    // normals
    vec3 n1 = materialNormal * 2.0 - 1.0;
    vec3 n2 = plasticNormal * 2.0 - 1.0;
    n2.xy *= plasticStrength;
    n2.xy *= plasticStrength * plasticNormalWeight;
    n2.z = mix(1.0, n2.z, plasticNormalWeight);

    vec3 blendedNormal = normalize(vec3(n1.xy + n2.xy, n1.z * n2.z));
    mat3 tbn = mat3(normalize(Tangent), normalize(BiTangent), normalize(Normal));
    vec3 normal = normalize(tbn * blendedNormal);

    vec3 gammaBaseColor = pow(baseColor, vec3(2.2));
    float roughness = rma.r;
    float metallic = rma.g;

    //vec3 camerav_worldPos = vec3(inverse(u_view)[3]);
    vec3 trueViewDir = normalize(u_viewPos - v_worldPos.xyz);

    float NdotV = max(dot(normal, trueViewDir), 0.0);
    float F0 = 0.04;
    float fresnel = F0 + (1.0 - F0) * pow(1.0 - NdotV, 5.0);
    float transmissionFactor = 1.0 - fresnel;

    vec3 accumulatedSpecular = vec3(0.0);
    vec3 accumulatedInternalDiffuse = vec3(0.0);
    vec3 accumulatedNoise = vec3(0.0);

    float maxLightAtt = 0;
    
    // TODO: tiled based deferred your light looop!
    // TODO: tiled based deferred your light looop!
    // TODO: tiled based deferred your light looop!
    // TODO: tiled based deferred your light looop!

    // Tile params
    uint tileIndex = px.y * u_tileXCount + px.x;
	uint lightCount = tileLights[tileIndex].lightCount;

    // TODO: tiled based deferred your light looop!
    // TODO: tiled based deferred your light looop!
    // TODO: tiled based deferred your light looop!
    // TODO: tiled based deferred your light looop!

	// u half did it, seems like your tileLights ssbo just isn't bound somehow



        
    // for (int i = 0; i < lightCount; i++) {
    for (int lightIndex = 0; lightIndex < 9; lightIndex++) {

        Light light = lights[lightIndex];
        vec3 lightPos = vec3(light.posX, light.posY, light.posZ);
        vec3 lightCol = vec3(light.colorR, light.colorG, light.colorB);

        vec3 toLight = lightPos - v_worldPos.xyz;
        float dist = length(toLight);

        if (dist > light.radius) continue;

        vec3 lightVector = lightPos - v_worldPos.xyz;
        float distanceSquared = max(dot(lightVector, lightVector), 0.0001);
        vec3 L = lightVector * inversesqrt(distanceSquared);

        float ndotl = dot(normal, L);
    
        if (ndotl <= 0.0) {
            continue;
        }

        float att = smoothstep(light.radius, 0.0, dist) * light.strength;

        float shadow = 1.0;
        if (light.hiResShadowMapIndex != -1) {
            shadow = ShadowCalculationNEW(light.hiResShadowMapIndex, lightPos, light.radius, v_worldPos.xyz, u_viewPos, normal.xyz, hiResShadowMapArray);
        }
        else if (light.lowResShadowMapIndex != -1) {
            shadow = ShadowCalculationNEW(light.lowResShadowMapIndex, lightPos, light.radius, v_worldPos.xyz, u_viewPos, normal.xyz, lowResShadowMapArray);
        }

        float candas = 1.0;
        if (light.iesTextureIndex != 0) {
            sampler2D iesSampler = sampler2D(textureSamplers[light.iesTextureIndex]);
            candas = ApplyIESProfile(v_worldPos.xyz, light, iesSampler);
        }

        maxLightAtt = max(att, maxLightAtt);

        accumulatedSpecular += GetDirectLightingSpecularOnly(lightPos, lightCol, light.radius, light.strength, normal, v_worldPos.xyz, gammaBaseColor, roughness, metallic, u_viewPos) * shadow * candas;

        float transparency = 1.0;
        accumulatedInternalDiffuse += GetDirectLightingDiffuseOnly(lightPos, lightCol, light.radius, light.strength, normal, v_worldPos.xyz, gammaBaseColor, roughness, transparency, u_viewPos) * shadow * candas;
    }

    // Refraction
    vec3 refractionColor = refraction2(normal);

    // Fake raytraced noise
    float fakeRaytracedNoise = fract(sin(dot(TexCoord, vec2(12.9898, 78.233))) * 43758.5453);
    float grainIntensity = 0.005;
    vec3 grain = vec3((fakeRaytracedNoise - 0.5) * grainIntensity);
    grain = min(grain, refractionColor * 1.1);
    accumulatedNoise += grain;

    // Volume calculation with jitter
    float noise = fakeRaytracedNoise;//InterleavedGradientNoise(gl_FragCoord.xy);
    float thicknessBase = mix(thicknessFloor, thicknessCeiling, 1.0 - NdotV);
    float thickness = (thicknessBase + (noise * noiseJitterStrength)) * transThicknessMaster;
    vec3 absorptionTint = exp(-((vec3(1.0) - baseColor) * absorptionFalloff) * thickness);
    float scatteringFactor = 1.0 - exp(-thickness * smokeDensity);

    // Translucency tuning
    float scatterLimit      = 1.125;
    float refractionFloor   = 0.25;
    float refractionCeiling = 1.0;
    float thicknessPower    = 0.1;
    float refractionScale   = 0.9;

    // evaluate thickness
    float thicknessFactor = pow(1.0 - NdotV, thicknessPower);
    float localThickness  = mix(0.2, 1.0, thicknessFactor) * transThicknessMaster;

    // get base scatter and invert for refraction
    float rawScatterAmount = (1.0 - exp(-localThickness * smokeDensity)) * scatterLimit;
    float rawRefraction    = 1.0 - rawScatterAmount;

    // clamp refraction to floor and ceiling limits
    float finalRefraction = clamp(rawRefraction, refractionFloor, refractionCeiling);
    float finalScatter    = 1.0 - finalRefraction;

    // composite lighting
    vec3 backgroundLight = refractionColor * absorptionTint * refractionScale;
    vec3 backgroundThroughSmoke = backgroundLight * finalRefraction;
    vec3 smokeGlow = smokeInternalTint * accumulatedInternalDiffuse * finalScatter;
    vec3 internalColor = backgroundThroughSmoke + smokeGlow;

    // mask edges and composite specular
    float transmissionMask = 1.0 - fresnel;
    vec3 finalInternal = internalColor * transmissionMask;
    vec3 finalComposite = accumulatedSpecular + finalInternal + accumulatedNoise;

    ColorOut.rgb = finalComposite;

    //ColorOut.rgb = refractionColor;
    //ColorOut.rgb = vec3(scatteringFactor);
    //ColorOut.rgb = backgroundTransmitted;
    //ColorOut.rgb = litSmoke;
    //ColorOut.rgb = internalColor;
    //ColorOut.rgb = vec3(internalColor);

    ColorOut.rgb += accumulatedInternalDiffuse * 0.025;
    ColorOut.rgb += rawRefraction * 0.01;

    ColorOut.rgb += 0.00001;
    ColorOut.a = 1.0;
}


void main2() {

    ColorOut.rgb = vec3(1,0,0);
    ColorOut.a = 1.0;
}
