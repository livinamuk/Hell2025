#pragma once
#include <glad/gl.h>
#include <cstdint>

struct IndirectBuffer {
public:
    uint32_t GetHandle() const {
        return m_handle;
    }

    void PreAllocate(size_t size) {
        CleanUp();
        glCreateBuffers(1, &m_handle);
        glNamedBufferStorage(m_handle, (GLsizeiptr)size, nullptr, GL_DYNAMIC_STORAGE_BIT);
        m_bufferSize = size;
    }

    void Update(size_t size, const void* data) {
        if (size == 0) {
            return;
        }
        // Reallocate only if the requested size exceeds the current capacity
        if (m_handle == 0 || m_bufferSize < size) {
            CleanUp();
            glCreateBuffers(1, &m_handle);
            glNamedBufferStorage(m_handle, (GLsizeiptr)size, nullptr, GL_DYNAMIC_STORAGE_BIT);
            m_bufferSize = size;
        }
        // Upload new data
        glNamedBufferSubData(m_handle, 0, (GLsizeiptr)size, data);
        glMemoryBarrier(GL_COMMAND_BARRIER_BIT);
    }

    void Bind() const {
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_handle);
    }

    void CopyFromSSBO(uint32_t ssboHandle, size_t size) {
        glBindBuffer(GL_COPY_READ_BUFFER, ssboHandle);
        glBindBuffer(GL_COPY_WRITE_BUFFER, m_handle);
        glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, (GLsizeiptr)size);
    }

    void CleanUp() {
        if (m_handle != 0) {
            glDeleteBuffers(1, &m_handle);
            m_handle = 0;
            m_bufferSize = 0;
        }
    }

private:
    uint32_t m_handle = 0;
    size_t m_bufferSize = 0;
};
