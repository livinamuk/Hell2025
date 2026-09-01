#include "GL_shadow_cube_map_array.h"
#include "Hell/Render/API/OpenGL/GL_util.h"
#include <glad/gl.h>
#include <iostream>

void OpenGLShadowCubeMapArray::Init(unsigned int numberOfCubemaps, int size) {
    m_numberOfCubemaps = numberOfCubemaps;
    m_size = size;
    m_internalFormat = GL_DEPTH_COMPONENT16;
    m_mipLevels = 1;

    glGenFramebuffers(1, &m_handle);
    glGenTextures(1, &m_depthTexture);
    glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, m_depthTexture);

    glTexStorage3D(GL_TEXTURE_CUBE_MAP_ARRAY, m_mipLevels, m_internalFormat, size, size, 6 * m_numberOfCubemaps);

    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glBindFramebuffer(GL_FRAMEBUFFER, m_handle);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_depthTexture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "OpenGLShadowCubeMapArray::Init(): Framebuffer not complete!\n";
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glGenTextures(1, &m_textureView);
    glTextureView( m_textureView, GL_TEXTURE_2D_ARRAY, m_depthTexture, m_internalFormat, 0, m_mipLevels, 0, 6 * m_numberOfCubemaps);

}

void OpenGLShadowCubeMapArray::CleanUp() {
    glDeleteTextures(1, &m_depthTexture);
    glDeleteTextures(1, &m_textureView);
    glDeleteFramebuffers(1, &m_handle);
    m_size = 0;
    m_handle = 0;
    m_depthTexture = 0;
    m_numberOfCubemaps = 0;
    m_textureView = 0;
    m_mipLevels = 1;
    m_internalFormat = GL_DEPTH_COMPONENT16;
}

void OpenGLShadowCubeMapArray::ClearDepthLayer(int layerIndex, float clearValue) {
    if (layerIndex < 0 || layerIndex >= static_cast<int>(m_numberOfCubemaps)) return;
    glClearTexSubImage(m_depthTexture, 0, 0, 0, layerIndex * 6, m_size, m_size, 6, GL_DEPTH_COMPONENT, GL_FLOAT, &clearValue);
}

void OpenGLShadowCubeMapArray::ClearDepthLayers(float clearValue) {
    glClearTexImage(m_depthTexture, 0, GL_DEPTH_COMPONENT, GL_FLOAT, &clearValue);
}

size_t OpenGLShadowCubeMapArray::GetGPUAllocatedByteCount() const {
    if (m_depthTexture == 0) {
        return 0;
    }

    return OpenGL::Util::CalculateTexture2DArrayByteCount(m_size, m_size, 6 * m_numberOfCubemaps, m_internalFormat, m_mipLevels);
}
