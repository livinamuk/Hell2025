#include "GL_frameBuffer.h"

#include "Hell/Logging.h"

#include <algorithm>
#include <cmath>

OpenGLFrameBuffer::OpenGLFrameBuffer(const std::string& name, int width, int height, uint32_t sampleCount) {
    Create(name, width, height, sampleCount);
}

OpenGLFrameBuffer::OpenGLFrameBuffer(const std::string& name, const glm::ivec2& size, uint32_t sampleCount) {
    Create(name, size, sampleCount);
}

void OpenGLFrameBuffer::SetName(const std::string& name) {
    m_name = name;
}

void OpenGLFrameBuffer::Create(const glm::ivec2& size, uint32_t sampleCount) {
    Create(size.x, size.y, sampleCount);
}

void OpenGLFrameBuffer::Create(int width, int height, uint32_t sampleCount) {
    const std::string name = m_name;
    CleanUp();

    glCreateFramebuffers(1, &m_handle);
    m_name = name;
    m_width = width;
    m_height = height;
    m_sampleCount = sampleCount;
}

void OpenGLFrameBuffer::Create(const std::string& name, const glm::ivec2& size, uint32_t sampleCount) {
    Create(name, size.x, size.y, sampleCount);
}

void OpenGLFrameBuffer::Create(const std::string& name, int width, int height, uint32_t sampleCount) {
    SetName(name);
    Create(width, height, sampleCount);
}

void OpenGLFrameBuffer::Bind() {
    glBindFramebuffer(GL_FRAMEBUFFER, m_handle);
}

void OpenGLFrameBuffer::SetViewport() {
    glViewport(0, 0, m_width, m_height);
}

void OpenGLFrameBuffer::CleanUp() {
    for (ColorAttachment& colorAttachment : m_colorAttachments) {
        if (colorAttachment.handle != 0) {
            glDeleteTextures(1, &colorAttachment.handle);
        }
    }
    m_colorAttachments.clear();

    if (m_depthAttachment.handle != 0) {
        glDeleteTextures(1, &m_depthAttachment.handle);
    }

    if (m_handle != 0) glDeleteFramebuffers(1, &m_handle);

    m_width = 0;
    m_height = 0;
    m_sampleCount = 0;
    m_handle = 0;
    m_depthAttachment.handle = 0;

    m_cachedAttachmentHandles.clear();;
    m_cachedAttachmentSlots.clear();
}

void OpenGLFrameBuffer::CreateAttachment(const std::string& name, GLenum internalFormat, GLenum minFilter, GLenum magFilter, GLenum wrapFilter, bool allocateMips) {
    GLenum target = IsMultisampled() ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;

    ColorAttachment& colorAttachment = m_colorAttachments.emplace_back();
    colorAttachment.name = name;
    colorAttachment.internalFormat = internalFormat;
    colorAttachment.format = OpenGL::Util::GLInternalFormatToGLFormat(internalFormat);
    colorAttachment.type = OpenGL::Util::GLInternalFormatToGLType(internalFormat);
    colorAttachment.target = target;
    colorAttachment.minFilter = minFilter;
    colorAttachment.magFilter = magFilter;
    colorAttachment.wrapFilter = wrapFilter;
    colorAttachment.allocateMips = allocateMips;

    glCreateTextures(target, 1, &colorAttachment.handle);

    if (IsMultisampled()) {
        glTextureStorage2DMultisample(colorAttachment.handle, m_sampleCount, internalFormat, m_width, m_height, GL_TRUE);
    }
    else {
        int levels = 1;
        if (allocateMips) {
            int maxDim = std::max(m_width, m_height);
            levels = 1 + (int)floor(log2(maxDim));
        }

        glTextureStorage2D(colorAttachment.handle, levels, internalFormat, m_width, m_height);
        glTextureParameteri(colorAttachment.handle, GL_TEXTURE_MIN_FILTER, allocateMips ? GL_LINEAR_MIPMAP_LINEAR : minFilter);
        glTextureParameteri(colorAttachment.handle, GL_TEXTURE_MAG_FILTER, magFilter);
        glTextureParameteri(colorAttachment.handle, GL_TEXTURE_WRAP_S, wrapFilter);
        glTextureParameteri(colorAttachment.handle, GL_TEXTURE_WRAP_T, wrapFilter);
    }

    GLenum attachment = GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(m_colorAttachments.size() - 1);
    glNamedFramebufferTexture(m_handle, attachment, colorAttachment.handle, 0);

    std::string debugLabel = "Texture (FBO: " + m_name + " Tex: " + name + ")";
    glObjectLabel(GL_TEXTURE, colorAttachment.handle, static_cast<GLsizei>(debugLabel.length()), debugLabel.c_str());

}

