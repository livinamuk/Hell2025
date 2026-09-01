#version 460
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "../common/Vulkan/VK_binding_indices.glsl"
#include "../common/lighting.glsl"
#include "../common/types.glsl"
#include "../common/viewport.glsl"
#include "../common/Vulkan/VK_push_constants.glsl"

layout(set = 0, binding = DESC_IDX_TEXTURES) uniform texture2D textures[];
layout(set = 0, binding = DESC_IDX_SAMPLERS) uniform sampler samplers[];
layout(set = 0, binding = DESC_IDX_TEXTURE_SAMPLERS) uniform sampler textureSamplers[];

#include "VK_point_shadows.glsl"
#include "VK_ddgi_upsample.glsl"

layout(location = 0) out vec4 LightingOut;

layout(location = 0) centroid in vec2 v_texCoord;
layout(location = 1) centroid in vec3 v_normal;
layout(location = 2) centroid in vec3 v_tangent;
layout(location = 3) centroid in vec4 v_worldPos;
layout(location = 4) flat in uint v_globalInstanceIndex;
layout(location = 5) flat in uint v_viewportIndex;

layout(push_constant, scalar) uniform PushConstants {
    PushConstantsHair data;
} pc;

void main() {
    RenderItemBuffer renderItemBuffer = pc.data.frame.sceneRenderItemBuffer;
    MaterialBuffer materialBuffer = pc.data.frame.materialBuffer;
    RendererDataBuffer rendererDataBuffer = pc.data.frame.rendererDataBuffer;
    ViewportDataBuffer viewportDataBuffer = pc.data.frame.viewportDataBuffer;
    LightBuffer lightBuffer = pc.data.frame.lightBuffer;

    RenderItem item = renderItemBuffer.renderItems[v_globalInstanceIndex];
    Material material = materialBuffer.materials[item.materialIndex];

    uint baseColorTextureIndex = uint(material.basecolor);
    uint normalTextureIndex = uint(material.normal);
    uint rmaTextureIndex = uint(material.rma);
    uint emissiveTextureIndex = uint(material.emissive);

    vec4 baseColor = texture(sampler2D(textures[nonuniformEXT(baseColorTextureIndex)], textureSamplers[nonuniformEXT(baseColorTextureIndex)]), v_texCoord);
    vec3 normalMap = texture(sampler2D(textures[nonuniformEXT(normalTextureIndex)], textureSamplers[nonuniformEXT(normalTextureIndex)]), v_texCoord).rgb;
    vec4 rma = texture(sampler2D(textures[nonuniformEXT(rmaTextureIndex)], textureSamplers[nonuniformEXT(rmaTextureIndex)]), v_texCoord).rgba;
    vec3 emissiveMapColor = texture(sampler2D(textures[nonuniformEXT(emissiveTextureIndex)], textureSamplers[nonuniformEXT(emissiveTextureIndex)]), v_texCoord).rgb;

    vec3 viewPos = viewportDataBuffer.viewportData[v_viewportIndex].inverseView[3].xyz;

    float roughness = clamp(rma.r * item.roughnessFactor, 0.0, 1.0);
    float metallic = clamp(rma.g * item.metallicFactor, 0.0, 1.0);
    float ao = rma.b;

    vec3 gammaBaseColor = pow(baseColor.rgb, vec3(2.2));

    ViewportData vd = viewportDataBuffer.viewportData[v_viewportIndex];
    mat4 inverseProjection = vd.inverseProjection;
    mat4 inverseView = vd.inverseView;
    mat4 viewMatrix = vd.view;
    bool thisViewportIsInShop = bool(vd.isInShop);

    normalMap = normalMap * 2.0 - 1.0;
    vec3 n = normalize(v_normal);
    vec3 t = normalize(v_tangent);
    if (!gl_FrontFacing) n = -n;
    t = normalize(t - dot(t, n) * n);
    vec3 b = cross(n, t);
    mat3 tbn = mat3(t, b, n);
    vec3 normal = normalize(tbn * normalMap);

    vec3 linearBaseColor = baseColor.rgb * baseColor.rgb;
    vec3 F0 = mix(vec3(0.04), linearBaseColor, metallic);
    float f0_lum = dot(F0, vec3(0.2126, 0.7152, 0.0722));

    float variation = length(fwidth(normal));
    float smoothnessFactor = 0.5;
    roughness = clamp(roughness + (variation * smoothnessFactor), 0.0, 1.0);

    vec3 directLighting = vec3(0.0);
    for (int i = 2; i < 4; i++) {
        int lightIndex = i;

        Light light = lightBuffer.lights[lightIndex];
        vec3 lightPosition = vec3(light.posX, light.posY, light.posZ);
        vec3 lightColor = vec3(light.colorR, light.colorG, light.colorB);
        float lightStrength = light.strength;
        float lightRadius = light.radius;

        float shadow = 1.0;
        if (light.hiResShadowMapIndex != -1) {
            shadow = ShadowCalculationBindless(light.hiResShadowMapIndex, lightPosition, lightRadius, v_worldPos.xyz, viewPos, normal.xyz, VULKAN_POINT_SHADOW_IDX_HIGH_RES);
        }
        else if (light.lowResShadowMapIndex != -1) {
            shadow = ShadowCalculationBindless(light.lowResShadowMapIndex, lightPosition, lightRadius, v_worldPos.xyz, viewPos, normal.xyz, VULKAN_POINT_SHADOW_IDX_LOW_RES);
        }

        vec3 directLight = GetDirectLighting(lightPosition, lightColor, lightRadius, lightStrength, normal.xyz, v_worldPos.xyz, gammaBaseColor.rgb, roughness, metallic, viewPos) * shadow;

        if (light.iesTextureIndex != 0) {
            uint iesTextureIndex = uint(light.iesTextureIndex);
            float candelas = ApplyIESProfile(v_worldPos.xyz, light, textures[nonuniformEXT(iesTextureIndex)], textureSamplers[nonuniformEXT(iesTextureIndex)]);
            directLight *= candelas;
        }

        directLighting += directLight;
    }

    vec3 indirectDiffuse = vec3(0.0);

    if (rendererDataBuffer.rendererData.enableIrradianceProbeSampling) {
        ivec2 outputImageSize = ivec2(rendererDataBuffer.rendererData.gBufferWidth, rendererDataBuffer.rendererData.gBufferHeight);
        vec2 screenUV = ScreenUVFromFragCoord(gl_FragCoord.xy, outputImageSize);
        ivec4 viewportRect = ivec4(vd.xOffset, vd.yOffset, vd.width, vd.height);
        vec3 probeIrradiance = SampleDDGIIndirectDiffuseBilateral_VK(screenUV, normal, distance(v_worldPos.xyz, viewPos), outputImageSize, viewportRect);
        vec3 diffuseAlbedo = gammaBaseColor.rgb * (1.0 - metallic);
        float indirectDiffuseScale = 1.0;
        indirectDiffuse = probeIrradiance * diffuseAlbedo * indirectDiffuseScale;
    }

    vec3 finalLitColor = (directLighting + indirectDiffuse) * ao;

    LightingOut.rgb = finalLitColor;
    LightingOut.a = 1.0;
}
