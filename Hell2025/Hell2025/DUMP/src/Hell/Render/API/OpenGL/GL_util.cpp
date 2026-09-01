#include "GL_util.h"
#include <string>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>

namespace OpenGL::Util {

    // These EXT_texture_sRGB/S3TC enums are not exposed by this project's
    // generated GLAD header, despite being valid OpenGL internal formats.
    constexpr GLenum GL_COMPRESSED_SRGB_S3TC_DXT1 = 0x8C4C;
    constexpr GLenum GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1 = 0x8C4D;
    constexpr GLenum GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3 = 0x8C4E;
    constexpr GLenum GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5 = 0x8C4F;

    GLenum ImageFormatToGLFormat(ImageFormat format) {
        switch (format) {
            case ImageFormat::R8_UNORM:
            case ImageFormat::R16_UNORM:
            case ImageFormat::R16_SFLOAT:
            case ImageFormat::R32_SFLOAT:
            case ImageFormat::BC4_R_UNORM:
                return GL_RED;
            case ImageFormat::RG8_UNORM:
            case ImageFormat::RG16_SFLOAT:
            case ImageFormat::RG32_SFLOAT:
            case ImageFormat::BC5_RG_UNORM:
                return GL_RG;
            case ImageFormat::RGB8_UNORM:
            case ImageFormat::RGB8_SRGB:
            case ImageFormat::RGB16_SFLOAT:
            case ImageFormat::RGB32_SFLOAT:
            case ImageFormat::BC1_RGB_UNORM:
            case ImageFormat::BC1_RGB_SRGB:
            case ImageFormat::BC6H_RGB_UFLOAT:
            case ImageFormat::BC6H_RGB_SFLOAT:
                return GL_RGB;
            case ImageFormat::RGBA8_UNORM:
            case ImageFormat::RGBA8_SRGB:
            case ImageFormat::RGBA16_SFLOAT:
            case ImageFormat::RGBA32_SFLOAT:
            case ImageFormat::BC1_RGBA_UNORM:
            case ImageFormat::BC1_RGBA_SRGB:
            case ImageFormat::BC2_RGBA_UNORM:
            case ImageFormat::BC2_RGBA_SRGB:
            case ImageFormat::BC3_RGBA_UNORM:
            case ImageFormat::BC3_RGBA_SRGB:
            case ImageFormat::BC7_RGBA_UNORM:
            case ImageFormat::BC7_RGBA_SRGB:
                return GL_RGBA;
            default:
                return GL_NONE;
        }
    }

    GLenum ImageFormatToGLInternalFormat(ImageFormat format) {
        switch (format) {
            case ImageFormat::R8_UNORM: return GL_R8;
            case ImageFormat::RG8_UNORM: return GL_RG8;
            case ImageFormat::RGB8_UNORM: return GL_RGB8;
            case ImageFormat::RGBA8_UNORM: return GL_RGBA8;
            case ImageFormat::RGB8_SRGB: return GL_SRGB8;
            case ImageFormat::RGBA8_SRGB: return GL_SRGB8_ALPHA8;
            case ImageFormat::R16_UNORM: return GL_R16;
            case ImageFormat::R16_SFLOAT: return GL_R16F;
            case ImageFormat::RG16_SFLOAT: return GL_RG16F;
            case ImageFormat::RGB16_SFLOAT: return GL_RGB16F;
            case ImageFormat::RGBA16_SFLOAT: return GL_RGBA16F;
            case ImageFormat::R32_SFLOAT: return GL_R32F;
            case ImageFormat::RG32_SFLOAT: return GL_RG32F;
            case ImageFormat::RGB32_SFLOAT: return GL_RGB32F;
            case ImageFormat::RGBA32_SFLOAT: return GL_RGBA32F;
            case ImageFormat::BC1_RGB_UNORM: return GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
            case ImageFormat::BC1_RGBA_UNORM: return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
            case ImageFormat::BC1_RGB_SRGB: return GL_COMPRESSED_SRGB_S3TC_DXT1;
            case ImageFormat::BC1_RGBA_SRGB: return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1;
            case ImageFormat::BC2_RGBA_UNORM: return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
            case ImageFormat::BC2_RGBA_SRGB: return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3;
            case ImageFormat::BC3_RGBA_UNORM: return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
            case ImageFormat::BC3_RGBA_SRGB: return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5;
            case ImageFormat::BC4_R_UNORM: return GL_COMPRESSED_RED_RGTC1;
            case ImageFormat::BC5_RG_UNORM: return GL_COMPRESSED_RG_RGTC2;
            case ImageFormat::BC6H_RGB_UFLOAT: return GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT;
            case ImageFormat::BC6H_RGB_SFLOAT: return GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT;
            case ImageFormat::BC7_RGBA_UNORM: return GL_COMPRESSED_RGBA_BPTC_UNORM;
            case ImageFormat::BC7_RGBA_SRGB: return GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM;
            default: return GL_NONE;
        }
    }

