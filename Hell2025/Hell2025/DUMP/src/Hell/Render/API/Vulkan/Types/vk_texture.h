#pragma once

#include "Hell/Math/GLM.h"
#include "Hell/Render/API/Vulkan/vk_common.h"
#include "Hell/Render/TextureTypes.h"

#include <cstddef>

struct VulkanTexture {
    VulkanTexture() = default;
    VulkanTexture(const VulkanTexture&) = delete;
    VulkanTexture& operator=(const VulkanTexture&) = delete;
    VulkanTexture(VulkanTexture&& other) noexcept;
    VulkanTexture& operator=(VulkanTexture&& other) noexcept;

    void Create(uint32_t width, uint32_t height, VkFormat format, uint32_t mipmapLevelCount);
    void CreateSampler(TextureWrapMode wrapModeS, TextureWrapMode wrapModeT, TextureFilter minFilter, TextureFilter magFilter, const glm::vec4& borderColor);
    void Cleanup();

    size_t GetAllocatedByteCount() const;
    size_t GetCPUAllocatedByteCount() const;
    size_t GetGPUAllocatedByteCount() const;
    uint32_t GetWidth() const             { return m_width; }
    uint32_t GetHeight() const            { return m_height; }
    uint32_t GetMipmapLevelCount() const  { return m_mipmapLevelCount; }
    VkFormat GetFormat() const            { return m_format; }
    VkImage GetImage() const              { return m_image; }
    VkImageView GetImageView() const      { return m_imageView; }
    VkSampler GetSampler() const          { return m_sampler; }

private:
    VkImage m_image = VK_NULL_HANDLE;
    VkImageView m_imageView = VK_NULL_HANDLE;
    VkSampler m_sampler = VK_NULL_HANDLE;
    VmaAllocation m_allocation = VK_NULL_HANDLE;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    uint32_t m_mipmapLevelCount = 0;
    VkFormat m_format = VK_FORMAT_UNDEFINED;
};
