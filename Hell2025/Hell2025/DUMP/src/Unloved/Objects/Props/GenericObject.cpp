#include "GenericObject.h"
#include "Unloved/Bible/Bible.h"
#include "Hell/Common/Constants.h"
#include "Hell/Common/Random.h"
#include "Hell/Logging.h"
#include "Unloved/Systems/Openables/OpenableManager.h"
#include "Unloved/Systems/NavMesh/NavMesh.h"
#include "Unloved/Render/Renderer.h"

namespace Unloved {

GenericObject::GenericObject(uint64_t id, const GenericObjectCreateInfo& createInfo, const SpawnOffset& spawnOffset) {
    m_createInfo = createInfo;

    // FOR THE LOVE OF SATAN REMOVE ME!!!!!!!!!
    if (m_createInfo.type == GenericObjectType::PLANT_BLACKBERRIES ||
        m_createInfo.type == GenericObjectType::PLANT_TREE) {
        m_createInfo.rotation.y = Hell::Random::Float(-HELL_PI, HELL_PI);
    }

    m_createInfo.position += spawnOffset.translation;
    m_createInfo.rotation.y += spawnOffset.yRotation;

    m_transform.position = m_createInfo.position;
    m_transform.rotation = m_createInfo.rotation;
    m_transform.scale = m_createInfo.scale;
    m_objectId = id;

    Bible::ConfigureMeshNodes(id, m_createInfo.type, &m_meshNodes);
}

void GenericObject::Update(float deltaTime) {
    m_meshNodes.Update(m_transform.to_mat4());

    if (m_navMeshTransformDirty) {
        NavMeshManager::MarkStaticDirty();
        m_navMeshTransformDirty = false;
    }
}

void GenericObject::CleanUp() {
    m_meshNodes.CleanUp();
}

void GenericObject::SetPosition(const glm::vec3& position) {
    m_createInfo.position = position;
    m_transform.position = position;
    m_navMeshTransformDirty = true;
}

void GenericObject::SetRotation(const glm::vec3& rotation) {
    m_createInfo.rotation = rotation;
    m_transform.rotation = rotation;
    m_navMeshTransformDirty = true;
}

void GenericObject::SetScale(const glm::vec3& scale) {
    m_createInfo.scale = scale;
    m_transform.scale = scale;
    m_navMeshTransformDirty = true;
}

void GenericObject::SetType(GenericObjectType type) {
    if (m_createInfo.type == type) return;

    m_createInfo.type = type;
    m_meshNodes.CleanUp();
    Bible::ConfigureMeshNodes(m_objectId, m_createInfo.type, &m_meshNodes);
    m_navMeshTransformDirty = true;
}

void GenericObject::ResetPhysics() {
    m_meshNodes.ForceDirty();
    m_meshNodes.ResetFirstFrame();
    m_meshNodes.Update(m_transform.to_mat4());
}

}
