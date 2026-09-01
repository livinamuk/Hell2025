#pragma once
#include "Unloved/Common/Types.h"
#include "Unloved/Common/CreateInfo.h"

#include "Hell/Math/Transform.h"

#include "Unloved/Objects/Renderables/MeshNodes.h"
#include "Unloved/Objects/Renderables/SpriteSheetObject.h"
#include "Unloved/Systems/House/BlockingVolume.h"

namespace Unloved {

struct Fireplace {
    Fireplace() = default;
    Fireplace(uint64_t id, const FireplaceCreateInfo& createInfo, const SpawnOffset& spawnOffset);
    Fireplace(const Fireplace&) = delete;
    Fireplace& operator=(const Fireplace&) = delete;
    Fireplace(Fireplace&&) noexcept = default;
    Fireplace& operator=(Fireplace&&) noexcept = default;
    ~Fireplace() = default;

    void Update(float deltaTime);
    void CleanUp();

    void SetPosition(const glm::vec3& position);
    void SetPositionX(float x);
    void SetPositionY(float y);
    void SetPositionZ(float z);
    void SetRotation(const glm::vec3& rotation);
    void SetType(FireplaceType type);

    uint64_t GetObjectId()                                      { return m_id; }
    AABB GetWallsAABB()                                         { return m_wallsAabb; }
	MeshNodes& GetMeshNodes()                                   { return m_meshNodes; }
	const std::vector<RenderItem>& GetRenderItems() const       { return m_meshNodes.GetRenderItems(); }
    const glm::vec3& GetPosition()                              { return m_transform.position; }
    const glm::vec3& GetRotation()                              { return m_transform.rotation; }
    const FireplaceCreateInfo& GetCreateInfo() const            { return m_createInfo; }
    const glm::mat4& GetWorldMatrix() const                     { return m_worldMatrix; }
    float GetWallDepth() const                                  { return m_wallDepth; }
    float GetWallWidth() const                                  { return m_wallWidth; }
    const glm::vec3 GetWorldForward() const                     { return m_worldForward; }
    const glm::vec3 GetWorldRight() const                       { return m_worldRight; }
    const BlockingVolume& GetBlockingVolume() const             { return m_blockingVolume; }

private:
    void ConfigureFire();
    void ConfigureMeshNodes();
    void UpdateBlockingVolume();
    void UpdateWorldMatrix();
    uint64_t m_id = 0;
    FireplaceCreateInfo m_createInfo;
    MeshNodes m_meshNodes;
    Hell::Transform m_transform;
    glm::mat4 m_worldMatrix = glm::mat4(1.0f);
    glm::vec3 m_worldForward = glm::vec3(0.0f);
    glm::vec3 m_worldRight = glm::vec3(0.0f);
    AABB m_wallsAabb;
    BlockingVolume m_blockingVolume;
    float m_wallDepth = 0.0f;
    float m_wallWidth = 0.0f;

    glm::vec3 m_firePosition = glm::vec3(0.0f);

    uint64_t m_lightId = 0;
    bool m_navMeshTransformDirty = true;
};
}
