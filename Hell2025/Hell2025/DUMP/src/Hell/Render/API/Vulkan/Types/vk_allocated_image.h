#pragma once

#include "Hell/Render/API/Vulkan/vk_common.h"
#include <cstddef>
#include <string>
#include <vector>

struct AllocatedImage {
    AllocatedImage() = default;
    AllocatedImage(VkFormat imageFormat, VkExtent3D imageExtent, VkSampleCountFlagBits sampleCount, VkImageUsageFlags usage, std::string debugName, uint32_t arrayLayerCount = 1, bool allocateMips = false);
    AllocatedImage(const AllocatedImage&) = delete;
    AllocatedImage& operator=(const AllocatedImage&) = delete;
    AllocatedImage(AllocatedImage&& other) noexcept;
    AllocatedImage& operator=(AllocatedImage&& other) noexcept;

    void Sync(VkCommandBuffer cmd, VkAccessFlags2 dstAccess, VkPipelineStageFlags2 dstStage);
    void GenerateMipmaps(VkCommandBuffer commandBuffer);
    void Cleanup();

    int32_t GetWidth() const            { return m_extent.width; }
    int32_t GetHeight() const           { return m_extent.height; }
    int32_t GetDepth() const            { return m_extent.depth; }
    uint32_t GetArrayLayerCount() const { return m_arrayLayerCount; }
    uint32_t GetMipLevelCount() const   { return m_mipLevelCount; }
    VkExtent3D GetExtent() const        { return m_extent; }
    VkExtent2D GetExtent2D() const      { return { m_extent.width, m_extent.height }; }
    VkFormat GetFormat() const          { return m_format; }
    VkSampleCountFlagBits GetSampleCount() const { return m_sampleCount; }
    VkImage GetImage() const            { return m_image; }
    VkImageView GetImageView() const    { return m_imageView; }
    VkImageView GetMipImageView(uint32_t mipLevel) const { return mipLevel < m_mipImageViews.size() ? m_mipImageViews[mipLevel] : VK_NULL_HANDLE; }
    VkImageView GetSampledImageView() const { return m_sampledImageView != VK_NULL_HANDLE ? m_sampledImageView : m_imageView; }
    VkImageView GetDepthOnlyImageView() const { return m_depthOnlyImageView != VK_NULL_HANDLE ? m_depthOnlyImageView : m_imageView; }
    VkImageView GetSampledDepthOnlyImageView() const { return m_sampledDepthOnlyImageView != VK_NULL_HANDLE ? m_sampledDepthOnlyImageView : (m_depthOnlyImageView != VK_NULL_HANDLE ? m_depthOnlyImageView : GetSampledImageView()); }
    VmaAllocation GetAllocation() const { return m_allocation; }
    size_t GetCPUAllocatedByteCount() const;
    size_t GetGPUAllocatedByteCount() const;

private:
    VkImage m_image = VK_NULL_HANDLE;
    VkImageView m_imageView = VK_NULL_HANDLE;
    std::vector<VkImageView> m_mipImageViews;
    VkImageView m_sampledImageView = VK_NULL_HANDLE;
    VkImageView m_depthOnlyImageView = VK_NULL_HANDLE;
    VkImageView m_sampledDepthOnlyImageView = VK_NULL_HANDLE;
    VmaAllocation m_allocation = VK_NULL_HANDLE;
    VkExtent3D m_extent = {};
    VkFormat m_format = VK_FORMAT_UNDEFINED;
    VkSampleCountFlagBits m_sampleCount = VK_SAMPLE_COUNT_1_BIT;
    uint32_t m_arrayLayerCount = 1;
    uint32_t m_mipLevelCount = 1;

    VkImageLayout m_currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkAccessFlags2 m_currentAccessMask = 0;
    VkPipelineStageFlags2 m_currentStageFlags = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
};
