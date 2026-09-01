#pragma once
#include "Unloved/Common/Types.h"
#include "Unloved/Common/Enums.h"
#include "Unloved/Common/CreateInfo.h"

#include "Hell/Math/Transform.h"

#include "Unloved/Bible/Info/ItemInfo.h"
#include "Unloved/Objects/Renderables/MeshNodes.h"

namespace Unloved {

struct PickUp {
    PickUp() = default;
    PickUp(uint64_t id, const PickUpCreateInfo& createInfo, const SpawnOffset& spawnOffset);
    PickUp(const PickUp&) = delete;
    PickUp& operator=(const PickUp&) = delete;
    PickUp(PickUp&&) noexcept = default;
    PickUp& operator=(PickUp&&) noexcept = default;
    ~PickUp() = default;

    void SetPosition(const glm::vec3& position);
    void SetRotation(const glm::vec3& rotation);
    void Update(float deltaTime);
    void CleanUp();
    void Respawn();
    void Despawn();

    void SetDisabledPhysicsAtSpawnState(bool state);
    void SetRespawnState(bool state);

    const bool IsDespawned() const {return m_despawned;}

    MeshNodes& GetMeshNodes()                           { return m_meshNodes; }
    const bool IsDirty() const                          { return m_meshNodes.IsDirty(); }
    const PickUpCreateInfo& GetCreateInfo() const       { return m_createInfo; }
    const std::string GetName() const                   { return m_createInfo.name; }
    const ItemType GetType() const                      { return m_createInfo.type; }
    const std::vector<RenderItem>& GetRenderItems()     { return m_meshNodes.GetRenderItems(); }
    const glm::vec3& GetPosition() const                { return m_createInfo.position; }
    const glm::vec3& GetRotation() const                { return m_initialTransform.rotation; }
    const glm::mat4& GetModelMatrix()                   { return m_modelMatrix; }
    const uint64_t GetObjectId()                        { return m_objectId; }
    const bool GetDisabledPhysicsAtSpawnState()         { return m_createInfo.disablePhysicsAtSpawn; }
    const bool GetRespawnState()                        { return m_createInfo.respawn; }

private:
    uint64_t m_objectId = 0;
    PickUpCreateInfo m_createInfo;
    Hell::Transform m_initialTransform;
    glm::mat4 m_modelMatrix = glm::mat4(1.0f);
    MeshNodes m_meshNodes;
    bool m_firstFrame = true;
    float m_respawnCounter = 0;
    bool m_despawned = false;

    void MarkDirtyInTracker();
};
}
