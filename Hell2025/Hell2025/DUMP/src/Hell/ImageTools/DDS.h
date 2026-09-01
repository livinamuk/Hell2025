#pragma once

#include "Hell/Render/TextureTypes.h"

#include <cstdint>
#include <optional>

namespace Hell::ImageTools::DDS {

    inline constexpr uint32_t Magic = 0x20534444; // "DDS "
    inline constexpr uint32_t HeaderSize = 124;
    inline constexpr uint32_t PixelFormatSize = 32;
    inline constexpr uint32_t FourCCDxt1 = 0x31545844; // "DXT1"
    inline constexpr uint32_t FourCCDxt3 = 0x33545844; // "DXT3"
    inline constexpr uint32_t FourCCDxt5 = 0x35545844; // "DXT5"
    inline constexpr uint32_t FourCCDx10 = 0x30315844; // "DX10"
    inline constexpr uint32_t FourCCAti1 = 0x31495441; // "ATI1"
    inline constexpr uint32_t FourCCAti2 = 0x32495441; // "ATI2"
    inline constexpr uint32_t FourCCBc4U = 0x55344342; // "BC4U"
    inline constexpr uint32_t FourCCBc5U = 0x55354342; // "BC5U"

    inline constexpr uint32_t PixelFormatFlagAlphaPixels = 0x00000001;
    inline constexpr uint32_t PixelFormatFlagRGB = 0x00000040;

    inline constexpr uint32_t Caps2Cubemap = 0x00000200;
    inline constexpr uint32_t Caps2Volume = 0x00200000;
    inline constexpr uint32_t ResourceDimensionTexture2D = 3;
    inline constexpr uint32_t ResourceMiscTextureCube = 0x4;

    struct Header {
        uint32_t magic;
        uint32_t size;
        uint32_t flags;
        uint32_t height;
        uint32_t width;
        uint32_t pitchOrLinearSize;
        uint32_t depth;
        uint32_t mipMapCount;
        uint32_t reserved1[11];
        uint32_t pixelFormatSize;
        uint32_t pixelFormatFlags;
        uint32_t fourCC;
        uint32_t rgbBitCount;
        uint32_t rBitMask;
        uint32_t gBitMask;
        uint32_t bBitMask;
        uint32_t aBitMask;
        uint32_t caps;
        uint32_t caps2;
        uint32_t caps3;
        uint32_t caps4;
        uint32_t reserved2;
    };

    struct HeaderDX10 {
        uint32_t dxgiFormat;
        uint32_t resourceDimension;
        uint32_t miscFlag;
        uint32_t arraySize;
        uint32_t miscFlags2;
    };

    static_assert(sizeof(Header) == 128);
    static_assert(sizeof(HeaderDX10) == 20);

    struct FormatInfo {
        ImageFormat format = ImageFormat::UNDEFINED;
        uint32_t blockSize = 0;
        uint32_t blockWidth = 4;
        uint32_t blockHeight = 4;
    };

    inline std::optional<FormatInfo> GetFormatInfo(const Header& header, const HeaderDX10* dx10Header) {
        if (header.fourCC == FourCCDxt1) {
            // Legacy DXT1 does not reliably advertise punch-through alpha in its pixel masks.
            return FormatInfo{ ImageFormat::BC1_RGBA_UNORM, 8 };
        }
        if (header.fourCC == FourCCDxt3) {
            return FormatInfo{ ImageFormat::BC2_RGBA_UNORM, 16 };
        }
        if (header.fourCC == FourCCDxt5) {
            return FormatInfo{ ImageFormat::BC3_RGBA_UNORM, 16 };
        }
        if (header.fourCC == FourCCAti1 || header.fourCC == FourCCBc4U) {
            return FormatInfo{ ImageFormat::BC4_R_UNORM, 8 };
        }
        if (header.fourCC == FourCCAti2 || header.fourCC == FourCCBc5U) {
            return FormatInfo{ ImageFormat::BC5_RG_UNORM, 16 };
        }
        if ((header.pixelFormatFlags & (PixelFormatFlagRGB | PixelFormatFlagAlphaPixels)) ==
                (PixelFormatFlagRGB | PixelFormatFlagAlphaPixels) &&
            header.fourCC == 0 &&
            header.rgbBitCount == 32 &&
            header.rBitMask == 0x000000ff &&
            header.gBitMask == 0x0000ff00 &&
            header.bBitMask == 0x00ff0000 &&
            header.aBitMask == 0xff000000) {
            return FormatInfo{ ImageFormat::RGBA8_UNORM, 4, 1, 1 };
        }
        if (header.fourCC == FourCCDx10 && dx10Header) {
            switch (dx10Header->dxgiFormat) {
            case 71: return FormatInfo{ ImageFormat::BC1_RGBA_UNORM, 8 };  // DXGI_FORMAT_BC1_UNORM
            case 72: return FormatInfo{ ImageFormat::BC1_RGBA_SRGB, 8 };   // DXGI_FORMAT_BC1_UNORM_SRGB
            case 74: return FormatInfo{ ImageFormat::BC2_RGBA_UNORM, 16 }; // DXGI_FORMAT_BC2_UNORM
            case 75: return FormatInfo{ ImageFormat::BC2_RGBA_SRGB, 16 };  // DXGI_FORMAT_BC2_UNORM_SRGB
            case 77: return FormatInfo{ ImageFormat::BC3_RGBA_UNORM, 16 }; // DXGI_FORMAT_BC3_UNORM
            case 78: return FormatInfo{ ImageFormat::BC3_RGBA_SRGB, 16 };  // DXGI_FORMAT_BC3_UNORM_SRGB
            case 80: return FormatInfo{ ImageFormat::BC4_R_UNORM, 8 };     // DXGI_FORMAT_BC4_UNORM
            case 83: return FormatInfo{ ImageFormat::BC5_RG_UNORM, 16 };   // DXGI_FORMAT_BC5_UNORM
            case 95: return FormatInfo{ ImageFormat::BC6H_RGB_UFLOAT, 16 };// DXGI_FORMAT_BC6H_UF16
            case 96: return FormatInfo{ ImageFormat::BC6H_RGB_SFLOAT, 16 };// DXGI_FORMAT_BC6H_SF16
            case 98: return FormatInfo{ ImageFormat::BC7_RGBA_UNORM, 16 }; // DXGI_FORMAT_BC7_UNORM
            case 99: return FormatInfo{ ImageFormat::BC7_RGBA_SRGB, 16 };  // DXGI_FORMAT_BC7_UNORM_SRGB
            default: return std::nullopt;
            }
        }
        return std::nullopt;
    }
}
