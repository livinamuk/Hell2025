#include "TextBlitter.h"
#include "FontSpriteSheet.h"
#include "Hell/File.h"
#include "Hell/Logging.h"

#include <algorithm>
#include <unordered_map>

namespace {
    bool DecodeNextCodepoint(const std::string& text, size_t& offset, uint32_t& codepoint) {
        if (offset >= text.size()) return false;

        uint8_t first = static_cast<uint8_t>(text[offset++]);
        if ((first & 0x80u) == 0) {
            codepoint = first;
            return true;
        }

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
            codepoint = 0xfffdu;
            return true;
        }

        if (offset + continuationCount > text.size()) {
            offset = text.size();
            codepoint = 0xfffdu;
            return true;
        }

        for (size_t i = 0; i < continuationCount; i++) {
            uint8_t continuation = static_cast<uint8_t>(text[offset++]);
            if ((continuation & 0xc0u) != 0x80u) {
                codepoint = 0xfffdu;
                return true;
            }
            codepoint = (codepoint << 6) | (continuation & 0x3fu);
        }

        return true;
    }

    const FontSpriteSheet::CharData* FindCharData(const FontSpriteSheet& spriteSheet, uint32_t codepoint) {
        auto it = spriteSheet.m_charData.find(codepoint);
        return it != spriteSheet.m_charData.end() ? &it->second : nullptr;
    }

    glm::vec2 CalculateTextSize(const std::string& text, const FontSpriteSheet& spriteSheet, float scale) {
        float cursorX = 0;
        float width = 0;
        size_t lineCount = 1;

        for (size_t i = 0; i < text.length();) {
            if (text.compare(i, 5, "[COL=") == 0) {
                size_t end = text.find("]", i);
                if (end != std::string::npos) {
                    i = end + 1;
                    continue;
                }
            }

            uint32_t codepoint = 0;
            if (!DecodeNextCodepoint(text, i, codepoint)) break;

            if (codepoint == '\n') {
                cursorX = 0;
                lineCount++;
                continue;
            }

            const FontSpriteSheet::CharData* charData = FindCharData(spriteSheet, codepoint);
            if (charData) {
                cursorX += charData->xAdvance * scale;
                width = std::max(width, cursorX);
            }
        }

        float height = lineCount * spriteSheet.m_lineHeight * scale;
        return glm::vec2(width, height);
    }
}

namespace TextBlitter {
    std::vector<FontSpriteSheet> g_fontSpriteSheets;
    std::unordered_map<std::string, uint32_t> g_fontIndices;

    glm::vec4 ParseColorTag(const std::string& tag);

    void Init() {
        // Export fonts, aka create spritesheets from single char files, no need to do every init but YOLO ¯\_(ツ)_/¯
        std::string name = "StandardFont";
        std::string characters = R"(!"#$%&'*+,-./0123456789:;<=>?_ABCDEFGHIJKLMNOPQRSTUVWXYZ\^_`abcdefghijklmnopqrstuvwxyz )";
        std::string textureSourcePath = "res/fonts/raw_images/standard_font/";
        std::string outputPath = "res/fonts/";
        FontSpriteSheetPacker::Export(name, characters, 0, 1, textureSourcePath, outputPath);

        name = "AmmoFont";
        characters = "0123456789/";
        textureSourcePath = "res/fonts/raw_images/ammo_font/";
        FontSpriteSheetPacker::Export(name, characters, 0, 1, textureSourcePath, outputPath);

        name = "BebasNeue";
        characters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-\xE2\x80\x99,. ";
        textureSourcePath = "res/fonts/raw_images/bebas_neue/";
        FontSpriteSheetPacker::Export(name, characters, 2, 1, textureSourcePath, outputPath);

        name = "RobotoCondensed";
        characters = R"(!"#$%&'*+,-./0123456789:;<=>?_ABCDEFGHIJKLMNOPQRSTUVWXYZ\^_`abcdefghijklmnopqrstuvwxyz )";
        textureSourcePath = "res/fonts/raw_images/roboto_condensed/";
        FontSpriteSheetPacker::Export(name, characters, 1, 1, textureSourcePath, outputPath);

        for (const auto& fileInfo : Hell::File::IterateDirectory("res/fonts", { "fnt" })) {
            AddFont(FontSpriteSheetPacker::Import(fileInfo.path));
        }
    }
    
