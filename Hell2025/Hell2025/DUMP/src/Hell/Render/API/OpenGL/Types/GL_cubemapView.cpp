#include "GL_cubemapView.h"
#include "Hell/Render/API/OpenGL/GL_util.h"
#include <algorithm>
#include <iostream>

OpenGLCubemapView::OpenGLCubemapView(const std::vector<GLuint>& tex2D) {
    CreateCubemap(tex2D);
}

void OpenGLCubemapView::CreateCubemap(const std::vector<GLuint>& tex2D) {
    CleanUp();

    if (tex2D.size() != 6) {
        std::cout << "Cubemap requires exactly 6 textures.\n";
        return;
    }

    glGenTextures(1, &m_textureArrayHandle);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_textureArrayHandle);

    GLint width, height, format;
    glBindTexture(GL_TEXTURE_2D, tex2D[0]);  // Query properties from the first texture
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &format);

    m_width = static_cast<GLuint>(width);
    m_height = static_cast<GLuint>(height);
    m_internalFormat = static_cast<GLenum>(format);
    m_mipLevelCount = 1;
    for (GLint mipSize = std::max(width, height); mipSize > 1; mipSize /= 2) {
        m_mipLevelCount++;
    }

    // Give reflections somewhere to go when their footprint gets too big
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, m_mipLevelCount, format, width, height, 6);

    // Copy each texture into the array
    for (int i = 0; i < 6; ++i) {
        glCopyImageSubData(tex2D[i], GL_TEXTURE_2D, 0, 0, 0, 0,
            m_textureArrayHandle, GL_TEXTURE_2D_ARRAY, 0, 0, 0, i,
            width, height, 1);
    }

    glGenerateTextureMipmap(m_textureArrayHandle);

    // Now create a cubemap view from the 2D array
    glGenTextures(1, &m_handle);
    glTextureView(m_handle, GL_TEXTURE_CUBE_MAP, m_textureArrayHandle, format, 0, m_mipLevelCount, 0, 6);

    glTextureParameteri(m_handle, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri(m_handle, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(m_handle, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_handle, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_handle, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

void OpenGLCubemapView::CleanUp() {
    if (m_handle != 0) {
        glDeleteTextures(1, &m_handle);
        m_handle = 0;
    }

    if (m_textureArrayHandle != 0) {
        glDeleteTextures(1, &m_textureArrayHandle);
        m_textureArrayHandle = 0;
    }

    m_width = 0;
    m_height = 0;
    m_mipLevelCount = 0;
    m_internalFormat = GL_NONE;
}


GLuint OpenGLCubemapView::GetHandle() const {
    return m_handle;
}

size_t OpenGLCubemapView::GetGPUAllocatedByteCount() const {
    if (m_textureArrayHandle == 0) {
        return 0;
    }

    return OpenGL::Util::CalculateTexture2DArrayByteCount(m_width, m_height, 6, m_internalFormat, m_mipLevelCount);
}
