#include "GL_mesh_buffer.h"
#include <glad/gl.h>

void OpenGLMeshBuffer::Init(const VertexLayoutDescription& layout) {
    if (m_vao != 0) {
        Reset();
    }

    m_vertexStride = layout.stride;
    glCreateVertexArrays(1, &m_vao);

    for (const VertexAttribute& attribute : layout.attributes) {
        glEnableVertexArrayAttrib(m_vao, attribute.location);

        GLenum type = GL_FLOAT;
        switch (attribute.type) {
            case VertexAttributeType::Float:       type = GL_FLOAT;        break;
            case VertexAttributeType::Int:         type = GL_INT;          break;
            case VertexAttributeType::UnsignedInt: type = GL_UNSIGNED_INT; break;
        }

        if(attribute.type == VertexAttributeType::Float) {
            glVertexArrayAttribFormat(m_vao, attribute.location, attribute.componentCount, type, attribute.normalized ? GL_TRUE : GL_FALSE, static_cast<GLuint>(attribute.offset));
        }
        else {
            glVertexArrayAttribIFormat(m_vao, attribute.location, attribute.componentCount, type, static_cast<GLuint>(attribute.offset));
        }

        glVertexArrayAttribBinding(m_vao, attribute.location, 0);
    }
}

void OpenGLMeshBuffer::Reset() {
    if (m_vao != 0) glDeleteVertexArrays(1, &m_vao);
    if (m_vbo != 0) glDeleteBuffers(1, &m_vbo);
    if (m_ebo != 0) glDeleteBuffers(1, &m_ebo);
    if (m_vertexWeightSSBO != 0) glDeleteBuffers(1, &m_vertexWeightSSBO);
    if (m_morphDeltaSSBO != 0) glDeleteBuffers(1, &m_morphDeltaSSBO);

    m_vao = 0;
    m_vbo = 0;
    m_ebo = 0;
    m_vertexWeightSSBO = 0;
    m_morphDeltaSSBO = 0;
    m_vertexStride = 0;
}

void OpenGLMeshBuffer::InsertVertices(const std::vector<Vertex>& vertices, uint32_t insertOffset) {
    if (vertices.empty()) return;

    size_t byteOffset = insertOffset * m_vertexStride;
    size_t byteSize = vertices.size() * m_vertexStride;

    glNamedBufferSubData(m_vbo, byteOffset, byteSize, vertices.data());
}

void OpenGLMeshBuffer::InsertIndices(const std::vector<uint32_t>& indices, uint32_t insertOffset) {
    if (indices.empty()) return;

    size_t byteOffset = insertOffset * sizeof(uint32_t);
    size_t byteSize = indices.size() * sizeof(uint32_t);

    glNamedBufferSubData(m_ebo, byteOffset, byteSize, indices.data());
}

void OpenGLMeshBuffer::InsertVertexWeights(const std::vector<VertexWeight>& vertexWeights, uint32_t insertOffset) {
    if (vertexWeights.empty()) return;

    size_t byteOffset = insertOffset * sizeof(VertexWeight);
    size_t byteSize = vertexWeights.size() * sizeof(VertexWeight);

    glNamedBufferSubData(m_vertexWeightSSBO, byteOffset, byteSize, vertexWeights.data());
}

void OpenGLMeshBuffer::InsertMorphDeltas(const std::vector<MorphTargetVertexDelta>& morphDeltas, uint32_t insertOffset) {
    if (morphDeltas.empty()) return;

    const size_t byteOffset = insertOffset * sizeof(MorphTargetVertexDelta);
    const size_t byteSize = morphDeltas.size() * sizeof(MorphTargetVertexDelta);
    glNamedBufferSubData(m_morphDeltaSSBO, byteOffset, byteSize, morphDeltas.data());
}

void OpenGLMeshBuffer::ResizeVertexBuffer(size_t newCapacity, const std::vector<Vertex>& vertices) {
    GLuint newVbo = 0;
    glCreateBuffers(1, &newVbo);
    glNamedBufferStorage(newVbo, newCapacity * m_vertexStride, nullptr, GL_DYNAMIC_STORAGE_BIT);

    if (!vertices.empty()) {
        glNamedBufferSubData(newVbo, 0, vertices.size() * m_vertexStride, vertices.data());
    }

    if (m_vbo != 0) glDeleteBuffers(1, &m_vbo);

    m_vbo = newVbo;

    glVertexArrayVertexBuffer(m_vao, 0, m_vbo, 0, static_cast<GLsizei>(m_vertexStride));
}