    void BlitText(const std::string& text, const std::string& fontName, int originX, int originY, glm::ivec2 viewportSize, Alignment alignment, float scale, int32_t textureIndex, std::vector<Vertex2D>& vertices, std::vector<uint32_t>& indices) {
        FontSpriteSheet* spriteSheet = GetFontSpriteSheet(fontName);
        if (!spriteSheet) return;

        // Construct the vertex data
        size_t firstVertex = vertices.size();
        float cursorX = static_cast<float>(originX);
        float cursorY = static_cast<float>(viewportSize.y - originY); // Top left corner
        float invTextureWidth = 1.0f / static_cast<float>(spriteSheet->m_textureWidth);
        float invTextureHeight = 1.0f / static_cast<float>(spriteSheet->m_textureHeight);
        glm::vec4 color(1.0f); // Default color

        // Reserve space for vertices and indices
        size_t estimatedVertices = text.length() * 4;
        size_t estimatedIndices = text.length() * 6;
        vertices.reserve(vertices.size() + estimatedVertices);
        indices.reserve(indices.size() + estimatedIndices);

        for (size_t i = 0; i < text.length();) {

            // Handle color tags
            if (text.compare(i, 5, "[COL=") == 0) {
                size_t end = text.find("]", i);

                if (end != std::string::npos) {
                    color = ParseColorTag(text.substr(i, end - i + 1));
                    i = end + 1; // Skip the tag
                    continue;
                }
            }
            uint32_t codepoint = 0;
            if (!DecodeNextCodepoint(text, i, codepoint)) break;

            // Handle newlines
            if (codepoint == '\n') {
                cursorX = static_cast<float>(originX);
                cursorY -= spriteSheet->m_lineHeight * scale;
                continue;
            }

            const FontSpriteSheet::CharData* charData = FindCharData(*spriteSheet, codepoint);
            if (charData) {
                if (charData->width > 0 && charData->height > 0) {
                    // Normalized uvs
                    float u0 = charData->atlasX * invTextureWidth;
                    float v0 = (charData->atlasY + charData->height) * invTextureHeight;
                    float u1 = (charData->atlasX + charData->width) * invTextureWidth;
                    float v1 = charData->atlasY * invTextureHeight;

                    float glyphX = cursorX + charData->xOffset * scale;
                    float glyphY = cursorY - charData->yOffset * scale;

                    // Normalized quad position
                    float x0 = (glyphX / viewportSize.x) * 2.0f - 1.0f;
                    float y0 = (glyphY / viewportSize.y) * 2.0f - 1.0f;
                    float x1 = ((glyphX + charData->width * scale) / viewportSize.x) * 2.0f - 1.0f;
                    float y1 = ((glyphY - charData->height * scale) / viewportSize.y) * 2.0f - 1.0f;

                    // Vertices
                    vertices.push_back({ {x0, y0}, {u0, v1}, color }); // Bottom left
                    vertices.push_back({ {x1, y0}, {u1, v1}, color }); // Bottom right
                    vertices.push_back({ {x1, y1}, {u1, v0}, color }); // Top right
                    vertices.push_back({ {x0, y1}, {u0, v0}, color }); // Top left

                    // Indices
                    uint32_t vertexOffset = static_cast<uint32_t>(vertices.size()) - 4;
                    indices.push_back(vertexOffset + 0);
                    indices.push_back(vertexOffset + 1);
                    indices.push_back(vertexOffset + 2);
                    indices.push_back(vertexOffset + 0);
                    indices.push_back(vertexOffset + 2);
                    indices.push_back(vertexOffset + 3);
                }

                cursorX += charData->xAdvance * scale;
            }
        }

        // Post process alignment
        if (alignment != Alignment::TOP_LEFT) {
            glm::vec2 textSize = CalculateTextSize(text, *spriteSheet, scale);
            float offsetX = (textSize.x / viewportSize.x) * 2.0f;
            float offsetY = (textSize.y / viewportSize.y) * 2.0f;

            for (size_t i = firstVertex; i < vertices.size(); i++) {
                Vertex2D& vertex = vertices[i];

                switch (alignment) {
                case Alignment::CENTERED:
                    vertex.position.x -= offsetX * 0.5f;
                    vertex.position.y += offsetY * 0.5f;
                    break;

                case Alignment::CENTERED_HORIZONTAL:
                    vertex.position.x -= offsetX * 0.5f;
                    break;

                case Alignment::CENTERED_VERTICAL:
                    vertex.position.y += offsetY * 0.5f;
                    break;

                case Alignment::TOP_RIGHT:
                    vertex.position.x -= offsetX;
                    break;

                case Alignment::BOTTOM_LEFT:
                    vertex.position.y += offsetY;
                    break;

                case Alignment::BOTTOM_RIGHT:
                    vertex.position.x -= offsetX;
                    vertex.position.y += offsetY;
                    break;

                default:
                    break;
                }
            }
        }
    }

    void AddFont(const FontSpriteSheet& spriteSheet) {
        if (g_fontIndices.find(spriteSheet.m_name) != g_fontIndices.end()) {
            Logging::Warning() << "TextBlitter::AddFont(..) font already exists: " << spriteSheet.m_name << "\n";
            return;
        }

        uint32_t index = g_fontSpriteSheets.size();
        g_fontIndices[spriteSheet.m_name] = index;
        g_fontSpriteSheets.push_back(spriteSheet);
    }

    FontSpriteSheet* GetFontSpriteSheet(const std::string& name) {
        auto it = g_fontIndices.find(name);
        return (it != g_fontIndices.end()) ? &g_fontSpriteSheets[it->second] : nullptr;
    }

    glm::vec4 ParseColorTag(const std::string& tag) {
        glm::vec4 color(1.0f);

        size_t start = tag.find("[COL=");
        if (start == std::string::npos) {
            return color;
        }
        start += 5;

        size_t end = tag.find("]", start);
        if (end == std::string::npos) {
            return color;
        }

        const char* cStr = tag.c_str() + start;
        char* endPtr;

        color.r = std::strtof(cStr, &endPtr);
        if (*endPtr != ',') {
            return color;
        }

        color.g = std::strtof(endPtr + 1, &endPtr);
        if (*endPtr != ',') {
            return color;
        }

        color.b = std::strtof(endPtr + 1, &endPtr);
        if (*endPtr != ',') {
            return color;
        }

        color.a = std::strtof(endPtr + 1, &endPtr);
        return color;
    }

    glm::ivec2 GetTextSize(const std::string& text, const std::string& fontName, float scale) {
        FontSpriteSheet* spriteSheet = GetFontSpriteSheet(fontName);
        if (!spriteSheet) return glm::ivec2(0, 0);

        return glm::ivec2(CalculateTextSize(text, *spriteSheet, scale));
    }

    bool FontExists(const std::string& fontName) {
        return GetFontSpriteSheet(fontName) != nullptr;
    }
}
