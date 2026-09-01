#pragma once

#include "Hell/Render/VertexAttributes.h"

#include <vector>
#include <glad/gl.h>

struct OpenGLMeshBufferOLD {
    void ReleaseBuffers();
    void UpdateBuffers(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);
    void AllocateBuffers(size_t vertexCount, size_t indexCount);

    const GLuint GetVAO() const { return m_VAO; }
    const GLuint GetVBO() const { return m_VBO; }
    const GLuint GetEBO() const { return m_EBO; }
    const GLsizei GetVertexCount() const { return m_vertexCount; }
    const GLsizei GetIndexCount() const { return m_indexCount; }
    const GLsizei GetAllocatedVertexCount() const { return m_allocatedVertexCount; }
    const GLsizei GetAllocatedIndexCount() const { return m_allocatedIndexCount; }

private:
    GLuint m_VAO = 0;
    GLuint m_VBO = 0;
    GLuint m_EBO = 0;
    GLsizei m_vertexCount = 0;
    GLsizei m_indexCount = 0;
    GLsizei m_allocatedVertexCount = 0;
    GLsizei m_allocatedIndexCount = 0;
};
