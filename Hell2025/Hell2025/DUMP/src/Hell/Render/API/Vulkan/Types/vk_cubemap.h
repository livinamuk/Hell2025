#pragma once

#include "Hell/Math/GLM.h"
#include "Hell/Render/API/Vulkan/vk_common.h"
#include "Hell/Render/TextureTypes.h"

#include <cstddef>
#include <string>

struct VulkanCubemap {
    VulkanCubemap() = default;
    VulkanCubemap(const VulkanCubemap&) = delete;
    VulkanCubemap& operator=(const VulkanCubemap&) = delete;
    VulkanCubemap(VulkanCubemap&& other) noexcept;
    VulkanCubemap& operator=(VulkanCubemap&& other) noexcept;

    void Create(uint32_t size, VkFormat format, uint32_t mipmapLevelCount, const std::string& debugName = {});
    void CreateSampler(TextureWrapMode wrapMode, TextureFilter minFilter, TextureFilter magFilter, const glm::vec4& borderColor);
    void Cleanup();

    size_t GetAllocatedByteCount() const;
    size_t GetCPUAllocatedByteCount() const;
    size_t GetGPUAllocatedByteCount() const;
    uint32_t GetSize() const             { return m_size; }
    uint32_t GetMipmapLevelCount() const { return m_mipmapLevelCount; }
    VkFormat GetFormat() const           { return m_format; }
    VkImage GetImage() const             { return m_image; }
    VkImageView GetImageView() const     { return m_imageView; }
    VkSampler GetSampler() const         { return m_sampler; }

private:
    VkImage m_image = VK_NULL_HANDLE;
    VkImageView m_imageView = VK_NULL_HANDLE;
    VkSampler m_sampler = VK_NULL_HANDLE;
    VmaAllocation m_allocation = VK_NULL_HANDLE;
    uint32_t m_size = 0;
    uint32_t m_mipmapLevelCount = 0;
    VkFormat m_format = VK_FORMAT_UNDEFINED;
};
