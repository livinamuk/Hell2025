#pragma once
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <cstddef>
#include <vector>

struct OpenGLCubemapView {
    OpenGLCubemapView() = default;
    OpenGLCubemapView(const std::vector<GLuint>& tex2D);
    void CreateCubemap(const std::vector<GLuint>& tex2D);
    void CleanUp();
    GLuint GetHandle() const;
    size_t GetGPUAllocatedByteCount() const;

private:
    GLuint m_handle = 0;
    GLuint m_textureArrayHandle = 0;
    GLuint m_width = 0;
    GLuint m_height = 0;
    GLuint m_mipLevelCount = 0;
    GLenum m_internalFormat = GL_NONE;
};
