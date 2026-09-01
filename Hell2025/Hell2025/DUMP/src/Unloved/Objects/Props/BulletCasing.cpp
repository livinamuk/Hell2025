#include "BulletCasing.h"

#include "Hell/Audio.h"
#include "Hell/Common/Random.h"
#include "Hell/Logging.h"
#include "Hell/Physics/Physics.h"
#include "Hell/ResourceManagement/ResourceManager.h"

#include "Unloved/Render/RendererConstants.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Render/RendererUtil.h"

namespace Unloved {

BulletCasing::BulletCasing(uint64_t id, BulletCasingCreateInfo createInfo) {
    m_createInfo = createInfo;
    m_objectId = id;
    m_materialIndex = m_createInfo.materialIndex;

    // Get model
    Model* model = Hell::ResourceManager::GetModelById(m_createInfo.modelId);
    if (!model) {
        Logging::Error() << "BulletCasing(BulletCasingCreateInfo createInfo) failed from invalid model\n";
        return;
    }
    if (model->GetMeshCount() < 1) {
        Logging::Error() << "BulletCasing(BulletCasingCreateInfo createInfo) failed from mesh count 0\n";
    }

    // Get mesh
    m_meshId = model->GetMeshIndices()[0];
    Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(m_meshId);
    if (!mesh) {
        Logging::Error() << "BulletCasing(BulletCasingCreateInfo createInfo) failed from invalid mesh\n";
        return;
    }

    Transform transform;
    transform.position = m_createInfo.position;
    transform.rotation = m_createInfo.rotation;

    m_renderItem.modelMatrix = transform.ToMat4();
    m_renderItem.prevModelMatrix = transform.ToMat4();
    m_renderItem.baseVertex = mesh->baseVertex;
    m_renderItem.baseIndex = mesh->baseIndex;
    m_renderItem.vertexCount = mesh->vertexCount;
    m_renderItem.indexCount = mesh->indexCount;
    m_renderItem.materialIndex = GetMaterialIndex();
    m_renderItem.meshId = GetMeshId();
    m_renderItem.shadowFlags = SHADOW_FLAG_NONE;
    m_renderItem.vulkanFlags = 0;

    PhysicsFilterData filterData;
    filterData.raycastGroup = RaycastGroup::RAYCAST_DISABLED;
    filterData.collisionGroup = CollisionGroup::BULLET_CASING;
    filterData.collidesWith = CollisionGroup::ENVIROMENT_OBSTACLE;

    glm::vec3 force = m_createInfo.force;
    glm::vec3 torque = glm::vec3(Hell::Random::Float(-10.0f, 10.0f), Hell::Random::Float(-10.0f, 10.0f), Hell::Random::Float(-10.0f, 10.0f));

    m_rigidDynamicId = Hell::Physics::CreateRigidDynamicFromBoxExtents(transform, mesh->extents, m_createInfo.mass, filterData, force, torque);
}

void BulletCasing::CleanUp() {
    Hell::Physics::MarkRigidDynamicForRemoval(m_rigidDynamicId);
}

const glm::mat4& BulletCasing::GetModelMatrix() {
    return m_modelMatrix;
}

void BulletCasing::Update(float deltaTime) {
    m_lifeTime += deltaTime;

    float maxLifeTime = 5.0f;
    if (m_lifeTime > maxLifeTime) {
        Hell::Physics::MarkRigidDynamicForRemoval(m_rigidDynamicId);
    }

    if (Hell::Physics::RigidDynamicExists(m_rigidDynamicId)) {
        m_modelMatrix = Hell::Physics::GetRigidDynamicWorldMatrix(m_rigidDynamicId);
    }
}

void BulletCasing::SubmitRenderItem() {
    m_renderItem.prevModelMatrix = m_renderItem.modelMatrix;
    m_renderItem.modelMatrix = GetModelMatrix();
    m_renderItem.inverseModelMatrix = glm::inverse(m_renderItem.modelMatrix);

    RendererUtil::UpdateRenderItemAABB(m_renderItem);
    RenderDataManager::SubmitRenderItem(m_renderItem);
}

}
