#pragma once
#include "Unloved/Common/CreateInfo.h"
#include "Unloved/Common/Types.h"
#include "Unloved/Objects/Renderables/MeshNodes.h"

namespace Unloved {

struct Fence {
    Fence() = default;
    Fence(uint64_t id, FenceCreateInfo& createInfo, SpawnOffset& spawnOffset);
    Fence(const Fence&) = delete;
    Fence& operator=(const Fence&) = delete;
    Fence(Fence&&) noexcept = default;
    Fence& operator=(Fence&&) noexcept = default;
    ~Fence() = default;

    void AddControlPoint(const glm::vec2& controlPoint2D);
    void SetPosition(const glm::vec3& position);
    void UpdateSequencePoints(const std::vector<SequencePoint>& sequencePoints);
    void Init();
    void Update();
    void CleanUp();

    const std::vector<RenderItem>& const GetRenderItems();

    const uint64_t GetObjectId() const           { return m_objectId; }
    const glm::vec3& GetPosition() const         { return m_position; }
    const FenceCreateInfo& GetCreateInfo() const { return m_createInfo; }

private:
    uint64_t m_objectId = 0;
    glm::vec3 m_position = glm::vec3(0.0f);
    FenceCreateInfo m_createInfo;
    SpawnOffset m_spawnOffset;

    RenderItem CreateWireRenderItem(RenderItem& localSpaceRenderItem, glm::vec3& position, glm::vec3 nextPosition);

    std::vector<glm::vec3> m_finalPositions;
    std::vector<glm::vec3> m_wirePositionsA;
    std::vector<glm::vec3> m_wirePositionsB;
    std::vector<glm::vec3> m_wirePositionsC;
    std::vector<glm::vec3> m_wirePositionsD;
    std::vector<glm::vec3> m_wirePositionsE;
    std::vector<RenderItem> m_renderItems;
    MeshNodes m_meshNodesFat;
    MeshNodes m_meshNodesThin;
    MeshNodes m_meshNodesWire;
    MeshNodes m_meshNodesWireBarbed;
};
}
