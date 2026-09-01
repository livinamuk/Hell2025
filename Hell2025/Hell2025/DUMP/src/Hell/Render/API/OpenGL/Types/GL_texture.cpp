#include "GL_texture.h"
#include "Hell/Logging.h"

#include "Hell/Render/API/OpenGL/GL_util.h"

#include <algorithm>
#include <iostream>

GLuint64 OpenGLTexture::GetBindlessID() {
    return m_bindlessID;
}

void OpenGLTexture::Create(int width, int height, int internalFormat, int mipmapLevelCount) {
    if (m_handle != 0 || m_bindlessID != 0) {
        Reset();
    }

    m_width = width;
    m_height = height;
    m_mipmapLevelCount = mipmapLevelCount;
    m_internalFormat = internalFormat;
    m_format = OpenGL::Util::GetFormatFromInternalFormat(internalFormat);

    glCreateTextures(GL_TEXTURE_2D, 1, &m_handle);
    glTextureStorage2D(m_handle, mipmapLevelCount, internalFormat, width, height);

    SetMinFilter(mipmapLevelCount > 1 ? TextureFilter::LINEAR_MIPMAP : TextureFilter::LINEAR);
    SetMagFilter(TextureFilter::LINEAR);
    SetWrapMode(TextureWrapMode::CLAMP_TO_EDGE);
}

void OpenGLTexture::ClearR(float value) {
    if (!m_handle) return;

    // Allow only non-integer color formats
    if (!(m_format == GL_RED || m_format == GL_RG || m_format == GL_RGB || m_format == GL_RGBA)) {
        std::cout << "OpenGLTexture::ClearR() Unsupported format\n";
        return;
    }

    const GLfloat color[4] = { value, 0.0f, 0.0f, 0.0f };

    for (int level = 0; level < m_mipmapLevelCount; ++level) {
        glClearTexImage(m_handle, level, m_format, GL_FLOAT, color);
    }
}

void OpenGLTexture::UploadData(const float* data) {
    if (!m_handle || !data) return;

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

    glTextureSubImage2D(m_handle, 0, 0, 0, m_width, m_height, m_format, GL_FLOAT, data);
}

void OpenGLTexture::UploadR16FData(const float* data, int width, int height, int xOffset, int yOffset, int mipLevel) {
    if (!m_handle || !data) return;

    if (m_internalFormat != GL_R16F) {
        Logging::Error() << "UploadR16FData(): failed coz m_internalFormat was not GL_R16F, but was " << OpenGL::Util::GLInternalFormatToString(m_internalFormat) << "(" << m_internalFormat << ")";
        return;
    }

    // Validate bounds against the mip level size
    GLint levelWidth = 0;
    GLint levelHeight = 0;
    glGetTextureLevelParameteriv(m_handle, mipLevel, GL_TEXTURE_WIDTH, &levelWidth);
    glGetTextureLevelParameteriv(m_handle, mipLevel, GL_TEXTURE_HEIGHT, &levelHeight);

    if (xOffset < 0 || yOffset < 0 || xOffset + width  > levelWidth || yOffset + height > levelHeight) {
        Logging::Error()
            << "UploadR16FData(): out of bounds subimage upload\n"
            << "-xOffset:     " << xOffset << "\n"
            << "-yOffset:     " << yOffset << "\n"
            << "-width:       " << width << "\n"
            << "-height:      " << height << "\n"
            << "-mipLevel:    " << mipLevel << "\n"
            << "-levelWidth:  " << levelWidth << "\n"
            << "-levelHeight: " << levelHeight;
        return;
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTextureSubImage2D(m_handle, mipLevel, xOffset, yOffset, width, height, GL_RED, GL_FLOAT, data);
}

void OpenGLTexture::Reset() {
    MakeBindlessTextureNonResident();

    if (m_handle) {
        glDeleteTextures(1, &m_handle);
        m_handle = 0;
    }
    m_width = 0;
    m_height = 0;
    m_mipmapLevelCount = 0;
    m_bindlessID = 0;
    m_format = 0;
    m_internalFormat = 0;
}

GLuint& OpenGLTexture::GetHandle() {
    return m_handle;
}

int OpenGLTexture::GetWidth() {
    return m_width;
}

int OpenGLTexture::GetHeight() {
    return m_height;
}

int OpenGLTexture::GetChannelCount() {
    return m_channelCount;
}

int OpenGLTexture::GetDataSize() {
    return m_dataSize;
}

size_t OpenGLTexture::GetAllocatedByteCount() const {
    return OpenGL::Util::CalculateTexture2DByteCount(m_width, m_height, m_internalFormat, m_mipmapLevelCount);
}

void* OpenGLTexture::GetData() {
    return m_data;
}

GLint OpenGLTexture::GetFormat() {
    return m_format;
}
GLint OpenGLTexture::GetInternalFormat() {
    return m_internalFormat;
}

GLint OpenGLTexture::GetMipmapLevelCount() {
    return m_mipmapLevelCount;
}

void OpenGLTexture::SetBorderColor(float r, float g, float b, float a) {
    float borderColor[] = { r, g, b, a };
    glTextureParameterfv(m_handle, GL_TEXTURE_BORDER_COLOR, borderColor);
}

void OpenGLTexture::SetBorderColor(const glm::vec4& color) {
    SetBorderColor(color.r, color.g, color.b, color.a);
}

void OpenGLTexture::SetWrapMode(TextureWrapMode wrapMode) {
    SetWrapModeS(wrapMode);
    SetWrapModeT(wrapMode);
}

void OpenGLTexture::SetWrapModeS(TextureWrapMode wrapMode) {
    glTextureParameteri(m_handle, GL_TEXTURE_WRAP_S, OpenGL::Util::TextureWrapModeToGLEnum(wrapMode));
}

void OpenGLTexture::SetWrapModeT(TextureWrapMode wrapMode) {
    glTextureParameteri(m_handle, GL_TEXTURE_WRAP_T, OpenGL::Util::TextureWrapModeToGLEnum(wrapMode));
}

void OpenGLTexture::SetMinFilter(TextureFilter filter) {
    glTextureParameteri(m_handle, GL_TEXTURE_MIN_FILTER, OpenGL::Util::TextureFilterToGLEnum(filter));
}

void OpenGLTexture::SetMagFilter(TextureFilter filter) {
    glTextureParameteri(m_handle, GL_TEXTURE_MAG_FILTER, OpenGL::Util::TextureFilterToGLEnum(filter));
}

void OpenGLTexture::MakeBindlessTextureResident() {
    if (m_handle == 0 || glIsTexture(m_handle) != GL_TRUE) {
        Logging::Error() << "OpenGLTexture::MakeBindlessTextureResident() failed: handle '" << m_handle << "' is not a valid texture\n";
        m_bindlessID = 0;
        return;
    }

    if (m_bindlessID == 0) {
        m_bindlessID = glGetTextureHandleARB(m_handle);
    }

    if (m_bindlessID == 0) {
        Logging::Error() << "OpenGLTexture::MakeBindlessTextureResident() failed: texture '" << m_handle << "' returned an invalid bindless handle\n";
        return;
    }

    glMakeTextureHandleResidentARB(m_bindlessID);
}

void OpenGLTexture::MakeBindlessTextureNonResident() {
    if (m_bindlessID != 0) {
        glMakeTextureHandleNonResidentARB(m_bindlessID);
        m_bindlessID = 0;
    }
}
