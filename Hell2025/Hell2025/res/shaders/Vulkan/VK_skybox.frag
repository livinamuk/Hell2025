#version 460
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "../common/util.glsl"
#include "../common/viewport.glsl"
#include "../common/Vulkan/binding_indices.glsl"
#include "../common/Vulkan/push_constants.glsl"
#include "../common/post_processing.glsl"

layout(set = 0, binding = DESC_IDX_TEXTURES) uniform textureCube textures[];
layout(set = 0, binding = DESC_IDX_TEXTURE_SAMPLERS) uniform sampler textureSamplers[];

layout(location = 0) out vec4 out_color;

layout(buffer_reference, scalar) readonly buffer ViewportDataBuffer {
    ViewportData viewportDataArr[];
};

layout(buffer_reference, scalar) readonly buffer RendererDataBuffer {
    RendererData rendererData;
};

layout(push_constant, scalar) uniform PushConstants {
    PushConstantsSkybox data;
} pushConstant;

mat3 GetSkyboxRotationMatrix() {
    float angle = radians(-90.0);
    float c = cos(angle);
    float s = sin(angle);
    return mat3(c, 0.0, s, 0.0, 1.0, 0.0, -s, 0.0, c);
}

void main() {
    RendererDataBuffer rendererDataBuffer = RendererDataBuffer(pushConstant.data.frame.rendererDataDeviceAddress);
    ViewportDataBuffer viewportDataBuffer = ViewportDataBuffer(pushConstant.data.frame.viewportDataDeviceAddress);

    ivec2 px = ivec2(gl_FragCoord.xy);
    ivec2 resolution = ivec2(rendererDataBuffer.rendererData.gBufferWidth, rendererDataBuffer.rendererData.gBufferHeight);

    uint viewportIndex = ViewportIndexFromSplitScreenMode_VK(px, resolution, rendererDataBuffer.rendererData.splitscreenMode);
    ViewportData viewportData = viewportDataBuffer.viewportDataArr[viewportIndex];

    mat4 inverseProjectionView = viewportData.inverseProjectionViewReverseZ;
    vec3 viewPos = viewportData.viewPos.xyz;
    vec2 viewportOrigin = vec2(viewportData.xOffset, resolution.y - viewportData.yOffset - viewportData.height);
    vec2 viewportSize = vec2(viewportData.width, viewportData.height);
    vec3 rayDir = GetWorldRay_VK(gl_FragCoord.xy, inverseProjectionView, viewPos, viewportOrigin, viewportSize);

    mat3 skyboxRotation = GetSkyboxRotationMatrix();
    vec3 skyboxSampleDir = normalize(skyboxRotation * rayDir);
    vec3 skyColor = texture(samplerCube(textures[VULKAN_TEXTURE_IDX_SKYBOX_NIGHT_SKY], textureSamplers[VULKAN_TEXTURE_IDX_SKYBOX_NIGHT_SKY]), skyboxSampleDir).rgb;
    vec3 skyLinear = pow(skyColor, vec3(2.2));

    vec3 horizonColor = vec3(0.6, 0.2, 0.6);
    vec3 downColor = vec3(0.4);
    float amount = 0.02;
    float colorCurve = 0.95;
    float fadeCurve = 0.69;
    float downwardness = clamp(-rayDir.y, 0.0, 1.0);
    float colorT = pow(downwardness, colorCurve);
    float fogT = pow(downwardness, fadeCurve);
    vec3 rayFogColor = mix(horizonColor, downColor, colorT) * amount;
    vec3 outColor = mix(skyLinear, rayFogColor, fogT);

    out_color = vec4(outColor, 1.0);
}
