#pragma once

#include "Hell/Render/TextureTypes.h"

#include <glad/gl.h>
#include <glm/vec4.hpp>

#include <string>
#include <memory>
#include <cstddef>

struct OpenGLTexture {
    OpenGLTexture() = default;
    GLuint& GetHandle();
    GLuint64 GetBindlessID();
    void Create(int width, int height, int internalFormat, int mipmapLevelCount);
    void ClearR(float value);
    void UploadData(const float* data);
    void UploadR16FData(const float* data, int width, int height, int xOffset, int yOffset, int mipLevel);
    void Reset();
    void SetBorderColor(float r, float g, float b, float a);
    void SetBorderColor(const glm::vec4& color);
    void SetWrapMode(TextureWrapMode wrapMode);
    void SetWrapModeS(TextureWrapMode wrapMode);
    void SetWrapModeT(TextureWrapMode wrapMode);
    void SetMinFilter(TextureFilter filter);
    void SetMagFilter(TextureFilter filter);
    void MakeBindlessTextureResident();
    void MakeBindlessTextureNonResident();
    int GetWidth();
    int GetHeight();
    int GetChannelCount();
    int GetDataSize();
    size_t GetAllocatedByteCount() const;
    void* GetData();
    GLint GetFormat();
    GLint GetInternalFormat();
    GLint GetMipmapLevelCount();

private:
    GLuint m_handle = 0;
    GLuint64 m_bindlessID = 0;
    int m_width = 0;
    int m_height = 0;
    int m_channelCount = 0;
    GLsizei m_dataSize = 0;
    void* m_data = nullptr;
    GLint m_format = 0;
    GLint m_internalFormat = 0;
    GLint m_mipmapLevelCount = 0;
    ImageDataType m_imageDataType = ImageDataType::UNCOMPRESSED;
};
