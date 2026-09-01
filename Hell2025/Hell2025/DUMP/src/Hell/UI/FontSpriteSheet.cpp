#include "FontSpriteSheet.h"
#include <cmath>
#include <fstream>
#include <vector>
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <regex>
#include <stdexcept>
#pragma warning(push)
#pragma warning(disable : 4996)
#include "stb_image.h"
#include "stb_image_write.h"
#pragma warning(pop)

namespace FontSpriteSheetPacker {

    struct ImageData {
        int m_width;
        int m_height;
        int m_channelCount;
        void* m_data;
    };

    ImageData LoadImageData(const std::string& filepath);
    std::unordered_map<std::string, std::string> ParseAttributes(const std::string& line);
    int FindRequiredInt(const std::unordered_map<std::string, std::string>& attributes, const std::string& key);
    std::string FindRequiredString(const std::unordered_map<std::string, std::string>& attributes, const std::string& key);
    std::vector<uint32_t> DecodeUTF8(const std::string& text);
    std::vector<std::string> GetSortedFilePaths(const std::string& directory);

    void ExampleUsage() {
        std::string name = "StandardFont";
        std::string characters = R"(!"#$%&'*+,-./0123456789:;<=>?_ABCDEFGHIJKLMNOPQRSTUVWXYZ\^_`abcdefghijklmnopqrstuvwxyz )";
        std::string textureSourcePath = "res/fonts/raw_images/standard_font/";
        std::string outputPath = "res/fonts/";

        Export(name, characters, 0, 0, textureSourcePath, outputPath);
        FontSpriteSheet fontSpriteSheet = Import("res/fonts/StandardFont.fnt");
    }

