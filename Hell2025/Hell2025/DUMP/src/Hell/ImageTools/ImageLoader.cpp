#include "ImageTools.h"
#include "DDS.h"

#include "Hell/Logging.h"

#include <stb_image.h>
#include <tinyexr.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <utility>

namespace Hell::ImageTools {

    namespace {
        ImageFormat GetUncompressedImageFormat(int channelCount) {
            switch (channelCount) {
            case 1: return ImageFormat::R8_UNORM;
            case 2: return ImageFormat::RG8_UNORM;
            case 3: return ImageFormat::RGB8_UNORM;
            case 4: return ImageFormat::RGBA8_UNORM;
            default: return ImageFormat::UNDEFINED;
            }
        }

        uint32_t GetMaximumMipCount(uint32_t width, uint32_t height) {
            uint32_t mipCount = 1;
            uint32_t largestDimension = std::max(width, height);
            while (largestDimension > 1) {
                largestDimension /= 2;
                ++mipCount;
            }
            return mipCount;
        }
    }

    ImageData LoadImageData(const std::string& path, ImageDataType type, uint32_t maxCompressedTextureResolution) {
        switch (type) {
        case ImageDataType::UNCOMPRESSED: return LoadUncompressedImage(path);
        case ImageDataType::COMPRESSED:   return LoadDDS(path, maxCompressedTextureResolution);
        case ImageDataType::EXR:          return LoadEXRImage(path);
        default:
            Logging::Error() << "ImageTools::LoadImageData(..) failed because image type was undefined for '" << path << "'\n";
            return {};
        }
    }

    ImageData LoadDDS(const std::string& filepath, uint32_t maxResolution) {
        ImageData imageData;
        imageData.type = ImageDataType::COMPRESSED;

        // Open the file in binary mode
        std::ifstream file(filepath, std::ios::binary);
        if (!file) {
            std::cout << "Failed to open DDS file: " << filepath << "\n";
            return imageData;
        }
        // Read and validate the DDS header
        DDS::Header header = {};
        file.read(reinterpret_cast<char*>(&header), sizeof(header));
        if (!file ||
            header.magic != DDS::Magic ||
            header.size != DDS::HeaderSize ||
            header.pixelFormatSize != DDS::PixelFormatSize ||
            header.width == 0 ||
            header.height == 0) {
            std::cout << "Not a valid DDS file: " << filepath << "\n";
            return imageData;
        }

        if ((header.caps2 & DDS::Caps2Cubemap) != 0 || (header.caps2 & DDS::Caps2Volume) != 0) {
            std::cout << "Unsupported DDS texture type (only 2D textures are supported): " << filepath << "\n";
            return imageData;
        }

        // Check for potential DX10 extended header
        DDS::HeaderDX10 dx10Header = {};
        if (header.fourCC == DDS::FourCCDx10) {
            file.read(reinterpret_cast<char*>(&dx10Header), sizeof(dx10Header));
            if (!file) {
                std::cout << "Failed to read DDS DX10 header: " << filepath << "\n";
                return imageData;
            }
            if (dx10Header.resourceDimension != DDS::ResourceDimensionTexture2D ||
                dx10Header.arraySize != 1 ||
                (dx10Header.miscFlag & DDS::ResourceMiscTextureCube) != 0) {
                std::cout << "Unsupported DDS DX10 texture type (only one 2D image is supported): " << filepath << "\n";
                return imageData;
            }
        }

        // Retrieve format information
        const std::optional<DDS::FormatInfo> formatInfo = DDS::GetFormatInfo(header, &dx10Header);
        if (!formatInfo) {
            std::cout << "Unsupported DDS format: " << filepath << "\n";
            return imageData;
        }
        imageData.format = formatInfo->format;

        // Iterate the mipmap levels
        uint32_t mipWidth = header.width;
        uint32_t mipHeight = header.height;
        const uint32_t mipCount = std::max(1u, header.mipMapCount);
        if (mipCount > GetMaximumMipCount(mipWidth, mipHeight)) {
            std::cout << "Invalid DDS mip count: " << filepath << "\n";
            imageData.format = ImageFormat::UNDEFINED;
            return imageData;
        }

        imageData.mips.reserve(mipCount);
        for (uint32_t i = 0; i < mipCount; ++i) {
            const uint64_t blocksWide =
                (static_cast<uint64_t>(mipWidth) + formatInfo->blockWidth - 1) / formatInfo->blockWidth;
            const uint64_t blocksHigh =
                (static_cast<uint64_t>(mipHeight) + formatInfo->blockHeight - 1) / formatInfo->blockHeight;
            if (blocksHigh != 0 && blocksWide > std::numeric_limits<uint64_t>::max() / blocksHigh) {
                std::cout << "DDS mip payload size overflow: " << filepath << "\n";
                return {};
            }
            const uint64_t blockCount = blocksWide * blocksHigh;
            if (blockCount > std::numeric_limits<uint64_t>::max() / formatInfo->blockSize) {
                std::cout << "DDS mip payload size overflow: " << filepath << "\n";
                return {};
            }
            const uint64_t dataSize64 = blockCount * formatInfo->blockSize;
            if (dataSize64 > std::numeric_limits<size_t>::max() ||
                dataSize64 > static_cast<uint64_t>(std::numeric_limits<std::streamsize>::max()) ||
                dataSize64 > static_cast<uint64_t>(std::numeric_limits<std::streamoff>::max())) {
                std::cout << "DDS mip payload is too large: " << filepath << "\n";
                return {};
            }
            const size_t dataSize = static_cast<size_t>(dataSize64);

            const bool exceedsResolutionLimit =
                maxResolution != 0 &&
                (mipWidth > maxResolution || mipHeight > maxResolution);

            if (exceedsResolutionLimit) {
                file.seekg(static_cast<std::streamoff>(dataSize), std::ios::cur);
                if (!file) {
                    std::cout << "Error skipping oversized DDS mip level " << i << ": " << filepath << "\n";
                    return {};
                }
            }
            else {
                TextureMip mip;
                mip.width = mipWidth;
                mip.height = mipHeight;
                mip.data.resize(dataSize);
                file.read(reinterpret_cast<char*>(mip.data.data()), static_cast<std::streamsize>(dataSize));
                if (file.gcount() != static_cast<std::streamsize>(dataSize)) {
                    std::cout << "Error reading DDS mip level " << i << ": " << filepath << "\n";
                    imageData = {};
                    return imageData;
                }
                imageData.mips.push_back(std::move(mip));
            }

            mipWidth = std::max(1u, mipWidth / 2);
            mipHeight = std::max(1u, mipHeight / 2);
        }

        if (imageData.mips.empty()) {
            std::cout << "DDS has no mip within maximum resolution " << maxResolution << ": " << filepath << "\n";
            return {};
        }

        return imageData;
    }

