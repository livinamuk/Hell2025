#include "PointPairObject.h"

#include "Hell/BVH/BVH.h"
#include "Hell/Geometry/Geometry.h"
#include "Hell/Logging.h"
#include "Hell/Physics/Physics.h"
#include "Hell/ResourceManagement/ResourceManager.h"

#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Render/RendererUtil.h"
#include "Unloved/Systems/House/HouseGeometryBuilder.h"
#include "Unloved/Systems/WorldBVH/WorldBVH.h"

#include <glm/gtc/quaternion.hpp>

namespace Unloved {

    PointPairObject::PointPairObject(uint64_t id, const PointPairCreateInfo& createInfo, const SpawnOffset& spawnOffset) {
        m_objectId = id;
        m_createInfo = createInfo;
        m_createInfo.position += spawnOffset.translation;
        m_createInfo.length = glm::max(m_createInfo.length, 0.01f);
        UpdateWorldPoints();
        Rebuild();
    }

    void PointPairObject::Reset() {
        for (uint64_t physicsId : m_physicsIds) Hell::Physics::MarkRigidStaticForRemoval(physicsId);

        Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("Procedural");
        for (uint32_t meshId : m_meshIds) {
            Mesh* mesh = meshBuffer.GetMeshById(meshId);
            if (mesh) Hell::Bvh::DestroyMeshBvh(mesh->meshBvhId);
            meshBuffer.RemoveMesh(meshId);
        }

        m_physicsIds.clear();
        m_meshIds.clear();
        m_renderItems.clear();
    }

    void PointPairObject::CleanUp() {
        Reset();
        m_objectId = 0;
        m_createInfo = {};
        UpdateWorldPoints();
        WorldBVH::MarkStaticSceneBvhDirty();
    }

    void PointPairObject::SetPosition(const glm::vec3& position) {
        m_createInfo.position = position;
        UpdateWorldPoints();
        Rebuild();
    }

    void PointPairObject::SetRotation(const glm::vec3& rotation) {
        m_createInfo.rotation = rotation;
        UpdateWorldPoints();
        Rebuild();
    }

    bool PointPairObject::SetPointPosition(uint32_t pointIndex, const glm::vec3& position) {
        if (pointIndex >= 2) return false;

        const glm::quat rotation = glm::quat(m_createInfo.rotation);
        const glm::vec3 currentPosition = pointIndex == 0 ? m_worldP0 : m_worldP1;
        const glm::vec3 localDelta = glm::inverse(rotation) * (position - currentPosition);
        float lengthDelta = pointIndex == 0 ? -localDelta.z : localDelta.z;
        lengthDelta = glm::max(lengthDelta, 0.01f - m_createInfo.length);

        const float endpointDelta = pointIndex == 0 ? -lengthDelta : lengthDelta;
        m_createInfo.position += rotation * glm::vec3(localDelta.x, localDelta.y, endpointDelta * 0.5f);
        m_createInfo.length += lengthDelta;
        UpdateWorldPoints();
        Rebuild();
        return true;
    }

    void PointPairObject::SetEditorName(const std::string& editorName) {
        m_createInfo.editorName = editorName;
    }

    void PointPairObject::SetCustomBool(uint32_t index, bool value) {
        if (index >= m_createInfo.customBools.size()) return;
        m_createInfo.customBools[index] = value;
        Rebuild();
    }

    void PointPairObject::SetCustomFloat(uint32_t index, float value) {
        if (index >= m_createInfo.customFloats.size()) return;
        m_createInfo.customFloats[index] = value;
        Rebuild();
    }

    void PointPairObject::SubmitRenderItems() const {
        for (const RenderItem& renderItem : m_renderItems) RenderDataManager::SubmitRenderItemProcedural(renderItem);
    }

    glm::vec3 PointPairObject::GetRight() const {
        return glm::quat(m_createInfo.rotation) * glm::vec3(1.0f, 0.0f, 0.0f);
    }

