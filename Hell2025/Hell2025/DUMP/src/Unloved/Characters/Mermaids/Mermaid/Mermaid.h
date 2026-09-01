#pragma once
#include "Unloved/Common/CreateInfo.h"
#include "Unloved/Objects/Renderables/MeshNodes.h"

namespace Unloved {

struct Mermaid {
    Mermaid() = default;
    Mermaid(uint64_t id, MermaidCreateInfo createInfo, SpawnOffset spawnOffset);

    void Init(uint64_t id, MermaidCreateInfo createInfo, SpawnOffset spawnOffset);
    void Update(float deltaTime);
    void CleanUp();
    void SetPosition(const glm::vec3& position);
    void SetRotation(const glm::vec3& rotation);
    void SetShopTeleportPosition(const glm::vec3& position)              { m_createInfo.shopTeleportPosition = position; }
    void SetShopTeleportEuler(const glm::vec3& euler)                    { m_createInfo.shopTeleportEuler = euler; }

    Unloved::MeshNodes& GetMeshNodes()                                            { return m_meshNodes; }
    const MermaidCreateInfo& GetCreateInfo() const                       { return m_createInfo; }
    const glm::vec3& GetPosition() const                                 { return m_transform.position; }
    const glm::vec3& GetShopTeleportPosition() const                     { return m_createInfo.shopTeleportPosition; }
    const glm::vec3& GetShopTeleportEuler() const                        { return m_createInfo.shopTeleportEuler; }
    const glm::vec3& GetLocalForward() const                             { return m_localForward; }
    const glm::vec3& GetWorldForward() const                             { return m_worldForward; }
    uint64_t GetObjectId() const                                         { return m_objectId; }
    const std::vector<RenderItem>& GetRenderItems() const                { return m_meshNodes.GetRenderItems(); }

private:
    void UpdateRenderItems();
    void DebugDraw();

    SpawnOffset m_spawnOffset;
    uint64_t m_objectId = 0;
    MermaidCreateInfo m_createInfo;
    Transform m_transform;
    Unloved::MeshNodes m_meshNodes;
    bool m_topVisible = true;
    glm::vec3 m_localForward = glm::vec3(-1.0f, 0.0f, 0.0f);
    glm::vec3 m_worldForward = glm::vec3(0.0f);
};

}
