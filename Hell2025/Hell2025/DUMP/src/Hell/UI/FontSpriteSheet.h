#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

struct FontSpriteSheet {
    struct CharData {
        uint32_t id = 0;
        int width = 0;
        int height = 0;
        int atlasX = 0;
        int atlasY = 0;
        int xOffset = 0;
        int yOffset = 0;
        int xAdvance = 0;
    };

    std::string m_name;
    std::string m_textureName;
    int m_textureWidth = 0;
    int m_textureHeight = 0;
    int m_lineHeight = 0;
    int m_base = 0;
    int m_charHeight = 0;
    int m_lineSpacing = 0;
    std::unordered_map<uint32_t, CharData> m_charData;
};

namespace FontSpriteSheetPacker {
    void ExampleUsage();
    void Export(const std::string& name, const std::string& characters, int charSpacing, int lineSpacing, const std::string& textureSourcePath, const std::string& outputPath);
    FontSpriteSheet Import(const std::string& filepath);
}
