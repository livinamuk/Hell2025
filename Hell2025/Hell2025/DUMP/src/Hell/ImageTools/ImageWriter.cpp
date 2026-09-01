#include "ImageTools.h"

#pragma warning(push)
#pragma warning(disable : 4996)
#include "stb_image_write.h"
#pragma warning(pop)

#include <lodepng/lodepng.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

namespace Hell::ImageTools{

    namespace {

    template<typename T>
    T ReadComponent(const void* data, size_t index) {
        T value;
        std::memcpy(&value, static_cast<const std::byte*>(data) + index * sizeof(T), sizeof(T));
        return value;
    }

    void SaveNormalizedFloatDataAsBitmap(const std::vector<float>& data, int width, int height, int channelCount, const std::string& filename) {
        const size_t pixelCount = static_cast<size_t>(width) * height;
        if (width <= 0 || height <= 0 || channelCount <= 0 || data.size() != pixelCount * channelCount) {
            std::cout << "SaveNormalizedFloatDataAsBitmap() failed: invalid image data\n";
            return;
        }

        std::vector<uint8_t> outputData(pixelCount * 3);

        for (size_t pixelIndex = 0; pixelIndex < pixelCount; ++pixelIndex) {
            const size_t componentIndex = pixelIndex * channelCount;
            const float r = data[componentIndex];
            const float g = channelCount > 1 ? data[componentIndex + 1] : r;
            const float b = channelCount > 2 ? data[componentIndex + 2] : (channelCount == 1 ? r : 0.0f);

            outputData[pixelIndex * 3 + 0] = static_cast<uint8_t>(std::clamp(r * 255.0f, 0.0f, 255.0f));
            outputData[pixelIndex * 3 + 1] = static_cast<uint8_t>(std::clamp(g * 255.0f, 0.0f, 255.0f));
            outputData[pixelIndex * 3 + 2] = static_cast<uint8_t>(std::clamp(b * 255.0f, 0.0f, 255.0f));
        }

        if (!stbi_write_bmp(filename.c_str(), width, height, 3, outputData.data())) {
            std::cout << "Error: Failed to save BMP file!\n";
        }
        else {
            std::cout << "Saved BMP successfully: " << filename << "\n";
        }
    }

    }

    void SaveFlippedBitmap(const std::string& filename, const uint8_t* data, int width, int height, int channelCount) {
        if (!data || width <= 0 || height <= 0 || channelCount <= 0) {
            std::cout << "SaveFlippedBitmap() failed: invalid image data\n";
            return;
        }

        const size_t rowSize = static_cast<size_t>(width) * channelCount;
        std::vector<uint8_t> flippedData(rowSize * height);
        for (int y = 0; y < height; ++y) {
            std::memcpy(
                flippedData.data() + static_cast<size_t>(height - y - 1) * rowSize,
                data + static_cast<size_t>(y) * rowSize,
                rowSize
            );
        }

        if (!stbi_write_bmp(filename.c_str(), width, height, channelCount, flippedData.data())) {
            std::cout << "Failed to save bitmap: " << filename << "\n";
        }
    }

    void SaveBitmap(const std::string& filename, const void* data, int width, int height, ImageFormat format) {
        if (!data || width <= 0 || height <= 0) {
            std::cout << "SaveBitmap() failed: invalid image data or dimensions\n";
            return;
        }

        const int channelCount = GetImageFormatChannelCount(format);
        if (channelCount == 0 || IsCompressedImageFormat(format)) {
            std::cout << "SaveBitmap() failed: Unsupported format " << ImageFormatToString(format) << "\n";
            return;
        }

        const size_t componentCount = static_cast<size_t>(width) * height * channelCount;
        std::vector<float> floatData(componentCount);

        if (IsEightBitImageFormat(format)) {
            const uint8_t* source = static_cast<const uint8_t*>(data);
            for (size_t i = 0; i < componentCount; ++i) {
                floatData[i] = source[i] / 255.0f;
            }
        }
        else if (format == ImageFormat::R16_UNORM) {
            for (size_t i = 0; i < componentCount; ++i) {
                floatData[i] = ReadComponent<uint16_t>(data, i) / 65535.0f;
            }
        }
        else if (IsHalfFloatImageFormat(format)) {
            for (size_t i = 0; i < componentCount; ++i) {
                floatData[i] = HalfToFloat(ReadComponent<uint16_t>(data, i));
            }
        }
        else if (IsFullFloatImageFormat(format)) {
            for (size_t i = 0; i < componentCount; ++i) {
                floatData[i] = ReadComponent<float>(data, i);
            }
        }
        else {
            std::cout << "SaveBitmap() failed: Unsupported format " << ImageFormatToString(format) << "\n";
            return;
        }

        SaveNormalizedFloatDataAsBitmap(floatData, width, height, channelCount, filename);
    }

    void SaveHeightMapR16UNorm(const std::string& filename, const void* data, int width, int height) {
        if (!data || width <= 0 || height <= 0) {
            std::cerr << "Error: Data pointer is null, cannot save PNG.\n";
            return;
        }

        const size_t pixelCount = static_cast<size_t>(width) * height;
        std::vector<unsigned char> pngInput(pixelCount * 2);
        for (size_t i = 0; i < pixelCount; ++i) {
            const uint16_t value = ReadComponent<uint16_t>(data, i);
            // PNG stores 16-bit samples in network byte order.
            pngInput[i * 2 + 0] = static_cast<unsigned char>(value >> 8);
            pngInput[i * 2 + 1] = static_cast<unsigned char>(value & 0xff);
        }

        std::vector<unsigned char> png;
        const unsigned error = lodepng::encode(png, pngInput, width, height, LCT_GREY, 16);

        if (error) {
            std::cerr << "SaveHeightMapR16UNorm() LodePNG error: " << lodepng_error_text(error) << "\n";
            return;
        }

        const unsigned saveError = lodepng::save_file(png, filename);
        if (saveError) {
            std::cerr << "SaveHeightMapR16UNorm() failed to write file: " << lodepng_error_text(saveError) << "\n";
        }
    }

    void SaveFloatArrayTextureAsBitmap(const std::vector<float>& data, int width, int height, ImageFormat format, const std::string& filename) {
        if (data.empty() || width <= 0 || height <= 0) {
            std::cout << "SaveFloatArrayTextureAsBitmap() failed: invalid image data\n";
            return;
        }

        const int channelCount = GetImageFormatChannelCount(format);
        if (channelCount == 0 || IsCompressedImageFormat(format)) {
            std::cout << "SaveFloatArrayTextureAsBitmap() failed: unsupported format " << ImageFormatToString(format) << "\n";
            return;
        }

        const size_t expectedSize = static_cast<size_t>(width) * height * channelCount;
        if (data.size() != expectedSize) {
            std::cout << "SaveFloatArrayTextureAsBitmap() failed: data size mismatch. Expected " << expectedSize << ", got " << data.size() << "\n";
            return;
        }

        std::vector<float> normalizedData = data;
        if (IsEightBitImageFormat(format)) {
            for (float& value : normalizedData) {
                value /= 255.0f;
            }
        }
        else if (format == ImageFormat::R16_UNORM) {
            for (float& value : normalizedData) {
                value /= 65535.0f;
            }
        }

        SaveNormalizedFloatDataAsBitmap(normalizedData, width, height, channelCount, filename);
    }
}
