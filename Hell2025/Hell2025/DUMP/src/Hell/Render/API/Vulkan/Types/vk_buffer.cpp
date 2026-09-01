#include "vk_buffer.h"
#include "Hell/Render/API/Vulkan/Managers/vk_device_manager.h"
#include "Hell/Render/API/Vulkan/Managers/vk_memory_manager.h"
#include "Hell/Render/API/Vulkan/Managers/vk_command_manager.h"
#include "Hell/Logging.h"

VulkanBuffer::VulkanBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags vmaFlags) {
    m_size = size;
    m_usage = usage;
    m_memoryUsage = memoryUsage;
    m_vmaFlags = vmaFlags;

    VkBufferCreateInfo bufferInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bufferInfo.size = size;
    bufferInfo.usage = usage;

    VmaAllocationCreateInfo vmaAllocInfo{};
    vmaAllocInfo.usage = memoryUsage;
    vmaAllocInfo.flags = vmaFlags;

    VmaAllocationInfo allocInfo; // Capture this to get the pointer
    vmaCreateBuffer(VulkanMemoryManager::GetAllocator(), &bufferInfo, &vmaAllocInfo, &m_buffer, &m_allocation, &allocInfo);

    VkMemoryPropertyFlags memFlags = 0;
    vmaGetMemoryTypeProperties(VulkanMemoryManager::GetAllocator(), allocInfo.memoryType, &memFlags);
    m_hostCoherent = (memFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0;

    // If you requested the buffer to be mapped at creation, store the pointer
    if (vmaFlags & VMA_ALLOCATION_CREATE_MAPPED_BIT) {
        m_mappedPtr = allocInfo.pMappedData;
    }
}

VulkanBuffer::VulkanBuffer(VulkanBuffer&& other) noexcept {
    *this = std::move(other);
}

VulkanBuffer& VulkanBuffer::operator=(VulkanBuffer&& other) noexcept {
    if (this != &other) {
        Cleanup();
        m_buffer = other.m_buffer;
        m_allocation = other.m_allocation;
        m_size = other.m_size;
        m_usage = other.m_usage;
        m_memoryUsage = other.m_memoryUsage;
        m_vmaFlags = other.m_vmaFlags;
        m_mappedPtr = other.m_mappedPtr;
        m_deviceAddress = other.m_deviceAddress;
        m_hostCoherent = other.m_hostCoherent;

        other.m_buffer = VK_NULL_HANDLE;
        other.m_allocation = VK_NULL_HANDLE;
        other.m_size = 0;
        other.m_usage = 0;
        other.m_memoryUsage = VMA_MEMORY_USAGE_UNKNOWN;
        other.m_vmaFlags = 0;
        other.m_mappedPtr = nullptr;
        other.m_deviceAddress = 0;
        other.m_hostCoherent = false;
    }
    return *this;
}

void VulkanBuffer::Cleanup() {
    if (m_buffer != VK_NULL_HANDLE) {
        vmaDestroyBuffer(VulkanMemoryManager::GetAllocator(), m_buffer, m_allocation);
        m_buffer = VK_NULL_HANDLE;
        m_allocation = VK_NULL_HANDLE;
        m_size = 0;
        m_mappedPtr = nullptr;
        m_deviceAddress = 0;
        m_hostCoherent = false;
    }
}

bool VulkanBuffer::EnsureSize(VkDeviceSize size) {
    if (size == 0) return true;
    if (m_buffer != VK_NULL_HANDLE && m_size >= size) return true;

    if (m_usage == 0) {
        Logging::Error() << "VulkanBuffer::EnsureSize(..) failed because buffer has no usage flags\n";
        return false;
    }

    Cleanup();

    VkBufferCreateInfo bufferInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bufferInfo.size = size;
    bufferInfo.usage = m_usage;

    VmaAllocationCreateInfo vmaAllocInfo{};
    vmaAllocInfo.usage = m_memoryUsage;
    vmaAllocInfo.flags = m_vmaFlags;

    VmaAllocationInfo allocInfo;
    if (vmaCreateBuffer(VulkanMemoryManager::GetAllocator(), &bufferInfo, &vmaAllocInfo, &m_buffer, &m_allocation, &allocInfo) != VK_SUCCESS) {
        Logging::Error() << "VulkanBuffer::EnsureSize(..) failed to create buffer\n";
        return false;
    }

    m_size = size;
    m_mappedPtr = nullptr;
    m_deviceAddress = 0;

    VkMemoryPropertyFlags memFlags = 0;
    vmaGetMemoryTypeProperties(VulkanMemoryManager::GetAllocator(), allocInfo.memoryType, &memFlags);
    m_hostCoherent = (memFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0;

    if (m_vmaFlags & VMA_ALLOCATION_CREATE_MAPPED_BIT) {
        m_mappedPtr = allocInfo.pMappedData;
    }

    return true;
}

bool VulkanBuffer::UpdateData(const void* data, VkDeviceSize size, VkDeviceSize offset) {
    if (size == 0) return true;

    if (!data) {
        Logging::Error() << "VulkanBuffer::UpdateData(..) received null data for a non-empty upload\n";
        return false;
    }

    if (offset > m_size || size > m_size - offset) {
        Logging::Error() << "VulkanBuffer::UpdateData(..) upload exceeds destination buffer size\n";
        return false;
    }

    // Use the direct mapping path if the pointer exists
    if (m_mappedPtr) {
        memcpy(static_cast<char*>(m_mappedPtr) + offset, data, size);

        // Ensure changes are visible to the GPU if memory is not coherent
        if (!m_hostCoherent) {
            if (vmaFlushAllocation(VulkanMemoryManager::GetAllocator(), m_allocation, offset, size) != VK_SUCCESS) {
                Logging::Error() << "VulkanBuffer::UpdateData(..) failed to flush mapped memory\n";
                return false;
            }
        }

        return true;
    }

    Logging::Error() << "VulkanBuffer::UpdateData(..) failed because it was called on a buffer with no mapped pointer!\n"
                     << "Ensure the buffer was created with VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT and VMA_ALLOCATION_CREATE_MAPPED_BIT.\n";
    return false;
}

void VulkanBuffer::UploadData(const void* data, VkDeviceSize size, VkDeviceSize dstOffset) {
    if (!data || size == 0) return;

    if (m_buffer == VK_NULL_HANDLE) {
        Logging::Error() << "VulkanBuffer::UploadData(..) called on a null buffer\n";
        return;
    }

    if (dstOffset > m_size || size > m_size - dstOffset) {
        Logging::Error() << "VulkanBuffer::UploadData(..) upload exceeds destination buffer size\n";
        return;
    }

    // Only use this for static/GPU_ONLY buffers
    VkBufferCreateInfo stagingInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    stagingInfo.size = size;
    stagingInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VmaAllocationCreateInfo stagingAllocInfo{};
    stagingAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    stagingAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VkBuffer stagingBuffer;
    VmaAllocation stagingAlloc;
    VmaAllocationInfo stagingResult;
    vmaCreateBuffer(VulkanMemoryManager::GetAllocator(), &stagingInfo, &stagingAllocInfo, &stagingBuffer, &stagingAlloc, &stagingResult);

    // Copy to the temporary staging memory
    memcpy(stagingResult.pMappedData, data, size);

    // Execute the transfer command
    VulkanCommandManager::SubmitImmediate([&](VkCommandBuffer cmd) {
        VkBufferCopy copyRegion{ 0, dstOffset, size };
        vkCmdCopyBuffer(cmd, stagingBuffer, m_buffer, 1, &copyRegion);

        // Add a barrier if this buffer is used immediately after in a build or compute task
        VkBufferMemoryBarrier barrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask =
            VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT |
            VK_ACCESS_INDEX_READ_BIT |
            VK_ACCESS_SHADER_READ_BIT |
            VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
        barrier.buffer = m_buffer;
        barrier.offset = dstOffset;
        barrier.size = size;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);
        });

    vmaDestroyBuffer(VulkanMemoryManager::GetAllocator(), stagingBuffer, stagingAlloc);
}

void VulkanBuffer::Map(void** data) {
    if (m_mappedPtr) {
        *data = m_mappedPtr;
    }
    else {
        vmaMapMemory(VulkanMemoryManager::GetAllocator(), m_allocation, data);
        m_mappedPtr = *data;
    }
}

void VulkanBuffer::Unmap() {
    if (m_mappedPtr) {
        vmaUnmapMemory(VulkanMemoryManager::GetAllocator(), m_allocation);
        m_mappedPtr = nullptr;
    }
}

uint64_t VulkanBuffer::GetDeviceAddress() const {
    if (m_deviceAddress != 0) return m_deviceAddress;
    if (m_buffer == VK_NULL_HANDLE) return 0;

    VkBufferDeviceAddressInfo addressInfo{ VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
    addressInfo.buffer = m_buffer;
    m_deviceAddress = vkGetBufferDeviceAddressKHR(VulkanDeviceManager::GetDevice(), &addressInfo);
    return m_deviceAddress;
}

VkDescriptorBufferInfo VulkanBuffer::GetDescriptorInfo() const {
    return VkDescriptorBufferInfo{ m_buffer, 0, m_size };
}

size_t VulkanBuffer::GetCPUAllocatedByteCount() const {
    return sizeof(VulkanBuffer);
}

size_t VulkanBuffer::GetGPUAllocatedByteCount() const {
    if (m_buffer == VK_NULL_HANDLE || m_allocation == VK_NULL_HANDLE) {
        return 0;
    }

    VmaAllocationInfo allocationInfo{};
    vmaGetAllocationInfo(VulkanMemoryManager::GetAllocator(), m_allocation, &allocationInfo);
    return static_cast<size_t>(allocationInfo.size);
}
