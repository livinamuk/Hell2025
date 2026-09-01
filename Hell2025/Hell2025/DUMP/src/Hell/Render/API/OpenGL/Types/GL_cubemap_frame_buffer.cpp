#include "GL_cubemap_frame_buffer.h"
#include "Hell/Logging.h"

OpenGLCubemapFrameBuffer::OpenGLCubemapFrameBuffer(const std::string& name, int size) {
    Create(name, size);
}

void OpenGLCubemapFrameBuffer::SetName(const std::string& name) {
    m_name = name;
}

void OpenGLCubemapFrameBuffer::Create(int size) {
    CleanUp();

    m_size = size;
    glCreateFramebuffers(1, &m_handle);
}

void OpenGLCubemapFrameBuffer::Create(const std::string& name, int size) {
    SetName(name);
    Create(size);
}

void OpenGLCubemapFrameBuffer::CleanUp() {
    if (m_colorAttachment.handle != 0) glDeleteTextures(1, &m_colorAttachment.handle);
    if (m_depthAttachment.handle != 0) glDeleteTextures(1, &m_depthAttachment.handle);
    glDeleteFramebuffers(1, &m_handle);
    m_colorAttachment.handle = 0;
    m_depthAttachment.handle = 0;
    m_handle = 0;
    m_size = 0;
}

void OpenGLCubemapFrameBuffer::CreateAttachment(GLenum internalFormat, GLenum minFilter, GLenum magFilter) {
    m_colorAttachment.internalFormat = internalFormat;
    m_colorAttachment.format = OpenGL::Util::GLInternalFormatToGLFormat(internalFormat);
    m_colorAttachment.type = OpenGL::Util::GLInternalFormatToGLType(internalFormat);

    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_colorAttachment.handle);
    glTextureStorage2D(m_colorAttachment.handle, 1, internalFormat, m_size, m_size);

    glTextureParameteri(m_colorAttachment.handle, GL_TEXTURE_MIN_FILTER, minFilter);
    glTextureParameteri(m_colorAttachment.handle, GL_TEXTURE_MAG_FILTER, magFilter);
    glTextureParameteri(m_colorAttachment.handle, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_colorAttachment.handle, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_colorAttachment.handle, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glNamedFramebufferTexture(m_handle, GL_COLOR_ATTACHMENT0, m_colorAttachment.handle, 0);

    std::string debugLabel = "Cubemap (FBO: " + m_name + " Color)";
    glObjectLabel(GL_TEXTURE, m_colorAttachment.handle, static_cast<GLsizei>(debugLabel.length()), debugLabel.c_str());
}

void OpenGLCubemapFrameBuffer::CreateDepthAttachment(GLenum internalFormat) {
    m_depthAttachment.internalFormat = internalFormat;

    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_depthAttachment.handle);
    glTextureStorage2D(m_depthAttachment.handle, 1, internalFormat, m_size, m_size);

    glTextureParameteri(m_depthAttachment.handle, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(m_depthAttachment.handle, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(m_depthAttachment.handle, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_depthAttachment.handle, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_depthAttachment.handle, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glNamedFramebufferTexture(m_handle, GL_DEPTH_ATTACHMENT, m_depthAttachment.handle, 0);

    std::string debugLabel = "Cubemap (FBO: " + m_name + " Depth)";
    glObjectLabel(GL_TEXTURE, m_depthAttachment.handle, static_cast<GLsizei>(debugLabel.length()), debugLabel.c_str());
}

void OpenGLCubemapFrameBuffer::Bind() {
    glBindFramebuffer(GL_FRAMEBUFFER, m_handle);
}

void OpenGLCubemapFrameBuffer::SetViewport() {
    glViewport(0, 0, m_size, m_size);
}

void OpenGLCubemapFrameBuffer::ClearFaceColor(const glm::vec4& color) {
    glClearBufferfv(GL_COLOR, 0, &color[0]);
}

void OpenGLCubemapFrameBuffer::ClearFaceColor(float r, float g, float b, float a) {
    ClearFaceColor(glm::vec4(r, g, b, a));
}
void OpenGLCubemapFrameBuffer::ClearFaceDepth(float depth) {
    glClearBufferfv(GL_DEPTH, 0, &depth);
}

void OpenGLCubemapFrameBuffer::BindFaceByIndex(int32_t faceIndex) {
    if (faceIndex < 0 || faceIndex >= 6) {
        Logging::Error() << "OpenGLCubemapFrameBuffer::BindFaceByIndex(..) for framebuffer '" << m_name << "' failed because index '" << faceIndex << "' is out of range 0 to 5\n";
    }

    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_colorAttachment.handle, 0, faceIndex);
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_depthAttachment.handle, 0, faceIndex);
}

size_t OpenGLCubemapFrameBuffer::GetGPUAllocatedByteCount() const {
    size_t byteCount = 0;

    if (m_colorAttachment.handle != 0) {
        byteCount += OpenGL::Util::CalculateTexture2DArrayByteCount(m_size, m_size, 6, m_colorAttachment.internalFormat);
    }

    if (m_depthAttachment.handle != 0) {
        byteCount += OpenGL::Util::CalculateTexture2DArrayByteCount(m_size, m_size, 6, m_depthAttachment.internalFormat);
    }

    return byteCount;
}
