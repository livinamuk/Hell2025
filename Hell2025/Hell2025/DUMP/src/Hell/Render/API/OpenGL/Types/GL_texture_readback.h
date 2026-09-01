#pragma once
#include "Hell/Math/GLM.h"
#include "GL_pbo.hpp"

#include <cstdint>
#include <vector>

struct OpenGLTextureReadBack {
    void IssueDataRequest(unsigned int framebufferHandle, unsigned int attachmentSlot, int offsetX, int offsetY, int width, int height, int internalFormat);
    void Update();
    void Reset();

    glm::vec4 GetFloatPixel(int index);

    int GetWidth()                         { return m_width; }
    int GetHeight()                        { return m_height; }
    bool IsResultReady()                   { return m_resultReady; }
    bool IsRequestInProgress()             { return m_requestInProgress; }
    std::vector<float>& GetFloatData()     { return m_floatData; }
    std::vector<uint16_t>& GetUint16Data() { return m_uint16Data; }

private:
    unsigned int m_framebufferHandle = 0;
    unsigned int m_attachmentSlot = 0;
    int m_offsetX = 0;
    int m_offsetY = 0;
    int m_width = 0;
    int m_height = 0;
    int m_channelCount = 0;
    int m_internalFormat = 0;
    bool m_resultReady = false;
    bool m_requestInProgress = false;
    size_t m_bufferSize = 0;
    PBO m_pbo;
    std::vector<float> m_floatData;
    std::vector<uint16_t> m_uint16Data;
};

struct OpenGLTextureReadBackR8 {
    void IssueDataRequest(GLuint framebufferHandle, GLuint textureHandle, int offsetX, int offsetY, int width, int height);
    void Update();
    void Reset();

    int GetWidth() { return m_width; }
    int GetHeight() { return m_height; }
    bool IsResultReady() { return m_resultReady; }
    bool IsRequestInProgress() { return m_requestInProgress; }
    std::vector<float>& GetData() { return m_data; }

private:
    GLuint m_framebufferHandle = 0;
    GLuint m_attachmentSlot = 0;
    int m_offsetX = 0;
    int m_offsetY = 0;
    int m_width = 0;
    int m_height = 0;
    bool m_resultReady = false;
    bool m_requestInProgress = false;
    size_t m_bufferSize = 0;
    PBO m_pbo;
    std::vector<float> m_data;
};

struct OpenGLTextureReadBackR16F {
    void IssueDataRequest(GLuint framebufferHandle, GLuint textureHandle, int offsetX, int offsetY, int width, int height);
    void Update();
    void Reset();

    int GetWidth() { return m_width; }
    int GetHeight() { return m_height; }
    bool IsResultReady() { return m_resultReady; }
    bool IsRequestInProgress() { return m_requestInProgress; }
    std::vector<float>& GetData() { return m_data; }

private:
    GLuint m_framebufferHandle = 0;
    GLuint m_attachmentSlot = 0;
    int m_offsetX = 0;
    int m_offsetY = 0;
    int m_width = 0;
    int m_height = 0;
    bool m_resultReady = false;
    bool m_requestInProgress = false;
    size_t m_bufferSize = 0;
    PBO m_pbo;
    std::vector<float> m_data;
};
