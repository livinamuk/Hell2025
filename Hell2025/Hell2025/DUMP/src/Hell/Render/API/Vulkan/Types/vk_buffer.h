#pragma once

#include "Hell/Render/API/Vulkan/vk_common.h"
#include "Hell/Common/Constants.h"

#include <cstddef>

struct VulkanBuffer {
    VulkanBuffer() = default;
    VulkanBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags vmaFlags = 0);

    VulkanBuffer(const VulkanBuffer&) = delete;
    VulkanBuffer& operator=(const VulkanBuffer&) = delete;

    VulkanBuffer(VulkanBuffer&& other) noexcept;
    VulkanBuffer& operator=(VulkanBuffer&& other) noexcept;

    void Cleanup();

    bool EnsureSize(VkDeviceSize size);
    bool UpdateData(const void* data, VkDeviceSize size, VkDeviceSize offset = 0);
    void UploadData(const void* data, VkDeviceSize size, VkDeviceSize dstOffset = 0);
    void Map(void** data);
    void Unmap();

    uint64_t GetDeviceAddress() const;
    VkDescriptorBufferInfo GetDescriptorInfo() const;

    VkBuffer GetBuffer() const   { return m_buffer; }
    VkDeviceSize GetSize() const { return m_size; }
    size_t GetCPUAllocatedByteCount() const;
    size_t GetGPUAllocatedByteCount() const;

private:
    VkBuffer m_buffer = VK_NULL_HANDLE;
    VmaAllocation m_allocation = VK_NULL_HANDLE;
    VkDeviceSize m_size = 0;
    VkBufferUsageFlags m_usage = 0;
    VmaMemoryUsage m_memoryUsage = VMA_MEMORY_USAGE_UNKNOWN;
    VmaAllocationCreateFlags m_vmaFlags = 0;
    void* m_mappedPtr = nullptr;
    mutable uint64_t m_deviceAddress = 0;
    bool m_hostCoherent = false;
};
