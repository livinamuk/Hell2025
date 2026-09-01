#include "PlanarQuadObject.h"

#include "Hell/BVH/BVH.h"
#include "Hell/Geometry/Geometry.h"
#include "Hell/Physics/Physics.h"
#include "Hell/ResourceManagement/ResourceManager.h"

#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Render/RendererUtil.h"
#include "Unloved/Systems/House/HouseGeometryBuilder.h"
#include "Unloved/Systems/WorldBVH/WorldBVH.h"

#include <glm/mat3x3.hpp>

#include <cmath>

namespace Unloved {

    void PlanarQuadObject::Rebuild() {
        Reset();

        switch (m_createInfo.type) {
            case PlanarQuadObjectType::DECKING_BOARDS: RebuildDeckingBoards(); break;
            case PlanarQuadObjectType::ROOFING_IRON:   RebuildRoofingIron(); break;
        default: break;
        }

        WorldBVH::MarkStaticSceneBvhDirty();
    }

    void PlanarQuadObject::RebuildDeckingBoards() {
        const HouseGeometrySourceMesh& sourceMesh = HouseGeometryBuilder::GetDeckingBoardsSourceMesh();
        std::vector<Vertex> vertices = sourceMesh.vertices;
        std::vector<uint32_t> indices = sourceMesh.indices;

        const glm::mat4& worldMatrix = m_planarQuad.GetWorldMatrixP1();
        const glm::mat3 rotationMatrix = glm::mat3(worldMatrix);
        const float width = m_planarQuad.GetWidth();
        const float depth = m_planarQuad.GetDepth();
        const float uvScale = m_createInfo.customFloats[0];
        const bool rotateUVs = m_createInfo.customBools[0];

        for (Vertex& vertex : vertices) {
            if (std::abs(vertex.position.x) > 0.01f) vertex.position.x = width;
            if (std::abs(vertex.position.z) > 0.01f) vertex.position.z = -depth;

            // if (std::abs(vertex.normal.y) > 0.5f) {
            //     vertex.uv.x *= width;
            //     vertex.uv.y *= depth;
            // }
            // else if (std::abs(vertex.normal.x) > 0.5f) vertex.uv.x *= depth;
            // else if (std::abs(vertex.normal.z) > 0.5f) vertex.uv.x *= width;

            vertex.uv = Hell::Geometry::CalculateUV(vertex.position, vertex.normal);

            vertex.uv *= uvScale;

            if (rotateUVs) {
                const glm::vec2 uv = vertex.uv;
                vertex.uv = glm::vec2(uv.y, -uv.x);
            }

            vertex.position = glm::vec3(worldMatrix * glm::vec4(vertex.position, 1.0f));
            vertex.normal = rotationMatrix * vertex.normal;
            vertex.tangent = rotationMatrix * vertex.tangent;
        }

        // Create mesh
        Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("Procedural");
        const uint32_t meshId = meshBuffer.AddMesh(vertices, indices, "DeckingBoards");
        m_meshIds.push_back(meshId);

        Mesh* mesh = meshBuffer.GetMeshById(meshId);
        if (!mesh) return;

        // Create BVH
        mesh->meshBvhId = Hell::Bvh::CreateMeshBvhFromVertexData(vertices, indices);

        // Create render item
        const int32_t materialIndex = Hell::ResourceManager::GetMaterialIndexByName(m_createInfo.materialNames[0]);
        const RenderItem renderItem = RendererUtil::CreateProceduralGeometryRenderItem(meshId, materialIndex, m_objectId);
        m_renderItems.push_back(renderItem);

        // Create physics object
        PhysicsFilterData filterData;
        filterData.raycastGroup = RAYCAST_ENABLED;
        filterData.collisionGroup = CollisionGroup::ENVIROMENT_OBSTACLE;
        filterData.collidesWith = (CollisionGroup)(GENERIC_BOUNCEABLE | BULLET_CASING | RAGDOLL_PLAYER | RAGDOLL_ENEMY | CHARACTER_CONTROLLER | ITEM_PICK_UP);

        uint64_t physicsId = Hell::Physics::CreateRigidStaticTriangleMeshFromVertexData(Hell::Transform(), vertices, indices, filterData);
        if (physicsId) m_physicsIds.push_back(physicsId);
    }

