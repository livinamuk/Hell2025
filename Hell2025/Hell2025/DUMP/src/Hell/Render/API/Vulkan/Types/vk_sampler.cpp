#include "vk_sampler.h"
#include "Hell/Render/API/Vulkan/Managers/vk_device_manager.h"
#include <utility>

VulkanSampler::VulkanSampler(VkFilter magFilter, VkFilter minFilter, VkSamplerAddressMode addressMode, float maxAnisotropy) {
    VkSamplerCreateInfo samplerInfo{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
    samplerInfo.magFilter = magFilter;
    samplerInfo.minFilter = minFilter;
    samplerInfo.addressModeU = addressMode;
    samplerInfo.addressModeV = addressMode;
    samplerInfo.addressModeW = addressMode;
    samplerInfo.anisotropyEnable = (maxAnisotropy > 1.0f) ? VK_TRUE : VK_FALSE;
    samplerInfo.maxAnisotropy = maxAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = VK_LOD_CLAMP_NONE;

    vkCreateSampler(VulkanDeviceManager::GetDevice(), &samplerInfo, nullptr, &m_sampler);
}

VulkanSampler::VulkanSampler(VulkanSampler&& other) noexcept {
    *this = std::move(other);
}

VulkanSampler& VulkanSampler::operator=(VulkanSampler&& other) noexcept {
    if (this != &other) {
        Cleanup();
        m_sampler = other.m_sampler;
        other.m_sampler = VK_NULL_HANDLE;
    }
    return *this;
}

void VulkanSampler::Cleanup() {
    if (m_sampler != VK_NULL_HANDLE) {
        vkDestroySampler(VulkanDeviceManager::GetDevice(), m_sampler, nullptr);
        m_sampler = VK_NULL_HANDLE;
    }
}

VkDescriptorImageInfo VulkanSampler::GetDescriptorInfo() const {
    VkDescriptorImageInfo info {};
    info.sampler = m_sampler;
    info.imageView = VK_NULL_HANDLE;
    info.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    return info;
}

size_t VulkanSampler::GetCPUAllocatedByteCount() const {
    return sizeof(VulkanSampler);
}

size_t VulkanSampler::GetGPUAllocatedByteCount() const {
    return 0;
}
