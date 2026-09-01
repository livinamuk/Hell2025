#pragma once

#include "Hell/Render/API/Vulkan/Types/vk_buffer.h"
#include "Hell/Render/VertexAttributes.h"

#include <cstddef>
#include <cstdint>
#include <vector>

struct VulkanMeshBuffer {
    VulkanMeshBuffer() = default;
    VulkanMeshBuffer(const VulkanMeshBuffer&) = delete;
    VulkanMeshBuffer& operator=(const VulkanMeshBuffer&) = delete;
    VulkanMeshBuffer(VulkanMeshBuffer&&) noexcept = default;
    VulkanMeshBuffer& operator=(VulkanMeshBuffer&&) noexcept = default;

    void Init(const VertexLayoutDescription& layout);
    void Reset();
    void Cleanup();

    void InsertVertices(const std::vector<Vertex>& vertices, uint32_t insertOffset);
    void InsertIndices(const std::vector<uint32_t>& indices, uint32_t insertOffset);
    void InsertVertexWeights(const std::vector<VertexWeight>& vertexWeights, uint32_t insertOffset);
    void PreAllocate(size_t vertexCapacity, size_t indexCapacity, size_t vertexWeightCapacity);
    void ResizeVertexBuffer(size_t newCapacity, const std::vector<Vertex>& vertices);
    void ResizeIndexBuffer(size_t newCapacity, const std::vector<uint32_t>& indices);
    void ResizeVertexWeightBuffer(size_t newCapacity, const std::vector<VertexWeight>& vertexWeights);

    VulkanBuffer* GetVertexBuffer() { return m_vertexBuffer.GetBuffer() == VK_NULL_HANDLE ? nullptr : &m_vertexBuffer; }
    VulkanBuffer* GetIndexBuffer()  { return m_indexBuffer.GetBuffer() == VK_NULL_HANDLE ? nullptr : &m_indexBuffer; }
    VulkanBuffer* GetVertexWeightBuffer() { return m_vertexWeightBuffer.GetBuffer() == VK_NULL_HANDLE ? nullptr : &m_vertexWeightBuffer; }
    const VulkanBuffer* GetVertexBuffer() const { return m_vertexBuffer.GetBuffer() == VK_NULL_HANDLE ? nullptr : &m_vertexBuffer; }
    const VulkanBuffer* GetIndexBuffer() const  { return m_indexBuffer.GetBuffer() == VK_NULL_HANDLE ? nullptr : &m_indexBuffer; }
    const VulkanBuffer* GetVertexWeightBuffer() const { return m_vertexWeightBuffer.GetBuffer() == VK_NULL_HANDLE ? nullptr : &m_vertexWeightBuffer; }

    uint64_t GetVertexBufferAddress() const;
    uint64_t GetIndexBufferAddress() const;
    uint64_t GetVertexWeightBufferAddress() const;
    size_t GetCPUAllocatedByteCount() const;
    size_t GetGPUAllocatedByteCount() const;

private:
    VulkanBuffer CreateVertexBuffer(size_t vertexCapacity) const;
    VulkanBuffer CreateIndexBuffer(size_t indexCapacity) const;
    VulkanBuffer CreateVertexWeightBuffer(size_t vertexWeightCapacity) const;

    VulkanBuffer m_vertexBuffer;
    VulkanBuffer m_indexBuffer;
    VulkanBuffer m_vertexWeightBuffer;
    size_t m_vertexStride = 0;
};
