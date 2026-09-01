#pragma once

#include "Unloved/Common/Types.h"
#include "Unloved/Common/CreateInfo.h"
#include "Unloved/Render/RendererTypes.h"

namespace Unloved {

    struct LadderDismount {
        LadderDismount() = default;
        LadderDismount(uint64_t id, LadderDismountCreateInfo& createInfo, SpawnOffset& spawnOffset);
        LadderDismount(const LadderDismount&) = delete;
        LadderDismount& operator=(const LadderDismount&) = delete;
        LadderDismount(LadderDismount&&) noexcept = default;
        LadderDismount& operator=(LadderDismount&&) noexcept = default;
        ~LadderDismount() = default;

        void CleanUp();
        void SetPosition(const glm::vec3& position);

        const LadderDismountCreateInfo& GetCreateInfo() const { return m_createInfo; }
        const uint64_t GetObjectId() const { return m_objectId; }
        const glm::vec3& GetPosition() const { return m_createInfo.position; }
        const RenderItem& GetRenderItem() const { return m_renderItem; }

    private:
        void UpdateRenderItem();

        uint64_t m_objectId = 0;
        LadderDismountCreateInfo m_createInfo;
        RenderItem m_renderItem;
    };
}
