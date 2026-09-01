#pragma once
#include "glad/gl.h"
#include <cstddef>

struct OpenGLShadowMapArray {
    void Init(unsigned int layerCount, int size, int internalFormat);
    void Bind();
    void SetTextureLayer(unsigned int layerIndex);
    void SetViewport();
    void CleanUp();
    void ClearDepth();
    unsigned int GetHandle()        const { return m_handle; }
    unsigned int GetSize()          const { return m_size; }
    unsigned int GetDepthTexture()  const { return m_depthTexture; }
    unsigned int GetLayerCount()    const { return m_layerCount; }
    size_t GetGPUAllocatedByteCount() const;

private:
    unsigned int m_size = 0;
    unsigned int m_handle = 0;
    unsigned int m_depthTexture = 0;
    unsigned int m_layerCount = 0;
    GLsizei m_mipLevels = 1;
    GLenum m_internalFormat = GL_DEPTH_COMPONENT16;
};
