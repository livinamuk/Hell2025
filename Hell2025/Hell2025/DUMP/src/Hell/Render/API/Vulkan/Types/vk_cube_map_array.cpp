#include "vk_cube_map_array.h"

#include "Hell/Logging.h"
#include "Hell/Render/API/Vulkan/Managers/vk_command_manager.h"
#include "Hell/Render/API/Vulkan/Managers/vk_device_manager.h"
#include "Hell/Render/API/Vulkan/Managers/vk_memory_manager.h"

#include <algorithm>
#include <utility>

namespace {
    VkImageAspectFlags GetImageAspectFlags(VkFormat format) {
        if (format == VK_FORMAT_D32_SFLOAT || format == VK_FORMAT_D16_UNORM) {
            return VK_IMAGE_ASPECT_DEPTH_BIT;
        }
        if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT) {
            return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        }
        return VK_IMAGE_ASPECT_COLOR_BIT;
    }

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
        if (objectHandle == 0 || name.empty()) return;

        VkDebugUtilsObjectNameInfoEXT nameInfo{ VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
        nameInfo.objectType = objectType;
        nameInfo.objectHandle = objectHandle;
        nameInfo.pObjectName = name.c_str();

        auto func = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetDeviceProcAddr(device, "vkSetDebugUtilsObjectNameEXT"));
        if (func) func(device, &nameInfo);
    }
}

VulkanCubeMapArray::VulkanCubeMapArray(VulkanCubeMapArray&& other) noexcept {
    *this = std::move(other);
}

VulkanCubeMapArray& VulkanCubeMapArray::operator=(VulkanCubeMapArray&& other) noexcept {
    if (this != &other) {
        Cleanup();
        m_image = other.m_image;
        m_imageView = other.m_imageView;
        m_arrayImageView = other.m_arrayImageView;
        m_faceImageViews = std::move(other.m_faceImageViews);
        m_sampler = other.m_sampler;
        m_allocation = other.m_allocation;
        m_cubeMapCount = other.m_cubeMapCount;
        m_size = other.m_size;
        m_mipmapLevelCount = other.m_mipmapLevelCount;
        m_format = other.m_format;
        m_currentAccessMask = other.m_currentAccessMask;
        m_currentStageFlags = other.m_currentStageFlags;

        other.m_image = VK_NULL_HANDLE;
        other.m_imageView = VK_NULL_HANDLE;
        other.m_arrayImageView = VK_NULL_HANDLE;
        other.m_faceImageViews.clear();
        other.m_sampler = VK_NULL_HANDLE;
        other.m_allocation = VK_NULL_HANDLE;
        other.m_cubeMapCount = 0;
        other.m_size = 0;
        other.m_mipmapLevelCount = 0;
        other.m_format = VK_FORMAT_UNDEFINED;
        other.m_currentAccessMask = 0;
        other.m_currentStageFlags = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
    }
    return *this;
}

void VulkanCubeMapArray::Create(uint32_t cubeMapCount, uint32_t size, VkFormat format, uint32_t mipmapLevelCount, VkImageUsageFlags usage, const std::string& debugName) {
    Cleanup();

    if (cubeMapCount == 0 || size == 0 || format == VK_FORMAT_UNDEFINED) {
        Logging::Error() << "VulkanCubeMapArray::Create(..) received invalid dimensions or format\n";
        return;
    }

    m_cubeMapCount = cubeMapCount;
    m_size = size;
    m_format = format;
    m_mipmapLevelCount = std::max(1u, mipmapLevelCount);
    const uint32_t arrayLayerCount = GetArrayLayerCount();
    const VkImageAspectFlags aspectMask = GetImageAspectFlags(m_format);

    VkImageCreateInfo imageInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = m_format;
    imageInfo.extent = { m_size, m_size, 1 };
    imageInfo.mipLevels = m_mipmapLevelCount;
    imageInfo.arrayLayers = arrayLayerCount;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = usage;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo allocationInfo{};
    allocationInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocationInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    if (vmaCreateImage(VulkanMemoryManager::GetAllocator(), &imageInfo, &allocationInfo, &m_image, &m_allocation, nullptr) != VK_SUCCESS) {
        Logging::Error() << "VulkanCubeMapArray::Create(..) failed to create image\n";
        Cleanup();
        return;
    }

    VkDevice device = VulkanDeviceManager::GetDevice();
    VkImageViewCreateInfo viewInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    viewInfo.image = m_image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
    viewInfo.format = m_format;
    viewInfo.subresourceRange = { aspectMask, 0, m_mipmapLevelCount, 0, arrayLayerCount };

    if (vkCreateImageView(device, &viewInfo, nullptr, &m_imageView) != VK_SUCCESS) {
        Logging::Error() << "VulkanCubeMapArray::Create(..) failed to create cube array image view\n";
        Cleanup();
        return;
    }

    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    if (vkCreateImageView(device, &viewInfo, nullptr, &m_arrayImageView) != VK_SUCCESS) {
        Logging::Error() << "VulkanCubeMapArray::Create(..) failed to create 2D array image view\n";
        Cleanup();
        return;
    }

    m_faceImageViews.resize(arrayLayerCount, VK_NULL_HANDLE);
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.layerCount = 1;

    for (uint32_t arrayLayer = 0; arrayLayer < arrayLayerCount; arrayLayer++) {
        viewInfo.subresourceRange.baseArrayLayer = arrayLayer;
        if (vkCreateImageView(device, &viewInfo, nullptr, &m_faceImageViews[arrayLayer]) != VK_SUCCESS) {
            Logging::Error() << "VulkanCubeMapArray::Create(..) failed to create face image view\n";
            Cleanup();
            return;
        }
    }

    VulkanCommandManager::SubmitImmediate([&](VkCommandBuffer commandBuffer) {
        VkImageMemoryBarrier2 barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier.image = m_image;
        barrier.subresourceRange = { aspectMask, 0, m_mipmapLevelCount, 0, arrayLayerCount };
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        barrier.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;

        VkDependencyInfo dependencyInfo{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
        dependencyInfo.imageMemoryBarrierCount = 1;
        dependencyInfo.pImageMemoryBarriers = &barrier;
        vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
    });

    m_currentAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
    m_currentStageFlags = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

    SetDebugName(device, VK_OBJECT_TYPE_IMAGE, (uint64_t)m_image, debugName);
    SetDebugName(device, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)m_imageView, debugName.empty() ? std::string() : debugName + " View");
    SetDebugName(device, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)m_arrayImageView, debugName.empty() ? std::string() : debugName + " Array View");
}

