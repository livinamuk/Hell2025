#include "WallSegment.h"

#include "Hell/Geometry/Geometry.h"
#include "Hell/Physics/Physics.h"
#include "Hell/ResourceManagement/ResourceManager.h"

#include "Unloved/Common/Types.h"
#include "Unloved/ObjectId.h"
#include "Unloved/Systems/House/HouseClipping.h"

#include "Unloved/Render/Renderer.h"

namespace Unloved {

void WallSegment::Init(glm::vec3 start, glm::vec3 end, float startHeight, float endHeight, uint64_t parentObjectId, const SpawnOffset& spawnOffset) {
    m_start = start;
    m_end = end;
    m_startHeight = startHeight;
    m_endHeight = endHeight;
    m_spawnOffset = spawnOffset;

    // Normal
    glm::vec3 dir = glm::normalize(m_end - m_start);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    m_normal = glm::normalize(glm::cross(dir, up));

    // Corners
    m_corners = {
        m_start,
        m_start + glm::vec3(0.0f, m_startHeight, 0.0f),
        m_end + glm::vec3(0.0f, m_endHeight, 0.0f),
        m_end
    };

    // AABB
    m_aabb = AABB(m_corners);

    // Store parent id
    m_parentObjectId = parentObjectId;
}

void WallSegment::SetMeshId(uint32_t meshId) {
    m_meshId = meshId;
}

void WallSegment::CleanUp() {
    Hell::Physics::MarkRigidStaticForRemoval(m_physicsId);

    Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("Procedural");
    meshBuffer.RemoveMesh(m_meshId);
}

void WallSegment::CreateVertexData(const std::vector<const ClippingVolume*>& clippingVolumes, float texOffsetX, float texOffsetY, float texScale) {
    m_vertices.clear();
    m_indices.clear();

    // Clip the volumes from the wall
    HouseClipping::SubtractClippingVolumesFromWallSegment(*this, clippingVolumes, m_vertices, m_indices);

    for (Vertex& vertex : m_vertices) {
        glm::vec3 origin = glm::vec3(0, 0, 0);

        // Correct any errors introduced by Clipper used integers the under hood
        float threshold = 0.01f;
        for (const glm::vec3& originalPosition : m_corners) {
            if (std::abs(vertex.position.x - originalPosition.x) < threshold) {
                vertex.position.x = originalPosition.x;
            }
            if (std::abs(vertex.position.y - originalPosition.y) < threshold) {
                vertex.position.y = originalPosition.y;
            }
            if (std::abs(vertex.position.z - originalPosition.z) < threshold) {
                vertex.position.z = originalPosition.z;
            }
        }

        // Update UVs
        origin = glm::vec3(0);
        vertex.uv = Hell::Geometry::CalculateUV(vertex.position - m_spawnOffset.translation, m_normal);
        vertex.uv *= texScale;
        vertex.uv.x += texOffsetX;
        vertex.uv.y += texOffsetY;
    }

    // Update normals and tangents
    for (int i = 0; i < m_indices.size(); i += 3) {
        Vertex& v0 = m_vertices[m_indices[i + 0]];
        Vertex& v1 = m_vertices[m_indices[i + 1]];
        Vertex& v2 = m_vertices[m_indices[i + 2]];
        Hell::Geometry::SetNormalsAndTangentsFromVertices(v0, v1, v2);
    }

    CreatePhysicsObject();
}

void WallSegment::CreatePhysicsObject() {
    Hell::Physics::MarkRigidStaticForRemoval(m_physicsId);
    m_physicsId = 0;
    m_objectId = Unloved::GetNextObjectId(ObjectType::WALL_SEGMENT);

    glm::vec3 horizontalDelta = m_end - m_start;
    horizontalDelta.y = 0.0f;
    if (glm::length(horizontalDelta) < 0.001f) return;

    PhysicsFilterData filterData;
    filterData.raycastGroup = RAYCAST_ENABLED;
    filterData.collisionGroup = CollisionGroup::ENVIROMENT_OBSTACLE;
    filterData.collidesWith = (CollisionGroup)(GENERIC_BOUNCEABLE | BULLET_CASING | RAGDOLL_PLAYER | RAGDOLL_ENEMY | CHARACTER_CONTROLLER | ITEM_PICK_UP);

    m_physicsId = Hell::Physics::CreateRigidStaticTriangleMeshFromVertexData(Transform(), m_vertices, m_indices, filterData);
    if (m_physicsId == 0) return;

    // Set PhysX user data
    PhysicsUserData userData;
    userData.physicsId = m_physicsId;
    userData.objectId = m_objectId;
    userData.physicsType = PhysicsType::RIGID_STATIC;
    //userData.objectType = ObjectType::WALL_SEGMENT;
    Hell::Physics::SetRigidStaticUserData(m_physicsId, userData);
}
}
