#pragma once
#include <glad/gl.h>

#include "Hell/Render/TextureTypes.h"

#include <cstddef>
#include <cstdint>
#include <string>

namespace OpenGL::Util {
    bool ExtensionExists(const std::string& extensionName);
    GLenum ImageFormatToGLFormat(ImageFormat format);
    GLenum ImageFormatToGLInternalFormat(ImageFormat format);
    GLenum ImageFormatToGLDataType(ImageFormat format);
    GLint GetFormatFromChannelCount(int channelCount);
    GLint GetInternalFormatFromChannelCount(int channelCount);
    const char* GetGLSyncStatusString(GLenum result);
    const char* GLFormatToString(GLenum format);
    const char* GLInternalFormatToString(GLenum internalFormat);
    const char* GLDataTypeToString(GLenum dataType);
    GLint GetChannelCountFromFormat(GLenum format);
    size_t CalculateCompressedDataSize(GLenum format, int width, int height);
    size_t GetBytesPerPixel(GLenum internalFormat);
    size_t GetCompressedBlockSize(GLenum internalFormat);
    size_t CalculateTexture2DByteCount(uint32_t width, uint32_t height, GLenum internalFormat, uint32_t mipmapLevelCount = 1, uint32_t sampleCount = 1);
    size_t CalculateTexture2DArrayByteCount(uint32_t width, uint32_t height, uint32_t layerCount, GLenum internalFormat, uint32_t mipmapLevelCount = 1);
    size_t CalculateTexture3DByteCount(uint32_t width, uint32_t height, uint32_t depth, GLenum internalFormat, uint32_t mipmapLevelCount = 1);
    GLint TextureWrapModeToGLEnum(TextureWrapMode wrapMode);
    GLint TextureFilterToGLEnum(TextureFilter filter);
    GLenum GLInternalFormatToGLType(GLenum internalFormat);
    GLenum GLInternalFormatToGLFormat(GLenum internalFormat);
    GLint GetFormatFromInternalFormat(GLint internalFormat);
}
