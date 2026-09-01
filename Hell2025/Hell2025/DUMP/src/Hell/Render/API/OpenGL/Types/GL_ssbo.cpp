#include "GL_ssbo.h"
#include <stdexcept>
#include <cstring>

OpenGLSSBO::OpenGLSSBO(size_t size, GLbitfield flags) {
    Create(size, flags);
}

void OpenGLSSBO::Create(size_t size, GLbitfield flags) {
    CleanUp();
    m_flags = flags;
    Reserve(size);
}

void OpenGLSSBO::Reserve(size_t size) {
    // Skip if there is already enough space
    if (m_handle != 0 && m_bufferSize >= size) {
        return;
    }

    CleanUp();

    glCreateBuffers(1, &m_handle);
    glNamedBufferStorage(m_handle, (GLsizeiptr)size, nullptr, m_flags);
    m_bufferSize = size;
}

void OpenGLSSBO::Update(size_t size, const void* data) {
    if (size == 0 || data == nullptr) {
        return;
    }

    Reserve(size);

    glNamedBufferSubData(m_handle, 0, (GLsizeiptr)size, data);
}

void OpenGLSSBO::UpdateRange(size_t offset, size_t size, const void* data) {
    if (size == 0 || data == nullptr) {
        return;
    }

    Reserve(offset + size);

    glNamedBufferSubData(m_handle, (GLintptr)offset, (GLsizeiptr)size, data);
}

void OpenGLSSBO::UploadStatic(size_t size, const void* data) {
    if (size == 0 || data == nullptr) {
        return;
    }

    CleanUp();

    glCreateBuffers(1, &m_handle);
    glNamedBufferStorage(m_handle, (GLsizeiptr)size, data, 0);

    m_bufferSize = size;
    m_flags = 0;
}

void OpenGLSSBO::Bind(uint32_t index) const {
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, m_handle);
}

void OpenGLSSBO::CleanUp() {
    if (m_handle != 0) {
        glDeleteBuffers(1, &m_handle);
        m_handle = 0;
        m_bufferSize = 0;
    }
}

void OpenGLSSBO::CopyFrom(const void* hostPtr, size_t sizeInBytes) {
    if (!hostPtr || sizeInBytes == 0 || m_handle == 0) {
        return;
    }

    // Map and copy
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_handle);
    void* devPtrRaw = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeInBytes, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

    if (!devPtrRaw) {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        throw std::runtime_error("glMapBufferRange failed in copyFrom.");
        return;
    }

    std::memcpy(devPtrRaw, hostPtr, sizeInBytes);
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void OpenGLSSBO::Clear() const {
    if (m_handle == 0) return;

    uint32_t zero = 0;
    glClearNamedBufferData(m_handle, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);
}

void OpenGLSSBO::ClearRange(size_t offset, size_t size) const {
    if (m_handle == 0) return;
    if (offset >= m_bufferSize || size == 0) return;

    // Clamp clear size to buffer bounds
    size_t actualSize = (offset + size > m_bufferSize) ? (m_bufferSize - offset) : size;
    uint32_t zero = 0;
    glClearNamedBufferSubData(m_handle, GL_R32UI, (GLintptr)offset, (GLsizeiptr)actualSize, GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);
}
