#include "LadderDismount.h"

#include "Hell/Logging.h"
#include "Hell/Math/Transform.h"
#include "Hell/ResourceManagement/ResourceManager.h"

#include "Unloved/Render/RendererUtil.h"
#include "Unloved/Systems/WorldBVH/WorldBVH.h"

#include <cmath>

namespace {
    constexpr float MARKER_HALF_WIDTH_METRES = 0.36666667f;
    constexpr float MARKER_HEIGHT_METRES = 0.18333333f;

    glm::vec3 ApplySpawnOffset(const glm::vec3& position, const SpawnOffset& spawnOffset) {
        const float c = std::cos(spawnOffset.yRotation);
        const float s = std::sin(spawnOffset.yRotation);
        const glm::vec3 rotated(position.x * c + position.z * s, position.y, -position.x * s + position.z * c);
        return rotated + spawnOffset.translation;
    }
}

namespace Unloved {

LadderDismount::LadderDismount(uint64_t id, LadderDismountCreateInfo& createInfo, SpawnOffset& spawnOffset) {
    m_objectId = id;
    m_createInfo = createInfo;
    m_createInfo.position = ApplySpawnOffset(m_createInfo.position, spawnOffset);

    UpdateRenderItem();
    WorldBVH::MarkStaticSceneBvhDirty();
}

void LadderDismount::CleanUp() {
    m_renderItem = {};
    WorldBVH::MarkStaticSceneBvhDirty();
}

void LadderDismount::SetPosition(const glm::vec3& position) {
    m_createInfo.position = position;
    UpdateRenderItem();
    WorldBVH::MarkStaticSceneBvhDirty();
}

void LadderDismount::UpdateRenderItem() {
    Transform transform;
    transform.position = m_createInfo.position + glm::vec3(0.0f, MARKER_HEIGHT_METRES * 0.5f, 0.0f);
    transform.scale = glm::vec3(MARKER_HALF_WIDTH_METRES * 2.0f, MARKER_HEIGHT_METRES, MARKER_HALF_WIDTH_METRES * 2.0f);

    const int32_t materialIndex = Hell::ResourceManager::GetMaterialIndexByName("CheckerBoard");
    if (materialIndex == -1) {
        __debugbreak();
        Logging::Fatal() << "Ladder dismount failed to load some hardcoded material\n";
    }

    m_renderItem = RendererUtil::CreateAssetGeometryRenderItem("Cube", "Cube", transform.ToMat4(), materialIndex, m_objectId);
}

}
