#include "Window.h"

#include "Hell/Physics/Physics.h"

#include "Unloved/Render/RenderDataManager.h"

#include "Unloved/Render/RendererEnums.h"
#include "Unloved/Systems/House/HouseBuilder.h"


namespace Unloved {

Window::Window(uint64_t id, const WindowCreateInfo& createInfo, const SpawnOffset& spawnOffset) {
    m_createInfo = createInfo;
    m_createInfo.position += spawnOffset.translation;
    m_createInfo.rotation.y += spawnOffset.yRotation;
    m_objectId = id;

    m_transform.position = m_createInfo.position;
    m_transform.rotation = m_createInfo.rotation;
    m_clippingVolume.SetOwner(ObjectType::WINDOW, m_objectId);
    UpdateClippingVolume();

    std::string interiorMaterial = "T_TrimInteriorRE";
    std::string exteriorMaterial = "T_TrimExteriorWP";
    std::string glassTopMaterial = "T_WindowsGlassTop";
    std::string glassBottomMaterial = "T_WindowsGlassBottom";

    std::vector<MeshNodeCreateInfo> meshNodeCreateInfoSet;

    MeshNodeCreateInfo& trimInterior = meshNodeCreateInfoSet.emplace_back();
    trimInterior.meshName = "TrimInterior";
    trimInterior.materialName = interiorMaterial;

    MeshNodeCreateInfo& trimExterior = meshNodeCreateInfoSet.emplace_back();
    trimExterior.meshName = "TrimExterior";
    trimExterior.materialName = exteriorMaterial;

    MeshNodeCreateInfo& sashTop = meshNodeCreateInfoSet.emplace_back();
    sashTop.meshName = "SashTop";
    sashTop.materialName = interiorMaterial;

    MeshNodeCreateInfo& sashBottom = meshNodeCreateInfoSet.emplace_back();
    sashBottom.meshName = "SashBottom";
    sashBottom.materialName = interiorMaterial;

    MeshNodeCreateInfo& lockTop = meshNodeCreateInfoSet.emplace_back();
    lockTop.meshName = "LockTop";
    lockTop.materialName = exteriorMaterial;

    MeshNodeCreateInfo& lockBottom = meshNodeCreateInfoSet.emplace_back();
    lockBottom.meshName = "LockBottom";
    lockBottom.materialName = exteriorMaterial;

    MeshNodeCreateInfo& handles = meshNodeCreateInfoSet.emplace_back();
    handles.meshName = "Handles";
    handles.materialName = exteriorMaterial;

    MeshNodeCreateInfo& glassTop = meshNodeCreateInfoSet.emplace_back();
    glassTop.meshName = "GlassTop";
    glassTop.materialName = glassTopMaterial;
    glassTop.blendingMode = BlendingMode::GLASS;
    glassTop.decalType = DecalType::GLASS;
    // glassTop.tintColor = glm::vec3(1.0f, 0.5, 0.0f);

    MeshNodeCreateInfo& glassBottom = meshNodeCreateInfoSet.emplace_back();
    glassBottom.meshName = "GlassBottom";
    glassBottom.materialName = glassBottomMaterial;
    glassBottom.blendingMode = BlendingMode::GLASS;
    glassBottom.decalType = DecalType::GLASS;
    // glassBottom.tintColor = glm::vec3(1.0f, 1.0, 0.0f);

    m_meshNodes.Init(m_objectId, "Window", meshNodeCreateInfoSet);
    m_meshNodes.EnableCSMShadows();
    m_meshNodes.DisableShadowsByBlendingMode(BlendingMode::GLASS);
    m_meshNodes.Update(m_transform.to_mat4());

    // Glass PhysX shapes
    //PhysicsFilterData filterData;
    //filterData.raycastGroup = RaycastGroup::RAYCAST_ENABLED;
    //filterData.collisionGroup = CollisionGroup::NO_COLLISION;
    //filterData.collidesWith = CollisionGroup::NO_COLLISION;
    //
    //filterData.collisionGroup = CollisionGroup::ENVIROMENT_OBSTACLE;
    //filterData.collidesWith = (CollisionGroup)(GENERIC_BOUNCEABLE | BULLET_CASING | RAGDOLL_PLAYER | RAGDOLL_ENEMY);
    //
    //m_physicsId = Hell::Physics::CreateRigidStaticTriangleMeshFromModel(m_transform, "WindowGlassPhysX", filterData);
    //
    //// Set PhysX user data
    //PhysicsUserData userData;
    //userData.physicsId = m_physicsId;
    //userData.objectId = m_objectId;
    //userData.physicsType = PhysicsType::RIGID_STATIC;
    //userData.objectType = ObjectType::WINDOW;
    //Hell::Physics::SetRigidStaticUserData(m_physicsId, userData);
}

void Window::CleanUp() {
    Hell::Physics::MarkRigidStaticForRemoval(m_physicsId);
}

void Window::UpdateClippingVolume() {
    Hell::Transform transform = m_transform;
    transform.position.y += 1.48f;
    transform.scale = glm::vec3(0.2f, 1.185074f, 0.85f);

    m_clippingVolume.Update(transform);
}

void Window::SetPosition(const glm::vec3& position) {
    m_createInfo.position = position;
    m_transform.position = position;
    UpdateClippingVolume();
    m_meshNodes.Update(m_transform.to_mat4());
    Hell::Physics::SetRigidStaticWorldTransform(m_physicsId, m_transform.to_mat4());
    HouseBuilder::MarkDirty();
}

void Window::SetRotation(const glm::vec3& rotation) {
    SetRotationY(rotation.y);
}

void Window::SetRotationY(float value) {
    m_createInfo.rotation.y = value;
    m_transform.rotation.y = value;
    UpdateClippingVolume();
    m_meshNodes.Update(m_transform.to_mat4());
    HouseBuilder::MarkDirty();
}

void Window::Update(float deltaTime) {
    // Nothing as of yet
}
}
