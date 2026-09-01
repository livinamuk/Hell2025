#include "vk_allocated_image.h"
#include "Hell/Render/API/Vulkan/Managers/vk_command_manager.h"
#include "Hell/Render/API/Vulkan/Managers/vk_device_manager.h"
#include "Hell/Render/API/Vulkan/Managers/vk_memory_manager.h"

#include <algorithm>
#include <utility>

VkImageAspectFlags GetImageAspectFlagsFromFormat(VkFormat format) {
    if (format == VK_FORMAT_D32_SFLOAT || format == VK_FORMAT_D16_UNORM) {
        return VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT) {
        return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    return VK_IMAGE_ASPECT_COLOR_BIT;
}

bool IsDepthStencilFormat(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

AllocatedImage::AllocatedImage(VkFormat imageFormat, VkExtent3D imageExtent, VkSampleCountFlagBits sampleCount, VkImageUsageFlags usage, std::string debugName, uint32_t arrayLayerCount, bool allocateMips) {
    VkDevice device = VulkanDeviceManager::GetDevice();
    VmaAllocator allocator = VulkanMemoryManager::GetAllocator();

    m_format = imageFormat;
    m_extent = imageExtent;
    m_sampleCount = sampleCount;
    m_arrayLayerCount = std::max(1u, arrayLayerCount);
    m_mipLevelCount = 1;
    if (allocateMips && m_sampleCount == VK_SAMPLE_COUNT_1_BIT) {
        uint32_t maxDimension = std::max(m_extent.width, m_extent.height);
        while (maxDimension > 1) {
            maxDimension >>= 1;
            m_mipLevelCount++;
        }
    }
    m_currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = m_format;
    imageInfo.extent = m_extent;
    imageInfo.mipLevels = m_mipLevelCount;
    imageInfo.arrayLayers = m_arrayLayerCount;
    imageInfo.samples = m_sampleCount;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    if (m_mipLevelCount > 1) usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    imageInfo.usage = usage;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    vmaCreateImage(allocator, &imageInfo, &allocInfo, &m_image, &m_allocation, nullptr);

    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.viewType = m_arrayLayerCount > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.image = m_image;
    viewInfo.format = m_format;
    viewInfo.subresourceRange.aspectMask = GetImageAspectFlagsFromFormat(m_format);
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = m_arrayLayerCount;

    vkCreateImageView(device, &viewInfo, nullptr, &m_imageView);

    if (IsDepthStencilFormat(m_format)) {
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        vkCreateImageView(device, &viewInfo, nullptr, &m_depthOnlyImageView);
    }

    if (m_mipLevelCount > 1) {
        m_mipImageViews.resize(m_mipLevelCount, VK_NULL_HANDLE);
        for (uint32_t mipLevel = 0; mipLevel < m_mipLevelCount; mipLevel++) {
            viewInfo.subresourceRange.aspectMask = GetImageAspectFlagsFromFormat(m_format);
            viewInfo.subresourceRange.baseMipLevel = mipLevel;
            viewInfo.subresourceRange.levelCount = 1;
            vkCreateImageView(device, &viewInfo, nullptr, &m_mipImageViews[mipLevel]);
        }

        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.aspectMask = GetImageAspectFlagsFromFormat(m_format);
        viewInfo.subresourceRange.levelCount = m_mipLevelCount;
        vkCreateImageView(device, &viewInfo, nullptr, &m_sampledImageView);

        if (IsDepthStencilFormat(m_format)) {
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            vkCreateImageView(device, &viewInfo, nullptr, &m_sampledDepthOnlyImageView);
        }
    }

    // Move image to its permanent layout
    VulkanCommandManager::SubmitImmediate([&](VkCommandBuffer cmd) {
        VkImageMemoryBarrier2 barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier.image = m_image;
        barrier.subresourceRange = { GetImageAspectFlagsFromFormat(m_format), 0, m_mipLevelCount, 0, m_arrayLayerCount };
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        barrier.srcAccessMask = 0;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        barrier.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;

        VkDependencyInfo dep{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
        dep.imageMemoryBarrierCount = 1;
        dep.pImageMemoryBarriers = &barrier;
        vkCmdPipelineBarrier2(cmd, &dep);
    });

    m_currentLayout = VK_IMAGE_LAYOUT_GENERAL;
    m_currentAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
    m_currentStageFlags = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

    VkDebugUtilsObjectNameInfoEXT nameInfo = {};
    nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    nameInfo.objectType = VK_OBJECT_TYPE_IMAGE;
    nameInfo.objectHandle = (uint64_t)m_image;
    nameInfo.pObjectName = debugName.c_str();

    auto func = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(device, "vkSetDebugUtilsObjectNameEXT");
    if (func) {
        func(device, &nameInfo);
    }
}

AllocatedImage::AllocatedImage(AllocatedImage&& other) noexcept {
    m_image = other.m_image;
    m_imageView = other.m_imageView;
    m_mipImageViews = std::move(other.m_mipImageViews);
    m_sampledImageView = other.m_sampledImageView;
    m_depthOnlyImageView = other.m_depthOnlyImageView;
    m_sampledDepthOnlyImageView = other.m_sampledDepthOnlyImageView;
    m_allocation = other.m_allocation;
    m_extent = other.m_extent;
    m_format = other.m_format;
    m_sampleCount = other.m_sampleCount;
    m_arrayLayerCount = other.m_arrayLayerCount;
    m_mipLevelCount = other.m_mipLevelCount;
    m_currentLayout = other.m_currentLayout;
    m_currentAccessMask = other.m_currentAccessMask;
    m_currentStageFlags = other.m_currentStageFlags;

    // Nullify the other object so its cleanup doesn't destroy our handles
    other.m_image = VK_NULL_HANDLE;
    other.m_imageView = VK_NULL_HANDLE;
    other.m_mipImageViews.clear();
    other.m_sampledImageView = VK_NULL_HANDLE;
    other.m_depthOnlyImageView = VK_NULL_HANDLE;
    other.m_sampledDepthOnlyImageView = VK_NULL_HANDLE;
    other.m_allocation = VK_NULL_HANDLE;
    other.m_sampleCount = VK_SAMPLE_COUNT_1_BIT;
    other.m_arrayLayerCount = 1;
    other.m_mipLevelCount = 1;
}

AllocatedImage& AllocatedImage::operator=(AllocatedImage&& other) noexcept {
    if (this != &other) {
        m_image = other.m_image;
        m_imageView = other.m_imageView;
        m_mipImageViews = std::move(other.m_mipImageViews);
        m_sampledImageView = other.m_sampledImageView;
        m_depthOnlyImageView = other.m_depthOnlyImageView;
        m_sampledDepthOnlyImageView = other.m_sampledDepthOnlyImageView;
        m_allocation = other.m_allocation;
        m_extent = other.m_extent;
        m_format = other.m_format;
        m_sampleCount = other.m_sampleCount;
        m_arrayLayerCount = other.m_arrayLayerCount;
        m_mipLevelCount = other.m_mipLevelCount;
        m_currentLayout = other.m_currentLayout;
        m_currentAccessMask = other.m_currentAccessMask;
        m_currentStageFlags = other.m_currentStageFlags;

        other.m_image = VK_NULL_HANDLE;
        other.m_imageView = VK_NULL_HANDLE;
        other.m_mipImageViews.clear();
        other.m_sampledImageView = VK_NULL_HANDLE;
        other.m_depthOnlyImageView = VK_NULL_HANDLE;
        other.m_sampledDepthOnlyImageView = VK_NULL_HANDLE;
        other.m_allocation = VK_NULL_HANDLE;
        other.m_sampleCount = VK_SAMPLE_COUNT_1_BIT;
        other.m_arrayLayerCount = 1;
        other.m_mipLevelCount = 1;
    }
    return *this;
}

void AllocatedImage::Sync(VkCommandBuffer cmd, VkAccessFlags2 dstAccess, VkPipelineStageFlags2 dstStage) {
    // only sync memory, no layout transition needed
    VkMemoryBarrier2 barrier{ VK_STRUCTURE_TYPE_MEMORY_BARRIER_2 };
    barrier.srcStageMask = m_currentStageFlags;
    barrier.srcAccessMask = m_currentAccessMask;
    barrier.dstStageMask = dstStage;
    barrier.dstAccessMask = dstAccess;

    VkDependencyInfo dep{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
    dep.memoryBarrierCount = 1;
    dep.pMemoryBarriers = &barrier;

    vkCmdPipelineBarrier2(cmd, &dep);

    m_currentAccessMask = dstAccess;
    m_currentStageFlags = dstStage;
}

void AllocatedImage::GenerateMipmaps(VkCommandBuffer commandBuffer) {
    if (m_mipLevelCount <= 1) return;
    if ((GetImageAspectFlagsFromFormat(m_format) & VK_IMAGE_ASPECT_COLOR_BIT) == 0) return;

    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(VulkanDeviceManager::GetPhysicalDevice(), m_format, &formatProperties);

    VkFormatFeatureFlags requiredFeatures = VK_FORMAT_FEATURE_BLIT_SRC_BIT | VK_FORMAT_FEATURE_BLIT_DST_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;
    if ((formatProperties.optimalTilingFeatures & requiredFeatures) != requiredFeatures) return;

    Sync(commandBuffer, VK_ACCESS_2_TRANSFER_READ_BIT | VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT);

    int32_t srcWidth = static_cast<int32_t>(m_extent.width);
    int32_t srcHeight = static_cast<int32_t>(m_extent.height);

    for (uint32_t dstMip = 1; dstMip < m_mipLevelCount; dstMip++) {
        int32_t dstWidth = std::max(srcWidth / 2, 1);
        int32_t dstHeight = std::max(srcHeight / 2, 1);

        VkImageBlit blit{};
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = dstMip - 1;
        blit.srcSubresource.layerCount = m_arrayLayerCount;
        blit.srcOffsets[1] = { srcWidth, srcHeight, 1 };
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = dstMip;
        blit.dstSubresource.layerCount = m_arrayLayerCount;
        blit.dstOffsets[1] = { dstWidth, dstHeight, 1 };

        vkCmdBlitImage(commandBuffer, m_image, VK_IMAGE_LAYOUT_GENERAL, m_image, VK_IMAGE_LAYOUT_GENERAL, 1, &blit, VK_FILTER_LINEAR);

        if (dstMip + 1 < m_mipLevelCount) {
            VkImageMemoryBarrier2 barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
            barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
            barrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
            barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
            barrier.image = m_image;
            barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, dstMip, 1, 0, m_arrayLayerCount };

            VkDependencyInfo dep{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
            dep.imageMemoryBarrierCount = 1;
            dep.pImageMemoryBarriers = &barrier;
            vkCmdPipelineBarrier2(commandBuffer, &dep);
        }

        srcWidth = dstWidth;
        srcHeight = dstHeight;
    }
}

void AllocatedImage::Cleanup() {
    VkDevice device = VulkanDeviceManager::GetDevice();
    VmaAllocator allocator = VulkanMemoryManager::GetAllocator();

    if (m_imageView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, m_imageView, nullptr);
    }
    for (VkImageView mipImageView : m_mipImageViews) {
        if (mipImageView != VK_NULL_HANDLE) {
            vkDestroyImageView(device, mipImageView, nullptr);
        }
    }
    m_mipImageViews.clear();
    if (m_sampledImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, m_sampledImageView, nullptr);
    }
    if (m_depthOnlyImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, m_depthOnlyImageView, nullptr);
    }
    if (m_sampledDepthOnlyImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, m_sampledDepthOnlyImageView, nullptr);
    }
    if (m_image != VK_NULL_HANDLE) {
        vmaDestroyImage(allocator, m_image, m_allocation);
    }
}

size_t AllocatedImage::GetCPUAllocatedByteCount() const {
    return sizeof(AllocatedImage);
}

size_t AllocatedImage::GetGPUAllocatedByteCount() const {
    if (m_image == VK_NULL_HANDLE || m_allocation == VK_NULL_HANDLE) {
        return 0;
    }

    VmaAllocationInfo allocationInfo{};
    vmaGetAllocationInfo(VulkanMemoryManager::GetAllocator(), m_allocation, &allocationInfo);
    return static_cast<size_t>(allocationInfo.size);
}
