#include "vk_generic_mesh.h"

#include "Hell/Logging.h"
#include "Hell/Render/API/Vulkan/Managers/vk_deletion_queue.h"
#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"

namespace {
    constexpr VkBufferUsageFlags VERTEX_BUFFER_USAGE = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    constexpr VkBufferUsageFlags INDEX_BUFFER_USAGE = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    constexpr VmaAllocationCreateFlags ALLOCATION_FLAGS = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
}

void VulkanGenericMesh::UpdateVertexData(const void* vertices, size_t vertexCount, const VertexLayoutDescription& layout) {
    if (m_vertexStride == 0) {
        m_vertexStride = layout.stride;
    }

    if (m_vertexStride == 0) {
        Logging::Error() << "VulkanGenericMesh::UpdateVertexData(..) received a zero vertex stride\n";
        return;
    }

    m_vertexCount = vertexCount;

    if (!vertices || vertexCount == 0) {
        return;
    }

    if (vertexCount > m_vertexCapacity) {
        ResizeVertexBuffer(vertexCount);
    }

    if (VulkanBuffer* vertexBuffer = GetVertexBuffer()) {
        vertexBuffer->UpdateData(vertices, static_cast<VkDeviceSize>(vertexCount * m_vertexStride));
    }
}

void VulkanGenericMesh::UpdateIndexData(const std::vector<uint32_t>& indices) {
    m_indexCount = indices.size();

    if (indices.empty()) {
        return;
    }

    if (indices.size() > m_indexCapacity) {
        ResizeIndexBuffer(indices.size());
    }

    if (VulkanBuffer* indexBuffer = GetIndexBuffer()) {
        indexBuffer->UpdateData(indices.data(), static_cast<VkDeviceSize>(indices.size() * sizeof(uint32_t)));
    }
}

void VulkanGenericMesh::CleanUp() {
    if (m_vertexBufferId != 0) {
        VulkanResourceManager::RemoveBuffer(m_vertexBufferId);
        m_vertexBufferId = 0;
    }

    if (m_indexBufferId != 0) {
        VulkanResourceManager::RemoveBuffer(m_indexBufferId);
        m_indexBufferId = 0;
    }

    m_vertexStride = 0;
    m_vertexCount = 0;
    m_indexCount = 0;
    m_vertexCapacity = 0;
    m_indexCapacity = 0;
}

void VulkanGenericMesh::ResizeVertexBuffer(size_t newCapacity) {
    if (m_vertexBufferId != 0) {
        VulkanDeletionQueue::Retire(m_vertexBufferId);
    }

    m_vertexBufferId = VulkanResourceManager::CreateBuffer(static_cast<VkDeviceSize>(newCapacity * m_vertexStride), VERTEX_BUFFER_USAGE, VMA_MEMORY_USAGE_AUTO, ALLOCATION_FLAGS);
    m_vertexCapacity = newCapacity;
}

void VulkanGenericMesh::ResizeIndexBuffer(size_t newCapacity) {
    if (m_indexBufferId != 0) {
        VulkanDeletionQueue::Retire(m_indexBufferId);
    }

    m_indexBufferId = VulkanResourceManager::CreateBuffer(static_cast<VkDeviceSize>(newCapacity * sizeof(uint32_t)), INDEX_BUFFER_USAGE, VMA_MEMORY_USAGE_AUTO, ALLOCATION_FLAGS);
    m_indexCapacity = newCapacity;
}

VulkanBuffer* VulkanGenericMesh::GetVertexBuffer() const {
    return m_vertexBufferId == 0 ? nullptr : VulkanResourceManager::GetBuffer(m_vertexBufferId);
}

VulkanBuffer* VulkanGenericMesh::GetIndexBuffer() const {
    return m_indexBufferId == 0 ? nullptr : VulkanResourceManager::GetBuffer(m_indexBufferId);
}

size_t VulkanGenericMesh::GetCPUAllocatedByteCount() const {
    return sizeof(VulkanGenericMesh);
}

size_t VulkanGenericMesh::GetGPUAllocatedByteCount() const {
    size_t byteCount = 0;

    if (VulkanBuffer* vertexBuffer = GetVertexBuffer()) {
        byteCount += vertexBuffer->GetGPUAllocatedByteCount();
    }
    if (VulkanBuffer* indexBuffer = GetIndexBuffer()) {
        byteCount += indexBuffer->GetGPUAllocatedByteCount();
    }

    return byteCount;
}
