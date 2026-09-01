#include "Door.h"

#include "Hell/Audio.h"
#include "Hell/Logging.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/Physics/Physics.h"

#include "Legacy/World/LegacyWorld.h"

#include "Unloved/Bible/Bible.h"
#include "Unloved/Debug/DebugDraw.h"
#include "Unloved/Objects/House/WorldPlane.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Render/RendererUtil.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Systems/Openables/OpenableManager.h"
#include "Unloved/Systems/House/HouseBuilder.h"
#include "Unloved/Systems/NavMesh/NavMesh.h"
#include "Unloved/Systems/WorldBVH/WorldBVH.h"
#include "Unloved/World/World.h"

namespace Unloved {

Door::Door(uint64_t id, DoorCreateInfo& createInfo, SpawnOffset& spawnOffset) {
    m_objectId = id;
	m_createInfo = createInfo;
    m_createInfo.position += spawnOffset.translation;
    m_createInfo.rotation.y += spawnOffset.yRotation;
	m_spawnOffset = SpawnOffset();

    m_position = m_createInfo.position;
    m_rotation = m_createInfo.rotation;
    m_clippingVolume.SetOwner(ObjectType::DOOR, m_objectId);
    UpdateClippingVolume();

    // Sensible defaults if for whatever reason your map file was missing this field
    if (m_createInfo.type == DoorType::UNDEFINED)                           m_createInfo.type = DoorType::STANDARD_A;
    if (m_createInfo.materialTypeFront == DoorMaterialType::UNDEFINED)      m_createInfo.materialTypeFront = DoorMaterialType::RESIDENT_EVIL;
    if (m_createInfo.materialTypeBack == DoorMaterialType::UNDEFINED)       m_createInfo.materialTypeBack = DoorMaterialType::RESIDENT_EVIL;
    if (m_createInfo.materialTypeFrameFront == DoorMaterialType::UNDEFINED) m_createInfo.materialTypeFrameFront = DoorMaterialType::RESIDENT_EVIL;
    if (m_createInfo.materialTypeFrameBack == DoorMaterialType::UNDEFINED)  m_createInfo.materialTypeFrameBack = DoorMaterialType::RESIDENT_EVIL;

    Bible::ConfigureDoorMeshNodes(id, m_createInfo, &m_meshNodes);

    if (m_createInfo.deadLockedAtInit) {
        m_deadLocked = true;

        // Iterate the mesh nodes, find any openable ID, and lock the cunt
        for (const MeshNode& meshNode : m_meshNodes.GetNodes()) {
            if (meshNode.openableId != 0) {
                Unloved::OpenableManager::LockOpenablebyId(meshNode.openableId);
            }
        }
    }

   // DeadLock& deadLock = m_deadLocks.emplace_back();
   // deadLock.Init(m_objectId, glm::vec3(0.0f, 0.131056f, 0.0f), DeadLockType::BOLT);
   //
   // DeadLock& deadLock2 = m_deadLocks.emplace_back();
   // deadLock2.Init(m_objectId, glm::vec3(0.0f, 1.95425f, 0.0f), DeadLockType::BOLT);

    UpdateFloor();
    UpdateWorldForward();
    CreateRaytracingVertices();
    RecreateStaticAddionalRenderItems();
}

void Door::UpdateFloor() {
    float half_w = 0.05f;
    float half_d = 0.4f;

    //half_w = 0.1f;
	//half_d = 0.3f;

    Transform transform;
    transform.position = m_position;
    transform.rotation = m_rotation;

    WorldPlaneCreateInfo createInfo;
    createInfo.p0 = glm::vec3(transform.to_mat4() * glm::vec4(-half_w, 0.0f, -half_d, 1.0f));
    createInfo.p1 = glm::vec3(transform.to_mat4() * glm::vec4(-half_w, 0.0f, +half_d, 1.0f));
    createInfo.p2 = glm::vec3(transform.to_mat4() * glm::vec4(+half_w, 0.0f, +half_d, 1.0f));
    createInfo.p3 = glm::vec3(transform.to_mat4() * glm::vec4(+half_w, 0.0f, -half_d, 1.0f));
    createInfo.parentDoorId = GetObjectId();
    createInfo.type = WorldPlaneType::FLOOR;

    createInfo.textureScale = m_createInfo.floorPlaneTextureScale;
    createInfo.textureOffsetU = m_createInfo.floorPlaneTextureOffsetU;
    createInfo.textureOffsetV = m_createInfo.floorPlaneTextureOffsetV;
    createInfo.materialName = m_createInfo.floorPlaneMaterialName;
    createInfo.rotateTexture90 = m_createInfo.floorPlaneRotateTexture90;
    createInfo.roughnessFactor = m_createInfo.floorPlaneRoughnessFactor;
    createInfo.metallicFactor = m_createInfo.floorPlaneMetallicFactor;

    for (WorldPlane& worldPlane : World::GetWorldPlanes()) {
        if (worldPlane.GetParentDoorId() != GetObjectId()) continue;

        WorldPlaneCreateInfo& floorCreateInfo = worldPlane.GetCreateInfo();
        floorCreateInfo.p0 = createInfo.p0;
        floorCreateInfo.p1 = createInfo.p1;
        floorCreateInfo.p2 = createInfo.p2;
        floorCreateInfo.p3 = createInfo.p3;
        floorCreateInfo.textureScale = createInfo.textureScale;
        floorCreateInfo.textureOffsetU = createInfo.textureOffsetU;
        floorCreateInfo.textureOffsetV = createInfo.textureOffsetV;
        floorCreateInfo.rotateTexture90 = createInfo.rotateTexture90;
        floorCreateInfo.roughnessFactor = createInfo.roughnessFactor;
        floorCreateInfo.metallicFactor = createInfo.metallicFactor;
        worldPlane.SetMaterial(createInfo.materialName);
        worldPlane.UpdateVertexDataFromCreateInfo();
        WorldBVH::MarkStaticSceneBvhDirty();
        return;
    }

    Unloved::World::AddWorldPlane(createInfo, SpawnOffset());
}

void Door::CleanUp() {
	m_meshNodes.CleanUp();
}

void Door::Update(float deltaTime) {
    // TODO: cache this at init rather than recompute every frame
    Transform transform;
    transform.position = m_position;
    transform.rotation = m_rotation;

    // Store it, this is used by the deadlocks
    m_doorModelMatrix = transform.to_mat4();

    m_meshNodes.Update(m_doorModelMatrix);

    if (m_meshNodes.IsDirty()) {
        NavMeshManager::MarkDynamicDirty();
    }

    m_renderItems = m_meshNodes.GetRenderItems();

    for (DeadLock& deadLock : m_deadLocks) {
        deadLock.Update(deltaTime);
        const std::vector<RenderItem>& deadLockRenderItems = deadLock.m_meshNodes.GetRenderItems();
        m_renderItems.insert(m_renderItems.end(), deadLockRenderItems.begin(), deadLockRenderItems.end());
    }

    // Retrieve physics AABB
    bool found = false;
    for (const MeshNode& meshNode : m_meshNodes.GetNodes()) {
        if (RigidDynamic* rigidDynamic = Hell::Physics::GetRigidDynamicById(meshNode.rigidDynamicId)) {
            if (found) {
                Logging::Warning() << "There's a door with more than 1 mesh node with a rigidDynamicId\n";
            }
            m_physicsAABB = rigidDynamic->GetAABB();
            found = true;
        }
    }

    // DebugDraw();

    //CreateRaytracingVertices();
}

void Door::CreateRaytracingVertices() {
    // TODO: get this out of here and make a raytracing shadow caster mesh manager thing for anything that casts shadows
    m_raytracingDoorMesh.Reset();

    float w = DOOR_DEPTH;
    float h = DOOR_HEIGHT;
    float d = DOOR_WIDTH;

    std::vector<Vertex> vertices;
    vertices.reserve(24);

    float paddingPosX = 0.01f;
    float paddingPosY = 0.03f;
    float paddingPosZ = 0.02f;
    float paddingNegX = 0.08f;
    float paddingNegY = 0.03f;
    float paddingNegZ = 0.02f;

    // Corners
    glm::vec3 p0 = glm::vec3(0 + paddingPosX, 0 - paddingNegY, 0 + paddingPosZ); // front bottom right
    glm::vec3 p1 = glm::vec3(-w - paddingNegX, 0 - paddingNegY, 0 + paddingPosZ); // front bottom left
    glm::vec3 p2 = glm::vec3(-w - paddingNegX, h + paddingPosY, 0 + paddingPosZ); // front top left
    glm::vec3 p3 = glm::vec3(0 + paddingPosX, h + paddingPosY, 0 + paddingPosZ); // front top right
    glm::vec3 p4 = glm::vec3(0 + paddingPosX, 0 - paddingNegY, -d - paddingNegZ); // back bottom right
    glm::vec3 p5 = glm::vec3(-w - paddingNegX, 0 - paddingNegY, -d - paddingNegZ); // back bottom left
    glm::vec3 p6 = glm::vec3(-w - paddingNegX, h + paddingPosY, -d - paddingNegZ); // back top left
    glm::vec3 p7 = glm::vec3(0 + paddingPosX, h + paddingPosY, -d - paddingNegZ); // back top right

    // front face
    vertices.emplace_back(Vertex(p0, glm::vec3(0, 0, 1)));
    vertices.emplace_back(Vertex(p3, glm::vec3(0, 0, 1)));
    vertices.emplace_back(Vertex(p2, glm::vec3(0, 0, 1)));
    vertices.emplace_back(Vertex(p1, glm::vec3(0, 0, 1)));

    // back face
    vertices.emplace_back(Vertex(p5, glm::vec3(0, 0, -1)));
    vertices.emplace_back(Vertex(p6, glm::vec3(0, 0, -1)));
    vertices.emplace_back(Vertex(p7, glm::vec3(0, 0, -1)));
    vertices.emplace_back(Vertex(p4, glm::vec3(0, 0, -1)));

    // left face
    vertices.emplace_back(Vertex(p1, glm::vec3(-1, 0, 0)));
    vertices.emplace_back(Vertex(p2, glm::vec3(-1, 0, 0)));
    vertices.emplace_back(Vertex(p6, glm::vec3(-1, 0, 0)));
    vertices.emplace_back(Vertex(p5, glm::vec3(-1, 0, 0)));

    // right face
    vertices.emplace_back(Vertex(p4, glm::vec3(1, 0, 0)));
    vertices.emplace_back(Vertex(p7, glm::vec3(1, 0, 0)));
    vertices.emplace_back(Vertex(p3, glm::vec3(1, 0, 0)));
    vertices.emplace_back(Vertex(p0, glm::vec3(1, 0, 0)));

    // top face
    vertices.emplace_back(Vertex(p3, glm::vec3(0, 1, 0)));
    vertices.emplace_back(Vertex(p7, glm::vec3(0, 1, 0)));
    vertices.emplace_back(Vertex(p6, glm::vec3(0, 1, 0)));
    vertices.emplace_back(Vertex(p2, glm::vec3(0, 1, 0)));

    // bottom face
    vertices.emplace_back(Vertex(p1, glm::vec3(0, -1, 0)));
    vertices.emplace_back(Vertex(p5, glm::vec3(0, -1, 0)));
    vertices.emplace_back(Vertex(p4, glm::vec3(0, -1, 0)));
    vertices.emplace_back(Vertex(p0, glm::vec3(0, -1, 0)));

    // Indices
    std::vector<uint32_t> indices = {
        0, 1, 2, 2, 3, 0,       // front
        4, 5, 6, 6, 7, 4,       // back
        8, 9, 10, 10, 11, 8,    // left
        12, 13, 14, 14, 15, 12, // right
        16, 17, 18, 18, 19, 16, // top
        20, 21, 22, 22, 23, 20  // bottom
    };

    m_raytracingDoorMesh.AddMesh(vertices, indices, "RaytracingDoor");
    m_raytracingDoorMesh.UpdateBuffers();
}

void Door::UpdateClippingVolume() {
    float padding = 0.02f;

    Hell::Transform transform;
    transform.position = m_position;
    transform.position.y += DOOR_HEIGHT * 0.5f;
    transform.rotation = m_rotation;
    transform.scale = glm::vec3(0.2f, DOOR_HEIGHT + padding, DOOR_WIDTH + padding);

    // Hacking commences now:
    transform.position.y -= 0.02f;
    transform.scale.y += 0.02f;

    m_clippingVolume.Update(transform);
}

void Door::UpdateWorldForward() {
    Transform transform;
    transform.rotation = m_rotation;
    m_worldForward = glm::vec3(transform.to_mat4() * glm::vec4(m_localForward, 1.0f));
}

bool Door::CameraFacingDoorWorldForward(const glm::vec3& cameraPositon, const glm::vec3& cameraForward) {
    glm::vec3 toCamera = cameraPositon - GetPosition();
    glm::vec3 toDoor = GetPosition() - cameraPositon;

    bool cameraOnFrontSide = glm::dot(m_worldForward, toDoor) > 0.0f;
    bool doorInFrontOfCamera = glm::dot(cameraForward, toDoor) > 0.0f;

    return !(cameraOnFrontSide && doorInFrontOfCamera);
}

void Door::DebugDraw() {

    glm::vec4 color = GREEN;

    Unloved::Player* player = Unloved::Session::GetLocalPlayerByViewportIndex(0);
    if (!player) return;

    if (CameraFacingDoorWorldForward(player->GetCameraPosition(), player->GetCameraForward())) {
        color = GREEN;
    }
    else {
        color = RED;
    }

    glm::vec3 p1 = GetPosition();
    glm::vec3 p2 = GetPosition() + m_worldForward;
    DebugDraw::DrawLine(p1, p2, color);
    DebugDraw::DrawPoint(p1, color);
    DebugDraw::DrawPoint(p2, color);
}


void Door::SetPosition(const glm::vec3& position) {
    m_createInfo.position = position;
    m_position = position;
    UpdateClippingVolume();
    UpdateFloor();
    RecreateStaticAddionalRenderItems();
    HouseBuilder::MarkDirty();
}

void Door::SetRotation(const glm::vec3& rotation) {
    SetRotationY(rotation.y);
    RecreateStaticAddionalRenderItems();
}

void Door::SetRotationY(float value) {
    m_createInfo.rotation.y = value;
    m_rotation.y = value;
    UpdateClippingVolume();
    UpdateWorldForward();
    UpdateFloor();
    RecreateStaticAddionalRenderItems();
    HouseBuilder::MarkDirty();
}

void Door::SetEditorName(const std::string& name) {
    m_createInfo.editorName = name;
}

void Door::SetType(DoorType type) {
    m_createInfo.type = type;
    Reset();
    HouseBuilder::MarkDirty();
}

void Door::SetFrontMaterial(DoorMaterialType type) {
    m_createInfo.materialTypeFront = type;
    Reset();
}

void Door::SetBackMaterial(DoorMaterialType type) {
    m_createInfo.materialTypeBack = type;
    Reset();
}

void Door::SetFrameFrontMaterial(DoorMaterialType type) {
    m_createInfo.materialTypeFrameFront = type;
    Reset();
}

void Door::SetFrameBackMaterial(DoorMaterialType type) {
    m_createInfo.materialTypeFrameBack = type;
    Reset();
}

void Door::SetDeadLockState(bool value) {
    m_createInfo.hasDeadLock = value;
    Reset();
}

void Door::SetSillState(bool value) {
    m_createInfo.hasSill = value;
}

void Door::SetDeadLockedAtInitState(bool value) {
    m_createInfo.deadLockedAtInit = value;
    Reset();
}

void Door::SetOpenAtStartState(bool value) {
    m_createInfo.openAtStart = value;
    Reset();
}

void Door::SetMaxOpenValue(float value) {
    m_createInfo.maxOpenValue = value;
    Reset();
}

void Door::SetFloorPlaneMaterial(const std::string& materialName) {
    m_createInfo.floorPlaneMaterialName = materialName;
    UpdateFloor();
}

void Door::SetFloorPlaneTextureScale(float value) {
    m_createInfo.floorPlaneTextureScale = value;
    UpdateFloor();
}

void Door::SetFloorPlaneTextureOffsetU(float value) {
    m_createInfo.floorPlaneTextureOffsetU = value;
    UpdateFloor();
}

void Door::SetFloorPlaneTextureOffsetV(float value) {
    m_createInfo.floorPlaneTextureOffsetV = value;
    UpdateFloor();
}

void Door::SetFloorPlaneRotateTexture90(bool value) {
    m_createInfo.floorPlaneRotateTexture90 = value;
    UpdateFloor();
}

void Door::SetFloorPlaneRoughnessFactor(float value) {
    m_createInfo.floorPlaneRoughnessFactor = glm::clamp(value, 0.0f, 10.0f);
    UpdateFloor();
}

void Door::SetFloorPlaneMetallicFactor(float value) {
    m_createInfo.floorPlaneMetallicFactor = glm::clamp(value, 0.0f, 10.0f);
    UpdateFloor();
}

void Door::Reset() {
    m_additonalStaticRenderItems.clear();

    Bible::ConfigureDoorMeshNodes(m_objectId, m_createInfo, &m_meshNodes);

    m_deadLocked = m_createInfo.deadLockedAtInit;
    for (const MeshNode& meshNode : m_meshNodes.GetNodes()) {
        if (meshNode.openableId == 0) continue;

        if (m_deadLocked) OpenableManager::LockOpenablebyId(meshNode.openableId);
        else OpenableManager::UnlockOpenablebyId(meshNode.openableId);
    }

    // Push the reset pose into the newly created physics
    Update(0.0f);

    // Check logic flow, maybe there's a better spot for this
    RecreateStaticAddionalRenderItems();
}

void Door::RecreateStaticAddionalRenderItems() {
    m_additonalStaticRenderItems.clear();

    // Attention! this is a duplicate of some shit in Door::Update()
    // double check lifetime shit, and then clean this up
    //

    // Sill
    if (m_createInfo.hasSill) {
        Transform transform;
        transform.position = m_position + glm::vec3(0.0, 0.02f, 0.0f);
        transform.rotation = m_rotation + glm::vec3(0.0f, HELL_PI, 0.0f);

        m_additonalStaticRenderItems.push_back(RendererUtil::CreateAssetGeometryRenderItem("DoorSill", "DoorSill", transform.ToMat4(), Hell::ResourceManager::GetMaterialIndexByName("Brass"), m_objectId));
    }
}

}
