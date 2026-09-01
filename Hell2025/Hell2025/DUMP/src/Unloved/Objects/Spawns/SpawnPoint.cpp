#include "SpawnPoint.h"

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

SpawnPoint::SpawnPoint(uint64_t id, const SpawnPointCreateInfo& createInfo, const SpawnOffset& spawnOffset) {
    m_objectId = id;
    m_createInfo = createInfo;
    m_createInfo.position = ApplySpawnOffset(m_createInfo.position, spawnOffset);
    m_createInfo.rotation.y += spawnOffset.yRotation;

    UpdateRenderItems();
    WorldBVH::MarkStaticSceneBvhDirty();
}

void SpawnPoint::CleanUp() {
    m_renderItems.clear();
    WorldBVH::MarkStaticSceneBvhDirty();
}

void SpawnPoint::SetPosition(const glm::vec3& position) {
    m_createInfo.position = position;

    UpdateRenderItems();
    WorldBVH::MarkStaticSceneBvhDirty();
}

void SpawnPoint::SetRotation(const glm::vec3& rotation) {
    m_createInfo.rotation = glm::vec2(rotation.x, rotation.y);

    UpdateRenderItems();
    WorldBVH::MarkStaticSceneBvhDirty();
}

void SpawnPoint::UpdateRenderItems() {
    m_renderItems.clear();

    Transform transform;
    transform.position = m_createInfo.position;
    transform.rotation = GetCameraEuler();
    transform.scale = glm::vec3(0.25f);

    int32_t materialIndex = Hell::ResourceManager::GetMaterialIndexByName("CheckerBoard");
    if (materialIndex == -1) {
        __debugbreak();
        Logging::Fatal() << "Spawn point failed to load some hardcoded material\n";
    }

    RenderItem renderItem = Unloved::RendererUtil::CreateAssetGeometryRenderItem("Cube", "Cube", transform.ToMat4(), materialIndex, m_objectId);
    // renderItem.shadowFlags = 
    m_renderItems.push_back(renderItem);
}

}
