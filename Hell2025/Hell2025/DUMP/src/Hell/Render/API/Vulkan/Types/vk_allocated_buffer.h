#pragma once
#include "Hell/Render/API/Vulkan/vk_common.h"

struct AllocatedBuffer {
    VkBuffer m_buffer = VK_NULL_HANDLE;
    VmaAllocation m_allocation = VK_NULL_HANDLE;
    void* m_mapped = nullptr;
    VkDeviceSize m_size = 0;

    void Destroy(VmaAllocator allocator) {
        if (m_buffer != VK_NULL_HANDLE) {
            vmaDestroyBuffer(allocator, m_buffer, m_allocation);
            m_buffer = VK_NULL_HANDLE;
            m_allocation = VK_NULL_HANDLE;
            m_mapped = nullptr;
        }
    }
};