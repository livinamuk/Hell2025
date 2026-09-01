#include "GL_shadow_map.h"
#include "Hell/Render/API/OpenGL/GL_util.h"
#include <iostream>

OpenGLShadowMap::OpenGLShadowMap(const std::string& name, int width, int height, int layers) {
    m_name = name;
    m_width = width;
    m_height = height;
    m_layers = layers;

    glCreateFramebuffers(1, &m_handle);
    glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &m_depthTexture);

    glTextureStorage3D(m_depthTexture, 1, GL_DEPTH_COMPONENT32F, m_width, m_height, m_layers);
    glTextureParameteri(m_depthTexture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(m_depthTexture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(m_depthTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTextureParameteri(m_depthTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTextureParameteri(m_depthTexture, GL_TEXTURE_COMPARE_MODE, GL_NONE);
    glTextureParameteri(m_depthTexture, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

    glNamedFramebufferTexture(m_handle, GL_DEPTH_ATTACHMENT, m_depthTexture, 0);

    std::string debugLabel = "ShadowMap: " + m_name;
    glObjectLabel(GL_FRAMEBUFFER, m_handle, static_cast<GLsizei>(debugLabel.length()), debugLabel.c_str());

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "ERROR: ShadowMap framebuffer '" << m_name << "' is incomplete\n";
    }
}

void OpenGLShadowMap::CleanUp() {
    glDeleteTextures(1, &m_depthTexture);
    glDeleteFramebuffers(1, &m_handle);
    m_handle = 0;
    m_depthTexture = 0;
    m_width = 0;
    m_height = 0;
    m_layers = 1;
}

void OpenGLShadowMap::Bind() {
    glBindFramebuffer(GL_FRAMEBUFFER, m_handle);
}

void OpenGLShadowMap::BindLayer(int layerIndex) {
    //glBindFramebuffer(GL_FRAMEBUFFER, m_handle);
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_depthTexture, 0, layerIndex);
    //glNamedFramebufferTextureLayer(m_handle, GL_DEPTH_ATTACHMENT, m_depthTexture, 0, layerIndex);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
}

void OpenGLShadowMap::ClearLayer(int layerIndex) {
    //glBindFramebuffer(GL_FRAMEBUFFER, m_handle);
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_depthTexture, 0, layerIndex);
    GLfloat clearDepth = 1.0f;
    glClearNamedFramebufferfv(m_handle, GL_DEPTH, 0, &clearDepth);
}

void OpenGLShadowMap::Unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void OpenGLShadowMap::Resize(int width, int height, int layers) {
    if (width == m_width && height == m_height && layers == m_layers) return;

    m_width = width;
    m_height = height;
    m_layers = layers;

    glDeleteTextures(1, &m_depthTexture);
    glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &m_depthTexture);

    glTextureStorage3D(m_depthTexture, 1, GL_DEPTH_COMPONENT32F, m_width, m_height, m_layers);
    glTextureParameteri(m_depthTexture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(m_depthTexture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(m_depthTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_depthTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_depthTexture, GL_TEXTURE_COMPARE_MODE, GL_NONE);
    glTextureParameteri(m_depthTexture, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

    glNamedFramebufferTexture(m_handle, GL_DEPTH_ATTACHMENT, m_depthTexture, 0);
}

//void OpenGLShadowMap::ClearDepth() {
//    glClear(GL_DEPTH_BUFFER_BIT);
//}

GLuint OpenGLShadowMap::GetDepthTextureHandle() const {
    return m_depthTexture;
}

GLuint OpenGLShadowMap::GetFramebuffer() const {
    return m_handle;
}

void OpenGLShadowMap::SetViewport() const {
    glViewport(0, 0, m_width, m_height);
}

size_t OpenGLShadowMap::GetGPUAllocatedByteCount() const {
    if (m_depthTexture == 0) {
        return 0;
    }

    return OpenGL::Util::CalculateTexture2DArrayByteCount(m_width, m_height, m_layers, GL_DEPTH_COMPONENT32F);
}
