#pragma once
#include "Unloved/Common/CreateInfo.h"

#include "Unloved/Systems/Openables/OpenableManager.h"
#include "Unloved/Objects/Renderables/MeshNodes.h"

namespace Unloved {

struct GenericObject {
    GenericObject() = default;
    GenericObject(uint64_t id, const GenericObjectCreateInfo& createInfo, const SpawnOffset& spawnOffset);
    GenericObject(const GenericObject&) = delete;
    GenericObject& operator=(const GenericObject&) = delete;
    GenericObject(GenericObject&&) noexcept = default;
    GenericObject& operator=(GenericObject&&) noexcept = default;
    ~GenericObject() = default;

    void Update(float deltaTime);
    void CleanUp();
    void SetPosition(const glm::vec3& position);
    void SetRotation(const glm::vec3& rotation);
    void SetScale(const glm::vec3& scale);
    void SetType(GenericObjectType type);
    void ResetPhysics();

    uint64_t GetObjectId()                                              { return m_objectId; }
    MeshNodes& GetMeshNodes()                                           { return m_meshNodes; }
    bool IsDirty() const                                                { return m_meshNodes.IsDirty(); }
    const std::string& GetEditorName() const                            { return m_createInfo.editorName; }
    const glm::vec3& GetPosition() const                                { return m_transform.position; }
    const glm::vec3& GetRotation() const                                { return m_transform.rotation; }
    const glm::vec3& GetScale() const                                   { return m_transform.scale; }
    const GenericObjectCreateInfo& GetCreateInfo() const                { return m_createInfo; }
    const GenericObjectType GetType() const                             { return m_createInfo.type; }
    const std::vector<RenderItem>& GetRenderItems() const               { return m_meshNodes.GetRenderItems(); }

private:
    GenericObjectCreateInfo m_createInfo;
    Hell::Transform m_transform;
    MeshNodes m_meshNodes;
    uint64_t m_objectId;
    bool m_navMeshTransformDirty = true;
};
}
