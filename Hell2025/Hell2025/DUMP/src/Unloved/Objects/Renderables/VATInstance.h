#pragma once

#include "Hell/Math/LocalFrame.h"
#include "Hell/Math/Transform.h"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <cstdint>
#include <string>

struct VATInstanceCreateInfo {
    std::string resourceName;
    float playbackSpeed = 1.0f;
    bool loop = true;
    bool mirror = true;
    glm::vec3 worldPosition = glm::vec3(0.0f);
    glm::vec3 worldForward = glm::vec3(0.0f);
    float scale = 1.0f;
};

struct VATRenderItem {
    glm::mat4 modelMatrix;
    glm::mat4 inverseModelMatrix;
    glm::vec4 boundsMin;
    glm::vec4 boundsMax;

    float fps;
    float frameCount;
    float currentTime;
    int32_t mirror;

    int32_t indexCount;
    int32_t vertexCount;
    int32_t baseIndex;
    int32_t baseVertex;

    int32_t positionTextureIdx;
    int32_t rotationTextureIdx;
    int32_t lookupTextureIdx;
    int32_t padding;
};

struct VATInstance {
    void Init(const VATInstanceCreateInfo& createInfo);
    void Update(float deltaTime);
    void ResetPlayTime()                              { m_currentTime = 0.0f; m_currentFrameIdx = 0; }
    void SetPlayTime(float playTime)                  { m_currentTime = playTime; }

    VATRenderItem CreateRenderItem();

    const glm::vec4& GetWorldBoundsMin() const      { return m_worldBoundsMin; }
    const glm::vec4& GetWorldBoundsMax() const      { return m_worldBoundsMax; }
    int32_t GetCurrentFrameIndex() const            { return m_currentFrameIdx; }
    int32_t GetFrameCount() const                   { return m_frameCount; }
    float GetCurrentTime() const                    { return m_currentTime; }
    float GetDuration() const                       { return m_duration; }
    float GetFPS() const                            { return m_fps; }
    const std::string& GetResourceName() const      { return m_createInfo.resourceName; }
    int32_t GetPositionTextureIndex() const         { return m_positionTextureIndex; }
    int32_t GetRotationTextureIndex() const         { return m_rotationTextureIndex; }
    int32_t GetLookupTextureIndex() const           { return m_lookupTextureIndex; }
    bool HasValidTextureIndices() const             { return m_positionTextureIndex != -1 && m_rotationTextureIndex != -1 && m_lookupTextureIndex != -1; }
    bool IsAnimationComplete() const                { return m_animationComplete; }

private:
    float m_currentTime = 0.0f;
    float m_duration = 0.0f;
    float m_fps = 0.0f;
    float m_animationComplete = 0.0f;
    int32_t m_frameCount = 0;
    int32_t m_currentFrameIdx = 0;
    int32_t m_positionTextureIndex = -1;
    int32_t m_rotationTextureIndex = -1;
    int32_t m_lookupTextureIndex = -1;
    glm::vec4 m_worldBoundsMin = glm::vec4(0.0f);
    glm::vec4 m_worldBoundsMax = glm::vec4(0.0f);
    VATInstanceCreateInfo m_createInfo;
    int32_t m_indexCount = 0;
    int32_t m_vertexCount = 0;
    int32_t m_baseIndex = 0;
    int32_t m_baseVertex = 0;
    Hell::QuatTransform m_transform;
};