    //void PlanarQuadObject::RebuildGutter() {
    //    // Gutter
    //    {
    //        const HouseGeometrySourceMesh& sourceMesh = HouseGeometryBuilder::GetGutterSourceMesh();
    //        std::vector<Vertex> vertices = sourceMesh.vertices;
    //        std::vector<uint32_t> indices = sourceMesh.indices;
    //
    //        const glm::mat4& worldMatrix = m_planarQuad.GetWorldMatrixP1();
    //        const glm::mat3 rotationMatrix = glm::mat3(worldMatrix);
    //        const float width = m_planarQuad.GetWidth();
    //        const float depth = m_planarQuad.GetDepth();
    //        const float uvScale = m_createInfo.customFloats[0];
    //        const bool rotateUVs = m_createInfo.customBools[0];
    //
    //        for (Vertex& vertex : vertices) {
    //            if (std::abs(vertex.position.x) > 0.5f) {
    //                vertex.position.x = width;
    //                vertex.uv.x = vertex.position.x;
    //            }
    //
    //            vertex.position = glm::vec3(worldMatrix * glm::vec4(vertex.position, 1.0f));
    //            vertex.normal = rotationMatrix * vertex.normal;
    //            vertex.tangent = rotationMatrix * vertex.tangent;
    //        }
    //
    //        // Create mesh
    //        Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("Procedural");
    //        const uint32_t meshId = meshBuffer.AddMesh(vertices, indices, "Gutter");
    //        m_meshIds.push_back(meshId);
    //
    //        Mesh* mesh = meshBuffer.GetMeshById(meshId);
    //        if (!mesh) return;
    //
    //        // Create BVH
    //        mesh->meshBvhId = Hell::Bvh::CreateMeshBvhFromVertexData(vertices, indices);
    //
    //        // Create render item
    //        const int32_t materialIndex = Hell::ResourceManager::GetMaterialIndexByName("Brass");
    //        const RenderItem renderItem = RendererUtil::CreateProceduralGeometryRenderItem(meshId, materialIndex, m_objectId);
    //        m_renderItems.push_back(renderItem);
    //    }
    //
    //    // Awning
    //    {
    //        std::vector<Vertex> vertices;
    //        std::vector<uint32_t> indices;
    //        HouseGeometryBuilder::CreateDownFacingPlane(m_planarQuad, vertices, indices);
    //
    //        // Mesh
    //        Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("Procedural");
    //        const uint32_t meshId = meshBuffer.AddMesh(vertices, indices, "Gutter");
    //        m_meshIds.push_back(meshId);
    //
    //        Mesh* mesh = meshBuffer.GetMeshById(meshId);
    //        if (!mesh) return;
    //
    //        // Create BVH
    //        mesh->meshBvhId = Hell::Bvh::CreateMeshBvhFromVertexData(vertices, indices);
    //
    //        // Create render item
    //        const int32_t materialIndex = Hell::ResourceManager::GetMaterialIndexByName("Brass");
    //        const RenderItem renderItem = RendererUtil::CreateProceduralGeometryRenderItem(meshId, materialIndex, m_objectId);
    //        m_renderItems.push_back(renderItem);
    //    }
    //}