    glm::vec3 PointPairObject::GetUp() const {
        return glm::quat(m_createInfo.rotation) * glm::vec3(0.0f, 1.0f, 0.0f);
    }

    glm::vec3 PointPairObject::GetForward() const {
        return glm::quat(m_createInfo.rotation) * glm::vec3(0.0f, 0.0f, 1.0f);
    }

    float PointPairObject::GetLength() const {
        return m_createInfo.length;
    }

    void PointPairObject::UpdateWorldPoints() {
        const glm::quat rotation = glm::quat(m_createInfo.rotation);
        const glm::vec3 halfLength = glm::vec3(0.0f, 0.0f, m_createInfo.length * 0.5f);
        m_worldP0 = m_createInfo.position - rotation * halfLength;
        m_worldP1 = m_createInfo.position + rotation * halfLength;
        m_worldMatrix = glm::mat4_cast(rotation);
        m_worldMatrixP0 = glm::mat4_cast(rotation);
        m_worldMatrixP1 = glm::mat4_cast(rotation);
        m_worldMatrix[3] = glm::vec4(m_createInfo.position, 1.0f);
        m_worldMatrixP0[3] = glm::vec4(m_worldP0, 1.0f);
        m_worldMatrixP1[3] = glm::vec4(m_worldP1, 1.0f);
    }

    void PointPairObject::Rebuild() {
        Reset();

        switch (m_createInfo.type) {
            case PointPairObjectType::DECKING_BEARER: RebuildDeckingBearer(); break;
            case PointPairObjectType::DECKING_POST:   RebuildDeckingPost();   break;
            case PointPairObjectType::GUTTER:         RebuildGutter();  break;
            case PointPairObjectType::RIDGE_CAPPING:  RebuildRidgeCapping();  break;
        default: break;
        }

        WorldBVH::MarkStaticSceneBvhDirty();
    }

    void PointPairObject::RebuildRidgeCapping() {
        const HouseGeometrySourceMesh& sourceMesh = HouseGeometryBuilder::GetRidgeCappingSourceMesh();
        if (sourceMesh.vertices.empty() || sourceMesh.indices.empty()) return;

        const glm::mat4& worldMatrix = GetWorldMatrixP1();
        const glm::mat3 rotationMatrix = glm::mat3(worldMatrix);

        std::vector<Vertex> vertices = sourceMesh.vertices;
        std::vector<uint32_t> indices = sourceMesh.indices;

        for (Vertex& vertex : vertices) {
            vertex.position.z *= m_createInfo.length;
            vertex.position = glm::vec3(worldMatrix * glm::vec4(vertex.position, 1.0f));
            vertex.normal = rotationMatrix * vertex.normal;
            vertex.tangent = rotationMatrix * vertex.tangent;
            vertex.uv = Hell::Geometry::CalculateUV(vertex.position, vertex.normal);
        }

        Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("Procedural");
        const uint32_t meshId = meshBuffer.AddMesh(vertices, indices, "RidgeCapping");
        m_meshIds.push_back(meshId);

        Mesh* mesh = meshBuffer.GetMeshById(meshId);
        if (!mesh) return;
        mesh->meshBvhId = Hell::Bvh::CreateMeshBvhFromVertexData(vertices, indices);

        const int32_t materialIndex = Hell::ResourceManager::GetMaterialIndexByName("Rust");
        m_renderItems.push_back(RendererUtil::CreateProceduralGeometryRenderItem(meshId, materialIndex, m_objectId));

        PhysicsFilterData filterData;
        filterData.raycastGroup = RAYCAST_ENABLED;
        filterData.collisionGroup = CollisionGroup::ENVIROMENT_OBSTACLE;
        filterData.collidesWith = (CollisionGroup)(GENERIC_BOUNCEABLE | BULLET_CASING | RAGDOLL_PLAYER | RAGDOLL_ENEMY | CHARACTER_CONTROLLER | ITEM_PICK_UP);

        const uint64_t physicsId = Hell::Physics::CreateRigidStaticTriangleMeshFromVertexData(Hell::Transform(), vertices, indices, filterData);
        if (physicsId) m_physicsIds.push_back(physicsId);
    }

