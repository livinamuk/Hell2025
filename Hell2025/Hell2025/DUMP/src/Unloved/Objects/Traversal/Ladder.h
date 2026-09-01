#pragma once

#include "Unloved/Common/Types.h"
#include "Unloved/Common/CreateInfo.h"
#include "Unloved/Objects/Renderables/MeshNodes.h"

namespace Unloved {

struct Ladder {
    Ladder() = default;
    Ladder(uint64_t id, LadderCreateInfo& createInfo, SpawnOffset& spawnOffset);
    Ladder(const Ladder&) = delete;
    Ladder& operator=(const Ladder&) = delete;
    Ladder(Ladder&&) noexcept = default;
    Ladder& operator=(Ladder&&) noexcept = default;
    ~Ladder() = default;

    void SetPosition(const glm::vec3& position);
    void SetRotation(const glm::vec3& rotation);
    bool SetEditableAxisSpanPointPosition(uint32_t pointIndex, const glm::vec3& position);
    void Update(float deltaTime);
    void Reset();
    void RenderDebug();
    void CleanUp();

    MeshNodes& GetMeshNodes()                           { return m_meshNodes; }
    const LadderCreateInfo& GetCreateInfo() const       { return m_createInfo; }
    const uint64_t GetObjectId() const                  { return m_objectId; }
    const glm::vec3& GetPosition() const                { return m_position; }
    const glm::vec3& GetRotation() const                { return m_rotation; }
    const glm::vec3& GetBottomPoint() const             { return m_bottomPoint; }
    const glm::vec3& GetTopPoint() const                { return m_topPoint; }
    const std::vector<RenderItem>& GetRenderItems()     { return m_meshNodes.GetRenderItems(); }

private:
    void RecomputeModelMatrix();
    void UpdateEditableAxisSpanWorldPoints();

    LadderCreateInfo m_createInfo;
    MeshNodes m_meshNodes;
    uint64_t m_objectId = 0;
    glm::vec3 m_position = glm::vec3(0.0f);
    glm::vec3 m_rotation = glm::vec3(0.0f);
    glm::mat4 m_modelMatrix = glm::mat4(1.0f);
    glm::vec3 m_localForward = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 m_worldForward = glm::vec3(0.0f);
    glm::vec3 m_worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 m_bottomPoint = glm::vec3(0.0f);
    glm::vec3 m_topPoint = glm::vec3(0.0f);
};
}
