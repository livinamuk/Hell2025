#include "HouseLocation.h"

#include "Hell/Logging.h"
#include "Hell/Math/Transform.h"
#include "Hell/ResourceManagement/ResourceManager.h"

#include "Unloved/Render/RendererUtil.h"
#include "Unloved/Systems/WorldBVH/WorldBVH.h"

#include <cmath>

namespace {
    glm::vec3 ApplySpawnOffset(const glm::vec3& position, const SpawnOffset& spawnOffset) {
        const float c = std::cos(spawnOffset.yRotation);
        const float s = std::sin(spawnOffset.yRotation);
        const glm::vec3 rotated(position.x * c + position.z * s, position.y, -position.x * s + position.z * c);
        return rotated + spawnOffset.translation;
    }
}

namespace Unloved {

HouseLocation::HouseLocation(uint64_t id, const HouseLocationCreateInfo& createInfo, const SpawnOffset& spawnOffset) {
    m_objectId = id;
    m_createInfo = createInfo;
    m_createInfo.position = ApplySpawnOffset(m_createInfo.position, spawnOffset);
    m_createInfo.rotation += spawnOffset.yRotation;

    UpdateRenderItems();
    WorldBVH::MarkStaticSceneBvhDirty();
}

void HouseLocation::CleanUp() {
    m_renderItems.clear();
    WorldBVH::MarkStaticSceneBvhDirty();
}

void HouseLocation::SetPosition(const glm::vec3& position) {
    m_createInfo.position = position;
    UpdateRenderItems();
    WorldBVH::MarkStaticSceneBvhDirty();
}

void HouseLocation::SetRotation(const glm::vec3& rotation) {
    m_createInfo.rotation = rotation.y;
    UpdateRenderItems();
    WorldBVH::MarkStaticSceneBvhDirty();
}

void HouseLocation::UpdateRenderItems() {
    m_renderItems.clear();

    Transform transform;
    transform.position = m_createInfo.position;
    transform.rotation.y = m_createInfo.rotation;
    transform.scale = glm::vec3(0.25f);

    const int32_t materialIndex = Hell::ResourceManager::GetMaterialIndexByName("CheckerBoard");
    if (materialIndex == -1) {
        __debugbreak();
        Logging::Fatal() << "House location failed to load some hardcoded material\n";
    }

    m_renderItems.push_back(Unloved::RendererUtil::CreateAssetGeometryRenderItem("Cube", "Cube", transform.ToMat4(), materialIndex, m_objectId));
}

}