    void PlanarQuadObject::RebuildRoofingIron() {
        const HouseGeometrySourceMesh& sourceMesh = HouseGeometryBuilder::GetRoofingIronSourceMesh();
        std::vector<Vertex> vertices; // start empty
        std::vector<uint32_t> indices; // Start empty

        uint32_t sourceMeshVertexCount = sourceMesh.vertices.size();

        const glm::mat4& worldMatrix = m_planarQuad.GetWorldMatrixP0();
        const glm::mat3 rotationMatrix = glm::mat3(worldMatrix);
        const float width = m_planarQuad.GetWidth();
        const float depth = m_planarQuad.GetDepth();
        //const float uvScale = m_createInfo.customFloats[0];
        //const bool rotateUVs = m_createInfo.customBools[0];

        float sheetWidth = 0.85f;
        int32_t sheetCount = static_cast<int32_t>(std::ceil(depth / sheetWidth));

        float myBoardCountPerMetre = 14.0f;
        float theirBoardCountPerMetre = 24.0f;

        float uvScale = myBoardCountPerMetre / theirBoardCountPerMetre;

        for (int32_t i = 0; i < sheetCount; i++) {
            uint32_t baseVertex = i * sourceMeshVertexCount;

            for (const Vertex& sourceVertex : sourceMesh.vertices) {
                Vertex& vertex = vertices.emplace_back(sourceVertex);

                // Stretch it on x
                if (vertex.position.x > 0.5f) {
                    vertex.position.x = width;
                }

                // Physically move it on z
                vertex.position.z += sheetWidth * i;

                vertex.uv.x = vertex.position.z * uvScale;
                vertex.uv.y = vertex.position.x * uvScale;
            }

            for (uint32_t index : sourceMesh.indices) {
                indices.push_back(index + baseVertex);
            }
        }

        // Now physically remove with force any triangles that are larger than the depth of your planar quad
        for (size_t i = 0; i < indices.size(); i += 3) {

            Vertex& v0 = vertices[indices[i + 0]];
            Vertex& v1 = vertices[indices[i + 1]];
            Vertex& v2 = vertices[indices[i + 2]];

            // If any of these have a z pos, greater than your planar quad depth then remove the whole tri
            if (v0.position.z > depth || v1.position.z > depth || v2.position.z > depth) {
                indices.erase(indices.begin() + i);
                indices.erase(indices.begin() + i);
                indices.erase(indices.begin() + i);
                i -= 3;
            }
        }

        // Now move that mesh from local space into world space
        for (Vertex& vertex : vertices) {
            vertex.position = glm::vec3(worldMatrix * glm::vec4(vertex.position, 1.0f));
            vertex.normal = rotationMatrix * vertex.normal;
            vertex.tangent = rotationMatrix * vertex.tangent;
        }

        // Create mesh
        Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("Procedural");
        const uint32_t meshId = meshBuffer.AddMesh(vertices, indices, "RoofingIron");
        m_meshIds.push_back(meshId);

        Mesh* mesh = meshBuffer.GetMeshById(meshId);
        if (!mesh) return;

        // Create BVH
        mesh->meshBvhId = Hell::Bvh::CreateMeshBvhFromVertexData(vertices, indices);

        // Create render item
        const int32_t materialIndex = Hell::ResourceManager::GetMaterialIndexByName("CorrugatedRoofing");
        const RenderItem renderItem = RendererUtil::CreateProceduralGeometryRenderItem(meshId, materialIndex, m_objectId);
        m_renderItems.push_back(renderItem);

        // Create physics object
        PhysicsFilterData filterData;
        filterData.raycastGroup = RAYCAST_ENABLED;
        filterData.collisionGroup = CollisionGroup::ENVIROMENT_OBSTACLE;
        filterData.collidesWith = (CollisionGroup)(GENERIC_BOUNCEABLE | BULLET_CASING | RAGDOLL_PLAYER | RAGDOLL_ENEMY | CHARACTER_CONTROLLER | ITEM_PICK_UP);

        uint64_t physicsId = Hell::Physics::CreateRigidStaticTriangleMeshFromVertexData(Hell::Transform(), vertices, indices, filterData);
        if (physicsId) m_physicsIds.push_back(physicsId);


        // Now create left flashing
        {
            const HouseGeometrySourceMesh& flashingSourceMesh = HouseGeometryBuilder::GetRoofingFlashingLeftSourceMesh();
            std::vector<Vertex> vertices = flashingSourceMesh.vertices;
            std::vector<uint32_t> indices = flashingSourceMesh.indices;

            for (Vertex& vertex : vertices) {

                // Stretch it
                if (vertex.position.x > 0.5f) {
                    vertex.position.x = width;
                }

                // Make uvs in local space
                vertex.uv = Hell::Geometry::CalculateUV(vertex.position, vertex.normal);

                // Transform into world space
                vertex.position = glm::vec3(m_planarQuad.GetWorldMatrixP0() * glm::vec4(vertex.position, 1.0f));
                vertex.normal = rotationMatrix * vertex.normal;
                vertex.tangent = rotationMatrix * vertex.tangent;
            }

            // Create mesh
            Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("Procedural");
            const uint32_t meshId = meshBuffer.AddMesh(vertices, indices, "RoofingFlasingLeft");
            m_meshIds.push_back(meshId);

            Mesh* mesh = meshBuffer.GetMeshById(meshId);
            if (!mesh) return;

            // Create BVH
            mesh->meshBvhId = Hell::Bvh::CreateMeshBvhFromVertexData(vertices, indices);

            // Create render item
            const int32_t materialIndex = Hell::ResourceManager::GetMaterialIndexByName("Rust");
            const RenderItem renderItem = RendererUtil::CreateProceduralGeometryRenderItem(meshId, materialIndex, m_objectId);
            m_renderItems.push_back(renderItem);
        }

        // Now create right flashing
        {
            const HouseGeometrySourceMesh& flashingSourceMesh = HouseGeometryBuilder::GetRoofingFlashingRightSourceMesh();
            std::vector<Vertex> vertices = flashingSourceMesh.vertices;
            std::vector<uint32_t> indices = flashingSourceMesh.indices;

            for (Vertex& vertex : vertices) {

                // Stretch it
                if (vertex.position.x > 0.5f) {
                    vertex.position.x = width;
                }

                // Make uvs in local space
                vertex.uv = Hell::Geometry::CalculateUV(vertex.position, vertex.normal);

                // Transform into world space
                vertex.position = glm::vec3(m_planarQuad.GetWorldMatrixP1() * glm::vec4(vertex.position, 1.0f));
                vertex.normal = rotationMatrix * vertex.normal;
                vertex.tangent = rotationMatrix * vertex.tangent;
            }

            // Create mesh
            Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("Procedural");
            const uint32_t meshId = meshBuffer.AddMesh(vertices, indices, "RoofingFlasingRight");
            m_meshIds.push_back(meshId);

            Mesh* mesh = meshBuffer.GetMeshById(meshId);
            if (!mesh) return;

            // Create BVH
            mesh->meshBvhId = Hell::Bvh::CreateMeshBvhFromVertexData(vertices, indices);

            // Create render item
            const int32_t materialIndex = Hell::ResourceManager::GetMaterialIndexByName("Rust");
            const RenderItem renderItem = RendererUtil::CreateProceduralGeometryRenderItem(meshId, materialIndex, m_objectId);
            m_renderItems.push_back(renderItem);
        }

        // Eaves lining
        {
            std::vector<Vertex> vertices;
            std::vector<uint32_t> indices;
            Unloved::HouseGeometryBuilder::CreateDownFacingPlane(m_planarQuad, vertices, indices);

            for (Vertex& vertex : vertices) {
                vertex.position += m_planarQuad.GetUp() * -0.03f;
            }

            // Create mesh
            Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("Procedural");
            const uint32_t meshId = meshBuffer.AddMesh(vertices, indices, "EavesLining");
            m_meshIds.push_back(meshId);

            Mesh* mesh = meshBuffer.GetMeshById(meshId);
            if (!mesh) return;

            // Create BVH
            mesh->meshBvhId = Hell::Bvh::CreateMeshBvhFromVertexData(vertices, indices);

            // Create render item
            const int32_t materialIndex = Hell::ResourceManager::GetMaterialIndexByName("Rust");
            const RenderItem renderItem = RendererUtil::CreateProceduralGeometryRenderItem(meshId, materialIndex, m_objectId);
            m_renderItems.push_back(renderItem);

        }
    }

}
