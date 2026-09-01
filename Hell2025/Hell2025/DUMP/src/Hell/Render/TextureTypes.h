#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

enum class ImageDataType {
    UNCOMPRESSED,
    COMPRESSED,
    EXR,
    UNDEFINED
};

enum class ImageFormat {
    UNDEFINED,

    R8_UNORM,
    RG8_UNORM,
    RGB8_UNORM,
    RGBA8_UNORM,
    RGB8_SRGB,
    RGBA8_SRGB,

    R16_UNORM,
    R16_SFLOAT,
    RG16_SFLOAT,
    RGB16_SFLOAT,
    RGBA16_SFLOAT,
    R32_SFLOAT,
    RG32_SFLOAT,
    RGB32_SFLOAT,
    RGBA32_SFLOAT,

    BC1_RGB_UNORM,
    BC1_RGBA_UNORM,
    BC1_RGB_SRGB,
    BC1_RGBA_SRGB,
    BC2_RGBA_UNORM,
    BC2_RGBA_SRGB,
    BC3_RGBA_UNORM,
    BC3_RGBA_SRGB,
    BC4_R_UNORM,
    BC5_RG_UNORM,
    BC6H_RGB_UFLOAT,
    BC6H_RGB_SFLOAT,
    BC7_RGBA_UNORM,
    BC7_RGBA_SRGB
};

enum class TextureWrapMode {
    REPEAT,
    MIRRORED_REPEAT,
    CLAMP_TO_EDGE,
    CLAMP_TO_BORDER,
    UNDEFINED
};

enum class TextureFilter {
    NEAREST,
    LINEAR,
    LINEAR_MIPMAP,
    UNDEFINED
};

struct TextureMip {
    uint32_t width = 0;
    uint32_t height = 0;
    std::vector<std::byte> data;
};

struct ImageData {
    ImageDataType type = ImageDataType::UNDEFINED;
    ImageFormat format = ImageFormat::UNDEFINED;
    std::vector<TextureMip> mips;
};

inline int GetImageFormatChannelCount(ImageFormat format) {
    switch (format) {
        case ImageFormat::R8_UNORM:
        case ImageFormat::R16_UNORM:
        case ImageFormat::R16_SFLOAT:
        case ImageFormat::R32_SFLOAT:
        case ImageFormat::BC4_R_UNORM:
            return 1;
        case ImageFormat::RG8_UNORM:
        case ImageFormat::RG16_SFLOAT:
        case ImageFormat::RG32_SFLOAT:
        case ImageFormat::BC5_RG_UNORM:
            return 2;
        case ImageFormat::RGB8_UNORM:
        case ImageFormat::RGB8_SRGB:
        case ImageFormat::RGB16_SFLOAT:
        case ImageFormat::RGB32_SFLOAT:
        case ImageFormat::BC1_RGB_UNORM:
        case ImageFormat::BC1_RGB_SRGB:
        case ImageFormat::BC6H_RGB_UFLOAT:
        case ImageFormat::BC6H_RGB_SFLOAT:
            return 3;
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
            return 4;
        default:
            return 0;
    }
}

inline bool IsCompressedImageFormat(ImageFormat format) {
    switch (format) {
        case ImageFormat::BC1_RGB_UNORM:
        case ImageFormat::BC1_RGBA_UNORM:
        case ImageFormat::BC1_RGB_SRGB:
        case ImageFormat::BC1_RGBA_SRGB:
        case ImageFormat::BC2_RGBA_UNORM:
        case ImageFormat::BC2_RGBA_SRGB:
        case ImageFormat::BC3_RGBA_UNORM:
        case ImageFormat::BC3_RGBA_SRGB:
        case ImageFormat::BC4_R_UNORM:
        case ImageFormat::BC5_RG_UNORM:
        case ImageFormat::BC6H_RGB_UFLOAT:
        case ImageFormat::BC6H_RGB_SFLOAT:
        case ImageFormat::BC7_RGBA_UNORM:
        case ImageFormat::BC7_RGBA_SRGB:
            return true;
        default:
            return false;
    }
}

inline const char* ImageFormatToString(ImageFormat format) {
    switch (format) {
        case ImageFormat::R8_UNORM: return "R8_UNORM";
        case ImageFormat::RG8_UNORM: return "RG8_UNORM";
        case ImageFormat::RGB8_UNORM: return "RGB8_UNORM";
        case ImageFormat::RGBA8_UNORM: return "RGBA8_UNORM";
        case ImageFormat::RGB8_SRGB: return "RGB8_SRGB";
        case ImageFormat::RGBA8_SRGB: return "RGBA8_SRGB";
        case ImageFormat::R16_UNORM: return "R16_UNORM";
        case ImageFormat::R16_SFLOAT: return "R16_SFLOAT";
        case ImageFormat::RG16_SFLOAT: return "RG16_SFLOAT";
        case ImageFormat::RGB16_SFLOAT: return "RGB16_SFLOAT";
        case ImageFormat::RGBA16_SFLOAT: return "RGBA16_SFLOAT";
        case ImageFormat::R32_SFLOAT: return "R32_SFLOAT";
        case ImageFormat::RG32_SFLOAT: return "RG32_SFLOAT";
        case ImageFormat::RGB32_SFLOAT: return "RGB32_SFLOAT";
        case ImageFormat::RGBA32_SFLOAT: return "RGBA32_SFLOAT";
        case ImageFormat::BC1_RGB_UNORM: return "BC1_RGB_UNORM";
        case ImageFormat::BC1_RGBA_UNORM: return "BC1_RGBA_UNORM";
        case ImageFormat::BC1_RGB_SRGB: return "BC1_RGB_SRGB";
        case ImageFormat::BC1_RGBA_SRGB: return "BC1_RGBA_SRGB";
        case ImageFormat::BC2_RGBA_UNORM: return "BC2_RGBA_UNORM";
        case ImageFormat::BC2_RGBA_SRGB: return "BC2_RGBA_SRGB";
        case ImageFormat::BC3_RGBA_UNORM: return "BC3_RGBA_UNORM";
        case ImageFormat::BC3_RGBA_SRGB: return "BC3_RGBA_SRGB";
        case ImageFormat::BC4_R_UNORM: return "BC4_R_UNORM";
        case ImageFormat::BC5_RG_UNORM: return "BC5_RG_UNORM";
        case ImageFormat::BC6H_RGB_UFLOAT: return "BC6H_RGB_UFLOAT";
        case ImageFormat::BC6H_RGB_SFLOAT: return "BC6H_RGB_SFLOAT";
        case ImageFormat::BC7_RGBA_UNORM: return "BC7_RGBA_UNORM";
        case ImageFormat::BC7_RGBA_SRGB: return "BC7_RGBA_SRGB";
        default: return "UNDEFINED";
    }
}
