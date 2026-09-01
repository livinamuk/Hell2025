#include "../common/Vulkan/VK_upscaling.glsl"

const float DDGI_UPSAMPLE_NORMAL_POWER = 32.0;
const float DDGI_UPSAMPLE_MIN_DEPTH_SIGMA = 0.25;
const float DDGI_UPSAMPLE_DEPTH_SIGMA_SCALE = 0.03;

vec3 SampleDDGIIndirectDiffuseBilateral_VK(vec2 screenUV, vec3 normal, float viewDistance, ivec2 fullSize, ivec4 viewportRect) {
    return SampleSurfaceGuidedBilateralUpscale_VK(textures[VULKAN_TEXTURE_IDX_INDIRECT_DIFFUSE], samplers[VULKAN_SAMPLER_IDX_LINEAR], textures[VULKAN_TEXTURE_IDX_INDIRECT_DIFFUSE_SURFACE], samplers[VULKAN_SAMPLER_IDX_NEAREST], screenUV, normal, viewDistance, fullSize, viewportRect, DDGI_UPSAMPLE_NORMAL_POWER, DDGI_UPSAMPLE_MIN_DEPTH_SIGMA, DDGI_UPSAMPLE_DEPTH_SIGMA_SCALE);
}
