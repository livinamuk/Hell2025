#pragma once

#include "Unloved/Common/Types.h"
#include "Unloved/Common/CreateInfo.h"

#include "Hell/Math/Transform.h"
#include "Hell/Physics/Types/RigidDynamic.h"
#include "Hell/ResourceManagement/Types/Model.h"

#include "Unloved/Objects/Renderables/MeshNodes.h"

namespace Unloved {

struct GameObject {
    GameObject() = default;
    GameObject(GameObjectCreateInfo createInfo);
    GameObject(uint64_t id, GameObjectCreateInfo createInfo);
    GameObject(uint64_t id, GameObjectCreateInfo createInfo, SpawnOffset spawnOffset);
    GameObjectCreateInfo GetCreateInfo();

    std::string m_name;
    Hell::Transform m_transform;
    MeshNodes m_meshNodes;

    void CleanUp();
    void Update(float deltaTime);
    void SetName(const std::string& name);
    void SetPosition(glm::vec3 position);
    void SetRotation(glm::vec3 rotation);
    void SetRotationY(float rotation);
    void SetScale(glm::vec3 scale);
    void SetModel(const std::string& name);
    void SetMeshMaterial(const std::string& meshName, const std::string& materialName);
    void SetMeshBlendingMode(const std::string& meshName, BlendingMode blendingMode);
    void PrintMeshNames();
    void UpdateRenderItems();
    void SetConvexHullsFromModel(const std::string modelName);
    
    void BeginFrame();
    void MarkAsSelected();
    bool IsSelected();

    const glm::vec3& GetPosition() const;
    const glm::vec3& GetRotation() const                    { return m_transform.rotation; }
    glm::vec3 GetEulerRotation() const;
    glm::vec3 GetScale() const;
    const glm::mat4 GetModelMatrix();

    MeshNodes& GetMeshNodes()                                           { return m_meshNodes; }

    const GameObjectCreateInfo& GetCreateInfo() const                   { return m_createInfo; }
    const std::vector<RenderItem>& GetRenderItems() const               { return m_meshNodes.GetRenderItems(); }
    const uint64_t GetObjectId() const                                  { return m_objectId; }

private:
    GameObjectCreateInfo m_createInfo;
    uint64_t m_physicsId = 0;
    uint64_t m_objectId = 0;
    bool m_selected = false;
    bool m_hasPhysics = false;   
};
}
