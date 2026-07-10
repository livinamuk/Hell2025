#include "PickUp.h"

#include "Hell/Physics/Physics.h"

#include "Unloved/Bible/Bible.h"
#include "Unloved/Systems/DirtyTracker/DirtyTracker.h"



namespace Unloved {

PickUp::PickUp(uint64_t id, const PickUpCreateInfo& createInfo, const SpawnOffset& spawnOffset) {
    m_createInfo = createInfo;
    m_createInfo.position += spawnOffset.translation;
    m_createInfo.rotation.y += spawnOffset.yRotation;
    m_objectId = id;

    ItemInfo* inventoryItemInfo = Bible::GetItemInfoByName(createInfo.name);
    if (!inventoryItemInfo) return; // Should never happen

    m_initialTransform.position = m_createInfo.position;
    m_initialTransform.rotation = m_createInfo.rotation;

    Bible::ConfigureMeshNodesByItemName(id, inventoryItemInfo->GetName(), &m_meshNodes, true);
}

void PickUp::Update(float deltaTime) {
    m_modelMatrix = m_initialTransform.to_mat4();
    m_meshNodes.Update(GetModelMatrix());

    if (m_firstFrame && m_createInfo.disablePhysicsAtSpawn) {
        m_meshNodes.SleepAllPhysics();
    }

    m_firstFrame = false;

    m_respawnCounter += deltaTime;

    if (m_despawned && m_respawnCounter >= 0.0f) {
        Respawn();
    }
}

void PickUp::Respawn() {
    m_meshNodes.SetBlendingModes(BlendingMode::DEFAULT);

    m_despawned = false;
    m_meshNodes.ForceDirty();

    MarkDirtyInTracker();
}

void PickUp::Despawn() {
    m_meshNodes.SetBlendingModes(BlendingMode::DO_NOT_RENDER);

    m_respawnCounter = -8.0f;
    m_despawned = true;

    MarkDirtyInTracker();
}

void PickUp::CleanUp() {
    MarkDirtyInTracker();

    m_meshNodes.CleanUp();
}

void PickUp::MarkDirtyInTracker() {
    AABB aabb = m_meshNodes.CalculateCurrentWorldspaceAABB(m_modelMatrix);

    DirtyBounds dirtyBounds;
    dirtyBounds.objectId = m_objectId;
    dirtyBounds.boundsMin = aabb.GetBoundsMin();
    dirtyBounds.boundsMax = aabb.GetBoundsMax();
    dirtyBounds.castShadows = true;

    DirtyTracker::AddDynamicDirtyBounds(dirtyBounds);
}

void PickUp::SetPosition(const glm::vec3& position) {
    m_createInfo.position = position;
    m_initialTransform.position = position;
    m_meshNodes.ResetFirstFrame();
}

void PickUp::SetRotation(const glm::vec3& rotation) {
    m_createInfo.rotation = rotation;
    m_initialTransform.rotation = rotation;
    m_meshNodes.ResetFirstFrame();
}

void PickUp::SetDisabledPhysicsAtSpawnState(bool state) {
    m_createInfo.disablePhysicsAtSpawn = state;
}

void PickUp::SetRespawnState(bool state) {
    m_createInfo.respawn = state;
}
}
