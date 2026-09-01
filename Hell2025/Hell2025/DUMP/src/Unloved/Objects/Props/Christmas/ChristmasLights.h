#pragma once

#include "Unloved/Common/CreateInfo.h"
#include "Unloved/Common/SequencePoint.h"

#include "Unloved/Render/RendererTypes.h"

namespace Unloved {

struct ChristmasLightSet {
    ChristmasLightSet() = default;
    ChristmasLightSet(uint64_t id, const ChristmasLightsCreateInfo& createInfo, const SpawnOffset& spawnOffset);
    ChristmasLightSet(const ChristmasLightSet&) = delete;
    ChristmasLightSet& operator=(const ChristmasLightSet&) = delete;
    ChristmasLightSet(ChristmasLightSet&&) noexcept = default;
    ChristmasLightSet& operator=(ChristmasLightSet&&) noexcept = default;
    ~ChristmasLightSet() = default;

    void UpdateSequencePoints(const std::vector<SequencePoint>& sequencePoints);
    void SetPosition(const glm::vec3& position);
    void SetSpacing(float spacing);
    void SetWireRadius(float wireRadius);
    void Update(float deltaTime);
    void RecreateLightRenderItems();
    void CleanUp();

    float m_time = 0;

    const std::vector<uint64_t>& GetWireIds() const              { return m_wireIds; }
    const std::vector<GPUChristmasLight> GetGPUChristmasLights() { return m_GPUChristmasLights; }
    const std::vector<RenderItem>& GetRenderItems() const        { return m_renderItems; }
    const ChristmasLightsCreateInfo& GetCreateInfo() const       { return m_createInfo; }
    const uint64_t GetObjectId() const                           { return m_objectId; }
    const glm::vec3& GetPosition() const                         { return m_position; }
    const glm::vec3& GetRotation() const                         { return m_rotation; }

private:
    std::vector<RenderItem> m_renderItems;
    std::vector<GPUChristmasLight> m_GPUChristmasLights;
    ChristmasLightsCreateInfo m_createInfo;
    uint64_t m_objectId = 0;
    glm::vec3 m_position = glm::vec3(0.0f);
    glm::vec3 m_rotation = glm::vec3(0.0f);
    std::vector<uint64_t> m_wireIds;
};
}
