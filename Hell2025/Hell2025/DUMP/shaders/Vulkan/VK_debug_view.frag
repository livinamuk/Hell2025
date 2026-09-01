#version 460
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "../common/normal_encoding.glsl"
#include "../common/post_processing.glsl"
#include "../common/util.glsl"
#include "../common/reconstruction.glsl"
#include "../common/renderer_override_modes.glsl"
#include "../common/viewport.glsl"
#include "../common/Vulkan/VK_binding_indices.glsl"
#include "../common/Vulkan/VK_push_constants.glsl"

layout(set = 0, binding = DESC_IDX_SAMPLERS) uniform sampler samplers[];
layout(set = 0, binding = DESC_IDX_TEXTURES) uniform texture2D textures[];
layout(set = 0, binding = DESC_IDX_UINT_TEXTURES) uniform utexture2D uintTextures[];

layout(location = 0) out vec4 out_color;

layout(push_constant, scalar) uniform PushConstants {
    PushConstantsDebugView data;
} pc;

vec3 IntegerToColor(uint x) {
    x = ((x >> 16) ^ x) * 0x45d9f3bu;
    x = ((x >> 16) ^ x) * 0x45d9f3bu;
    x = (x >> 16) ^ x;
    return vec3(float(x & 0xffu), float((x >> 8) & 0xffu), float((x >> 16) & 0xffu)) / 255.0;
}

vec4 GetBaseColorMetallic(ivec2 px) {
    return texelFetch(sampler2D(textures[VULKAN_TEXTURE_IDX_BASE_COLOR_METALLIC], samplers[VULKAN_SAMPLER_IDX_NEAREST]), px, 0);
}

vec4 GetNormalXYRoughnessMisc(ivec2 px) {
    return texelFetch(sampler2D(textures[VULKAN_TEXTURE_IDX_NORMAL_XY_ROUGHNESS_MISC], samplers[VULKAN_SAMPLER_IDX_NEAREST]), px, 0);
}

vec4 GetVelocityXYOcclusionSubSurface(ivec2 px) {
    return texelFetch(sampler2D(textures[VULKAN_TEXTURE_IDX_VELOCITY_XY_OCCLUSION_SUBSURFACE], samplers[VULKAN_SAMPLER_IDX_NEAREST]), px, 0);
}

vec3 GetBaseColor(ivec2 px) {
    return GetBaseColorMetallic(px).rgb;
}

vec3 GetNormal(ivec2 px) {
    return DecodeOct(GetNormalXYRoughnessMisc(px).rg);
}

vec3 GetRoughness(ivec2 px) {
    return vec3(GetNormalXYRoughnessMisc(px).b);
}

vec3 GetMetallic(ivec2 px) {
    return vec3(GetBaseColorMetallic(px).a);
}

vec3 GetAO(ivec2 px) {
    return vec3(GetVelocityXYOcclusionSubSurface(px).b);
}

vec3 GetRMA(ivec2 px) {
    return vec3(GetRoughness(px).r, GetMetallic(px).r, GetAO(px).r);
}

vec3 GetCameraNdotL(ivec2 px, ivec2 outputImageSize, RendererData rendererData, ViewportDataBuffer viewportDataBuffer) {
    uint viewportIndex = ViewportIndexFromPixel(px, outputImageSize, rendererData.viewportLayout, vec2(rendererData.viewportSplitX, rendererData.viewportSplitY));
    mat4 inverseView = viewportDataBuffer.viewportData[viewportIndex].inverseView;
    vec3 normal = DecodeOct(GetNormalXYRoughnessMisc(px).rg);
    vec3 lightDir = normalize(inverseView[2].xyz);
    float ndotl = max(dot(normal, lightDir), 0.0);

    return GetBaseColor(px) * ndotl;
}

