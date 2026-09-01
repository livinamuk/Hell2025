#include "GL_texture_3d.h"
#include "Hell/Logging.h"
#include "Hell/Render/API/OpenGL/GL_util.h"

void OpenGLTexture3D::Create(int size, GLenum internalFormat, bool allocateMips) {
    if (m_handle != 0) {
        Reset();
    }

    if (size <= 0) {
        Logging::Error() << "OpenGLTexture3D::Create failed because size was " << size;
        return;
    }

    m_levels = 1;
    m_allocatedMips = allocateMips;

    if (allocateMips) {
        int tmp = size;
        while (tmp > 1) {
            tmp >>= 1;
            ++m_levels;
        }
    }

    glGenTextures(1, &m_handle);
    glBindTexture(GL_TEXTURE_3D, m_handle);
    glTexStorage3D(GL_TEXTURE_3D, (GLsizei)m_levels, internalFormat, size, size, size);

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAX_LEVEL, (GLint)m_levels - 1);

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (allocateMips) {
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    }
    else {
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }

    m_internalFormat = internalFormat;
    m_size = (GLuint)size;
}

void OpenGLTexture3D::Reset() {
    if (m_handle != 0) {
        glDeleteTextures(1, &m_handle);
        m_handle = 0;
    }
    m_internalFormat = 0;
    m_levels = 0;
    m_size = 0;
    m_allocatedMips = false;
}

void OpenGLTexture3D::GenerateMipmaps() {
    if (m_handle == 0 || m_size == 0) {
        return;
    }
    if (!m_allocatedMips || m_levels <= 1) {
        Logging::Error() << "OpenGLTexture3D::GenerateMipmaps called on texture without allocated mip levels!";
        return;
    }

    glGenerateTextureMipmap(m_handle);
}

size_t OpenGLTexture3D::GetGPUAllocatedByteCount() const {
    if (m_handle == 0) {
        return 0;
    }

    return OpenGL::Util::CalculateTexture3DByteCount(m_size, m_size, m_size, m_internalFormat, m_levels);
}
