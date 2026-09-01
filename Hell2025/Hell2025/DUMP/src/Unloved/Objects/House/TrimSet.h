#pragma once

#include "Unloved/Common/CreateInfo.h"
#include "Unloved/Render/RendererTypes.h"

#include <vector>

namespace Unloved {

struct TrimCorner {
    TrimCorner() = default;
    TrimCorner(const glm::vec3& position) {
        m_position = position;
    }

    glm::vec3 m_position = glm::vec3(0.0f);
    bool m_internal = true;
};

struct TrimSet {
    TrimSet() = default;
    TrimSet(uint64_t id, const TrimSetCreateInfo& createInfo, const SpawnOffset& spawnOffset);
    TrimSet(const TrimSet&) = delete;
    TrimSet& operator=(const TrimSet&) = delete;
    TrimSet(TrimSet&&) noexcept = default;
    TrimSet& operator=(TrimSet&&) noexcept = default;
    ~TrimSet() = default;

    void CleanUp();
    void CreateRenderItems();
    void Update();
    void RenderDebug(const glm::vec3& color);

    std::vector<RenderItem>& GetRenderItems()   { return m_renderItems; }
    TrimSetType GetType() const                 { return m_createInfo.type; }
    uint64_t GetObjectId() const                { return m_objectId; }
    const TrimSetCreateInfo& GetCreateInfo() const { return m_createInfo; }
    const std::string& GetEditorName() const    { return m_createInfo.editorName; }

private:
    std::vector<TrimCorner> m_corners;
    std::vector<glm::vec3> m_internalCorners;
    std::vector<glm::vec3> m_externalCorners;
    uint64_t m_objectId = 0;
    TrimSetCreateInfo m_createInfo;
    std::vector<RenderItem> m_renderItems;
};
}