    void Export(const std::string& name, const std::string& characters, int charSpacing, int lineSpacing, const std::string& textureSourcePath, const std::string& outputPath) {
        // Padding in pixels around each glyph
        const int padX = 1;
        const int padY = 1;

        // Configure filepaths
        std::filesystem::path outputDir = outputPath;
        std::filesystem::path outputImagePath = outputDir / (name + ".png");
        std::filesystem::path outputFontPath = outputDir / (name + ".fnt");

        // Load the image data
        std::vector<std::string> filePaths = GetSortedFilePaths(textureSourcePath);
        std::vector<ImageData> imageDataList;

        for (std::string& filePath : filePaths) {
            imageDataList.push_back(LoadImageData(filePath));
        }

        // Dirty error check
        for (ImageData& imageData : imageDataList) {
            if (!imageData.m_data) {
                std::cout << "Failed to load font sprite sheet image data\n";
            }
        }

        std::vector<uint32_t> codepoints = DecodeUTF8(characters);
        if (imageDataList.empty()) {
            throw std::runtime_error("No glyph images found for font: " + name);
        }
        if (codepoints.size() != imageDataList.size()) {
            throw std::runtime_error(
                "Font glyph count mismatch for " + name + ": " +
                std::to_string(codepoints.size()) + " characters but " +
                std::to_string(imageDataList.size()) + " images"
            );
        }

        // Calculate total area and max character height
        int totalArea = 0;
        int maxCharHeight = 0;

        for (const ImageData& imageData : imageDataList) {
            //totalArea += imageData.m_width * imageData.m_height;
            totalArea += (imageData.m_width + padX) * (imageData.m_height + padY);
            maxCharHeight = std::max(maxCharHeight, imageData.m_height);
        }

        // Determine the minimum texture width for roughly a square (the ceiling of the square root)
        int textureWidth = static_cast<int>(std::ceil(std::sqrt(totalArea)));

        // Calculate the first character's data
        int charCount = imageDataList.size();

        std::vector<FontSpriteSheet::CharData> charDataList(charCount);
        charDataList[0].id = codepoints[0];
        charDataList[0].width = imageDataList[0].m_width;
        charDataList[0].height = imageDataList[0].m_height;
        charDataList[0].atlasX = padX;
        charDataList[0].atlasY = padY;
        charDataList[0].xAdvance = charDataList[0].width + (charDataList[0].id == ' ' ? 0 : charSpacing);

        // Calculate the remaining character's data
        int cursorX = charDataList[0].atlasX + charDataList[0].width + padX;
        int cursorY = padY;

        for (int i = 1; i < charCount; i++) {
            int charWidth = imageDataList[i].m_width;

            if (cursorX + charWidth > textureWidth) {
                cursorX = padX;
                cursorY += maxCharHeight + padY;
            }

            charDataList[i].id = codepoints[i];
            charDataList[i].width = imageDataList[i].m_width;
            charDataList[i].height = imageDataList[i].m_height;
            charDataList[i].atlasX = cursorX;
            charDataList[i].atlasY = cursorY;
            charDataList[i].xAdvance = charDataList[i].width + (charDataList[i].id == ' ' ? 0 : charSpacing);
            cursorX += charWidth + padX;
        }

        // Calculate texture height
        int textureHeight = cursorY + maxCharHeight;
        int textureSize = std::max(textureWidth, textureHeight);
        textureWidth = textureSize;
        textureHeight = textureSize;

        // Create an empty transparent image
        std::vector<unsigned char> finalImage(textureWidth * textureHeight * 4, 0);

        // Fill the pixel data
        for (size_t i = 0; i < charCount; i++) {
            unsigned char* srcPixels = static_cast<unsigned char*>(imageDataList[i].m_data);

            for (int y = 0; y < imageDataList[i].m_height; y++) {
                for (int x = 0; x < imageDataList[i].m_width; x++) {
                    int srcIndex = (y * imageDataList[i].m_width + x) * 4;
                    int destIndex = ((charDataList[i].atlasY + y) * textureWidth + (charDataList[i].atlasX + x)) * 4;
                    finalImage[destIndex + 0] = srcPixels[srcIndex + 0]; // R
                    finalImage[destIndex + 1] = srcPixels[srcIndex + 1]; // G
                    finalImage[destIndex + 2] = srcPixels[srcIndex + 2]; // B
                    finalImage[destIndex + 3] = srcPixels[srcIndex + 3]; // A
                }
            }
        }

        // Ensure the directory exists, create it if it doesn't
        if (!std::filesystem::exists(outputDir)) {
            std::filesystem::create_directories(outputDir);
        }

        // Save the image
        if (stbi_write_png(outputImagePath.string().c_str(), textureWidth, textureHeight, 4, finalImage.data(), textureWidth * 4)) {
            //std::cout << "Spritesheet saved successfully: " << outputImagePath << "\n";
        }
        else {
            std::cout << "Failed to save image: " << outputImagePath << "\n";
        }

        // Write the BMFont text descriptor
        std::ofstream fontFile(outputFontPath);
        if (fontFile.is_open()) {
            const int lineHeight = maxCharHeight + lineSpacing;
            const bool unicode = std::any_of(codepoints.begin(), codepoints.end(), [](uint32_t codepoint) { return codepoint > 255; });

            fontFile << "info face=\"" << name << "\" size=" << maxCharHeight
                     << " bold=0 italic=0 charset=\"\" unicode=" << unicode
                     << " stretchH=100 smooth=1 aa=1 padding=" << padY << "," << padX << "," << padY << "," << padX
                     << " spacing=" << padX << "," << padY << "\n";
            fontFile << "common lineHeight=" << lineHeight << " base=" << maxCharHeight
                     << " scaleW=" << textureWidth << " scaleH=" << textureHeight
                     << " pages=1 packed=0\n";
            fontFile << "page id=0 file=\"" << outputImagePath.filename().string() << "\"\n";
            fontFile << "chars count=" << charDataList.size() << "\n";

            for (const FontSpriteSheet::CharData& charData : charDataList) {
                fontFile << "char id=" << charData.id
                         << " x=" << charData.atlasX
                         << " y=" << charData.atlasY
                         << " width=" << charData.width
                         << " height=" << charData.height
                         << " xoffset=" << charData.xOffset
                         << " yoffset=" << charData.yOffset
                         << " xadvance=" << charData.xAdvance
                         << " page=0 chnl=15\n";
            }

            fontFile << "kernings count=0\n";
            fontFile.close();
        }
        else {
            std::cout << "Failed to save font descriptor: " << outputFontPath << "\n";
        }

        // Free the image data
        for (ImageData& imageData : imageDataList) {
            if (imageData.m_data) {
                stbi_image_free(imageData.m_data);
            }
        }
    }

