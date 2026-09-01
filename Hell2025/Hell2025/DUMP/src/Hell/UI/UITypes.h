#pragma once

#include "Hell/Common/Enums.h"
#include "Hell/Render/TextureTypes.h"

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#include <cstdint>
#include <string>

enum class UICanvas : uint8_t {
    INTERNAL, // Fixed-resolution canvas composed before presentation
    NATIVE,   // Drawable-resolution canvas composed after presentation
    COUNT
};

struct RenderItemUI {
    uint32_t baseVertex = 0;
    uint32_t baseIndex = 0;
    uint32_t indexCount = 0;
    uint32_t textureIndex = 0;

    int32_t clipMinX = -1;
    int32_t clipMinY = -1;
    int32_t clipMaxX = -1;
    int32_t clipMaxY = -1;

    uint32_t filterMode = 0; // 0 for linear, 1 for nearest
    int32_t padding0 = 0;
    int32_t padding1 = 0;
    int32_t padding2 = 0;
};

struct BlitTextureInfo {
    std::string textureName;
    glm::ivec2 location;
    Alignment alignment;
    glm::vec4 colorTint = glm::vec4(1, 1, 1, 1);
    glm::ivec2 size = glm::ivec2(-1, -1);
    TextureFilter textureFilter = TextureFilter::NEAREST;
    float rotation = 0.0f;
    int clipMinX = -1;
    int clipMinY = -1;
    int clipMaxX = -1;
    int clipMaxY = -1;
};
