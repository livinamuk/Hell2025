#include "GL_generic_mesh.h"
#include <glad/gl.h>

void OpenGLGenericMesh::UpdateVertexData(const void* vertices, size_t vertexCount, const VertexLayoutDescription& layout) {
    if (m_vao == 0) {
        Init(layout);
    }

    m_vertexCount = vertexCount;

    if (vertexCount == 0) {
        return;
    }

    if (vertexCount > m_vertexCapacity) {
        ResizeVertexBuffer(vertexCount);
    }

    glNamedBufferSubData(m_vbo, 0, vertexCount * m_vertexStride, vertices);
}

void OpenGLGenericMesh::UpdateIndexData(const std::vector<uint32_t>& indices) {
    m_indexCount = indices.size();

    if (indices.empty()) {
        return;
    }

    if (indices.size() > m_indexCapacity) {
        ResizeIndexBuffer(indices.size());
    }

    glNamedBufferSubData(m_ebo, 0, indices.size() * sizeof(uint32_t), indices.data());
}

void OpenGLGenericMesh::Init(const VertexLayoutDescription& layout) {
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

        if (attribute.type == VertexAttributeType::Float) {
            glVertexArrayAttribFormat(m_vao, attribute.location, attribute.componentCount, type, attribute.normalized ? GL_TRUE : GL_FALSE, static_cast<GLuint>(attribute.offset));
        }
        else {
            glVertexArrayAttribIFormat(m_vao, attribute.location, attribute.componentCount, type, static_cast<GLuint>(attribute.offset));
        }

        glVertexArrayAttribBinding(m_vao, attribute.location, 0);
    }

    if (m_ebo != 0) {
        glVertexArrayElementBuffer(m_vao, m_ebo);
    }
}

void OpenGLGenericMesh::ResizeVertexBuffer(size_t newCapacity) {
    uint32_t newVbo = 0;
    glCreateBuffers(1, &newVbo);
    glNamedBufferStorage(newVbo, newCapacity * m_vertexStride, nullptr, GL_DYNAMIC_STORAGE_BIT);

    if (m_vbo != 0) {
        glDeleteBuffers(1, &m_vbo);
    }

    m_vbo = newVbo;
    m_vertexCapacity = newCapacity;

    glVertexArrayVertexBuffer(m_vao, 0, m_vbo, 0, static_cast<GLsizei>(m_vertexStride));
}

void OpenGLGenericMesh::ResizeIndexBuffer(size_t newCapacity) {
    uint32_t newEbo = 0;
    glCreateBuffers(1, &newEbo);
    glNamedBufferStorage(newEbo, newCapacity * sizeof(uint32_t), nullptr, GL_DYNAMIC_STORAGE_BIT);

    if (m_ebo != 0) {
        glDeleteBuffers(1, &m_ebo);
    }

    m_ebo = newEbo;
    m_indexCapacity = newCapacity;

    if (m_vao != 0) {
        glVertexArrayElementBuffer(m_vao, m_ebo);
    }
}

void OpenGLGenericMesh::CleanUp() {
    if (m_vao != 0) glDeleteVertexArrays(1, &m_vao);
    if (m_vbo != 0) glDeleteBuffers(1, &m_vbo);
    if (m_ebo != 0) glDeleteBuffers(1, &m_ebo);

    m_vao = 0;
    m_vbo = 0;
    m_ebo = 0;
    m_vertexStride = 0;
    m_vertexCount = 0;
    m_indexCount = 0;
    m_vertexCapacity = 0;
    m_indexCapacity = 0;
}
