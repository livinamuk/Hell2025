#pragma once
#include <glad/gl.h>
#include <cstddef>
#include <cstdint>

struct OpenGLSSBO {
    OpenGLSSBO() = default;
    OpenGLSSBO(size_t size, GLbitfield flags);

    void Create(size_t size, GLbitfield flags);
    void Reserve(size_t size);
    void Update(size_t size, const void* data);
    void UpdateRange(size_t offset, size_t size, const void* data);
    void UploadStatic(size_t size, const void* data);
    void Bind(uint32_t index) const;
    void CleanUp();
    void CopyFrom(const void* hostPtr, size_t sizeInBytes);
    void Clear() const;
    void ClearRange(size_t offset, size_t size) const;

    uint32_t GetHandle() const { return m_handle; }
    size_t GetSize() const { return m_bufferSize; }
    size_t GetGPUAllocatedByteCount() const { return m_bufferSize; }

private:
    uint32_t m_handle = 0;
    size_t m_bufferSize = 0;
    GLbitfield m_flags = 0;
};
