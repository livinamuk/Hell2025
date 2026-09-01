#include "PointAnimationInstance.h"

#include "Hell/ResourceManagement/ResourceManager.h"

#include <glm/vec4.hpp>

#include <algorithm>
#include <cmath>

void PointAnimationInstance::Init(const PointAnimationInstanceCreateInfo& createInfo) {
    m_createInfo = createInfo;

    Hell::PointAnimation* pointAnimation = Hell::ResourceManager::GetPointAnimationPtr(m_createInfo.resourceName);
    if (!pointAnimation) return;

    const Hell::PointAnimationMetadata& metadata = pointAnimation->GetMetadata();
    m_frameCount = std::max(metadata.frameCount, 1);

    const float fps = metadata.fps > 0.0f ? metadata.fps : 24.0f;
    m_fps = std::max(fps * m_createInfo.playbackSpeed, 0.01f);
    m_points.reserve(std::max(metadata.pointCount, 0));

    m_currentTime = 0.0f;
}

void PointAnimationInstance::Clear() {
    m_createInfo = {};
    m_currentTime = 0.0f;
    m_fps = 0.0f;
    m_frameCount = 0;
    m_currentFrameIdx = 0;
    m_points.clear();
}

void PointAnimationInstance::Update(float deltaTime, const glm::mat4& modelMatrix) {
    m_currentTime += deltaTime;
    m_points.clear();

    if (m_createInfo.resourceName.empty()) {
        return;
    }

    Hell::PointAnimation* pointAnimation = Hell::ResourceManager::GetPointAnimationPtr(m_createInfo.resourceName);
    if (!pointAnimation) return;

    const float loopDuration = static_cast<float>(m_frameCount) / m_fps;
    const float stopTime = static_cast<float>(m_frameCount - 1) / m_fps;

    if (m_createInfo.loop) {
        m_currentTime = std::fmod(m_currentTime, loopDuration);
    }
    else {
        m_currentTime = std::min(m_currentTime, stopTime);
    }

    m_currentFrameIdx = std::min(static_cast<int32_t>(m_currentTime * m_fps), m_frameCount - 1);

    const std::vector<glm::vec3>& currentPoints = pointAnimation->GetFrame(m_currentFrameIdx);
    m_points.reserve(currentPoints.size());

    for (const glm::vec3& point : currentPoints) {
        const glm::vec3 localPoint = point * m_createInfo.localAxisScale;
        m_points.push_back(glm::vec3(modelMatrix * glm::vec4(localPoint, 1.0f)));
    }
}