    void PointPairObject::RebuildDeckingPost() {
        const HouseGeometrySourceMesh& sourceMesh = HouseGeometryBuilder::GetCubeSourceMesh();
        if (sourceMesh.vertices.empty() || sourceMesh.indices.empty()) return;

        std::vector<Vertex> vertices = sourceMesh.vertices;
        std::vector<uint32_t> indices = sourceMesh.indices;

        const glm::mat4& worldMatrix = GetWorldMatrix();
        const glm::mat3 rotationMatrix = glm::mat3(worldMatrix);  // SEEMS SKETCHY TO HAVE SCALE BAKED INTO THIS MATRIX

        for (Vertex& vertex : vertices) {
            vertex.position.z *= m_createInfo.length;
            vertex.position = glm::vec3(worldMatrix * glm::vec4(vertex.position * glm::vec3(0.09f, 0.09f, 1.0f), 1.0f));
            vertex.normal = rotationMatrix * vertex.normal;
            vertex.tangent = rotationMatrix * vertex.tangent;
            vertex.uv = Hell::Geometry::CalculateUV(vertex.position, vertex.normal);

            const glm::vec2 uv = vertex.uv;
            vertex.uv = glm::vec2(uv.y, -uv.x);
        }

        // Create mesh
        Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("Procedural");
        const uint32_t meshId = meshBuffer.AddMesh(vertices, indices, "DeckingPost");
        m_meshIds.push_back(meshId);

        Mesh* mesh = meshBuffer.GetMeshById(meshId);
        if (!mesh) return;
        mesh->meshBvhId = Hell::Bvh::CreateMeshBvhFromVertexData(vertices, indices);

        // Create render item
        const int32_t materialIndex = Hell::ResourceManager::GetMaterialIndexByName("WoodOak");
        m_renderItems.push_back(RendererUtil::CreateProceduralGeometryRenderItem(meshId, materialIndex, m_objectId));

        // Create physics
        PhysicsFilterData filterData;
        filterData.raycastGroup = RAYCAST_ENABLED;
        filterData.collisionGroup = CollisionGroup::ENVIROMENT_OBSTACLE;
        filterData.collidesWith = (CollisionGroup)(GENERIC_BOUNCEABLE | BULLET_CASING | RAGDOLL_PLAYER | RAGDOLL_ENEMY | CHARACTER_CONTROLLER | ITEM_PICK_UP);

        const uint64_t physicsId = Hell::Physics::CreateRigidStaticTriangleMeshFromVertexData(Hell::Transform(), vertices, indices, filterData);
        if (physicsId) m_physicsIds.push_back(physicsId);
    }





