#pragma once

#include "Unloved/Common/CreateInfo.h"
#include "Unloved/Render/RendererTypes.h"

#include <glm/vec3.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace Unloved {

struct HouseLocation {
    HouseLocation() = default;
    HouseLocation(uint64_t id, const HouseLocationCreateInfo& createInfo, const SpawnOffset& spawnOffset);
    HouseLocation(const HouseLocation&) = delete;
    HouseLocation& operator=(const HouseLocation&) = delete;
    HouseLocation(HouseLocation&&) noexcept = default;
    HouseLocation& operator=(HouseLocation&&) noexcept = default;
    ~HouseLocation() = default;

    void CleanUp();
    void SetPosition(const glm::vec3& position);
    void SetRotation(const glm::vec3& rotation);
    void SetRandomHouse(bool randomHouse)                    { m_createInfo.randomHouse = randomHouse; }
    void SetHouseName(const std::string& houseName)          { m_createInfo.houseName = houseName; }

    uint64_t GetObjectId() const                             { return m_objectId; }
    const glm::vec3& GetPosition() const                     { return m_createInfo.position; }
    glm::vec3 GetRotation() const                            { return glm::vec3(0.0f, m_createInfo.rotation, 0.0f); }
    const HouseLocationCreateInfo& GetCreateInfo() const     { return m_createInfo; }
    const std::vector<RenderItem>& GetRenderItems() const    { return m_renderItems; }

private:
    void UpdateRenderItems();

    uint64_t m_objectId = 0;
    HouseLocationCreateInfo m_createInfo;
    std::vector<RenderItem> m_renderItems;
};
}