vec3 GetVelocity(ivec2 px) {
    // The GBuffer stores FidelityFX-style raw NDC motion. Display the
    // corresponding texture-UV displacement used by temporal consumers.
    vec2 velocity = GetVelocityXYOcclusionSubSurface(px).rg * 0.5;
    return vec3(velocity * 20.0 + 0.5, 0.5);
}

vec3 GetIndirectDiffuse(ivec2 px) {
    vec3 indirectDiffuseTexture = texelFetch(sampler2D(textures[VULKAN_TEXTURE_IDX_INDIRECT_DIFFUSE], samplers[VULKAN_SAMPLER_IDX_LINEAR]), px / 2, 0).rgb;
    vec3 result = Tonemap_ACES(indirectDiffuseTexture);
    result = pow(result, vec3(1.0 / 2.2));
    result = clamp(result, 0.0, 1.0);
    result = mix(result, Tonemap_ACES(result), 0.125);
    return result;
}

vec3 GetIndirectSpecularAMDInput(ivec2 px, ivec2 outputImageSize) {
    vec3 incidentRadiance = texelFetch(
        sampler2D(textures[VULKAN_TEXTURE_IDX_INDIRECT_SPECULAR_AMD_INPUT], samplers[VULKAN_SAMPLER_IDX_NEAREST]),
        px / 1,
        0).rgb;
    vec3 result = Tonemap_ACES(incidentRadiance);
    result = pow(result, vec3(1.0 / 2.2));
    return clamp(result, 0.0, 1.0);
}

vec3 GetIndirectSpecularAMDReprojected(ivec2 px) {
    vec3 reprojectedRadiance = texelFetch(
        sampler2D(textures[VULKAN_TEXTURE_IDX_INDIRECT_SPECULAR_AMD_REPROJECTED], samplers[VULKAN_SAMPLER_IDX_NEAREST]),
        px,
        0).rgb;
    vec3 result = Tonemap_ACES(reprojectedRadiance);
    result = pow(result, vec3(1.0 / 2.2));
    return clamp(result, 0.0, 1.0);
}

vec3 GetIndirectSpecularAMDPrefiltered(ivec2 px) {
    vec3 prefilteredRadiance = texelFetch(
        sampler2D(textures[VULKAN_TEXTURE_IDX_INDIRECT_SPECULAR_AMD_PREFILTERED], samplers[VULKAN_SAMPLER_IDX_NEAREST]),
        px,
        0).rgb;
    vec3 result = Tonemap_ACES(prefilteredRadiance);
    result = pow(result, vec3(1.0 / 2.2));
    return clamp(result, 0.0, 1.0);
}

vec3 GetIndirectSpecularAMDPrefilteredVariance(ivec2 px) {
    float variance = texelFetch(
        sampler2D(textures[VULKAN_TEXTURE_IDX_INDIRECT_SPECULAR_AMD_PREFILTERED_VARIANCE], samplers[VULKAN_SAMPLER_IDX_NEAREST]),
        px,
        0).r;
    return vec3(clamp(variance, 0.0, 1.0));
}

vec3 GetIndirectSpecularAMDTemporal(ivec2 px) {
    vec3 temporalRadiance = texelFetch(
        sampler2D(textures[VULKAN_TEXTURE_IDX_INDIRECT_SPECULAR_AMD_TEMPORAL], samplers[VULKAN_SAMPLER_IDX_NEAREST]),
        px,
        0).rgb;
    vec3 result = Tonemap_ACES(temporalRadiance);
    result = pow(result, vec3(1.0 / 2.2));
    return clamp(result, 0.0, 1.0);
}

vec3 GetIndirectSpecularAMDTemporalVariance(ivec2 px) {
    float variance = texelFetch(
        sampler2D(textures[VULKAN_TEXTURE_IDX_INDIRECT_SPECULAR_AMD_VARIANCE_HISTORY], samplers[VULKAN_SAMPLER_IDX_NEAREST]),
        px,
        0).r;
    return vec3(clamp(variance, 0.0, 1.0));
}

