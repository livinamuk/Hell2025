#include "Ladder.h"

#include "Unloved/Common/Constants.h"
#include "Unloved/Debug/DebugDraw.h"
#include "Unloved/Systems/WorldBVH/WorldBVH.h"

#include <glm/geometric.hpp>

namespace Unloved {

Ladder::Ladder(uint64_t id, LadderCreateInfo& createInfo, SpawnOffset& spawnOffset) {
    m_objectId = id;
    m_createInfo = createInfo;
    m_createInfo.position += spawnOffset.translation;
    m_createInfo.rotation.y += spawnOffset.yRotation;

    m_position = m_createInfo.position;
    m_rotation = m_createInfo.rotation;

    constexpr float MINIMUM_SPAN_LENGTH = 0.01f;
    if (m_createInfo.editableAxisSpan.maximum - m_createInfo.editableAxisSpan.minimum < MINIMUM_SPAN_LENGTH) {
        m_createInfo.editableAxisSpan.maximum = m_createInfo.editableAxisSpan.minimum + MINIMUM_SPAN_LENGTH;
    }

    Reset();
}

void Ladder::Reset() {
    std::vector<MeshNodeCreateInfo> meshNodeCreateInfoSet;

    MeshNodeCreateInfo& ladder = meshNodeCreateInfoSet.emplace_back();
    ladder.meshName = "Ladder";
    ladder.materialName = "Ladder";
    ladder.rigidDynamic.createObject = true;
    ladder.rigidDynamic.kinematic = true;
    ladder.rigidDynamic.shapeType = PhysicsShapeType::BOX;
    ladder.rigidDynamic.filterData.raycastGroup = RAYCAST_DISABLED;
    ladder.rigidDynamic.filterData.collisionGroup = CollisionGroup(ENVIROMENT_OBSTACLE | LADDER);
    ladder.rigidDynamic.filterData.collidesWith = (CollisionGroup)(GENERIC_BOUNCEABLE | ITEM_PICK_UP | BULLET_CASING | RAGDOLL_PLAYER | RAGDOLL_ENEMY);
    ladder.addtoNavMesh = true;

    m_meshNodes.Init(m_objectId, "Ladder", meshNodeCreateInfoSet);
    m_meshNodes.EnableCSMShadows();

    RecomputeModelMatrix();
    m_meshNodes.Update(m_modelMatrix);
    WorldBVH::MarkStaticSceneBvhDirty();
}

void Ladder::SetPosition(const glm::vec3& position) {
    m_createInfo.position = position;
    m_position = position;
    RecomputeModelMatrix();
    m_meshNodes.Update(m_modelMatrix);
    WorldBVH::MarkStaticSceneBvhDirty();
}

void Ladder::SetRotation(const glm::vec3& rotation) {
    m_createInfo.rotation = rotation;
    m_rotation = rotation;
    RecomputeModelMatrix();
    m_meshNodes.Update(m_modelMatrix);
    WorldBVH::MarkStaticSceneBvhDirty();
}

bool Ladder::SetEditableAxisSpanPointPosition(uint32_t pointIndex, const glm::vec3& position) {
    if (pointIndex >= 2) return false;

    constexpr float MINIMUM_SPAN_LENGTH = 0.01f;
    const float axisOffset = glm::dot(position - m_position, m_worldUp);
    if (pointIndex == 0) {
        m_createInfo.editableAxisSpan.minimum = glm::min(axisOffset, m_createInfo.editableAxisSpan.maximum - MINIMUM_SPAN_LENGTH);
    }
    else {
        m_createInfo.editableAxisSpan.maximum = glm::max(axisOffset, m_createInfo.editableAxisSpan.minimum + MINIMUM_SPAN_LENGTH);
    }
    UpdateEditableAxisSpanWorldPoints();
    return true;
}

void Ladder::Update(float deltaTime) {
    m_meshNodes.Update(m_modelMatrix);

    //RenderDebug();
}

void Ladder::RenderDebug() {
    glm::vec3 p1 = GetPosition();
    glm::vec3 p2 = GetPosition() + (m_worldForward * 0.5f);
    DebugDraw::DrawPoint(p1, YELLOW);
    DebugDraw::DrawPoint(p2, YELLOW);
    DebugDraw::DrawLine(p1, p2, YELLOW);
}

void Ladder::CleanUp() {
    m_meshNodes.CleanUp();
}

void Ladder::RecomputeModelMatrix() {
    Transform transform;
    transform.position = m_position;
    transform.rotation = m_rotation;
    m_modelMatrix = transform.to_mat4();
    m_worldForward = glm::vec3(m_modelMatrix * glm::vec4(m_localForward, 0.0f));
    m_worldUp = glm::normalize(glm::vec3(m_modelMatrix * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f)));
    UpdateEditableAxisSpanWorldPoints();
}

void Ladder::UpdateEditableAxisSpanWorldPoints() {
    m_bottomPoint = m_position + m_worldUp * m_createInfo.editableAxisSpan.minimum;
    m_topPoint = m_position + m_worldUp * m_createInfo.editableAxisSpan.maximum;
}
}
