#include "ImageTools.h"

#include "Hell/Common/Enum.h"

#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <utility>
#include <vector>

namespace Hell::ImageTools {

    void PrintDebugInfo(const ImageData& imageData) {
        std::cout << "Format: " << ImageFormatToString(imageData.format) << "\n";
        std::cout << "Image Data Type: " << Hell::Enum::ToString(imageData.type) << "\n";
        std::cout << "Channel Count: " << GetImageFormatChannelCount(imageData.format) << "\n";
        std::cout << "Mip Count: " << imageData.mips.size() << "\n";

        for (size_t i = 0; i < imageData.mips.size(); ++i) {
            const TextureMip& mip = imageData.mips[i];
            std::cout << "Mip " << i << ": " << mip.width << "x" << mip.height
                << ", " << mip.data.size() << " bytes\n";
        }
    }

    void ConvertRGBA8ToR16SFloat(ImageData& imageData) {
        if (imageData.format != ImageFormat::RGBA8_UNORM) {
            std::cout << "ConvertRGBA8ToR16SFloat() requires RGBA8_UNORM image data\n";
            return;
        }

        for (TextureMip& mip : imageData.mips) {
            const size_t pixelCount = static_cast<size_t>(mip.width) * mip.height;
            if (mip.data.size() != pixelCount * 4) {
                std::cout << "ConvertRGBA8ToR16SFloat() encountered an invalid mip payload\n";
                return;
            }

            const uint8_t* rgbaData = reinterpret_cast<const uint8_t*>(mip.data.data());
            std::vector<std::byte> halfFloatData(pixelCount * sizeof(uint16_t));

            for (size_t pixelIndex = 0; pixelIndex < pixelCount; ++pixelIndex) {
                const uint16_t redValue = FloatToHalf(rgbaData[pixelIndex * 4] / 255.0f);
                std::memcpy(
                    halfFloatData.data() + pixelIndex * sizeof(redValue),
                    &redValue,
                    sizeof(redValue)
                );
            }

            mip.data = std::move(halfFloatData);
        }

        imageData.format = ImageFormat::R16_SFLOAT;
        imageData.type = ImageDataType::UNCOMPRESSED;
    }

    bool IsEightBitImageFormat(ImageFormat format) {
        switch (format) {
        case ImageFormat::R8_UNORM:
        case ImageFormat::RG8_UNORM:
        case ImageFormat::RGB8_UNORM:
        case ImageFormat::RGBA8_UNORM:
        case ImageFormat::RGB8_SRGB:
        case ImageFormat::RGBA8_SRGB:
            return true;
        default:
            return false;
        }
    }

    bool IsHalfFloatImageFormat(ImageFormat format) {
        switch (format) {
        case ImageFormat::R16_SFLOAT:
        case ImageFormat::RG16_SFLOAT:
        case ImageFormat::RGB16_SFLOAT:
        case ImageFormat::RGBA16_SFLOAT:
            return true;
        default:
            return false;
        }
    }

    bool IsFullFloatImageFormat(ImageFormat format) {
        switch (format) {
        case ImageFormat::R32_SFLOAT:
        case ImageFormat::RG32_SFLOAT:
        case ImageFormat::RGB32_SFLOAT:
        case ImageFormat::RGBA32_SFLOAT:
            return true;
        default:
            return false;
        }
    }

    float HalfToFloat(uint16_t value) {
        const uint32_t sign = static_cast<uint32_t>(value & 0x8000u) << 16;
        uint32_t exponent = (value >> 10) & 0x1fu;
        uint32_t mantissa = value & 0x03ffu;

        uint32_t floatBits = 0;
        if (exponent == 0) {
            if (mantissa == 0) {
                floatBits = sign;
            }
            else {
                exponent = 127 - 15 + 1;
                while ((mantissa & 0x0400u) == 0) {
                    mantissa <<= 1;
                    --exponent;
                }
                mantissa &= 0x03ffu;
                floatBits = sign | (exponent << 23) | (mantissa << 13);
            }
        }
        else if (exponent == 31) {
            floatBits = sign | 0x7f800000u | (mantissa << 13);
        }
        else {
            floatBits = sign | ((exponent + 112) << 23) | (mantissa << 13);
        }

        return std::bit_cast<float>(floatBits);
    }

    uint16_t FloatToHalf(float value) {
        const uint32_t floatBits = std::bit_cast<uint32_t>(value);
        const uint16_t sign = static_cast<uint16_t>((floatBits >> 16) & 0x8000u);
        const uint32_t sourceExponent = (floatBits >> 23) & 0xffu;
        uint32_t mantissa = floatBits & 0x7fffffu;

        if (sourceExponent == 0xffu) {
            return static_cast<uint16_t>(sign | (mantissa == 0 ? 0x7c00u : 0x7e00u));
        }

        int32_t exponent = static_cast<int32_t>(sourceExponent) - 127 + 15;
        if (exponent >= 31) {
            return static_cast<uint16_t>(sign | 0x7c00u);
        }

        if (exponent <= 0) {
            if (exponent < -10) {
                return sign;
            }
            mantissa |= 0x800000u;
            const uint32_t shift = static_cast<uint32_t>(14 - exponent);
            uint32_t halfMantissa = mantissa >> shift;
            const uint32_t roundBit = 1u << (shift - 1);
            if ((mantissa & roundBit) != 0 &&
                ((mantissa & (roundBit - 1)) != 0 || (halfMantissa & 1u) != 0)) {
                ++halfMantissa;
            }
            return static_cast<uint16_t>(sign | halfMantissa);
        }

        uint32_t halfMantissa = mantissa >> 13;
        if ((mantissa & 0x1000u) != 0 &&
            ((mantissa & 0x0fffu) != 0 || (halfMantissa & 1u) != 0)) {
            ++halfMantissa;
            if (halfMantissa == 0x0400u) {
                halfMantissa = 0;
                ++exponent;
                if (exponent >= 31) {
                    return static_cast<uint16_t>(sign | 0x7c00u);
                }
            }
        }

        return static_cast<uint16_t>(sign | (static_cast<uint16_t>(exponent) << 10) | halfMantissa);
    }
}