    void PointPairObject::RebuildGutter() {
        Hell::Transform worldTransform;
        worldTransform.position = m_worldP1;
        worldTransform.rotation = m_createInfo.rotation;
        worldTransform.rotation.y += HELL_PI * 0.5f;

        glm::mat3 rotationMatrix = glm::mat3(worldTransform.ToMat4());

        // Gutter
        {
            const HouseGeometrySourceMesh& sourceMesh = HouseGeometryBuilder::GetGutterSourceMesh();
            if (sourceMesh.vertices.empty() || sourceMesh.indices.empty()) return;

            std::vector<Vertex> vertices = sourceMesh.vertices;
            std::vector<uint32_t> indices = sourceMesh.indices;

            for (Vertex& vertex : vertices) {
                // Stretch the far edge to be as long as the point pair
                if (vertex.position.x > 0.5f) {
                    vertex.position.x = m_createInfo.length;
                }

                // Calculate UVs in local space
                vertex.uv = Hell::Geometry::CalculateUV(vertex.position, vertex.normal);

                // Transform vertex position into world space
                vertex.position = worldTransform.ToMat4() * glm::vec4(vertex.position, 1.0f);
                vertex.normal = rotationMatrix * vertex.normal;
                vertex.tangent = rotationMatrix * vertex.tangent;
            }

            Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("Procedural");
            const uint32_t meshId = meshBuffer.AddMesh(vertices, indices, "Gutter");
            m_meshIds.push_back(meshId);

            Mesh* mesh = meshBuffer.GetMeshById(meshId);
            if (!mesh) return;
            mesh->meshBvhId = Hell::Bvh::CreateMeshBvhFromVertexData(vertices, indices);

            const int32_t materialIndex = Hell::ResourceManager::GetMaterialIndexByName("Rust");
            m_renderItems.push_back(RendererUtil::CreateProceduralGeometryRenderItem(meshId, materialIndex, m_objectId));

            PhysicsFilterData filterData;
            filterData.raycastGroup = RAYCAST_ENABLED;
            filterData.collisionGroup = CollisionGroup::ENVIROMENT_OBSTACLE;
            filterData.collidesWith = (CollisionGroup)(GENERIC_BOUNCEABLE | BULLET_CASING | RAGDOLL_PLAYER | RAGDOLL_ENEMY | CHARACTER_CONTROLLER | ITEM_PICK_UP);

            const uint64_t physicsId = Hell::Physics::CreateRigidStaticTriangleMeshFromVertexData(Hell::Transform(), vertices, indices, filterData);
            if (physicsId) m_physicsIds.push_back(physicsId);
        }



        // Fascia
        {
            const HouseGeometrySourceMesh& sourceMesh = HouseGeometryBuilder::GetGutterFasciaSourceMesh();
            if (sourceMesh.vertices.empty() || sourceMesh.indices.empty()) return;

            std::vector<Vertex> vertices = sourceMesh.vertices;
            std::vector<uint32_t> indices = sourceMesh.indices;

            for (Vertex& vertex : vertices) {
                // Stretch the far edge to be as long as the point pair
                if (vertex.position.x > 0.5f) {
                    vertex.position.x = m_createInfo.length;
                }

                // Calculate UVs in local space
                vertex.uv = Hell::Geometry::CalculateUV(vertex.position, vertex.normal);

                // Transform vertex position into world space
                vertex.position = worldTransform.ToMat4() * glm::vec4(vertex.position, 1.0f);
                vertex.normal = rotationMatrix * vertex.normal;
                vertex.tangent = rotationMatrix * vertex.tangent;
            }

            Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("Procedural");
            const uint32_t meshId = meshBuffer.AddMesh(vertices, indices, "Gutter");
            m_meshIds.push_back(meshId);

            Mesh* mesh = meshBuffer.GetMeshById(meshId);
            if (!mesh) return;
            mesh->meshBvhId = Hell::Bvh::CreateMeshBvhFromVertexData(vertices, indices);

            const int32_t materialIndex = Hell::ResourceManager::GetMaterialIndexByName("WoodOak");
            m_renderItems.push_back(RendererUtil::CreateProceduralGeometryRenderItem(meshId, materialIndex, m_objectId));

            PhysicsFilterData filterData;
            filterData.raycastGroup = RAYCAST_ENABLED;
            filterData.collisionGroup = CollisionGroup::ENVIROMENT_OBSTACLE;
            filterData.collidesWith = (CollisionGroup)(GENERIC_BOUNCEABLE | BULLET_CASING | RAGDOLL_PLAYER | RAGDOLL_ENEMY | CHARACTER_CONTROLLER | ITEM_PICK_UP);

            const uint64_t physicsId = Hell::Physics::CreateRigidStaticTriangleMeshFromVertexData(Hell::Transform(), vertices, indices, filterData);
            if (physicsId) m_physicsIds.push_back(physicsId);
        }



        // End Cap Left
        {
            const HouseGeometrySourceMesh& sourceMesh = HouseGeometryBuilder::GetGutterEndCapLeftSourceMesh();
            if (sourceMesh.vertices.empty() || sourceMesh.indices.empty()) return;

            std::vector<Vertex> vertices = sourceMesh.vertices;
            std::vector<uint32_t> indices = sourceMesh.indices;

            for (Vertex& vertex : vertices) {
                // Calculate UVs in local space
                //vertex.uv = Hell::Geometry::CalculateUV(vertex.position, vertex.normal);

                // Transform vertex position into world space
                vertex.position = worldTransform.ToMat4() * glm::vec4(vertex.position, 1.0f);
                vertex.normal = rotationMatrix * vertex.normal;
                vertex.tangent = rotationMatrix * vertex.tangent;
            }

            Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("Procedural");
            const uint32_t meshId = meshBuffer.AddMesh(vertices, indices, "Gutter");
            m_meshIds.push_back(meshId);

            Mesh* mesh = meshBuffer.GetMeshById(meshId);
            if (!mesh) return;
            mesh->meshBvhId = Hell::Bvh::CreateMeshBvhFromVertexData(vertices, indices);

            const int32_t materialIndex = Hell::ResourceManager::GetMaterialIndexByName("Rust");
            m_renderItems.push_back(RendererUtil::CreateProceduralGeometryRenderItem(meshId, materialIndex, m_objectId));

            PhysicsFilterData filterData;
            filterData.raycastGroup = RAYCAST_ENABLED;
            filterData.collisionGroup = CollisionGroup::ENVIROMENT_OBSTACLE;
            filterData.collidesWith = (CollisionGroup)(GENERIC_BOUNCEABLE | BULLET_CASING | RAGDOLL_PLAYER | RAGDOLL_ENEMY | CHARACTER_CONTROLLER | ITEM_PICK_UP);

            const uint64_t physicsId = Hell::Physics::CreateRigidStaticTriangleMeshFromVertexData(Hell::Transform(), vertices, indices, filterData);
            if (physicsId) m_physicsIds.push_back(physicsId);
        }

        // End Cap Right
        {
            const HouseGeometrySourceMesh& sourceMesh = HouseGeometryBuilder::GetGutterEndCapRightSourceMesh();
            if (sourceMesh.vertices.empty() || sourceMesh.indices.empty()) return;

            std::vector<Vertex> vertices = sourceMesh.vertices;
            std::vector<uint32_t> indices = sourceMesh.indices;

            Hell::Transform worldTransform;
            worldTransform.position = m_worldP0;
            worldTransform.rotation = m_createInfo.rotation;
            worldTransform.rotation.y += HELL_PI * 0.5f;

            glm::mat3 rotationMatrix = glm::mat3(worldTransform.ToMat4());

            for (Vertex& vertex : vertices) {
                // Calculate UVs in local space
                //vertex.uv = Hell::Geometry::CalculateUV(vertex.position, vertex.normal);

                // Transform vertex position into world space
                vertex.position = worldTransform.ToMat4() * glm::vec4(vertex.position, 1.0f);
                vertex.normal = rotationMatrix * vertex.normal;
                vertex.tangent = rotationMatrix * vertex.tangent;
            }

            Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("Procedural");
            const uint32_t meshId = meshBuffer.AddMesh(vertices, indices, "Gutter");
            m_meshIds.push_back(meshId);

            Mesh* mesh = meshBuffer.GetMeshById(meshId);
            if (!mesh) return;
            mesh->meshBvhId = Hell::Bvh::CreateMeshBvhFromVertexData(vertices, indices);

            const int32_t materialIndex = Hell::ResourceManager::GetMaterialIndexByName("Rust");
            m_renderItems.push_back(RendererUtil::CreateProceduralGeometryRenderItem(meshId, materialIndex, m_objectId));

            PhysicsFilterData filterData;
            filterData.raycastGroup = RAYCAST_ENABLED;
            filterData.collisionGroup = CollisionGroup::ENVIROMENT_OBSTACLE;
            filterData.collidesWith = (CollisionGroup)(GENERIC_BOUNCEABLE | BULLET_CASING | RAGDOLL_PLAYER | RAGDOLL_ENEMY | CHARACTER_CONTROLLER | ITEM_PICK_UP);

            const uint64_t physicsId = Hell::Physics::CreateRigidStaticTriangleMeshFromVertexData(Hell::Transform(), vertices, indices, filterData);
            if (physicsId) m_physicsIds.push_back(physicsId);
        }
    }





