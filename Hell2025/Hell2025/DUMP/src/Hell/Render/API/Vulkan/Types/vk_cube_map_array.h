#pragma once

#include "Hell/Math/GLM.h"
#include "Hell/Render/API/Vulkan/vk_common.h"
#include "Hell/Render/TextureTypes.h"

#include <cstddef>
#include <string>
#include <vector>

struct VulkanCubeMapArray {
    VulkanCubeMapArray() = default;
    VulkanCubeMapArray(const VulkanCubeMapArray&) = delete;
    VulkanCubeMapArray& operator=(const VulkanCubeMapArray&) = delete;
    VulkanCubeMapArray(VulkanCubeMapArray&& other) noexcept;
    VulkanCubeMapArray& operator=(VulkanCubeMapArray&& other) noexcept;

    void Create(uint32_t cubeMapCount, uint32_t size, VkFormat format, uint32_t mipmapLevelCount, VkImageUsageFlags usage, const std::string& debugName = {});
    void CreateSampler(TextureWrapMode wrapMode, TextureFilter minFilter, TextureFilter magFilter, const glm::vec4& borderColor, bool compareEnable = false, VkCompareOp compareOp = VK_COMPARE_OP_ALWAYS);
    void Sync(VkCommandBuffer commandBuffer, VkAccessFlags2 dstAccess, VkPipelineStageFlags2 dstStage);
    void Cleanup();

    size_t GetAllocatedByteCount() const;
    size_t GetCPUAllocatedByteCount() const;
    size_t GetGPUAllocatedByteCount() const;
    uint32_t GetCubeMapCount() const    { return m_cubeMapCount; }
    uint32_t GetArrayLayerCount() const { return m_cubeMapCount * 6; }
    uint32_t GetSize() const            { return m_size; }
    uint32_t GetMipmapLevelCount() const { return m_mipmapLevelCount; }
    VkFormat GetFormat() const           { return m_format; }
    VkImage GetImage() const             { return m_image; }
    VkImageView GetImageView() const     { return m_imageView; }
    VkImageView GetArrayImageView() const { return m_arrayImageView; }
    VkImageView GetFaceImageView(uint32_t arrayLayer) const { return arrayLayer < m_faceImageViews.size() ? m_faceImageViews[arrayLayer] : VK_NULL_HANDLE; }
    VkSampler GetSampler() const         { return m_sampler; }

private:
    VkImage m_image = VK_NULL_HANDLE;
    VkImageView m_imageView = VK_NULL_HANDLE;
    VkImageView m_arrayImageView = VK_NULL_HANDLE;
    std::vector<VkImageView> m_faceImageViews;
    VkSampler m_sampler = VK_NULL_HANDLE;
    VmaAllocation m_allocation = VK_NULL_HANDLE;
    uint32_t m_cubeMapCount = 0;
    uint32_t m_size = 0;
    uint32_t m_mipmapLevelCount = 0;
    VkFormat m_format = VK_FORMAT_UNDEFINED;
    VkAccessFlags2 m_currentAccessMask = 0;
    VkPipelineStageFlags2 m_currentStageFlags = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
};
