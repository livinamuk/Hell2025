#include "GL_texture_array.h"
#include "Hell/Render/API/OpenGL/GL_util.h"
#include <glad/gl.h>
#include "Hell/Logging.h"

void OpenGLTextureArray::AllocateMemory(uint32_t width, uint32_t height, uint32_t internalFormat, uint32_t mipmapLevelCount, uint32_t count) {
    if (m_memoryAllocated) return;

    glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &m_handle);
    glTextureStorage3D(m_handle, mipmapLevelCount, internalFormat, width, height, count);
    glTextureParameteri(m_handle, GL_TEXTURE_MIN_FILTER, GL_LINEAR);// GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri(m_handle, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(m_handle, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(m_handle, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTextureParameteri(m_handle, GL_TEXTURE_WRAP_R, GL_REPEAT);

    m_width = width;
    m_height = height;
    m_internalFormat = internalFormat;
    m_mipmapLevelCount = mipmapLevelCount;
    m_count = count;
    m_memoryAllocated = true;
    m_format = OpenGL::Util::GLInternalFormatToGLFormat(m_internalFormat);
    m_type = OpenGL::Util::GLInternalFormatToGLType(m_internalFormat);
}

void OpenGLTextureArray::CleanUp() {
    if (m_handle != 0) {
        glDeleteTextures(1, &m_handle);
        m_handle = 0;
    }

    m_width = 0;
    m_height = 0;
    m_count = 0;
    m_format = 0;
    m_internalFormat = 0;
    m_mipmapLevelCount = 0;
    m_type = 0;
    m_memoryAllocated = false;
}

void OpenGLTextureArray::SetLayerDataR16(uint32_t layerIndex, const std::vector<float>& data) {
    if (layerIndex >= m_count) {
        Logging::Error() << "OpenGLTextureArray::SetLayerDataR16(() failed: layerIndex out of range!\n";
        return;
    }
    if (data.size() != static_cast<size_t>(m_width * m_height)) {
        Logging::Error() << "OpenGLTextureArray::SetLayerDataR16() failed because data.size() was " << data.size() << " but texture array width(" << m_width << ") * height(" << m_height << ") equals " << (m_width * m_height) << "\n";
        return;
    }
    glTextureSubImage3D(m_handle, 0, 0, 0, layerIndex, m_width, m_height, 1, GL_RED, GL_FLOAT, data.data());
}

void OpenGLTextureArray::GenerateMipmaps() {
    if (m_memoryAllocated && m_mipmapLevelCount > 1) {
        glGenerateTextureMipmap(m_handle);
    }
}

void OpenGLTextureArray::SetWrapMode(TextureWrapMode wrapMode) {
    if (!m_memoryAllocated) return;
    glTextureParameteri(m_handle, GL_TEXTURE_WRAP_S, OpenGL::Util::TextureWrapModeToGLEnum(wrapMode));
    glTextureParameteri(m_handle, GL_TEXTURE_WRAP_T, OpenGL::Util::TextureWrapModeToGLEnum(wrapMode));
    glTextureParameteri(m_handle, GL_TEXTURE_WRAP_R, OpenGL::Util::TextureWrapModeToGLEnum(wrapMode));
}

void OpenGLTextureArray::SetMinFilter(TextureFilter filter) {
    if (!m_memoryAllocated) return;
    glTextureParameteri(m_handle, GL_TEXTURE_MIN_FILTER, OpenGL::Util::TextureFilterToGLEnum(filter));
}

void OpenGLTextureArray::SetMagFilter(TextureFilter filter) {
    if (!m_memoryAllocated) return;
    glTextureParameteri(m_handle, GL_TEXTURE_MAG_FILTER, OpenGL::Util::TextureFilterToGLEnum(filter));
}

void OpenGLTextureArray::Clear(float r, float g, float b, float a) {
    if (!m_memoryAllocated) return;
    const float clearColor[4] = { r, g, b, a };
    glClearTexImage(m_handle, 0, m_format, m_type, clearColor);
}

void OpenGLTextureArray::ClearLayer(float r, float g, float b, float a, int layerIndex) {
    if (!m_memoryAllocated) return;
    int baseWidth = GetWidth();
    int baseHeight = GetHeight();
    const float clearColor[4] = { r, g, b, a };
    int mipmapLevel = 0;
    int w = std::max(1, baseWidth >> mipmapLevel);
    int h = std::max(1, baseHeight >> mipmapLevel);
    glClearTexSubImage(m_handle, mipmapLevel, 0, 0, layerIndex, w, h, 1, m_format, m_type, clearColor);
}

void OpenGLTextureArray::ClearAllMipLevels(float r, float g, float b, float a) {
    if (!m_memoryAllocated) return;

    const float clearColor[4] = { r, g, b, a };

    for (uint32_t mip = 0; mip < m_mipmapLevelCount; mip++) {
        glClearTexImage(
            m_handle,
            mip,
            m_format,
            m_type,
            clearColor
        );
    }
}

size_t OpenGLTextureArray::GetGPUAllocatedByteCount() const {
    if (!m_memoryAllocated) {
        return 0;
    }

    return OpenGL::Util::CalculateTexture2DArrayByteCount(m_width, m_height, m_count, m_internalFormat, m_mipmapLevelCount);
}
