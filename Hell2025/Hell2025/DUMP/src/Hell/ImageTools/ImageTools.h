#pragma once

#include "Hell/Render/TextureTypes.h"

#include <cstdint>
#include <string>
#include <vector>

namespace Hell::ImageTools {
    // Offline compression
    void CreateAndExportDDS(const std::string& inputFilepath, const std::string& outputFilepath, bool createMipMaps);

    // Loading
    ImageData LoadImageData(const std::string& path, ImageDataType type, uint32_t maxCompressedTextureResolution = 0);
    ImageData LoadDDS(const std::string& filepath, uint32_t maxResolution = 0);
    ImageData LoadUncompressedImage(const std::string& filepath);
    ImageData LoadR16UNormImage(const std::string& filepath);
    ImageData LoadEXRImage(const std::string& filepath);

    // Writing
    void SaveFlippedBitmap(const std::string& filename, const uint8_t* data, int width, int height, int channelCount);
    void SaveBitmap(const std::string& filename, const void* data, int width, int height, ImageFormat format);
    void SaveHeightMapR16UNorm(const std::string& filename, const void* data, int width, int height);
    void SaveFloatArrayTextureAsBitmap(const std::vector<float>& data, int width, int height, ImageFormat format, const std::string& filename);

    // Conversion
    void ConvertRGBA8ToR16SFloat(ImageData& imageData);
    
    // Util
    void PrintDebugInfo(const ImageData& imageData);
    bool IsEightBitImageFormat(ImageFormat format);
    bool IsHalfFloatImageFormat(ImageFormat format);
    bool IsFullFloatImageFormat(ImageFormat format);
    float HalfToFloat(uint16_t value);
    uint16_t FloatToHalf(float value);
}