    GLenum ImageFormatToGLDataType(ImageFormat format) {
        switch (format) {
            case ImageFormat::R16_UNORM:
                return GL_UNSIGNED_SHORT;
            case ImageFormat::R16_SFLOAT:
            case ImageFormat::RG16_SFLOAT:
            case ImageFormat::RGB16_SFLOAT:
            case ImageFormat::RGBA16_SFLOAT:
                return GL_HALF_FLOAT;
            case ImageFormat::RGB32_SFLOAT:
            case ImageFormat::R32_SFLOAT:
            case ImageFormat::RG32_SFLOAT:
            case ImageFormat::RGBA32_SFLOAT:
                return GL_FLOAT;
            default:
                return GL_UNSIGNED_BYTE;
        }
    }
    bool ExtensionExists(const std::string& extensionName) {
        static std::vector<std::string> extensionsCache;
        if (extensionsCache.empty()) {
            GLint numExtensions;
            glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);
            for (GLint i = 0; i < numExtensions; ++i) {
                const char* extension = (const char*)glGetStringi(GL_EXTENSIONS, i);
                extensionsCache.push_back(extension);
            }
        }
        return std::find(extensionsCache.begin(), extensionsCache.end(), extensionName) != extensionsCache.end();
    }

    GLint GetFormatFromChannelCount(int channelCount) {
        switch (channelCount) {
        case 4:  return GL_RGBA;
        case 3:  return GL_RGB;
        case 1:  return GL_RED;
        default:
            std::cout << "Unsupported channel count: " << channelCount << "\n";
            return -1;
        }
    }

    GLint GetInternalFormatFromChannelCount(int channelCount) {
        switch (channelCount) {
        case 4:  return GL_RGBA8;
        case 3:  return GL_RGB8;
        case 1:  return GL_R8;
        default:
            std::cout << "Unsupported channel count: " << channelCount << "\n";
            return -1;
        }
    }

    const char* GetGLSyncStatusString(GLenum result) {
        switch (result) {
        case GL_ALREADY_SIGNALED: return "GL_ALREADY_SIGNALED";
        case GL_CONDITION_SATISFIED: return "GL_CONDITION_SATISFIED";
        case GL_TIMEOUT_EXPIRED: return "GL_TIMEOUT_EXPIRED";
        case GL_WAIT_FAILED: return "GL_WAIT_FAILED";
        default: return "UNKNOWN_GL_SYNC_STATUS";
        }
    }

    const char* GLFormatToString(GLenum format) {
        switch (format) {
        case GL_RED: return "GL_RED";
        case GL_RG: return "GL_RG";
        case GL_RGB: return "GL_RGB";
        case GL_BGR: return "GL_BGR";
        case GL_RGBA: return "GL_RGBA";
        case GL_BGRA: return "GL_BGRA";
        default: return "Unknown Format";
        }
    }

    const char* GLInternalFormatToString(GLenum internalFormat) {
        switch (internalFormat) {
        case GL_R8: return "GL_R8";
        case GL_RG8: return "GL_RG8";
        case GL_RGB8: return "GL_RGB8";
        case GL_RGBA8: return "GL_RGBA8";
        case GL_R16F: return "GL_R16F";
        case GL_RG16F: return "GL_RG16F";
        case GL_RGB16F: return "GL_RGB16F";
        case GL_RGBA16F: return "GL_RGBA16F";
        case GL_R11F_G11F_B10F: return "GL_R11F_G11F_B10F";
        default: return "Unknown Internal Format";
        }
    }

    const char* GLDataTypeToString(GLenum dataType) {
        switch (dataType) {
        case GL_UNSIGNED_BYTE: return "GL_UNSIGNED_BYTE";
        case GL_BYTE: return "GL_BYTE";
        case GL_UNSIGNED_SHORT: return "GL_UNSIGNED_SHORT";
        case GL_SHORT: return "GL_SHORT";
        case GL_UNSIGNED_INT: return "GL_UNSIGNED_INT";
        case GL_INT: return "GL_INT";
        case GL_HALF_FLOAT: return "GL_HALF_FLOAT";
        case GL_FLOAT: return "GL_FLOAT";
        case GL_DOUBLE: return "GL_DOUBLE";
        default: return "Unknown Data Type";
        }
    }

    GLint GetChannelCountFromFormat(GLenum format) {
        switch (format) {
        case GL_RED: return 1;
        case GL_RG: return 2;
        case GL_RGB: return 3;
        case GL_RGBA: return 4;
        default: return -1;
        }
    }

    size_t CalculateCompressedDataSize(GLenum format, int width, int height) {
        int blockSize = (format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT || format == GL_COMPRESSED_RGB_S3TC_DXT1_EXT) ? 8 : 16;
        int blocksWide = std::max(1, (width + 3) / 4);
        int blocksHigh = std::max(1, (height + 3) / 4);
        return blocksWide * blocksHigh * blockSize;
    }

    size_t GetBytesPerPixel(GLenum internalFormat) {
        switch (internalFormat) {
            case GL_R8:
            case GL_R8_SNORM:
            case GL_R8UI:
            case GL_R8I:
                return 1;

            case GL_RG8:
            case GL_RG8_SNORM:
            case GL_RG8UI:
            case GL_RG8I:
            case GL_R16:
            case GL_R16_SNORM:
            case GL_R16F:
            case GL_R16UI:
            case GL_R16I:
            case GL_DEPTH_COMPONENT16:
                return 2;

            case GL_RGB8:
            case GL_RGB8_SNORM:
            case GL_RGB8UI:
            case GL_RGB8I:
            case GL_SRGB8:
            case GL_DEPTH_COMPONENT24:
                return 3;

            case GL_RGBA8:
            case GL_RGBA8_SNORM:
            case GL_RGBA8UI:
            case GL_RGBA8I:
            case GL_SRGB8_ALPHA8:
            case GL_RG16:
            case GL_RG16_SNORM:
            case GL_RG16F:
            case GL_RG16UI:
            case GL_RG16I:
            case GL_R32F:
            case GL_R32UI:
            case GL_R32I:
            case GL_R11F_G11F_B10F:
            case GL_RGB10_A2:
            case GL_RGB10_A2UI:
            case GL_DEPTH24_STENCIL8:
                return 4;

            case GL_DEPTH32F_STENCIL8:
                return 5;

            case GL_RGB16:
            case GL_RGB16_SNORM:
            case GL_RGB16F:
            case GL_RGB16UI:
            case GL_RGB16I:
                return 6;

            case GL_RGBA16:
            case GL_RGBA16_SNORM:
            case GL_RGBA16F:
            case GL_RGBA16UI:
            case GL_RGBA16I:
            case GL_RG32F:
            case GL_RG32UI:
            case GL_RG32I:
                return 8;

            case GL_RGB32F:
            case GL_RGB32UI:
            case GL_RGB32I:
                return 12;

            case GL_RGBA32F:
            case GL_RGBA32UI:
            case GL_RGBA32I:
                return 16;

            case GL_DEPTH_COMPONENT32F:
                return 4;

            default:
                return 0;
        }
    }

    size_t GetCompressedBlockSize(GLenum internalFormat) {
        switch (internalFormat) {
            case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
            case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
            case GL_COMPRESSED_SRGB_S3TC_DXT1:
            case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1:
            case GL_COMPRESSED_RED_RGTC1:
                return 8;

            case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
            case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3:
            case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
            case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5:
            case GL_COMPRESSED_RG_RGTC2:
            case GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT:
            case GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT:
            case GL_COMPRESSED_RGBA_BPTC_UNORM:
            case GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM:
                return 16;

            default:
                return 0;
        }
    }

    size_t CalculateTexture2DByteCount(uint32_t width, uint32_t height, GLenum internalFormat, uint32_t mipmapLevelCount, uint32_t sampleCount) {
        if (width == 0 || height == 0 || mipmapLevelCount == 0 || sampleCount == 0) {
            return 0;
        }

        size_t byteCount = 0;
        const size_t bytesPerPixel = GetBytesPerPixel(internalFormat);
        const size_t compressedBlockSize = GetCompressedBlockSize(internalFormat);

        for (uint32_t mipLevel = 0; mipLevel < mipmapLevelCount; mipLevel++) {
            const size_t mipWidth = static_cast<size_t>(std::max(1u, width >> mipLevel));
            const size_t mipHeight = static_cast<size_t>(std::max(1u, height >> mipLevel));

            if (compressedBlockSize > 0) {
                const size_t blockCountX = (mipWidth + 3) / 4;
                const size_t blockCountY = (mipHeight + 3) / 4;
                byteCount += blockCountX * blockCountY * compressedBlockSize * sampleCount;
            }
            else {
                byteCount += mipWidth * mipHeight * bytesPerPixel * sampleCount;
            }
        }

        return byteCount;
    }

    size_t CalculateTexture2DArrayByteCount(uint32_t width, uint32_t height, uint32_t layerCount, GLenum internalFormat, uint32_t mipmapLevelCount) {
        if (layerCount == 0) {
            return 0;
        }

        return CalculateTexture2DByteCount(width, height, internalFormat, mipmapLevelCount) * layerCount;
    }

    size_t CalculateTexture3DByteCount(uint32_t width, uint32_t height, uint32_t depth, GLenum internalFormat, uint32_t mipmapLevelCount) {
        if (width == 0 || height == 0 || depth == 0 || mipmapLevelCount == 0) {
            return 0;
        }

        size_t byteCount = 0;
        const size_t bytesPerPixel = GetBytesPerPixel(internalFormat);
        const size_t compressedBlockSize = GetCompressedBlockSize(internalFormat);

        for (uint32_t mipLevel = 0; mipLevel < mipmapLevelCount; mipLevel++) {
            const size_t mipWidth = static_cast<size_t>(std::max(1u, width >> mipLevel));
            const size_t mipHeight = static_cast<size_t>(std::max(1u, height >> mipLevel));
            const size_t mipDepth = static_cast<size_t>(std::max(1u, depth >> mipLevel));

            if (compressedBlockSize > 0) {
                const size_t blockCountX = (mipWidth + 3) / 4;
                const size_t blockCountY = (mipHeight + 3) / 4;
                byteCount += blockCountX * blockCountY * compressedBlockSize * mipDepth;
            }
            else {
                byteCount += mipWidth * mipHeight * mipDepth * bytesPerPixel;
            }
        }

        return byteCount;
    }

    GLint TextureWrapModeToGLEnum(TextureWrapMode wrapMode) {
        switch (wrapMode) {
        case TextureWrapMode::REPEAT: return GL_REPEAT;
        case TextureWrapMode::MIRRORED_REPEAT: return GL_MIRRORED_REPEAT;
        case TextureWrapMode::CLAMP_TO_EDGE: return GL_CLAMP_TO_EDGE;
        case TextureWrapMode::CLAMP_TO_BORDER: return GL_CLAMP_TO_BORDER;
        default: return GL_NONE;
        }
    }

    GLint TextureFilterToGLEnum(TextureFilter filter) {
        switch (filter) {
        case TextureFilter::NEAREST: return GL_NEAREST;
        case TextureFilter::LINEAR: return GL_LINEAR;
        case TextureFilter::LINEAR_MIPMAP: return GL_LINEAR_MIPMAP_LINEAR;
        default: return GL_NONE;
        }
    }

    GLenum GetTypeForClearArray(GLenum internalFormat) {
        switch (internalFormat) {

            case GL_R8UI: case GL_RG8UI: case GL_RGBA8UI:
            case GL_R16UI: case GL_RG16UI: case GL_RGBA16UI:
            case GL_R32UI: case GL_RG32UI: case GL_RGBA32UI:
                return GL_UNSIGNED_INT;

            case GL_R8I: case GL_RG8I: case GL_RGBA8I:
            case GL_R16I: case GL_RG16I: case GL_RGBA16I:
            case GL_R32I: case GL_RG32I: case GL_RGBA32I:
                return GL_INT;

            default:
                return GL_FLOAT;
        }
    }

    GLenum GLInternalFormatToGLType(GLenum internalFormat) {
        switch (internalFormat) {
            // Integers
            case GL_R8UI:  case GL_RG8UI:  case GL_RGBA8UI:    return GL_UNSIGNED_BYTE;
            case GL_R8I:   case GL_RG8I:   case GL_RGBA8I:     return GL_BYTE;
            case GL_R16UI: case GL_RG16UI: case GL_RGBA16UI:   return GL_UNSIGNED_SHORT;
            case GL_R16I:  case GL_RG16I:  case GL_RGBA16I:    return GL_SHORT;
            case GL_R32UI: case GL_RG32UI: case GL_RGBA32UI:   return GL_UNSIGNED_INT;
            case GL_R32I:  case GL_RG32I:  case GL_RGBA32I:    return GL_INT;
            case GL_RGB10_A2UI:                                return GL_UNSIGNED_INT_2_10_10_10_REV;
			case GL_RGB10_A2:                                  return GL_UNSIGNED_INT_2_10_10_10_REV;

            // Normalized
            case GL_R8: case GL_RG8: case GL_RGBA8:
            case GL_SRGB8: case GL_SRGB8_ALPHA8:               return GL_UNSIGNED_BYTE;
            case GL_R16: case GL_RG16: case GL_RGBA16:         return GL_UNSIGNED_SHORT;

            // Floats
            case GL_R16F: case GL_RG16F: case GL_RGBA16F:      return GL_HALF_FLOAT;
			case GL_R32F: case GL_RG32F: case GL_RGBA32F:      return GL_FLOAT;
            case GL_R11F_G11F_B10F:                            return GL_FLOAT;

            // Depth/stencil
            case GL_DEPTH_COMPONENT16:                         return GL_UNSIGNED_SHORT;
            case GL_DEPTH_COMPONENT24:                         return GL_UNSIGNED_INT;
            case GL_DEPTH_COMPONENT32F:                        return GL_FLOAT;
            case GL_DEPTH24_STENCIL8:                          return GL_UNSIGNED_INT_24_8;
            case GL_DEPTH32F_STENCIL8:                         return GL_FLOAT_32_UNSIGNED_INT_24_8_REV;

            default:
            std::cout << "GLInternalFormatToGLType(): unsupported type\n";
            return 0;
        }
    }

    GLenum GLInternalFormatToGLFormat(GLenum internalFormat) {
        switch (internalFormat) {
            // Red channel formats
        case GL_R8:
        case GL_R8_SNORM:
        case GL_R16:
        case GL_R16_SNORM:
        case GL_R16F:
        case GL_R32F:
            return GL_RED;
        case GL_R8UI:
        case GL_R8I:
        case GL_R16UI:
        case GL_R16I:
        case GL_R32UI:
        case GL_R32I:
            return GL_RED_INTEGER;

            // Red-Green channel formats
        case GL_RG8:
        case GL_RG8_SNORM:
        case GL_RG16:
        case GL_RG16_SNORM:
        case GL_RG16F:
        case GL_RG32F:
            return GL_RG;
        case GL_RG8UI:
        case GL_RG8I:
        case GL_RG16UI:
        case GL_RG16I:
        case GL_RG32UI:
        case GL_RG32I:
            return GL_RG_INTEGER;

            // RGB channel formats
        case GL_RGB8:
        case GL_RGB8_SNORM:
        case GL_RGB16:
        case GL_RGB16_SNORM:
        case GL_RGB16F:
        case GL_RGB32F:
        case GL_SRGB8:
        case GL_R11F_G11F_B10F:
        case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
        case GL_COMPRESSED_SRGB_S3TC_DXT1:
        case GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT:
        case GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT:
            return GL_RGB;
        case GL_RGB8UI:
        case GL_RGB8I:
        case GL_RGB16UI:
        case GL_RGB16I:
        case GL_RGB32UI:
        case GL_RGB32I:
            return GL_RGB_INTEGER;

            // RGBA channel formats
        case GL_RGBA8:
        case GL_RGBA8_SNORM:
        case GL_RGBA16:
        case GL_RGBA16_SNORM:
        case GL_RGBA16F:
        case GL_RGBA32F:
        case GL_SRGB8_ALPHA8:
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1:
        case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3:
        case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5:
        case GL_COMPRESSED_RGBA_BPTC_UNORM:
        case GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM:
            return GL_RGBA;
        case GL_RGBA8UI:
        case GL_RGBA8I:
        case GL_RGBA16UI:
        case GL_RGBA16I:
        case GL_RGBA32UI:
        case GL_RGBA32I:
            return GL_RGBA_INTEGER;

            // Special packed formats
        case GL_RGB10_A2:
        case GL_RGB10_A2UI:
            return GL_RGBA;

            // Compressed one/two-channel formats
        case GL_COMPRESSED_RED_RGTC1:
            return GL_RED;
        case GL_COMPRESSED_RG_RGTC2:
            return GL_RG;

            // Depth formats
        case GL_DEPTH_COMPONENT16:
        case GL_DEPTH_COMPONENT24:
        case GL_DEPTH_COMPONENT32F:
            return GL_DEPTH_COMPONENT;

            // Depth-stencil formats
        case GL_DEPTH24_STENCIL8:
        case GL_DEPTH32F_STENCIL8:
            return GL_DEPTH_STENCIL;

        default:
            std::cout << "GLInternalFormatToGLFormat: Unsupported internal format " << internalFormat << "\n";
            return 0;
        }
    }

    GLint GetFormatFromInternalFormat(GLint internalFormat) {
        switch (internalFormat) {
            // R
            case GL_R8:
            case GL_R8_SNORM:
            case GL_R16:
            case GL_R16_SNORM:
            case GL_R16F:
            case GL_R32F:
            return GL_RED;

            case GL_R8UI:
            case GL_R8I:
            case GL_R16UI:
            case GL_R16I:
            case GL_R32UI:
            case GL_R32I:
            return GL_RED_INTEGER;

            // RG
            case GL_RG8:
            case GL_RG8_SNORM:
            case GL_RG16:
            case GL_RG16_SNORM:
            case GL_RG16F:
            case GL_RG32F:
            return GL_RG;

            case GL_RG8UI:
            case GL_RG8I:
            case GL_RG16UI:
            case GL_RG16I:
            case GL_RG32UI:
            case GL_RG32I:
            return GL_RG_INTEGER;

            // RGB
            case GL_RGB8:
            case GL_RGB8_SNORM:
            case GL_RGB16:
            case GL_RGB16_SNORM:
            case GL_RGB16F:
            case GL_RGB32F:
            case GL_SRGB8:
            case GL_R11F_G11F_B10F:
            case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
            case GL_COMPRESSED_SRGB_S3TC_DXT1:
            case GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT:
            case GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT:
            return GL_RGB;

            case GL_RGB8UI:
            case GL_RGB8I:
            case GL_RGB16UI:
            case GL_RGB16I:
            case GL_RGB32UI:
            case GL_RGB32I:
            return GL_RGB_INTEGER;

            // RGBA
            case GL_RGBA8:
            case GL_RGBA8_SNORM:
            case GL_RGBA16:
            case GL_RGBA16_SNORM:
            case GL_RGBA16F:
            case GL_RGBA32F:
            case GL_SRGB8_ALPHA8:
            case GL_RGB10_A2: // packed but uploads as RGBA
            case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
            case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1:
            case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
            case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3:
            case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
            case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5:
            case GL_COMPRESSED_RGBA_BPTC_UNORM:
            case GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM:
            return GL_RGBA;

            case GL_RGBA8UI:
            case GL_RGBA8I:
            case GL_RGBA16UI:
            case GL_RGBA16I:
            case GL_RGBA32UI:
            case GL_RGBA32I:
            case GL_RGB10_A2UI:
            return GL_RGBA_INTEGER;

            // Compressed one/two-channel formats
            case GL_COMPRESSED_RED_RGTC1:
            return GL_RED;

            case GL_COMPRESSED_RG_RGTC2:
            return GL_RG;

            // Depth / stencil
            case GL_DEPTH_COMPONENT16:
            case GL_DEPTH_COMPONENT24:
            case GL_DEPTH_COMPONENT32F:
            return GL_DEPTH_COMPONENT;

            case GL_DEPTH24_STENCIL8:
            case GL_DEPTH32F_STENCIL8:
            return GL_DEPTH_STENCIL;

            default:
            std::cout << "GetFormatFromInternalFormat: unsupported internal format " << internalFormat << "\n";
            return -1;
        }
    }
}