    ImageData LoadUncompressedImage(const std::string& filepath) {
        ImageData imageData;
        imageData.type = ImageDataType::UNCOMPRESSED;

        int width = 0;
        int height = 0;
        int channelCount = 0;
        uint8_t* pixels = stbi_load(filepath.c_str(), &width, &height, &channelCount, 0);
        if (!pixels) {
            std::cout << "Failed to load image: " << filepath << "\n";
            return imageData;
        }

        TextureMip& mip = imageData.mips.emplace_back();
        mip.width = width;
        mip.height = height;

        if (channelCount == 3) {
            const size_t newSize = static_cast<size_t>(width) * height * 4;
            mip.data.resize(newSize);
            uint8_t* rgbaData = reinterpret_cast<uint8_t*>(mip.data.data());
            for (size_t i = 0, j = 0; i < newSize; i += 4, j += 3) {
                rgbaData[i] = pixels[j];         // R
                rgbaData[i + 1] = pixels[j + 1]; // G
                rgbaData[i + 2] = pixels[j + 2]; // B
                rgbaData[i + 3] = 255;              // A
            }
            imageData.format = ImageFormat::RGBA8_UNORM;
        }
        else {
            const size_t dataSize = static_cast<size_t>(width) * height * channelCount;
            mip.data.resize(dataSize);
            std::memcpy(mip.data.data(), pixels, dataSize);
            imageData.format = GetUncompressedImageFormat(channelCount);
        }

        stbi_image_free(pixels);
        return imageData;
    }

    ImageData LoadEXRImage(const std::string& filepath) {
        ImageData imageData;
        imageData.type = ImageDataType::EXR;
        const char* err = nullptr;
        float* pixels = nullptr;
        int width = 0;
        int height = 0;
        const int status = LoadEXR(&pixels, &width, &height, filepath.c_str(), &err);
        if (status != TINYEXR_SUCCESS) {
            std::cout << "Failed to load EXR: " << filepath;
            if (err) {
                std::cout << " (" << err << ")";
                FreeEXRErrorMessage(err);
            }
            std::cout << "\n";
            return imageData;
        }

        TextureMip& mip = imageData.mips.emplace_back();
        mip.width = width;
        mip.height = height;
        mip.data.resize(static_cast<size_t>(width) * height * 4 * sizeof(float));
        std::memcpy(mip.data.data(), pixels, mip.data.size());
        std::free(pixels);

        imageData.format = ImageFormat::RGBA32_SFLOAT;
        return imageData;
    }

    ImageData LoadR16UNormImage(const std::string& filepath) {
        ImageData imageData;
        imageData.type = ImageDataType::UNCOMPRESSED;

        int width = 0;
        int height = 0;
        int channels = 0;
        uint16_t* pixels = stbi_load_16(filepath.c_str(), &width, &height, &channels, 1); // Force single-channel

        if (!pixels) {
            std::cout << "Failed to load R16_UNORM image: " << filepath << "\n";
            return imageData;
        }

        TextureMip& mip = imageData.mips.emplace_back();
        mip.width = width;
        mip.height = height;
        mip.data.resize(static_cast<size_t>(width) * height * sizeof(uint16_t));
        std::memcpy(mip.data.data(), pixels, mip.data.size());
        stbi_image_free(pixels);

        // stb_image returns normalized unsigned 16-bit samples, not IEEE half floats.
        imageData.format = ImageFormat::R16_UNORM;
        return imageData;
    }
}
