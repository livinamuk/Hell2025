#include "WorldPlane.h"
#include "Hell/Common/Bit.h"
#include "Hell/Geometry/Geometry.h"
#include "Unloved/Debug/DebugDraw.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/Physics/Physics.h"
#include "Hell/Transform.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Render/RendererConstants.h"
#include "Legacy/World/LegacyWorld.h"
#include "Unloved/Systems/House/HouseBuilder.h"

namespace Unloved {

WorldPlane::WorldPlane(uint64_t id, const WorldPlaneCreateInfo& createInfo, const SpawnOffset& spawnOffset) {
    m_objectId = id;

    m_createInfo = createInfo;

    m_createInfo.p0 += spawnOffset.translation; // is this correct/safe?
    m_createInfo.p1 += spawnOffset.translation; // is this correct/safe?
    m_createInfo.p2 += spawnOffset.translation; // is this correct/safe?
    m_createInfo.p3 += spawnOffset.translation; // is this correct/safe?

    m_materialIndex = Hell::ResourceManager::GetMaterialIndexByName(m_createInfo.materialName);

    UpdateVertexDataFromCreateInfo();
}

void WorldPlane::UpdateVertexDataFromCreateInfo() {
    const std::array<glm::vec3, 4> createInfoPoints = { m_createInfo.p0, m_createInfo.p1, m_createInfo.p2, m_createInfo.p3 };
    bool pointsChanged = !m_planarQuad.IsValid();
    if (!pointsChanged) {
        const std::array<glm::vec3, 4>& points = m_planarQuad.GetPoints();
        for (uint32_t i = 0; i < points.size(); i++) {
            if (createInfoPoints[i].x != points[i].x || createInfoPoints[i].y != points[i].y || createInfoPoints[i].z != points[i].z) pointsChanged = true;
        }
    }
    if (pointsChanged) m_planarQuad.SetPoints(createInfoPoints);
    if (!m_planarQuad.IsValid()) return;

    SyncCreateInfoFromPlanarQuad();
    const std::array<glm::vec3, 4>& points = m_planarQuad.GetPoints();

    // Vertices
    m_vertices.resize(4);
    for (size_t i = 0; i < points.size(); i++) m_vertices[i].position = points[i];

    // Indices
    m_indices = { 0, 1, 2, 2, 3, 0 };

    // Update UVs
    for (Vertex& vertex : m_vertices) {
        vertex.uv = Hell::Geometry::CalculateUV(vertex.position, points[0], m_planarQuad.GetRight(), m_planarQuad.GetForward());
        if (m_createInfo.rotateTexture90) vertex.uv = glm::vec2(vertex.uv.y, -vertex.uv.x);
        vertex.uv *= m_createInfo.textureScale;
        vertex.uv.x += m_createInfo.textureOffsetU;
        vertex.uv.y += m_createInfo.textureOffsetV;
    }

    // Update normals and tangents
    for (int i = 0; i < m_indices.size(); i += 3) {
        Vertex& v0 = m_vertices[m_indices[i + 0]];
        Vertex& v1 = m_vertices[m_indices[i + 1]];
        Vertex& v2 = m_vertices[m_indices[i + 2]];
        Hell::Geometry::SetNormalsAndTangentsFromVertices(v0, v1, v2);
    }

    Hell::Physics::MarkRigidStaticForRemoval(m_physicsId);
    CreatePhysicsObject();

    // Calculate worldspace center
    m_worldSpaceCenter = m_planarQuad.GetCenter();

    // Nav mesh poly
    m_navMeshPoly.clear();
    m_navMeshPoly.reserve(4);
    for (const glm::vec3& point : points) m_navMeshPoly.emplace_back(point.x, point.z);

    HouseBuilder::MarkDirty();
}

void WorldPlane::CleanUp() {
    Hell::Physics::MarkRigidStaticForRemoval(m_physicsId);
    m_vertices.clear();
    m_indices.clear();
    m_objectId = 0;
    m_physicsId = 0;
    m_planarQuad = {};
    m_materialIndex = -1;

    Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("Procedural");
    meshBuffer.RemoveMesh(m_meshId);
}

void WorldPlane::SetPosition(const glm::vec3& position) {
    UpdateWorldSpaceCenter(position);
}

bool WorldPlane::SetPointPosition(uint32_t pointIndex, const glm::vec3& position) {
    if (!m_planarQuad.SetPointPosition(pointIndex, position)) return false;

    SyncCreateInfoFromPlanarQuad();
    UpdateVertexDataFromCreateInfo();
    return true;
}

bool WorldPlane::SetRotation(const glm::vec3& rotation) {
    if (!m_planarQuad.SetRotation(rotation)) return false;

    SyncCreateInfoFromPlanarQuad();
    UpdateVertexDataFromCreateInfo();
    return true;
}

void WorldPlane::UpdateWorldSpaceCenter(glm::vec3 worldSpaceCenter) {
    const glm::vec3 offset = worldSpaceCenter - m_worldSpaceCenter;
    m_planarQuad.Translate(offset);
    SyncCreateInfoFromPlanarQuad();
    UpdateVertexDataFromCreateInfo();
}

void WorldPlane::SetMaterial(const std::string& materialName) {
    m_createInfo.materialName = materialName;
    m_materialIndex = Hell::ResourceManager::GetMaterialIndexByName(materialName);
}

Material* WorldPlane::GetMaterial() {
    return Hell::ResourceManager::GetMaterialByIndex(m_materialIndex);
}

void WorldPlane::SetMeshId(uint32_t meshId) {
    m_meshId = meshId;
}

void WorldPlane::SetTextureScale(float value) {
    m_createInfo.textureScale = value;
    UpdateVertexDataFromCreateInfo();
}

void WorldPlane::SetTextureOffsetU(float value) {
    m_createInfo.textureOffsetU = value;
    UpdateVertexDataFromCreateInfo();
}

void WorldPlane::SetTextureOffsetV(float value) {
    m_createInfo.textureOffsetV = value;
    UpdateVertexDataFromCreateInfo();
}

void WorldPlane::SetRotateTexture90(bool value) {
    m_createInfo.rotateTexture90 = value;
    UpdateVertexDataFromCreateInfo();
}

void WorldPlane::SetRoughnessFactor(float value) {
    m_createInfo.roughnessFactor = glm::clamp(value, 0.0f, 10.0f);
    HouseBuilder::MarkDirty();
}

void WorldPlane::SetMetallicFactor(float value) {
    m_createInfo.metallicFactor = glm::clamp(value, 0.0f, 10.0f);
    HouseBuilder::MarkDirty();
}

void WorldPlane::CreatePhysicsObject() {
    Hell::Physics::MarkRigidStaticForRemoval(m_physicsId);

    PhysicsFilterData filterData;
    filterData.raycastGroup = RAYCAST_ENABLED;
    filterData.collisionGroup = CollisionGroup::ENVIROMENT_OBSTACLE;
    filterData.collidesWith = (CollisionGroup)(GENERIC_BOUNCEABLE | BULLET_CASING | RAGDOLL_PLAYER | RAGDOLL_ENEMY | CHARACTER_CONTROLLER | ITEM_PICK_UP);

    m_physicsId = Hell::Physics::CreateRigidStaticTriangleMeshFromVertexData(Hell::Transform(), m_vertices, m_indices, filterData);
    if (m_physicsId == 0) return;

    // Set PhysX user data
    PhysicsUserData userData;
    userData.physicsId = m_physicsId;
    userData.objectId = m_objectId;
    userData.physicsType = PhysicsType::RIGID_STATIC;
    //userData.objectType = ObjectType::WORLD_PLANE;
    Hell::Physics::SetRigidStaticUserData(m_physicsId, userData);
}

void WorldPlane::SubmitRenderItem() {
    Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("Procedural");

    Mesh* mesh = meshBuffer.GetMeshById(m_meshId);
    if (!mesh) return;

    Material* material = Hell::ResourceManager::GetMaterialByIndex(m_materialIndex);
    if (!material) return;

	RenderItem renderItem;
    renderItem.materialIndex = m_materialIndex;
    renderItem.roughnessFactor = m_createInfo.roughnessFactor;
    renderItem.metallicFactor = m_createInfo.metallicFactor;
	renderItem.modelMatrix = glm::mat4(1.0f);
	renderItem.inverseModelMatrix = glm::mat4(1.0f);
	renderItem.aabbMin = glm::vec4(mesh->aabbMin, 0.0f);
	renderItem.aabbMax = glm::vec4(mesh->aabbMax, 0.0f);
    renderItem.meshId = m_meshId;
    renderItem.vertexCount = mesh->vertexCount;
    renderItem.indexCount = mesh->indexCount;
    renderItem.baseVertex = mesh->baseVertex;
    renderItem.baseIndex = mesh->baseIndex;
    renderItem.shadowFlags |= (SHADOW_FLAG_POINT_LIGHT | SHADOW_FLAG_CSM);
    Hell::Bit::PackUint64(m_objectId, renderItem.objectIdLowerBit, renderItem.objectIdUpperBit);

	RenderDataManager::SubmitRenderItemProcedural(renderItem);
}

void WorldPlane::DrawVertices(glm::vec4 color) {
    for (const glm::vec3& point : m_planarQuad.GetPoints()) DebugDraw::DrawPoint(point, color);
}

void WorldPlane::DrawEdges(glm::vec4 color) {
    const std::array<glm::vec3, 4>& points = m_planarQuad.GetPoints();
    for (size_t i = 0; i < points.size(); i++) DebugDraw::DrawLine(points[i], points[(i + 1) % points.size()], color);
}

void WorldPlane::HideInEditor() {
    m_hiddenInEditor = true;
}

void WorldPlane::UnhideInEditor() {
	m_hiddenInEditor = false;
}

void WorldPlane::SyncCreateInfoFromPlanarQuad() {
    const std::array<glm::vec3, 4>& points = m_planarQuad.GetPoints();
    m_createInfo.p0 = points[0];
    m_createInfo.p1 = points[1];
    m_createInfo.p2 = points[2];
    m_createInfo.p3 = points[3];
}
}
