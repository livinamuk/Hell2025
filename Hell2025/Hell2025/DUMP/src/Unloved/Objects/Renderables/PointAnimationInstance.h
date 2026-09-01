#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <cstdint>
#include <string>
#include <vector>

struct PointAnimationInstanceCreateInfo {
    std::string resourceName;
    float playbackSpeed = 1.0f;
    bool loop = true;
    glm::vec3 localAxisScale = glm::vec3(1.0f);
};

struct PointAnimationInstance {
    void Init(const PointAnimationInstanceCreateInfo& createInfo);
    void Update(float deltaTime, const glm::mat4& modelMatrix);
    void Clear();
    void ResetPlayTime()                                      { m_currentTime = 0.0f; m_currentFrameIdx = 0; }
    void SetPlayTime(float playTime)                          { m_currentTime = playTime; }

    int32_t GetCurrentFrameIndex() const                      { return m_currentFrameIdx; }
    int32_t GetFrameCount() const                             { return m_frameCount; }
    float GetCurrentTime() const                              { return m_currentTime; }
    float GetFPS() const                                      { return m_fps; }
    const std::string& GetResourceName() const                { return m_createInfo.resourceName; }
    const std::vector<glm::vec3>& GetPoints() const           { return m_points; }

private:
    float m_currentTime = 0.0f;
    float m_fps = 0.0f;
    int32_t m_frameCount = 0;
    int32_t m_currentFrameIdx = 0;
    PointAnimationInstanceCreateInfo m_createInfo;
    std::vector<glm::vec3> m_points;
};
