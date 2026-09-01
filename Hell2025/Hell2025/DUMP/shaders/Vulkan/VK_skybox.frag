#version 460
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "../common/util.glsl"
#include "../common/viewport.glsl"
#include "../common/Vulkan/VK_binding_indices.glsl"
#include "../common/Vulkan/VK_push_constants.glsl"
#include "../common/post_processing.glsl"

layout(set = 0, binding = DESC_IDX_TEXTURES) uniform textureCube textures[];
layout(set = 0, binding = DESC_IDX_TEXTURE_SAMPLERS) uniform sampler textureSamplers[];

layout(location = 0) out vec4 out_color;

layout(push_constant, scalar) uniform PushConstants {
    PushConstantsSkybox data;
} pc;

mat3 GetSkyboxRotationMatrix() {
    float angle = radians(-90.0);
    float c = cos(angle);
    float s = sin(angle);
    return mat3(c, 0.0, s, 0.0, 1.0, 0.0, -s, 0.0, c);
}

void main() {
    RendererDataBuffer rendererDataBuffer = pc.data.frame.rendererDataBuffer;
    ViewportDataBuffer viewportDataBuffer = pc.data.frame.viewportDataBuffer;

    ivec2 px = ivec2(gl_FragCoord.xy);
    ivec2 resolution = ivec2(rendererDataBuffer.rendererData.gBufferWidth, rendererDataBuffer.rendererData.gBufferHeight);

    uint viewportIndex = ViewportIndexFromPixel(px, resolution, rendererDataBuffer.rendererData.viewportLayout, vec2(rendererDataBuffer.rendererData.viewportSplitX, rendererDataBuffer.rendererData.viewportSplitY));
    ivec4 viewportRect = ivec4(viewportDataBuffer.viewportData[viewportIndex].xOffset, viewportDataBuffer.viewportData[viewportIndex].yOffset, viewportDataBuffer.viewportData[viewportIndex].width, viewportDataBuffer.viewportData[viewportIndex].height);
    vec3 viewPosition = viewportDataBuffer.viewportData[viewportIndex].viewPos.xyz;
    mat4 inverseProjectionView = viewportDataBuffer.viewportData[viewportIndex].inverseProjectionViewReverseZ;
    vec3 rayDir = WorldRayFromPixel(px, resolution, viewportRect, viewPosition, inverseProjectionView);

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