void OpenGLMeshBuffer::ResizeIndexBuffer(size_t newCapacity, const std::vector<uint32_t>& indices) {
    GLuint newEbo = 0;
    glCreateBuffers(1, &newEbo);
    glNamedBufferStorage(newEbo, newCapacity * sizeof(uint32_t), nullptr, GL_DYNAMIC_STORAGE_BIT);

    if (!indices.empty()) {
        glNamedBufferSubData(newEbo, 0, indices.size() * sizeof(uint32_t), indices.data());
    }

    if (m_ebo != 0) glDeleteBuffers(1, &m_ebo);

    m_ebo = newEbo;

    glVertexArrayElementBuffer(m_vao, m_ebo);
}

void OpenGLMeshBuffer::ResizeVertexWeightBuffer(size_t newCapacity, const std::vector<VertexWeight>& vertexWeights) {
    GLuint newVertexWeightSSBO = 0;
    glCreateBuffers(1, &newVertexWeightSSBO);
    glNamedBufferStorage(newVertexWeightSSBO, newCapacity * sizeof(VertexWeight), nullptr, GL_DYNAMIC_STORAGE_BIT);

    if (!vertexWeights.empty()) {
        glNamedBufferSubData(newVertexWeightSSBO, 0, vertexWeights.size() * sizeof(VertexWeight), vertexWeights.data());
    }

    if (m_vertexWeightSSBO != 0) glDeleteBuffers(1, &m_vertexWeightSSBO);

    m_vertexWeightSSBO = newVertexWeightSSBO;
}

void OpenGLMeshBuffer::ResizeMorphDeltaBuffer(size_t newCapacity, const std::vector<MorphTargetVertexDelta>& morphDeltas) {
    GLuint newMorphDeltaSSBO = 0;
    glCreateBuffers(1, &newMorphDeltaSSBO);
    glNamedBufferStorage(newMorphDeltaSSBO, newCapacity * sizeof(MorphTargetVertexDelta), nullptr, GL_DYNAMIC_STORAGE_BIT);

    if (!morphDeltas.empty()) {
        glNamedBufferSubData(newMorphDeltaSSBO, 0, morphDeltas.size() * sizeof(MorphTargetVertexDelta), morphDeltas.data());
    }

    if (m_morphDeltaSSBO != 0) glDeleteBuffers(1, &m_morphDeltaSSBO);
    m_morphDeltaSSBO = newMorphDeltaSSBO;
}

void OpenGLMeshBuffer::PreAllocate(size_t vertexCapacity, size_t indexCapacity, size_t vertexWeightCapacity, size_t morphDeltaCapacity) {
    if (m_vbo != 0) glDeleteBuffers(1, &m_vbo);
    if (m_ebo != 0) glDeleteBuffers(1, &m_ebo);
    if (m_vertexWeightSSBO != 0) glDeleteBuffers(1, &m_vertexWeightSSBO);
    if (m_morphDeltaSSBO != 0) glDeleteBuffers(1, &m_morphDeltaSSBO);

    glCreateBuffers(1, &m_vbo);
    glCreateBuffers(1, &m_ebo);

    glNamedBufferStorage(m_vbo, vertexCapacity * m_vertexStride, nullptr, GL_DYNAMIC_STORAGE_BIT);
    glNamedBufferStorage(m_ebo, indexCapacity * sizeof(uint32_t), nullptr, GL_DYNAMIC_STORAGE_BIT);

    m_vertexWeightSSBO = 0;
    if (vertexWeightCapacity > 0) {
        glCreateBuffers(1, &m_vertexWeightSSBO);
        glNamedBufferStorage(m_vertexWeightSSBO, vertexWeightCapacity * sizeof(VertexWeight), nullptr, GL_DYNAMIC_STORAGE_BIT);
    }

    m_morphDeltaSSBO = 0;
    if (morphDeltaCapacity > 0) {
        glCreateBuffers(1, &m_morphDeltaSSBO);
        glNamedBufferStorage(m_morphDeltaSSBO, morphDeltaCapacity * sizeof(MorphTargetVertexDelta), nullptr, GL_DYNAMIC_STORAGE_BIT);
    }

    glVertexArrayElementBuffer(m_vao, m_ebo);
    glVertexArrayVertexBuffer(m_vao, 0, m_vbo, 0, static_cast<GLsizei>(m_vertexStride));
}