void OpenGLFrameBuffer::CreateDepthAttachment(GLenum internalFormat, GLenum minFilter, GLenum magFilter, GLint wrap, glm::vec4 borderColor) {
    m_depthAttachment.internalFormat = internalFormat;
    m_depthAttachment.minFilter = minFilter;
    m_depthAttachment.magFilter = magFilter;
    m_depthAttachment.wrapFilter = wrap;
    m_depthAttachment.borderColor = borderColor;

    GLenum target = IsMultisampled() ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
    glCreateTextures(target, 1, &m_depthAttachment.handle);

    if (IsMultisampled()) {
        glTextureStorage2DMultisample(m_depthAttachment.handle, m_sampleCount, internalFormat, m_width, m_height, GL_TRUE);
    }
    else {
        glTextureStorage2D(m_depthAttachment.handle, 1, internalFormat, m_width, m_height);
        glTextureParameteri(m_depthAttachment.handle, GL_TEXTURE_MIN_FILTER, minFilter);
        glTextureParameteri(m_depthAttachment.handle, GL_TEXTURE_MAG_FILTER, magFilter);
        glTextureParameteri(m_depthAttachment.handle, GL_TEXTURE_WRAP_S, wrap);
        glTextureParameteri(m_depthAttachment.handle, GL_TEXTURE_WRAP_T, wrap);
        glTextureParameterfv(m_depthAttachment.handle, GL_TEXTURE_BORDER_COLOR, &borderColor[0]);
    }

    GLenum attachmentPoint = (internalFormat == GL_DEPTH24_STENCIL8 || internalFormat == GL_DEPTH32F_STENCIL8)
        ? GL_DEPTH_STENCIL_ATTACHMENT
        : GL_DEPTH_ATTACHMENT;

    glNamedFramebufferTexture(m_handle, attachmentPoint, m_depthAttachment.handle, 0);

    std::string debugLabel = "Texture (FBO: " + m_name + " Tex: Depth)";
    glObjectLabel(GL_TEXTURE, m_depthAttachment.handle, static_cast<GLsizei>(debugLabel.length()), debugLabel.c_str());

}

void OpenGLFrameBuffer::BindDepthAttachmentFrom(const OpenGLFrameBuffer& srcFrameBuffer) {
    GLenum attach = (srcFrameBuffer.m_depthAttachment.internalFormat == GL_DEPTH24_STENCIL8 ||
        srcFrameBuffer.m_depthAttachment.internalFormat == GL_DEPTH32F_STENCIL8)
        ? GL_DEPTH_STENCIL_ATTACHMENT
        : GL_DEPTH_ATTACHMENT;

    glNamedFramebufferTexture(m_handle, attach, srcFrameBuffer.m_depthAttachment.handle, 0);
}

void OpenGLFrameBuffer::DrawBuffers(const std::vector<std::string>& attachmentNames) {
    m_drawBuffers.clear();
    for (const std::string& attachmentName : attachmentNames) {
        m_drawBuffers.push_back(GetColorAttachmentSlotByName(attachmentName));
    }
    glDrawBuffers(static_cast<GLsizei>(m_drawBuffers.size()), m_drawBuffers.data());
}

void OpenGLFrameBuffer::DrawBuffer(GLenum buffer) {
    m_drawBuffers = { buffer };
    glDrawBuffer(buffer);
}

void OpenGLFrameBuffer::DrawBuffer(const std::string& attachmentName) {
    DrawBuffer(GetColorAttachmentSlotByName(attachmentName));
}

void OpenGLFrameBuffer::ClearTexImage(const std::string& attachmentName, GLfloat r, GLfloat g, GLfloat b, GLfloat a) {
    int index = -1;
    for (int i = 0; i < m_colorAttachments.size(); i++) {
        if (attachmentName == m_colorAttachments[i].name) {
            index = i;
            break;
        }
    }
    if (index >= 0) {
        GLuint tex = m_colorAttachments[index].handle;
        float cc[4] = { r, g, b, a };
        glClearTexImage(tex, 0, GL_RGBA, GL_FLOAT, cc);
    }
    else {
        std::cout << "OpenGLFrameBuffer::ClearTexImage() failed: '" << attachmentName << "' not found!\n";
    }
}

void OpenGLFrameBuffer::ClearAttachment2(const std::string& attachmentName, float r, float g, float b, float a) {
    for (int i = 0; i < (int)m_colorAttachments.size(); i++) {
        if (attachmentName == m_colorAttachments[i].name) {
            float clearColor[4] = { r, g, b, a };
            GLenum drawBuffer = GL_COLOR_ATTACHMENT0 + i;
            glNamedFramebufferDrawBuffers(m_handle, 1, &drawBuffer);
            glClearNamedFramebufferfv(m_handle, GL_COLOR, 0, clearColor);
            return;
        }
    }
}

