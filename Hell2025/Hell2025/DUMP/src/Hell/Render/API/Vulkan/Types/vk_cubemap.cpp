#include "vk_cubemap.h"

#include "Hell/Logging.h"
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
        if (color.r >= 1.0f && color.g >= 1.0f && color.b >= 1.0f && color.a >= 1.0f) {
            return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        }
        return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    }

    void SetDebugName(VkDevice device, VkObjectType objectType, uint64_t objectHandle, const std::string& name) {
        if (objectHandle == 0 || name.empty()) {
            return;
        }

        VkDebugUtilsObjectNameInfoEXT nameInfo{ VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
        nameInfo.objectType = objectType;
        nameInfo.objectHandle = objectHandle;
        nameInfo.pObjectName = name.c_str();

        auto func = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetDeviceProcAddr(device, "vkSetDebugUtilsObjectNameEXT"));
        if (func) {
            func(device, &nameInfo);
        }
    }
}

VulkanCubemap::VulkanCubemap(VulkanCubemap&& other) noexcept {
    *this = std::move(other);
}

VulkanCubemap& VulkanCubemap::operator=(VulkanCubemap&& other) noexcept {
    if (this != &other) {
        Cleanup();
        m_image = other.m_image;
        m_imageView = other.m_imageView;
        m_sampler = other.m_sampler;
        m_allocation = other.m_allocation;
        m_size = other.m_size;
        m_mipmapLevelCount = other.m_mipmapLevelCount;
        m_format = other.m_format;

        other.m_image = VK_NULL_HANDLE;
        other.m_imageView = VK_NULL_HANDLE;
        other.m_sampler = VK_NULL_HANDLE;
        other.m_allocation = VK_NULL_HANDLE;
        other.m_size = 0;
        other.m_mipmapLevelCount = 0;
        other.m_format = VK_FORMAT_UNDEFINED;
    }
    return *this;
}

void VulkanCubemap::Create(uint32_t size, VkFormat format, uint32_t mipmapLevelCount, const std::string& debugName) {
    Cleanup();

    if (size == 0) {
        Logging::Error() << "VulkanCubemap::Create(..) failed because size was zero\n";
        return;
    }

    if (format == VK_FORMAT_UNDEFINED) {
        Logging::Error() << "VulkanCubemap::Create(..) failed because format was VK_FORMAT_UNDEFINED\n";
        return;
    }

    m_size = size;
    m_format = format;
    m_mipmapLevelCount = std::max(1u, mipmapLevelCount);

    VkImageCreateInfo imageInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = m_format;
    imageInfo.extent = { m_size, m_size, 1 };
    imageInfo.mipLevels = m_mipmapLevelCount;
    imageInfo.arrayLayers = 6;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo allocationInfo = {};
    allocationInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocationInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    VkResult createImageResult = vmaCreateImage(VulkanMemoryManager::GetAllocator(), &imageInfo, &allocationInfo, &m_image, &m_allocation, nullptr);
    if (createImageResult != VK_SUCCESS) {
        Logging::Error() << "VulkanCubemap::Create(..) failed to create image\n";
        Cleanup();
        return;
    }

    VkImageViewCreateInfo viewInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    viewInfo.image = m_image;
    viewInfo.format = m_format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = m_mipmapLevelCount;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 6;

    VkDevice device = VulkanDeviceManager::GetDevice();
    VkResult createViewResult = vkCreateImageView(device, &viewInfo, nullptr, &m_imageView);
    if (createViewResult != VK_SUCCESS) {
        Logging::Error() << "VulkanCubemap::Create(..) failed to create image view\n";
        Cleanup();
        return;
    }

    SetDebugName(device, VK_OBJECT_TYPE_IMAGE, (uint64_t)m_image, debugName);
    SetDebugName(device, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)m_imageView, debugName.empty() ? std::string() : debugName + " View");
}

void VulkanCubemap::CreateSampler(TextureWrapMode wrapMode, TextureFilter minFilter, TextureFilter magFilter, const glm::vec4& borderColor) {
    if (m_image == VK_NULL_HANDLE) {
        Logging::Error() << "VulkanCubemap::CreateSampler(..) failed because cubemap image was VK_NULL_HANDLE\n";
        return;
    }

    VkDevice device = VulkanDeviceManager::GetDevice();

    if (m_sampler != VK_NULL_HANDLE) {
        vkDestroySampler(device, m_sampler, nullptr);
        m_sampler = VK_NULL_HANDLE;
    }

    VkSamplerCreateInfo samplerInfo{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
    samplerInfo.magFilter = TextureFilterToVkFilter(magFilter);
    samplerInfo.minFilter = TextureFilterToVkFilter(minFilter);
    samplerInfo.mipmapMode = minFilter == TextureFilter::NEAREST ? VK_SAMPLER_MIPMAP_MODE_NEAREST : VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = TextureWrapModeToVkAddressMode(wrapMode);
    samplerInfo.addressModeV = TextureWrapModeToVkAddressMode(wrapMode);
    samplerInfo.addressModeW = TextureWrapModeToVkAddressMode(wrapMode);
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = static_cast<float>(m_mipmapLevelCount);
    samplerInfo.borderColor = BorderColorToVkBorderColor(borderColor);
    samplerInfo.unnormalizedCoordinates = VK_FALSE;

    VkResult createSamplerResult = vkCreateSampler(device, &samplerInfo, nullptr, &m_sampler);
    if (createSamplerResult != VK_SUCCESS) {
        Logging::Error() << "VulkanCubemap::CreateSampler(..) failed to create sampler\n";
    }
}

void VulkanCubemap::Cleanup() {
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

    m_size = 0;
    m_mipmapLevelCount = 0;
    m_format = VK_FORMAT_UNDEFINED;
}

size_t VulkanCubemap::GetAllocatedByteCount() const {
    return GetGPUAllocatedByteCount();
}

size_t VulkanCubemap::GetCPUAllocatedByteCount() const {
    return sizeof(VulkanCubemap);
}

size_t VulkanCubemap::GetGPUAllocatedByteCount() const {
    if (m_image == VK_NULL_HANDLE || m_allocation == VK_NULL_HANDLE) {
        return 0;
    }

    VmaAllocationInfo allocationInfo{};
    vmaGetAllocationInfo(VulkanMemoryManager::GetAllocator(), m_allocation, &allocationInfo);
    return static_cast<size_t>(allocationInfo.size);
}