vec3 GetIndirectSpecularAMDSampleCount(ivec2 px, ivec2 outputImageSize) {
    float sampleCount = texelFetch(
        sampler2D(textures[VULKAN_TEXTURE_IDX_INDIRECT_SPECULAR_AMD_SAMPLE_COUNT_HISTORY], samplers[VULKAN_SAMPLER_IDX_NEAREST]),
        px / 1,
        0).r;
    return vec3(clamp(sampleCount / 32.0, 0.0, 1.0));
}

vec3 GetHiZ(ivec2 px, ivec2 outputImageSize) {
    vec2 screenUV = ScreenUVFromPixel(px, outputImageSize);
    vec2 tiledUV = screenUV * 2.0;
    ivec2 tile = clamp(ivec2(floor(tiledUV)), ivec2(0), ivec2(1));
    vec2 localUV = fract(tiledUV);

    // Top-left/right and bottom-left/right show mips 0, 2, 4, and 6.
    float mipLevel = float((tile.x + tile.y * 2) * 2);
    float reverseDepth = textureLod(
        sampler2D(textures[VULKAN_TEXTURE_IDX_HIZ], samplers[VULKAN_SAMPLER_IDX_NEAREST]),
        localUV,
        mipLevel).r;
    return vec3(clamp(reverseDepth, 0.0, 1.0));
}

vec3 GetVisBuffer(ivec2 px) {
    uvec2 visibilityData = texelFetch(usampler2D(uintTextures[VULKAN_UINT_TEXTURE_IDX_GBUFFER_VISIBILITY], samplers[VULKAN_SAMPLER_IDX_NEAREST]), px, 0).rg;
    uint meshID = visibilityData.x;
    uint primitiveID = visibilityData.y;

    if (meshID == 0u && primitiveID == 0u) {
        return vec3(0.05);
    }

    vec3 meshColor = IntegerToColor(meshID);
    vec3 primitiveColor = IntegerToColor(primitiveID);
    return mix(meshColor, primitiveColor, 0.2);
}

vec3 GetDepth(ivec2 px) {
    float depth = texelFetch(sampler2D(textures[VULKAN_TEXTURE_IDX_GBUFFER_DEPTH], samplers[VULKAN_SAMPLER_IDX_NEAREST]), px, 0).r;
    return vec3(depth, 0.0, 0.0);
}

vec3 GetEmissive(ivec2 px) {
    return texelFetch(sampler2D(textures[VULKAN_TEXTURE_IDX_EMISSIVE], samplers[VULKAN_SAMPLER_IDX_NEAREST]), px, 0).rgb;
}

vec3 GetWorldPosition(ivec2 px, ivec2 outputImageSize, RendererData rendererData, ViewportDataBuffer viewportDataBuffer) {
    uint viewportIndex = ViewportIndexFromPixel(px, outputImageSize, rendererData.viewportLayout, vec2(rendererData.viewportSplitX, rendererData.viewportSplitY));
    ivec4 viewportRect = ivec4(viewportDataBuffer.viewportData[viewportIndex].xOffset, viewportDataBuffer.viewportData[viewportIndex].yOffset, viewportDataBuffer.viewportData[viewportIndex].width, viewportDataBuffer.viewportData[viewportIndex].height);
    mat4 inverseProjectionView = viewportDataBuffer.viewportData[viewportIndex].inverseProjectionViewReverseZ;
    vec2 viewportUV = ViewportUVFromPixel(px, outputImageSize, viewportRect);
    float depth = texelFetch(sampler2D(textures[VULKAN_TEXTURE_IDX_GBUFFER_DEPTH], samplers[VULKAN_SAMPLER_IDX_NEAREST]), px, 0).r;
    return WorldPosFromDepth(viewportUV, depth, inverseProjectionView);
}

