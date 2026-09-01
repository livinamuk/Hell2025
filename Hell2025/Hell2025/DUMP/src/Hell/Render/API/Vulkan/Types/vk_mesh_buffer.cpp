#include "vk_mesh_buffer.h"

#include "Hell/Logging.h"

#include <utility>

namespace {
    constexpr VkBufferUsageFlags VERTEX_BUFFER_USAGE =
        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

    constexpr VkBufferUsageFlags INDEX_BUFFER_USAGE =
        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

    constexpr VkBufferUsageFlags VERTEX_WEIGHT_BUFFER_USAGE =
        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
}

void VulkanMeshBuffer::Init(const VertexLayoutDescription& layout) {
    Reset();
    m_vertexStride = layout.stride;

    if (m_vertexStride == 0) {
        Logging::Error() << "VulkanMeshBuffer::Init(..) received a zero vertex stride\n";
    }
}

void VulkanMeshBuffer::Reset() {
    m_vertexBuffer.Cleanup();
    m_indexBuffer.Cleanup();
    m_vertexWeightBuffer.Cleanup();
}

void VulkanMeshBuffer::Cleanup() {
    Reset();
    m_vertexStride = 0;
}

void VulkanMeshBuffer::InsertVertices(const std::vector<Vertex>& vertices, uint32_t insertOffset) {
    if (vertices.empty() || !GetVertexBuffer()) return;

    const VkDeviceSize byteOffset = static_cast<VkDeviceSize>(insertOffset) * m_vertexStride;
    const VkDeviceSize byteSize = static_cast<VkDeviceSize>(vertices.size()) * m_vertexStride;
    m_vertexBuffer.UploadData(vertices.data(), byteSize, byteOffset);
}

void VulkanMeshBuffer::InsertIndices(const std::vector<uint32_t>& indices, uint32_t insertOffset) {
    if (indices.empty() || !GetIndexBuffer()) return;

    const VkDeviceSize byteOffset = static_cast<VkDeviceSize>(insertOffset) * sizeof(uint32_t);
    const VkDeviceSize byteSize = static_cast<VkDeviceSize>(indices.size()) * sizeof(uint32_t);
    m_indexBuffer.UploadData(indices.data(), byteSize, byteOffset);
}

void VulkanMeshBuffer::InsertVertexWeights(const std::vector<VertexWeight>& vertexWeights, uint32_t insertOffset) {
    if (vertexWeights.empty() || !GetVertexWeightBuffer()) return;

    const VkDeviceSize byteOffset = static_cast<VkDeviceSize>(insertOffset) * sizeof(VertexWeight);
    const VkDeviceSize byteSize = static_cast<VkDeviceSize>(vertexWeights.size()) * sizeof(VertexWeight);
    m_vertexWeightBuffer.UploadData(vertexWeights.data(), byteSize, byteOffset);
}

void VulkanMeshBuffer::PreAllocate(size_t vertexCapacity, size_t indexCapacity, size_t vertexWeightCapacity) {
    Reset();

    if (vertexCapacity > 0) {
        m_vertexBuffer = CreateVertexBuffer(vertexCapacity);
    }

    if (indexCapacity > 0) {
        m_indexBuffer = CreateIndexBuffer(indexCapacity);
    }

    if (vertexWeightCapacity > 0) {
        m_vertexWeightBuffer = CreateVertexWeightBuffer(vertexWeightCapacity);
    }
}

void VulkanMeshBuffer::ResizeVertexBuffer(size_t newCapacity, const std::vector<Vertex>& vertices) {
    VulkanBuffer newBuffer = CreateVertexBuffer(newCapacity);

    if (!vertices.empty()) {
        const VkDeviceSize byteSize = static_cast<VkDeviceSize>(vertices.size()) * m_vertexStride;
        newBuffer.UploadData(vertices.data(), byteSize);
    }

    m_vertexBuffer = std::move(newBuffer);
}

void VulkanMeshBuffer::ResizeIndexBuffer(size_t newCapacity, const std::vector<uint32_t>& indices) {
    VulkanBuffer newBuffer = CreateIndexBuffer(newCapacity);

    if (!indices.empty()) {
        const VkDeviceSize byteSize = static_cast<VkDeviceSize>(indices.size()) * sizeof(uint32_t);
        newBuffer.UploadData(indices.data(), byteSize);
    }

    m_indexBuffer = std::move(newBuffer);
}

void VulkanMeshBuffer::ResizeVertexWeightBuffer(size_t newCapacity, const std::vector<VertexWeight>& vertexWeights) {
    VulkanBuffer newBuffer = CreateVertexWeightBuffer(newCapacity);

    if (!vertexWeights.empty()) {
        const VkDeviceSize byteSize = static_cast<VkDeviceSize>(vertexWeights.size()) * sizeof(VertexWeight);
        newBuffer.UploadData(vertexWeights.data(), byteSize);
    }

    m_vertexWeightBuffer = std::move(newBuffer);
}

uint64_t VulkanMeshBuffer::GetVertexBufferAddress() const {
    return GetVertexBuffer() ? m_vertexBuffer.GetDeviceAddress() : 0;
}

uint64_t VulkanMeshBuffer::GetIndexBufferAddress() const {
    return GetIndexBuffer() ? m_indexBuffer.GetDeviceAddress() : 0;
}

uint64_t VulkanMeshBuffer::GetVertexWeightBufferAddress() const {
    return GetVertexWeightBuffer() ? m_vertexWeightBuffer.GetDeviceAddress() : 0;
}

VulkanBuffer VulkanMeshBuffer::CreateVertexBuffer(size_t vertexCapacity) const {
    const VkDeviceSize size = static_cast<VkDeviceSize>(vertexCapacity) * m_vertexStride;
    return VulkanBuffer(size, VERTEX_BUFFER_USAGE, VMA_MEMORY_USAGE_GPU_ONLY);
}

VulkanBuffer VulkanMeshBuffer::CreateIndexBuffer(size_t indexCapacity) const {
    const VkDeviceSize size = static_cast<VkDeviceSize>(indexCapacity) * sizeof(uint32_t);
    return VulkanBuffer(size, INDEX_BUFFER_USAGE, VMA_MEMORY_USAGE_GPU_ONLY);
}

VulkanBuffer VulkanMeshBuffer::CreateVertexWeightBuffer(size_t vertexWeightCapacity) const {
    const VkDeviceSize size = static_cast<VkDeviceSize>(vertexWeightCapacity) * sizeof(VertexWeight);
    return VulkanBuffer(size, VERTEX_WEIGHT_BUFFER_USAGE, VMA_MEMORY_USAGE_GPU_ONLY);
}

size_t VulkanMeshBuffer::GetCPUAllocatedByteCount() const {
    return sizeof(VulkanMeshBuffer);
}

size_t VulkanMeshBuffer::GetGPUAllocatedByteCount() const {
    return m_vertexBuffer.GetGPUAllocatedByteCount() +
        m_indexBuffer.GetGPUAllocatedByteCount() +
        m_vertexWeightBuffer.GetGPUAllocatedByteCount();
}
