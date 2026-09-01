#include "vk_texture.h"

#include "Hell/Render/API/Vulkan/Managers/vk_device_manager.h"
#include "Hell/Render/API/Vulkan/Managers/vk_memory_manager.h"

#include <algorithm>
#include <utility>

namespace {
    VkSamplerAddressMode TextureWrapModeToVkAddressMode(TextureWrapMode wrapMode) {
        switch (wrapMode) {
        case TextureWrapMode::REPEAT:          return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        case TextureWrapMode::MIRRORED_REPEAT: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        case TextureWrapMode::CLAMP_TO_EDGE:   return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        case TextureWrapMode::CLAMP_TO_BORDER: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        default:                               return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        }
    }

    VkFilter TextureFilterToVkFilter(TextureFilter filter) {
        return filter == TextureFilter::NEAREST ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
    }

    VkBorderColor BorderColorToVkBorderColor(const glm::vec4& color) {
        if (color.r >= 1.0f && color.g >= 1.0f && color.b >= 1.0f && color.a >= 1.0f) return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    }
}

VulkanTexture::VulkanTexture(VulkanTexture&& other) noexcept {
    *this = std::move(other);
}

VulkanTexture& VulkanTexture::operator=(VulkanTexture&& other) noexcept {
    if (this != &other) {
        Cleanup();
        m_image = other.m_image;
        m_imageView = other.m_imageView;
        m_sampler = other.m_sampler;
        m_allocation = other.m_allocation;
        m_width = other.m_width;
        m_height = other.m_height;
        m_mipmapLevelCount = other.m_mipmapLevelCount;
        m_format = other.m_format;

        other.m_image = VK_NULL_HANDLE;
        other.m_imageView = VK_NULL_HANDLE;
        other.m_sampler = VK_NULL_HANDLE;
        other.m_allocation = VK_NULL_HANDLE;
        other.m_width = 0;
        other.m_height = 0;
        other.m_mipmapLevelCount = 0;
        other.m_format = VK_FORMAT_UNDEFINED;
    }
    return *this;
}

void VulkanTexture::Create(uint32_t width, uint32_t height, VkFormat format, uint32_t mipmapLevelCount) {
    Cleanup();

    m_width = width;
    m_height = height;
    m_format = format;
    m_mipmapLevelCount = std::max(1u, mipmapLevelCount);

    VkImageCreateInfo imageInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = format;
    imageInfo.extent = { width, height, 1 };
    imageInfo.mipLevels = m_mipmapLevelCount;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo allocationInfo = {};
    allocationInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocationInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    vmaCreateImage(VulkanMemoryManager::GetAllocator(), &imageInfo, &allocationInfo, &m_image, &m_allocation, nullptr);

    VkImageViewCreateInfo viewInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.image = m_image;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = m_mipmapLevelCount;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    vkCreateImageView(VulkanDeviceManager::GetDevice(), &viewInfo, nullptr, &m_imageView);
}

void VulkanTexture::CreateSampler(TextureWrapMode wrapModeS, TextureWrapMode wrapModeT, TextureFilter minFilter, TextureFilter magFilter, const glm::vec4& borderColor) {
    if (m_sampler != VK_NULL_HANDLE) {
        vkDestroySampler(VulkanDeviceManager::GetDevice(), m_sampler, nullptr);
        m_sampler = VK_NULL_HANDLE;
    }

    VkSamplerCreateInfo samplerInfo{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
    samplerInfo.magFilter = TextureFilterToVkFilter(magFilter);
    samplerInfo.minFilter = TextureFilterToVkFilter(minFilter);
    samplerInfo.mipmapMode = minFilter == TextureFilter::NEAREST ? VK_SAMPLER_MIPMAP_MODE_NEAREST : VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = TextureWrapModeToVkAddressMode(wrapModeS);
    samplerInfo.addressModeV = TextureWrapModeToVkAddressMode(wrapModeT);
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = static_cast<float>(m_mipmapLevelCount);
    samplerInfo.borderColor = BorderColorToVkBorderColor(borderColor);
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    vkCreateSampler(VulkanDeviceManager::GetDevice(), &samplerInfo, nullptr, &m_sampler);
}

void VulkanTexture::Cleanup() {
    VkDevice device = VulkanDeviceManager::GetDevice();
    VmaAllocator allocator = VulkanMemoryManager::GetAllocator();

    if (m_sampler != VK_NULL_HANDLE) {
        vkDestroySampler(device, m_sampler, nullptr);
        m_sampler = VK_NULL_HANDLE;
    }
    if (m_imageView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, m_imageView, nullptr);
        m_imageView = VK_NULL_HANDLE;
    }
    if (m_image != VK_NULL_HANDLE) {
        vmaDestroyImage(allocator, m_image, m_allocation);
        m_image = VK_NULL_HANDLE;
        m_allocation = VK_NULL_HANDLE;
    }

    m_width = 0;
    m_height = 0;
    m_mipmapLevelCount = 0;
    m_format = VK_FORMAT_UNDEFINED;
}

size_t VulkanTexture::GetAllocatedByteCount() const {
    return GetGPUAllocatedByteCount();
}

size_t VulkanTexture::GetCPUAllocatedByteCount() const {
    return sizeof(VulkanTexture);
}

size_t VulkanTexture::GetGPUAllocatedByteCount() const {
    if (m_image == VK_NULL_HANDLE || m_allocation == VK_NULL_HANDLE) {
        return 0;
    }

    VmaAllocationInfo allocationInfo{};
    vmaGetAllocationInfo(VulkanMemoryManager::GetAllocator(), m_allocation, &allocationInfo);
    return static_cast<size_t>(allocationInfo.size);
}
