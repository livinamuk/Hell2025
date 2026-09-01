#include "GL_mesh_buffer_old.h"

void OpenGLMeshBufferOLD::UpdateBuffers(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {
    GLsizei newVertexCount = static_cast<GLsizei>(vertices.size());
    GLsizei newIndexCount = static_cast<GLsizei>(indices.size());

    // Bail if either vertices or indices are empty
    if (newVertexCount == 0 || newIndexCount == 0) {
        ReleaseBuffers();
        return;
    }

    // If the buffer doesn't exist yet
    if (m_VAO == 0) {
        glGenVertexArrays(1, &m_VAO);
        glGenBuffers(1, &m_VBO);
        glGenBuffers(1, &m_EBO);

        glBindVertexArray(m_VAO);

        // Vertices
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        glBufferData(GL_ARRAY_BUFFER, newVertexCount * sizeof(Vertex), vertices.data(), GL_DYNAMIC_DRAW);
        m_allocatedVertexCount = newVertexCount;

        // Indices
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, newIndexCount * sizeof(uint32_t), indices.data(), GL_DYNAMIC_DRAW);
        m_allocatedIndexCount = newIndexCount;

        // Vertex attributes
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tangent));
    }
    // If the buffer does exist
    else {
        glBindVertexArray(0);

        // Vertices
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        if (newVertexCount > m_allocatedVertexCount) {
            glBufferData(GL_ARRAY_BUFFER, newVertexCount * sizeof(Vertex), vertices.data(), GL_DYNAMIC_DRAW);
            m_allocatedVertexCount = newVertexCount;
        }
        else {
            glBufferSubData(GL_ARRAY_BUFFER, 0, newVertexCount * sizeof(Vertex), vertices.data());
        }
        // Indices
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
        if (newIndexCount > m_allocatedIndexCount) {
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, newIndexCount * sizeof(uint32_t), indices.data(), GL_DYNAMIC_DRAW);
            m_allocatedIndexCount = newIndexCount;
        }
        else {
            glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, newIndexCount * sizeof(uint32_t), indices.data());
        }
    }

    m_vertexCount = newVertexCount;
    m_indexCount = newIndexCount;

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void OpenGLMeshBufferOLD::AllocateBuffers(size_t vertexCount, size_t indexCount) {
    GLsizei requestedVertexCount = static_cast<GLsizei>(vertexCount);
    GLsizei requestedIndexCount = static_cast<GLsizei>(indexCount);

    // Bail if either vertices or indices are empty
    if (requestedVertexCount == 0 || requestedIndexCount == 0) {
        ReleaseBuffers();
        return;
    }

    // Create buffers if they don't exist
    if (m_VAO == 0) {
        glGenVertexArrays(1, &m_VAO);
        glGenBuffers(1, &m_VBO);
        glGenBuffers(1, &m_EBO);

        glBindVertexArray(m_VAO);

        // Vertex buffer
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        glBufferData(GL_ARRAY_BUFFER, requestedVertexCount * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);
        m_allocatedVertexCount = requestedVertexCount;

        // Index buffer
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, requestedIndexCount * sizeof(uint32_t), nullptr, GL_DYNAMIC_DRAW);
        m_allocatedIndexCount = requestedIndexCount;

        // Vertex attributes
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position)); // Assuming 'position' member
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tangent));

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    // Reallocate existing buffers if needed
    else {
        // Vertex buffer
        if (requestedVertexCount > m_allocatedVertexCount) {
            glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
            glBufferData(GL_ARRAY_BUFFER, requestedVertexCount * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            m_allocatedVertexCount = requestedVertexCount;
        }

        // Index buffer
        if (requestedIndexCount > m_allocatedIndexCount) {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, requestedIndexCount * sizeof(uint32_t), nullptr, GL_DYNAMIC_DRAW);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            m_allocatedIndexCount = requestedIndexCount;
        }
    }

    m_vertexCount = 0;
    m_indexCount = 0;
}

void OpenGLMeshBufferOLD::ReleaseBuffers() {
    if (m_VAO != 0) {
        glDeleteVertexArrays(1, &m_VAO);
        if (m_VBO != 0) {
            glDeleteBuffers(1, &m_VBO);
        }
        if (m_EBO != 0) {
            glDeleteBuffers(1, &m_EBO);
        }
        m_VAO = 0;
        m_VBO = 0;
        m_EBO = 0;
        m_vertexCount = 0;
        m_indexCount = 0;
        m_allocatedVertexCount = 0;
        m_allocatedIndexCount = 0;
    }
}