    void PointPairObject::RebuildDeckingBearer() {
        const HouseGeometrySourceMesh& sourceMesh = HouseGeometryBuilder::GetCubeSourceMesh();
        if (sourceMesh.vertices.empty() || sourceMesh.indices.empty()) return;

        std::vector<Vertex> vertices = sourceMesh.vertices;
        std::vector<uint32_t> indices = sourceMesh.indices;

        // SKETCHY
        const glm::mat4& worldMatrix = GetWorldMatrix();
        const glm::mat3 rotationMatrix = glm::mat3(worldMatrix); // SEEMS SKETCHY TO HAVE SCALE BAKED INTO THIS MATRIX
        // SKETCHY

        float bearerHeight = 0.150f;
        float bearerLength = m_createInfo.length;
        float bearerDepth = 0.04f;
        float boardHeight = 0.02f;

        Hell::Transform worldTransform;
        worldTransform.position = m_createInfo.position;
        worldTransform.rotation = m_createInfo.rotation;


        // First scale it in local space
        for (Vertex& vertex : vertices) {
            vertex.position *= glm::vec3(bearerDepth, bearerHeight, bearerLength);
            vertex.position.x += bearerDepth * 0.5f;
            vertex.position.y -= bearerHeight * 0.5f;
            vertex.position.y -= boardHeight;
        }

        // Now do the world space stuff
        for (Vertex& vertex : vertices) {
            vertex.position = glm::vec3(worldTransform.ToMat4() * glm::vec4(vertex.position, 1.0f));
            vertex.normal = rotationMatrix * vertex.normal;
            vertex.tangent = rotationMatrix * vertex.tangent;
            vertex.uv = Hell::Geometry::CalculateUV(vertex.position, vertex.normal);
        }

        // Create mesh
        Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("Procedural");
        const uint32_t meshId = meshBuffer.AddMesh(vertices, indices, "DeckingPost");
        m_meshIds.push_back(meshId);

        Mesh* mesh = meshBuffer.GetMeshById(meshId);
        if (!mesh) return;
        mesh->meshBvhId = Hell::Bvh::CreateMeshBvhFromVertexData(vertices, indices);

        // Create render item
        const int32_t materialIndex = Hell::ResourceManager::GetMaterialIndexByName("WoodOak");
        m_renderItems.push_back(RendererUtil::CreateProceduralGeometryRenderItem(meshId, materialIndex, m_objectId));

        // Create physics
        PhysicsFilterData filterData;
        filterData.raycastGroup = RAYCAST_ENABLED;
        filterData.collisionGroup = CollisionGroup::ENVIROMENT_OBSTACLE;
        filterData.collidesWith = (CollisionGroup)(GENERIC_BOUNCEABLE | BULLET_CASING | RAGDOLL_PLAYER | RAGDOLL_ENEMY | CHARACTER_CONTROLLER | ITEM_PICK_UP);

        const uint64_t physicsId = Hell::Physics::CreateRigidStaticTriangleMeshFromVertexData(Hell::Transform(), vertices, indices, filterData);
        if (physicsId) m_physicsIds.push_back(physicsId);
    }
}
