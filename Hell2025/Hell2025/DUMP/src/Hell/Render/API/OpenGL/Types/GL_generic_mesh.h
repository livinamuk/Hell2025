#pragma once
#include "Hell/Render/VertexAttributes.h"

#include <cstddef>
#include <cstdint>
#include <vector>

struct OpenGLGenericMesh {
    void UpdateVertexData(const void* vertices, size_t vertexCount, const VertexLayoutDescription& layout);
    void UpdateIndexData(const std::vector<uint32_t>& indices);
    void CleanUp();

    size_t GetVertexCount() const { return m_vertexCount; }
    size_t GetIndexCount() const  { return m_indexCount; }
    uint32_t GetVAO() const       { return m_vao; }
    uint32_t GetVBO() const       { return m_vbo; }
    uint32_t GetEBO() const       { return m_ebo; }

private:
    void Init(const VertexLayoutDescription& layout);
    void ResizeVertexBuffer(size_t newCapacity);
    void ResizeIndexBuffer(size_t newCapacity);

    uint32_t m_vao = 0;
    uint32_t m_vbo = 0;
    uint32_t m_ebo = 0;
    size_t m_vertexStride = 0;
    size_t m_vertexCount = 0;
    size_t m_indexCount = 0;
    size_t m_vertexCapacity = 0;
    size_t m_indexCapacity = 0;
};