void main() {
    ivec2 px = ivec2(gl_FragCoord.xy);
    ivec2 outputImageSize = textureSize(sampler2D(textures[VULKAN_TEXTURE_IDX_BASE_COLOR_METALLIC], samplers[VULKAN_SAMPLER_IDX_NEAREST]), 0);

    if (px.x < 0 || px.y < 0 || px.x >= outputImageSize.x || px.y >= outputImageSize.y) {
        discard;
    }

    ViewportDataBuffer viewportDataBuffer = pc.data.frame.viewportDataBuffer;
    RendererDataBuffer rendererDataBuffer = pc.data.frame.rendererDataBuffer;
    RendererData rendererData = rendererDataBuffer.rendererData;
    vec3 finalColor = vec3(0.0);

    if (rendererData.rendererOverrideState == OVERRIDE_BASE_COLOR)                 finalColor = GetBaseColor(px);
    if (rendererData.rendererOverrideState == OVERRIDE_NORMALS)                    finalColor = GetNormal(px);
    if (rendererData.rendererOverrideState == OVERRIDE_RMA)                        finalColor = GetRMA(px);
    if (rendererData.rendererOverrideState == OVERRIDE_ROUGHNESS)                  finalColor = GetRoughness(px);
    if (rendererData.rendererOverrideState == OVERRIDE_METALLIC)                   finalColor = GetMetallic(px);
    if (rendererData.rendererOverrideState == OVERRIDE_AO)                         finalColor = GetAO(px);
    if (rendererData.rendererOverrideState == OVERRIDE_CAMERA_NDOTL)               finalColor = GetCameraNdotL(px, outputImageSize, rendererData, viewportDataBuffer);
    if (rendererData.rendererOverrideState == OVERRIDE_VELOCITY)                   finalColor = GetVelocity(px);
    if (rendererData.rendererOverrideState == OVERRIDE_VIS_BUFFER)                 finalColor = GetVisBuffer(px);
    if (rendererData.rendererOverrideState == OVERRIDE_DEPTH)                      finalColor = GetDepth(px);
    if (rendererData.rendererOverrideState == OVERRIDE_WORLD_POSITION)             finalColor = GetWorldPosition(px, outputImageSize, rendererData, viewportDataBuffer);
    if (rendererData.rendererOverrideState == OVERRIDE_EMISSIVE)                   finalColor = GetEmissive(px);
    if (rendererData.rendererOverrideState == OVERRIDE_INDIRECT_DIFFUSE)           finalColor = GetIndirectDiffuse(px);
    if (rendererData.rendererOverrideState == OVERRIDE_HIZ) finalColor = GetHiZ(px, outputImageSize);
    if (rendererData.rendererOverrideState == OVERRIDE_INDIRECT_SPECULAR_AMD_SAMPLE_COUNT) finalColor = GetIndirectSpecularAMDSampleCount(px, outputImageSize);
    if (rendererData.rendererOverrideState == OVERRIDE_INDIRECT_SPECULAR_AMD_INPUT) finalColor = GetIndirectSpecularAMDInput(px, outputImageSize);
    if (rendererData.rendererOverrideState == OVERRIDE_INDIRECT_SPECULAR_AMD_REPROJECTED) finalColor = GetIndirectSpecularAMDReprojected(px);
    if (rendererData.rendererOverrideState == OVERRIDE_INDIRECT_SPECULAR_AMD_PREFILTERED) finalColor = GetIndirectSpecularAMDPrefiltered(px);
    if (rendererData.rendererOverrideState == OVERRIDE_INDIRECT_SPECULAR_AMD_PREFILTERED_VARIANCE) finalColor = GetIndirectSpecularAMDPrefilteredVariance(px);
    if (rendererData.rendererOverrideState == OVERRIDE_INDIRECT_SPECULAR_AMD_TEMPORAL) finalColor = GetIndirectSpecularAMDTemporal(px);
    if (rendererData.rendererOverrideState == OVERRIDE_INDIRECT_SPECULAR_AMD_TEMPORAL_VARIANCE) finalColor = GetIndirectSpecularAMDTemporalVariance(px);

    out_color = vec4(finalColor, 1.0);
}
