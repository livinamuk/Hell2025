#include "Staircase.h"
#include "Unloved/Debug/DebugDraw.h"
#include "Hell/Logging.h"

namespace Unloved {

Staircase::Staircase(uint64_t id, StaircaseCreateInfo& createInfo, SpawnOffset& spawnOffset) {
    m_objectId = id;
    m_createInfo = createInfo;
    m_createInfo.position += spawnOffset.translation;
    m_createInfo.rotation.y += spawnOffset.yRotation;

    m_position = m_createInfo.position;
    m_rotation = m_createInfo.rotation;
    m_scale = m_createInfo.scale;

    RecreateSteps();
    RecomputeModelMatrix();
}

void Staircase::RecreateSteps() {
    if (m_createInfo.stepCount == 0) m_createInfo.stepCount = 1;
    CleanUp();
    m_meshNodesList.clear();

    std::vector<MeshNodeCreateInfo> meshNodeCreateInfoSet;

    MeshNodeCreateInfo& stairs = meshNodeCreateInfoSet.emplace_back();
    stairs.meshName = "Stairs";
    stairs.materialName = "Stairs";
    stairs.rigidDynamic.createObject = true;
    stairs.rigidDynamic.kinematic = true;
    stairs.rigidDynamic.shapeType = PhysicsShapeType::CONVEX_MESH;
    stairs.rigidDynamic.convexMeshModelName = "CollisionMesh_Stairs";
    stairs.rigidDynamic.filterData.raycastGroup = RAYCAST_DISABLED;
    stairs.rigidDynamic.filterData.collisionGroup = CollisionGroup::ENVIROMENT_OBSTACLE;
    stairs.rigidDynamic.filterData.collidesWith = (CollisionGroup)(GENERIC_BOUNCEABLE | ITEM_PICK_UP | BULLET_CASING | RAGDOLL_PLAYER | RAGDOLL_ENEMY);
    stairs.addtoNavMesh = true;

    for (uint32_t i = 0; i < m_createInfo.stepCount; i++) {
        MeshNodes& meshNodes = m_meshNodesList.emplace_back();
        meshNodes.Init(m_objectId, "Stairs", meshNodeCreateInfoSet);
        meshNodes.SetMeshMaterials("Stairs");
    }
}

void Staircase::SetPosition(const glm::vec3& position) {
    m_createInfo.position = position;
    m_position = position;
    RecomputeModelMatrix();
}

void Staircase::SetRotation(const glm::vec3& rotation) {
    m_createInfo.rotation = rotation;
    m_rotation = rotation;
    RecomputeModelMatrix();
}

void Staircase::SetScale(const glm::vec3& scale) {
    m_createInfo.scale = scale;
    m_scale = scale;
    RecomputeModelMatrix();
}

void Staircase::SetStepCount(uint32_t stepCount) {
    m_createInfo.stepCount = stepCount == 0 ? 1 : stepCount;
    RecreateSteps();
}

void Staircase::Update(float deltaTime) {
    m_renderItems.clear();

    for (int i = 0; i < m_meshNodesList.size(); i++) {
      
        Transform transform;
        transform.position.y = 0.4375f * i;
        transform.position.z = 0.45f * i;              

        m_meshNodesList[i].Update(m_modelMatrix * transform.to_mat4());
        const std::vector<RenderItem>& renderItems = m_meshNodesList[i].GetRenderItems();
        m_renderItems.insert(m_renderItems.end(), renderItems.begin(), renderItems.end());
    }

    //RenderDebug();
}

void Staircase::RenderDebug() {
    glm::vec3 p1 = GetPosition();
    glm::vec3 p2 = GetPosition() + (m_worldForward * 0.5f);
    DebugDraw::DrawPoint(p1, YELLOW);
    DebugDraw::DrawPoint(p2, YELLOW);
    DebugDraw::DrawLine(p1, p2, YELLOW);
}

void Staircase::CleanUp() {
    for (int i = 0; i < m_meshNodesList.size(); i++) {
        m_meshNodesList[i].CleanUp();
    }
}

void Staircase::RecomputeModelMatrix() {
    Transform transform;
    transform.position = m_position;
    transform.rotation = m_rotation;
    transform.scale = m_scale;
    m_modelMatrix = transform.to_mat4();
    m_worldForward = glm::vec3(m_modelMatrix * glm::vec4(m_localForward, 0.0f));
}
}
