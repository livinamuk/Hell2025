#version 460
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_ray_query : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

layout(early_fragment_tests) in;

#include "../common/constants.glsl"
#include "../common/flags.glsl"
#include "../common/lighting.glsl"
#include "../common/types.glsl"
#include "../common/viewport.glsl"
#include "../common/Vulkan/VK_binding_indices.glsl"
#include "../common/Vulkan/VK_push_constants.glsl"

layout(set = 0, binding = DESC_IDX_SAMPLERS) uniform sampler samplers[];
layout(set = 0, binding = DESC_IDX_TEXTURES) uniform texture2D textures[];
layout(set = 0, binding = DESC_IDX_TEXTURE_SAMPLERS) uniform sampler textureSamplers[];
layout(set = 1, binding = 0) uniform accelerationStructureEXT u_RayQueryAccelerationStructure;

#include "../common/Vulkan/VK_ray_query_scene.glsl"
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
    PushConstantsDeferredLighting data;
} pc;

const int MAX_GPU_LIGHTS = 16;
float DirectLightVisibility(RayQueryContext rayQueryContext, vec3 worldPos, vec3 normal, vec3 lightPos) {
    return TraceSceneLineOfSight(u_RayQueryAccelerationStructure, rayQueryContext, worldPos + normal * 0.001, lightPos);
}

vec3 ComputeDirectLighting(LightBuffer lightBuffer, vec3 worldPos, vec3 normal, vec3 baseColor, float roughness, float metallic, vec3 viewPos) {
    vec3 directLighting = vec3(0.0);
    RendererData rendererData = pc.data.frame.rendererDataBuffer.rendererData;
    RayQueryContext rayQueryContext = CreateRayQueryContext(pc.data.rayQueryBLASDataDeviceAddress, pc.data.rayQuerySceneRenderItemIndicesDeviceAddress, uint64_t(pc.data.frame.sceneRenderItemBuffer), uint64_t(pc.data.frame.materialBuffer));

    for (int i = 0; i < MAX_GPU_LIGHTS; i++) {
        Light light = lightBuffer.lights[i];
        if (light.radius <= 0.0 || light.strength <= 0.0) {
            continue;
        }

        vec3 lightPosition = vec3(light.posX, light.posY, light.posZ);
        vec3 lightColor = vec3(light.colorR, light.colorG, light.colorB);
        float visibility = 1.0;
        if (light.hiResShadowMapIndex != -1 || light.lowResShadowMapIndex != -1) {
            visibility = rendererData.directPointShadowMode == POINT_SHADOW_MODE_RAY_QUERY
                ? DirectLightVisibility(rayQueryContext, worldPos, normal, lightPosition)
                : GetPointShadowMapVisibility(light, worldPos, normal, viewPos);
        }

        if (visibility <= 0.0) {
            continue;
        }

        vec3 directLight = GetDirectLighting(lightPosition, lightColor, light.radius, light.strength, normal, worldPos, baseColor, roughness, metallic, viewPos) * visibility;

        if (light.iesTextureIndex != 0) {
            uint iesTextureIndex = uint(light.iesTextureIndex);
            directLight *= ApplyIESProfile(worldPos, light, textures[nonuniformEXT(iesTextureIndex)], textureSamplers[nonuniformEXT(iesTextureIndex)]);
        }

        directLighting += directLight;
    }

    return directLighting;
}

vec3 BuildNormal(vec3 vertexNormal, vec3 vertexTangent, vec3 normalMap) {
    vec3 n = normalize(vertexNormal);
    vec3 t = normalize(vertexTangent);
    if (!gl_FrontFacing) {
        n = -n;
    }

    t = normalize(t - dot(t, n) * n);
    vec3 b = cross(n, t);
    normalMap = normalMap * 2.0 - 1.0;
    return normalize(mat3(t, b, n) * normalMap);
}

void main() {
    RenderItemBuffer renderItemBuffer = pc.data.frame.sceneRenderItemBuffer;
    MaterialBuffer materialBuffer = pc.data.frame.materialBuffer;
    RendererDataBuffer rendererDataBuffer = pc.data.frame.rendererDataBuffer;
    ViewportDataBuffer viewportDataBuffer = pc.data.frame.viewportDataBuffer;
    LightBuffer lightBuffer = pc.data.frame.lightBuffer;

    RenderItem renderItem = renderItemBuffer.renderItems[v_globalInstanceIndex];
    Material material = materialBuffer.materials[renderItem.materialIndex];
    RendererData rendererData = rendererDataBuffer.rendererData;
    ViewportData viewport = viewportDataBuffer.viewportData[v_viewportIndex];

    if (material.basecolor < 0 || material.normal < 0 || material.rma < 0) {
        discard;
    }

    uint baseColorTextureIndex = uint(material.basecolor);
    uint normalTextureIndex = uint(material.normal);
    uint rmaTextureIndex = uint(material.rma);

    vec4 baseColor = texture(sampler2D(textures[nonuniformEXT(baseColorTextureIndex)], textureSamplers[nonuniformEXT(baseColorTextureIndex)]), v_texCoord);
    vec3 normalMap = texture(sampler2D(textures[nonuniformEXT(normalTextureIndex)], textureSamplers[nonuniformEXT(normalTextureIndex)]), v_texCoord).rgb;
    vec4 rma = texture(sampler2D(textures[nonuniformEXT(rmaTextureIndex)], textureSamplers[nonuniformEXT(rmaTextureIndex)]), v_texCoord).rgba;

    float roughness = clamp(rma.r * renderItem.roughnessFactor, 0.0, 1.0);
    float metallic = clamp(rma.g * renderItem.metallicFactor, 0.0, 1.0);
    float ao = rma.b;
    vec3 linearBaseColor = pow(baseColor.rgb, vec3(2.2));

    vec3 normal = BuildNormal(v_normal, v_tangent, normalMap);

    float variation = length(fwidth(normal));
    float smoothnessFactor = 0.5;
    roughness = clamp(roughness + (variation * smoothnessFactor), 0.0, 1.0);

    vec3 viewPos = viewport.viewPos.xyz;
    vec3 directLighting = ComputeDirectLighting(lightBuffer, v_worldPos.xyz, normal, linearBaseColor, roughness, metallic, viewPos);
    vec3 indirectDiffuse = vec3(0.0);

    if (rendererData.enableIrradianceProbeSampling) {
        ivec2 outputImageSize = ivec2(rendererData.gBufferWidth, rendererData.gBufferHeight);
        vec2 screenUV = ScreenUVFromFragCoord(gl_FragCoord.xy, outputImageSize);
        ivec4 viewportRect = ivec4(viewport.xOffset, viewport.yOffset, viewport.width, viewport.height);
        vec3 probeIrradiance = SampleDDGIIndirectDiffuseBilateral_VK(screenUV, normal, distance(v_worldPos.xyz, viewPos), outputImageSize, viewportRect);
        vec3 diffuseAlbedo = linearBaseColor.rgb * (1.0 - metallic);
        indirectDiffuse = probeIrradiance * diffuseAlbedo;
    }

    vec3 finalLitColor = (directLighting + indirectDiffuse) * ao;

    LightingOut.rgb = finalLitColor;
    LightingOut.a = baseColor.a;
}