void OpenGLFrameBuffer::ClearAttachmentR(const std::string& attachmentName, GLfloat r) {
    ClearAttachment(attachmentName, r, 0.0f, 0.0f, 0.0f);
}

void OpenGLFrameBuffer::ClearAttachment(const std::string& attachmentName, float r, float g, float b, float a) {
    for (int i = 0; i < (int)m_colorAttachments.size(); i++) {
        if (attachmentName == m_colorAttachments[i].name) {
            float clearColor[4] = { r, g, b, a };
            GLenum drawBuffer = GL_COLOR_ATTACHMENT0 + i;
            glNamedFramebufferDrawBuffers(m_handle, 1, &drawBuffer);
            glClearNamedFramebufferfv(m_handle, GL_COLOR, 0, clearColor);
            return;
        }
    }
}

void OpenGLFrameBuffer::ClearAttachmentI(const std::string& attachmentName, GLint r, GLint g, GLint b, GLint a) {
    for (int i = 0; i < m_colorAttachments.size(); i++) {
        if (attachmentName == m_colorAttachments[i].name) {
            GLuint texture = m_colorAttachments[i].handle;
            GLenum format = m_colorAttachments[i].format;
            GLenum type = m_colorAttachments[i].type;
            GLint clearColor[4] = { r, g, b, a };
            glClearTexSubImage(texture, 0, 0, 0, 0, GetWidth(), GetHeight(), 1, format, type, clearColor);
            return;
        }
    }
}

void OpenGLFrameBuffer::ClearAttachmentUI(const std::string& attachmentName, GLint r, GLint g, GLint b, GLint a) {
    for (int i = 0; i < m_colorAttachments.size(); i++) {
        if (attachmentName == m_colorAttachments[i].name) {
            GLuint texture = m_colorAttachments[i].handle;
            GLenum format = m_colorAttachments[i].format;
            GLenum type = m_colorAttachments[i].type;
            GLuint clearColor[4] = { r, g, b, a };
            glClearTexSubImage(texture, 0, 0, 0, 0, GetWidth(), GetHeight(), 1, format, type, clearColor);
            return;
        }
    }
}

void OpenGLFrameBuffer::ClearAttachmenSubRegion(const std::string& attachmentName, GLint xOffset, GLint yOffset, GLsizei width, GLsizei height, GLfloat r, GLfloat g, GLfloat b, GLfloat a) {
    for (int i = 0; i < m_colorAttachments.size(); i++) {
        if (attachmentName == m_colorAttachments[i].name) {
            GLuint texture = m_colorAttachments[i].handle;
            GLenum format = m_colorAttachments[i].format;
            GLenum type = m_colorAttachments[i].type;
            GLfloat clearColor[4] = { r, g, b, a };
            glClearTexSubImage(texture, 0, xOffset, yOffset, 0, width, height, 1, format, type, clearColor);
            return;
        }
    }
}

void OpenGLFrameBuffer::ClearAttachmenSubRegionInt(const std::string& attachmentName, GLint xOffset, GLint yOffset, GLsizei width, GLsizei height, GLint r, GLint g, GLint b, GLint a) {
    for (int i = 0; i < m_colorAttachments.size(); i++) {
        if (attachmentName == m_colorAttachments[i].name) {
            GLuint texture = m_colorAttachments[i].handle;
            GLenum format = m_colorAttachments[i].format;
            GLenum type = m_colorAttachments[i].type;
            GLint clearColor[4] = { r, g, b, a };
            glClearTexSubImage(texture, 0, xOffset, yOffset, 0, width, height, 1, format, type, clearColor);
            return;
        }
    }
}

void OpenGLFrameBuffer::ClearAttachmenSubRegionUInt(const std::string& attachmentName, GLint xOffset, GLint yOffset, GLsizei width, GLsizei height, GLuint r, GLuint g, GLuint b, GLuint a) {
    for (int i = 0; i < m_colorAttachments.size(); i++) {
        if (attachmentName == m_colorAttachments[i].name) {
            GLuint texture = m_colorAttachments[i].handle;
            GLenum format = m_colorAttachments[i].format;
            GLenum type = m_colorAttachments[i].type;
            GLuint clearColor[4] = { r, g, b, a };
            glClearTexSubImage(texture, 0, xOffset, yOffset, 0, width, height, 1, format, type, clearColor);
            return;
        }
    }
}

void OpenGLFrameBuffer::ClearDepthAttachment() {
    glClear(GL_DEPTH_BUFFER_BIT);
}

void OpenGLFrameBuffer::ClearDepthAttachment(float value) {
    if (m_depthAttachment.handle == 0) return;

    glClearNamedFramebufferfv(m_handle, GL_DEPTH, 0, &value);
}