    FontSpriteSheet Import(const std::string& filepath) {
        FontSpriteSheet fontSpriteSheet;
        std::ifstream file(filepath);
        if (!file.is_open()) throw std::runtime_error("Failed to open file: " + filepath);

        fontSpriteSheet.m_name = std::filesystem::path(filepath).stem().string();

        bool foundCommon = false;
        bool foundPage = false;
        std::string line;

        while (std::getline(file, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();

            if (line.rfind("common ", 0) == 0) {
                const auto attributes = ParseAttributes(line);
                fontSpriteSheet.m_lineHeight = FindRequiredInt(attributes, "lineHeight");
                fontSpriteSheet.m_base = FindRequiredInt(attributes, "base");
                fontSpriteSheet.m_textureWidth = FindRequiredInt(attributes, "scaleW");
                fontSpriteSheet.m_textureHeight = FindRequiredInt(attributes, "scaleH");
                fontSpriteSheet.m_charHeight = fontSpriteSheet.m_base;
                fontSpriteSheet.m_lineSpacing = fontSpriteSheet.m_lineHeight - fontSpriteSheet.m_base;

                if (FindRequiredInt(attributes, "pages") != 1) {
                    throw std::runtime_error("Only single-page BMFont files are supported: " + filepath);
                }
                foundCommon = true;
            }
            else if (line.rfind("page ", 0) == 0) {
                const auto attributes = ParseAttributes(line);
                if (FindRequiredInt(attributes, "id") != 0) continue;

                std::filesystem::path texturePath = FindRequiredString(attributes, "file");
                fontSpriteSheet.m_textureName = texturePath.stem().string();
                foundPage = true;
            }
            else if (line.rfind("char ", 0) == 0) {
                const auto attributes = ParseAttributes(line);
                if (FindRequiredInt(attributes, "page") != 0) {
                    throw std::runtime_error("Only BMFont page 0 is supported: " + filepath);
                }

                FontSpriteSheet::CharData charData;
                charData.id = static_cast<uint32_t>(FindRequiredInt(attributes, "id"));
                charData.atlasX = FindRequiredInt(attributes, "x");
                charData.atlasY = FindRequiredInt(attributes, "y");
                charData.width = FindRequiredInt(attributes, "width");
                charData.height = FindRequiredInt(attributes, "height");
                charData.xOffset = FindRequiredInt(attributes, "xoffset");
                charData.yOffset = FindRequiredInt(attributes, "yoffset");
                charData.xAdvance = FindRequiredInt(attributes, "xadvance");
                fontSpriteSheet.m_charData[charData.id] = charData;
            }
        }

        if (!foundCommon) throw std::runtime_error("BMFont common record not found: " + filepath);
        if (!foundPage) throw std::runtime_error("BMFont page 0 record not found: " + filepath);
        if (fontSpriteSheet.m_charData.empty()) throw std::runtime_error("No BMFont glyph records found: " + filepath);
        return fontSpriteSheet;
    }

    std::unordered_map<std::string, std::string> ParseAttributes(const std::string& line) {
        std::unordered_map<std::string, std::string> attributes;
        size_t position = line.find(' ');

        while (position != std::string::npos && position < line.size()) {
            position = line.find_first_not_of(' ', position);
            if (position == std::string::npos) break;

            size_t equals = line.find('=', position);
            if (equals == std::string::npos) break;

            std::string key = line.substr(position, equals - position);
            position = equals + 1;

            std::string value;
            if (position < line.size() && line[position] == '\"') {
                size_t valueStart = ++position;
                size_t valueEnd = line.find('\"', valueStart);
                if (valueEnd == std::string::npos) {
                    throw std::runtime_error("Unterminated BMFont attribute: " + key);
                }
                value = line.substr(valueStart, valueEnd - valueStart);
                position = valueEnd + 1;
            }
            else {
                size_t valueEnd = line.find(' ', position);
                value = line.substr(position, valueEnd - position);
                position = valueEnd;
            }

            attributes[key] = value;
        }

        return attributes;
    }

    int FindRequiredInt(const std::unordered_map<std::string, std::string>& attributes, const std::string& key) {
        auto it = attributes.find(key);
        if (it == attributes.end()) throw std::runtime_error("BMFont attribute not found: " + key);
        return std::stoi(it->second);
    }

    std::string FindRequiredString(const std::unordered_map<std::string, std::string>& attributes, const std::string& key) {
        auto it = attributes.find(key);
        if (it == attributes.end()) throw std::runtime_error("BMFont attribute not found: " + key);
        return it->second;
    }

    std::vector<uint32_t> DecodeUTF8(const std::string& text) {
        std::vector<uint32_t> codepoints;

        for (size_t i = 0; i < text.size();) {
            uint8_t first = static_cast<uint8_t>(text[i++]);
            if ((first & 0x80u) == 0) {
                codepoints.push_back(first);
                continue;
            }

            uint32_t codepoint = 0;
            size_t continuationCount = 0;
            if ((first & 0xe0u) == 0xc0u) {
                codepoint = first & 0x1fu;
                continuationCount = 1;
            }
            else if ((first & 0xf0u) == 0xe0u) {
                codepoint = first & 0x0fu;
                continuationCount = 2;
            }
            else if ((first & 0xf8u) == 0xf0u) {
                codepoint = first & 0x07u;
                continuationCount = 3;
            }
            else {
                codepoints.push_back(0xfffdu);
                continue;
            }

            if (i + continuationCount > text.size()) {
                codepoints.push_back(0xfffdu);
                break;
            }

            bool valid = true;
            for (size_t j = 0; j < continuationCount; j++) {
                uint8_t continuation = static_cast<uint8_t>(text[i++]);
                if ((continuation & 0xc0u) != 0x80u) {
                    valid = false;
                    break;
                }
                codepoint = (codepoint << 6) | (continuation & 0x3fu);
            }

            codepoints.push_back(valid ? codepoint : 0xfffdu);
        }

        return codepoints;
    }

    std::vector<std::string> GetSortedFilePaths(const std::string& directory) {
        std::vector<std::string> filePaths;
        auto entries = std::filesystem::directory_iterator(directory);

        for (const auto& entry : entries) {
            if (std::filesystem::is_regular_file(entry)) {
                filePaths.push_back(entry.path().string());
            }
        }

        // Sort file paths numerically by filename
        std::sort(filePaths.begin(), filePaths.end(), [](const std::string& a, const std::string& b) {
            std::regex numberRegex("(\\d+)"); // Regex to extract numbers
            std::smatch matchA, matchB;
            std::string stemA = std::filesystem::path(a).stem().string();
            std::string stemB = std::filesystem::path(b).stem().string();
            bool foundA = std::regex_search(stemA, matchA, numberRegex);
            bool foundB = std::regex_search(stemB, matchB, numberRegex);
            if (foundA && foundB) {
                int numA = std::stoi(matchA.str());
                int numB = std::stoi(matchB.str());
                if (numA != numB) return numA < numB; // Compare numbers if found
            }
            return a < b; // Fall back to lexicographic order
        });

        return filePaths;
    }

    ImageData LoadImageData(const std::string& filepath) {
        stbi_set_flip_vertically_on_load(false);

        ImageData imageData{};
        imageData.m_data = stbi_load(filepath.data(), &imageData.m_width, &imageData.m_height, &imageData.m_channelCount, 4);
        if (imageData.m_data) imageData.m_channelCount = 4;

        return imageData;
    }
}
