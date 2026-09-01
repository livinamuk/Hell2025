#version 460 core
#extension GL_ARB_bindless_texture : enable
#include "../common/OpenGL/GL_binding_indices.glsl"
#include "../common/lighting.glsl"
#include "../common/post_processing.glsl"
#include "../common/types.glsl"

layout (location = 0) out vec4 FragOut;

layout (binding = TEX_IDX_SHADOW_MAP_FLASHLIGHT) uniform sampler2DArray FlashlighShadowMapTextureArray;

layout (binding = 4) uniform sampler2D baseColorTexture;
layout (binding = 5) uniform sampler2D normalTexture;
layout (binding = 6) uniform sampler2D rmaTexture;
layout (binding = 8) uniform sampler2D MainImageGuassianBlurredTexture; // Contains the final lit scene

readonly restrict layout(std430, binding = SSBO_IDX_SAMPLERS) buffer textureSamplersBuffer { uvec2 textureSamplers[]; };
readonly restrict layout(std430, binding = SSBO_IDX_RENDERER_DATA) buffer rendererDataBuffer { RendererData  rendererData;   };
readonly restrict layout(std430, binding = SSBO_IDX_VIEWPORT_DATA) buffer viewportDataBuffer { ViewportData  viewportData[]; };
readonly restrict layout(std430, binding = SSBO_IDX_LIGHTS) buffer lightsBuffer       { Light         lights[];       };
readonly restrict layout(std430, binding = SSBO_IDX_SPOT_LIGHTS) buffer spotLightsBuffer { SpotLight spotLights[]; };

in vec2 TexCoord;
in vec3 Normal;
in vec3 Tangent;
in vec3 BiTangent;
in vec4 WorldPos;
in vec3 ViewPos;
in vec3 TintColor;

uniform int u_viewportIndex;
uniform bool u_flipNormalMapY;
uniform int u_meshIndex;
    

void main() {

    vec4 baseColor = texture2D(baseColorTexture, TexCoord);
    vec3 normalMap = texture2D(normalTexture, TexCoord).rgb;
    vec3 rma = texture2D(rmaTexture, TexCoord).rgb;

    normalMap = mix(normalMap, vec3(0.5, 0.5, 1), 0.7);

    mat3 tbn = mat3(normalize(Tangent), normalize(BiTangent), normalize(Normal));
    normalMap.rgb = normalMap.rgb * 2.0 - 1.0;
    normalMap = normalize(normalMap);

    if (u_flipNormalMapY) {
        normalMap.y *= -1;
    }

    vec3 normal = normalize(tbn * (normalMap));

    vec3 gammaBaseColor = pow(baseColor.rgb, vec3(2.2));
    float roughness = rma.r;
    float metallic = rma.g;

    //ivec2 tile = ivec2(gl_FragCoord.xy) / TILE_SIZE;
    //uint tileIndex = tile.y * rendererData.tileCountX + tile.x;
    //uint lightCount = tileData[tileIndex].lightCount;

    
    mat4 inverseProjection = viewportData[u_viewportIndex].inverseProjection;
    mat4 inverseView = viewportData[u_viewportIndex].inverseView;
    mat4 viewMatrix = viewportData[u_viewportIndex].view;
    vec3 viewPos = inverseView[3].xyz;    


    vec3 directLighting = vec3(0); 

    //for (uint i = 0; i < lightCount; ++i) {
    //    uint lightIndex = tileData[tileIndex].lightIndices[i];
    //    Light light = lights[lightIndex];

    for (uint i = 0; i < 8; ++i) {
        Light light = lights[i];

        vec3 lightPosition = vec3(light.posX, light.posY, light.posZ);
        vec3 lightColor =  vec3(light.colorR, light.colorG, light.colorB);
        float lightStrength = light.strength;
        float lightRadius = light.radius;
             
        directLighting += GetDirectLighting(lightPosition, lightColor, lightRadius, lightStrength, normal.xyz, WorldPos.xyz, gammaBaseColor.rgb, roughness, metallic, ViewPos);
        
        vec3 toLight = lightPosition - WorldPos.xyz;
        float dist = length(toLight);
        vec3 lightDir = toLight / dist;
        vec3 viewDir = normalize(viewPos - WorldPos.xyz);
        float att = smoothstep(lightRadius, 0.0, dist) * lightStrength;
        directLighting += vec3(roughness * roughness * 0.01 * att) * lightColor;
    }

    
    vec3 worldSpacePosition = WorldPos.xyz;
    float fragDistance = distance(viewPos, worldSpacePosition);

    sampler2D flashlightIES = sampler2D(textureSamplers[max(rendererData.flashlightIESTextureIndex, 0)]);
    for (uint i = 0u; i < rendererData.spotLightCount; i++) {
        directLighting += GetSpotLightContribution(spotLights[i], rendererData, uint(u_viewportIndex), viewPos, normal.xyz, worldSpacePosition, gammaBaseColor.rgb, roughness, metallic, fragDistance, -1000.0, flashlightIES, FlashlighShadowMapTextureArray);
    }


    vec3 finalColor = directLighting;
    FragOut.rgb = vec3(finalColor);
	FragOut.a = 1.0;

    vec3 tintColorWIP = TintColor;

    int offset = 1;

    // Clear
    //if (u_meshIndex == 145 + offset) {   
    //    tintColorWIP = vec3(1.0, 1.0, 1.0); 
    //}
    
    // Red
    //if (u_meshIndex == 146 + offset) {   
    //    tintColorWIP = vec3(0.95, 0.00, 0);
    //}

    // Yellow
    //if (u_meshIndex == 147 + offset) {   
    //    tintColorWIP = vec3(1.00, 0.8, 0.25); 
    //}

    // Purple
    //if (u_meshIndex == 149 + offset) {   
    //    tintColorWIP = vec3(0.15, 0.00, 0.50); 
    //}

    // Green
    //if (u_meshIndex == 154 + offset) {   
    //    tintColorWIP = vec3(0, 0.95, 0.5); 
    //}
    
    // Orange
    //if (u_meshIndex == 155 + offset) {   
    //    tintColorWIP = vec3(0.6, 0.2, 0.0); 
    //}

    // Center Circle
    //if (u_meshIndex == 156 + offset) {   
    //    tintColorWIP = vec3(0.5, 0.35, 0.);  
    //}
    
    ivec2 pixelCoords = ivec2(gl_FragCoord.xy);
    vec3 mainImage = texelFetch(MainImageGuassianBlurredTexture, pixelCoords, 0).rgb;

    vec3 finalStainedGlassColor = (mainImage * tintColorWIP) + directLighting * 5;
    
     finalStainedGlassColor += (tintColorWIP * 0.0025);
    FragOut.rgb = finalStainedGlassColor;  
}