void OpenGLFrameBuffer::ClearStencilBits(GLint value) {
    if (m_depthAttachment.handle == 0) return;

    glStencilMask(0xFF);
    glClearNamedFramebufferiv(m_handle, GL_STENCIL, 0, &value);
}

void OpenGLFrameBuffer::Resize(int width, int height) {
    if (width <= 0 || height <= 0) {
        return;
    }

    std::string name = m_name;
    GLuint sampleCount = m_sampleCount;
    std::vector<ColorAttachment> colorAttachments = m_colorAttachments;
    DepthAttachment depthAttachment = m_depthAttachment;

    CleanUp();
    SetName(name);
    Create(width, height, sampleCount);

    for (const ColorAttachment& colorAttachment : colorAttachments) {
        CreateAttachment(colorAttachment.name, colorAttachment.internalFormat, colorAttachment.minFilter, colorAttachment.magFilter, colorAttachment.wrapFilter, colorAttachment.allocateMips);
    }

    if (depthAttachment.internalFormat != GL_NONE) {
        CreateDepthAttachment(depthAttachment.internalFormat, depthAttachment.minFilter, depthAttachment.magFilter, depthAttachment.wrapFilter, depthAttachment.borderColor);
    }
}

GLuint OpenGLFrameBuffer::GetColorAttachmentHandleByName(const std::string& name) {
    auto it = m_cachedAttachmentHandles.find(name);
    if (it != m_cachedAttachmentHandles.end()) {
        return it->second;
    }

    for (int i = 0; i < m_colorAttachments.size(); i++) {
        if (name == m_colorAttachments[i].name) {
            m_cachedAttachmentHandles[name] = m_colorAttachments[i].handle;
            return m_colorAttachments[i].handle;
        }
    }

    Logging::Fatal() << "OpenGLFrameBuffer::GetColorAttachmentHandleByName() with name '" << name << "' failed. Name does not exist in FrameBuffer '" << this->m_name << "'\n";
    return GL_NONE;
}

GLenum OpenGLFrameBuffer::GetColorAttachmentSlotByName(const std::string& name) {
    auto it = m_cachedAttachmentSlots.find(name);
    if (it != m_cachedAttachmentSlots.end()) {
        return it->second;
    }

    for (int i = 0; i < m_colorAttachments.size(); i++) {
        if (name == m_colorAttachments[i].name) {
            GLenum slot = GL_COLOR_ATTACHMENT0 + i;
            m_cachedAttachmentSlots[name] = slot;
            return slot;
        }
    }

    Logging::Fatal() << "OpenGLFrameBuffer::GetColorAttachmentSlotByName() with name '" << name << "' failed. Name does not exist in FrameBuffer '" << this->m_name << "'\n";
    return GL_INVALID_VALUE;
}

void OpenGLFrameBuffer::BlitToDefaultFrameBuffer(const std::string& srcName, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, GetHandle());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glReadBuffer(GetColorAttachmentSlotByName(srcName));
    glDrawBuffer(GL_BACK);
    glBlitFramebuffer(0, 0, GetWidth(), GetHeight(), dstX0, dstY0, dstX1, dstY1, mask, filter);
}

void OpenGLFrameBuffer::SetColorAttachmentMipLevel(const std::string& name, int mipLevel) {
    for (int i = 0; i < m_colorAttachments.size(); i++) {
        if (name == m_colorAttachments[i].name) {
            GLenum attachment = GL_COLOR_ATTACHMENT0 + i;
            glNamedFramebufferTexture(m_handle, attachment, m_colorAttachments[i].handle, mipLevel);
            return;
        }
    }
}

size_t OpenGLFrameBuffer::GetGPUAllocatedByteCount() const {
    size_t byteCount = 0;

    for (const ColorAttachment& colorAttachment : m_colorAttachments) {
        if (colorAttachment.handle == 0) {
            continue;
        }

        uint32_t mipmapLevelCount = 1;
        if (colorAttachment.allocateMips && !IsMultisampled()) {
            const GLuint maxDimension = std::max(m_width, m_height);
            mipmapLevelCount = maxDimension > 0 ? 1 + static_cast<uint32_t>(std::floor(std::log2(maxDimension))) : 0;
        }

        byteCount += OpenGL::Util::CalculateTexture2DByteCount(m_width, m_height, colorAttachment.internalFormat, mipmapLevelCount, m_sampleCount);
    }

    if (m_depthAttachment.handle != 0) {
        byteCount += OpenGL::Util::CalculateTexture2DByteCount(m_width, m_height, m_depthAttachment.internalFormat, 1, m_sampleCount);
    }

    return byteCount;
}