void VulkanCubeMapArray::CreateSampler(TextureWrapMode wrapMode, TextureFilter minFilter, TextureFilter magFilter, const glm::vec4& borderColor, bool compareEnable, VkCompareOp compareOp) {
    if (m_image == VK_NULL_HANDLE) {
        Logging::Error() << "VulkanCubeMapArray::CreateSampler(..) failed because image was VK_NULL_HANDLE\n";
        return;
    }

    VkDevice device = VulkanDeviceManager::GetDevice();
    if (m_sampler != VK_NULL_HANDLE) vkDestroySampler(device, m_sampler, nullptr);

    VkSamplerCreateInfo samplerInfo{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
    samplerInfo.magFilter = TextureFilterToVkFilter(magFilter);
    samplerInfo.minFilter = TextureFilterToVkFilter(minFilter);
    samplerInfo.mipmapMode = minFilter == TextureFilter::NEAREST ? VK_SAMPLER_MIPMAP_MODE_NEAREST : VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = TextureWrapModeToVkAddressMode(wrapMode);
    samplerInfo.addressModeV = TextureWrapModeToVkAddressMode(wrapMode);
    samplerInfo.addressModeW = TextureWrapModeToVkAddressMode(wrapMode);
    samplerInfo.compareEnable = compareEnable ? VK_TRUE : VK_FALSE;
    samplerInfo.compareOp = compareOp;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = static_cast<float>(m_mipmapLevelCount);
    samplerInfo.borderColor = BorderColorToVkBorderColor(borderColor);

    if (vkCreateSampler(device, &samplerInfo, nullptr, &m_sampler) != VK_SUCCESS) {
        Logging::Error() << "VulkanCubeMapArray::CreateSampler(..) failed to create sampler\n";
        m_sampler = VK_NULL_HANDLE;
    }
}

void VulkanCubeMapArray::Sync(VkCommandBuffer commandBuffer, VkAccessFlags2 dstAccess, VkPipelineStageFlags2 dstStage) {
    VkMemoryBarrier2 barrier{ VK_STRUCTURE_TYPE_MEMORY_BARRIER_2 };
    barrier.srcStageMask = m_currentStageFlags;
    barrier.srcAccessMask = m_currentAccessMask;
    barrier.dstStageMask = dstStage;
    barrier.dstAccessMask = dstAccess;

    VkDependencyInfo dependencyInfo{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
    dependencyInfo.memoryBarrierCount = 1;
    dependencyInfo.pMemoryBarriers = &barrier;
    vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);

    m_currentAccessMask = dstAccess;
    m_currentStageFlags = dstStage;
}

void VulkanCubeMapArray::Cleanup() {
    VkDevice device = VulkanDeviceManager::GetDevice();

    if (m_sampler != VK_NULL_HANDLE) vkDestroySampler(device, m_sampler, nullptr);
    for (VkImageView faceImageView : m_faceImageViews) {
        if (faceImageView != VK_NULL_HANDLE) vkDestroyImageView(device, faceImageView, nullptr);
    }
    if (m_arrayImageView != VK_NULL_HANDLE) vkDestroyImageView(device, m_arrayImageView, nullptr);
    if (m_imageView != VK_NULL_HANDLE) vkDestroyImageView(device, m_imageView, nullptr);
    if (m_image != VK_NULL_HANDLE) vmaDestroyImage(VulkanMemoryManager::GetAllocator(), m_image, m_allocation);

    m_image = VK_NULL_HANDLE;
    m_imageView = VK_NULL_HANDLE;
    m_arrayImageView = VK_NULL_HANDLE;
    m_faceImageViews.clear();
    m_sampler = VK_NULL_HANDLE;
    m_allocation = VK_NULL_HANDLE;
    m_cubeMapCount = 0;
    m_size = 0;
    m_mipmapLevelCount = 0;
    m_format = VK_FORMAT_UNDEFINED;
    m_currentAccessMask = 0;
    m_currentStageFlags = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
}

size_t VulkanCubeMapArray::GetAllocatedByteCount() const {
    return GetGPUAllocatedByteCount();
}

size_t VulkanCubeMapArray::GetCPUAllocatedByteCount() const {
    return sizeof(VulkanCubeMapArray) + m_faceImageViews.capacity() * sizeof(VkImageView);
}

size_t VulkanCubeMapArray::GetGPUAllocatedByteCount() const {
    if (m_image == VK_NULL_HANDLE || m_allocation == VK_NULL_HANDLE) return 0;

    VmaAllocationInfo allocationInfo{};
    vmaGetAllocationInfo(VulkanMemoryManager::GetAllocator(), m_allocation, &allocationInfo);
    return static_cast<size_t>(allocationInfo.size);
}
