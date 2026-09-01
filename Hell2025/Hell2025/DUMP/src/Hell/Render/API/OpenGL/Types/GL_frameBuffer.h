#pragma once
#include "GL_attachments.h"
#include "../GL_util.h"
#include "Hell/Math/GLM.h"
#include <glad/gl.h>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>

struct OpenGLFrameBuffer {
    OpenGLFrameBuffer() = default;
    OpenGLFrameBuffer(const std::string& name, int width, int height, uint32_t sampleCount = 1);
    OpenGLFrameBuffer(const std::string& name, const glm::ivec2& resolution, uint32_t sampleCount = 1);

    void SetName(const std::string& name);
    void Create(int width, int height, uint32_t sampleCount = 1);
    void Create(const glm::ivec2& resolution, uint32_t sampleCount = 1);
    void Create(const std::string& name, int width, int height, uint32_t sampleCount = 1);
    void Create(const std::string& name, const glm::ivec2& resolution, uint32_t sampleCount = 1);
    void CleanUp();
    void CreateAttachment(const std::string& name, GLenum internalFormat, GLenum minFilter = GL_LINEAR, GLenum magFilter = GL_LINEAR, GLenum wrapFilter = GL_CLAMP_TO_EDGE, bool allocateMips = false);
    void CreateDepthAttachment(GLenum internalFormat, GLenum minFilter = GL_LINEAR, GLenum magFilter = GL_LINEAR, GLint wrap = GL_CLAMP_TO_EDGE, glm::vec4 borderColor = glm::vec4(1.0f));
    void BindDepthAttachmentFrom(const OpenGLFrameBuffer& srcFrameBuffer);
    void Bind();
    void SetViewport();
    void DrawBuffer(GLenum buffer);
    void DrawBuffer(const std::string& attachmentName);
    void DrawBuffers(const std::vector<std::string>& attachmentNames);
    void ClearAttachment(const std::string& attachmentName, float r, float g, float b, float a);
    void ClearAttachment2(const std::string& attachmentName, float r, float g, float b, float a);
    void ClearAttachmentR(const std::string& attachmentName, GLfloat r);
    void ClearTexImage(const std::string& attachmentName, GLfloat r, GLfloat g, GLfloat b, GLfloat a);
    void ClearAttachmentI(const std::string& attachmentName, GLint r, GLint g = 0, GLint b = 0, GLint a = 0);
    void ClearAttachmentUI(const std::string& attachmentName, GLint r, GLint g = 0, GLint b = 0, GLint a = 0);
    void ClearAttachmenSubRegion(const std::string& attachmentName, GLint xOffset, GLint yOffset, GLsizei width, GLsizei height, GLfloat r, GLfloat g = 0.0f, GLfloat b = 0.0f, GLfloat a = 0.0f);
    void ClearAttachmenSubRegionInt(const std::string& attachmentName, GLint xOffset, GLint yOffset, GLsizei width, GLsizei height, GLint r, GLint g = 0, GLint b = 0, GLint a = 0);
    void ClearAttachmenSubRegionUInt(const std::string& attachmentName, GLint xOffset, GLint yOffset, GLsizei width, GLsizei height, GLuint r, GLuint g = 0, GLuint b = 0, GLuint a = 0);
    void ClearDepthAttachment();
    void ClearDepthAttachment(float value);
    void ClearStencilBits(GLint value);
    void Resize(int width, int height);
    void SetColorAttachmentMipLevel(const std::string& name, int mipLevel);

    GLuint GetColorAttachmentHandleByName(const std::string& name);
    GLenum GetColorAttachmentSlotByName(const std::string& name);
    void BlitToDefaultFrameBuffer(const std::string& srcName, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);

    GLuint GetHandle() const                { return m_handle; }
    GLuint GetWidth() const                 { return m_width; }
    GLuint GetHeight() const                { return m_height; }
    GLuint GetDepthAttachmentHandle() const { return m_depthAttachment.handle; }
    bool IsMultisampled() const             { return m_sampleCount > 1; }
    size_t GetGPUAllocatedByteCount() const;

private:
    std::string m_name = "undefined";
    GLuint m_handle = 0;
    GLuint m_width = 0;
    GLuint m_height = 0;
    GLuint m_sampleCount = 1;
    std::vector<ColorAttachment> m_colorAttachments;
    DepthAttachment m_depthAttachment;
    std::unordered_map<std::string, GLuint> m_cachedAttachmentHandles;
    std::unordered_map<std::string, GLenum> m_cachedAttachmentSlots;
    std::vector<GLenum> m_drawBuffers;
};
