#pragma once
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <string>
#include <memory>
#include <cstddef>

struct OpenGLTexture3D {
    OpenGLTexture3D() = default;
    void Create(int size, GLenum internalFormat, bool allocateMips);
    void Reset();
    void GenerateMipmaps();

    GLuint GetSize() const              { return m_size; }
    GLuint GetHandle() const            { return m_handle; }
    GLenum GetInternalFormat() const    { return m_internalFormat; }
    GLuint GetLevels() const            { return m_levels; }
    size_t GetGPUAllocatedByteCount() const;

private:
    GLuint m_handle = 0;
    GLuint m_size = 0;
    GLenum m_internalFormat = 0;
    GLuint m_levels = 0;
    bool m_allocatedMips = false;
};
