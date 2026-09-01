#pragma once

#include "Unloved/Common/CreateInfo.h"
#include "Unloved/Render/RendererTypes.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <cstdint>
#include <vector>

namespace Unloved {

struct SpawnPoint {
    SpawnPoint() = default;
    SpawnPoint(uint64_t id, const SpawnPointCreateInfo& createInfo, const SpawnOffset& spawnOffset);
    SpawnPoint(const SpawnPoint&) = delete;
    SpawnPoint& operator=(const SpawnPoint&) = delete;
    SpawnPoint(SpawnPoint&&) noexcept = default;
    SpawnPoint& operator=(SpawnPoint&&) noexcept = default;
    ~SpawnPoint() = default;

    void CleanUp();
    void SetPosition(const glm::vec3& position);
    void SetRotation(const glm::vec3& rotation);

    const glm::vec3& GetPosition() const                  { return m_createInfo.position; }
    glm::vec3 GetRotation() const                         { return glm::vec3(m_createInfo.rotation, 0.0f); }
    uint64_t GetObjectId() const                          { return m_objectId; }
    glm::vec3 GetCameraEuler() const                      { return GetRotation(); }
    const SpawnPointCreateInfo& GetCreateInfo() const     { return m_createInfo; }
    const std::vector<RenderItem>& GetRenderItems() const { return m_renderItems; }

private:
    void UpdateRenderItems();

    uint64_t m_objectId = 0;
    SpawnPointCreateInfo m_createInfo;
    std::vector<RenderItem> m_renderItems;
};
}
