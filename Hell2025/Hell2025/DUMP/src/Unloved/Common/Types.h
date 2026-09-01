#pragma once
#include "Enums.h"

#include "Hell/Math/Transform.h"
#include "Hell/Math/VecXZ.h"

#include "Unloved/Objects/ObjectEnums.h"

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <cstdint>
#include <string>

using Transform = Hell::Transform;
using AnimatedTransform = Hell::QuatTransform;

struct Resolutions {
    glm::ivec2 gBuffer;
    glm::ivec2 gBufferHalfRes;
    glm::ivec2 finalImage;
    glm::ivec2 ui;
    glm::ivec2 hair;
};

struct SelectionRectangleState {
    int beginX = 0;
    int beginY = 0;
    int currentX = 0;
    int currentY = 0;
};

struct SpawnOffset {
    glm::vec3 translation = glm::vec3(0.0);
    float yRotation = 0;
};

struct HeightMapChunk {
    Hell::ivecXZ coord;
    uint32_t meshId = 0;
    glm::vec3 aabbMin;
    glm::vec3 aabbMax;
};

struct OceanReadbackData {
    float heightPlayer0 = 0.0f;
    float heightPlayer1 = 0.0f;
    float heightPlayer2 = 0.0f;
    float heightPlayer3 = 0.0f;
};

struct BloodDecalInstanceData {
    glm::mat4 modelMatrix;
    glm::mat4 inverseModelMatrix;
    int type;
    int textureIndex;
    float aspectScaleX;
    float aspectScaleY;
};

