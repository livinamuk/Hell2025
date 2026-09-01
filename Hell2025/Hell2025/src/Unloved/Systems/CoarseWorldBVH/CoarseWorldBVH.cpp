#include "CoarseWorldBVH.h"
#include "CoarseWorldBVHGeometry.h"

#include "Hell/BVH/BVH.h"
#include "Hell/Math/Math.h"
#include "Hell/Render/VertexAttributes.h"

#include <cstdint>
#include <vector>

namespace Unloved::CoarseWorldBVH {
    namespace {
        std::vector<DoorProxyInstance> g_doorProxyInstances;
        std::vector<PrimitiveInstance> g_withDoorsSceneInstances;
        uint64_t g_doorProxyMeshBvhId = 0;
        uint64_t g_staticWorldMeshBvhId = 0;
        uint64_t g_withoutDoorsSceneBvhId = 0;
        uint64_t g_withDoorsSceneBvhId = 0;

        void ClearWithoutDoorsScene() {
            Hell::Bvh::DestroySceneBvh(g_withoutDoorsSceneBvhId);
            g_withoutDoorsSceneBvhId = 0;
        }

        void ClearWithDoorsScene() {
            Hell::Bvh::DestroySceneBvh(g_withDoorsSceneBvhId);
            g_withDoorsSceneBvhId = 0;
            g_doorProxyInstances.clear();
            g_withDoorsSceneInstances.clear();
        }

        void AddStaticWorldInstance(std::vector<PrimitiveInstance>& instances) {
            const MeshBvh* meshBvh = Hell::Bvh::GetMeshBvhById(g_staticWorldMeshBvhId);
            if (!meshBvh || meshBvh->m_nodes.empty()) return;

            const BvhNode& rootNode = meshBvh->m_nodes[0];
            PrimitiveInstance& instance = instances.emplace_back(PrimitiveInstance{});
            instance.meshBvhId = g_staticWorldMeshBvhId;
            instance.worldAabbBoundsMin = rootNode.boundsMin;
            instance.worldAabbBoundsMax = rootNode.boundsMax;
            instance.worldAabbCenter = (rootNode.boundsMin + rootNode.boundsMax) * 0.5f;
            instance.worldTransform = glm::mat4(1.0f);
            instance.inverseWorldTransform = glm::mat4(1.0f);
        }

        bool AnyHit(uint64_t sceneBvhId, glm::vec3 pointA, glm::vec3 pointB) {
            const glm::vec3 rayVector = pointB - pointA;
            const float rayLength = glm::length(rayVector);
            if (rayLength == 0.0f) return false;

            SceneBvh* sceneBvh = Hell::Bvh::GetSceneBvhById(sceneBvhId);
            if (!sceneBvh) return false;

            return sceneBvh->AnyHit(pointA, rayVector / rayLength, rayLength).hitFound;
        }

        BvhRayResult ClosestHit(uint64_t sceneBvhId, glm::vec3 rayOrigin, glm::vec3 rayDir, float maxRayDistance) {
            BvhRayResult rayResult;
            rayResult.distanceToHit = maxRayDistance;

            if (Hell::Math::IsNan(rayDir)) return rayResult;

            if (SceneBvh* sceneBvh = Hell::Bvh::GetSceneBvhById(sceneBvhId)) {
                return sceneBvh->ClosestHit(rayOrigin, rayDir, maxRayDistance);
            }

            return rayResult;
        }
    }

    void Init() {
        if (g_doorProxyMeshBvhId != 0) return;

        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        BuildDoorProxyMesh(vertices, indices);

        if (!vertices.empty() && !indices.empty()) {
            g_doorProxyMeshBvhId = Hell::Bvh::CreateMeshBvhFromVertexData(vertices, indices);
        }
    }

    void ClearScenes() {
        ClearWithoutDoorsScene();
        ClearWithDoorsScene();
        Hell::Bvh::DestroyMeshBvh(g_staticWorldMeshBvhId);
        g_staticWorldMeshBvhId = 0;
    }

    void CleanUp() {
        ClearScenes();
        Hell::Bvh::DestroyMeshBvh(g_doorProxyMeshBvhId);
        g_doorProxyMeshBvhId = 0;
    }

    void Rebuild() {
        ClearScenes();

        std::vector<Vertex> staticWorldVertices;
        std::vector<uint32_t> staticWorldIndices;
        BuildHouseMesh(staticWorldVertices, staticWorldIndices);

        if (!staticWorldVertices.empty() && !staticWorldIndices.empty()) {
            g_staticWorldMeshBvhId = Hell::Bvh::CreateMeshBvhFromVertexData(staticWorldVertices, staticWorldIndices);
        }

        if (g_staticWorldMeshBvhId != 0) {
            g_withoutDoorsSceneBvhId = Hell::Bvh::CreateSceneBvh();

            if (!Hell::Bvh::AddMeshBvhToSceneBvh(g_withoutDoorsSceneBvhId, g_staticWorldMeshBvhId)) {
                ClearWithoutDoorsScene();
            }
            else if (SceneBvh* sceneBvh = Hell::Bvh::GetSceneBvhById(g_withoutDoorsSceneBvhId)) {
                std::vector<PrimitiveInstance> instances;
                AddStaticWorldInstance(instances);
                sceneBvh->UpdateInstances(instances);
            }
        }

        if (g_staticWorldMeshBvhId == 0 && g_doorProxyMeshBvhId == 0) return;

        g_withDoorsSceneBvhId = Hell::Bvh::CreateSceneBvh();

        if (g_staticWorldMeshBvhId != 0 && !Hell::Bvh::AddMeshBvhToSceneBvh(g_withDoorsSceneBvhId, g_staticWorldMeshBvhId)) {
            ClearWithDoorsScene();
            return;
        }

        if (g_doorProxyMeshBvhId != 0 && !Hell::Bvh::AddMeshBvhToSceneBvh(g_withDoorsSceneBvhId, g_doorProxyMeshBvhId)) {
            ClearWithDoorsScene();
            return;
        }

        Update();
    }

    void Update() {
        SceneBvh* sceneBvh = Hell::Bvh::GetSceneBvhById(g_withDoorsSceneBvhId);
        if (!sceneBvh) return;

        if (g_doorProxyMeshBvhId != 0) {
            CollectDoorProxyInstances(g_doorProxyInstances);
        }
        else {
            g_doorProxyInstances.clear();
        }

        g_withDoorsSceneInstances.clear();
        g_withDoorsSceneInstances.reserve(g_doorProxyInstances.size() + 1);
        AddStaticWorldInstance(g_withDoorsSceneInstances);

        for (const DoorProxyInstance& doorProxyInstance : g_doorProxyInstances) {
            PrimitiveInstance& instance = g_withDoorsSceneInstances.emplace_back(PrimitiveInstance{});
            instance.objectId = doorProxyInstance.objectId;
            instance.meshBvhId = g_doorProxyMeshBvhId;
            instance.worldAabbBoundsMin = doorProxyInstance.worldAabb.GetBoundsMin();
            instance.worldAabbBoundsMax = doorProxyInstance.worldAabb.GetBoundsMax();
            instance.worldAabbCenter = doorProxyInstance.worldAabb.GetCenter();
            instance.worldTransform = doorProxyInstance.worldTransform;
            instance.inverseWorldTransform = glm::inverse(doorProxyInstance.worldTransform);
        }

        sceneBvh->UpdateInstances(g_withDoorsSceneInstances);
    }

    uint64_t GetDoorProxyBvhId() {
        return g_doorProxyMeshBvhId;
    }

    bool AnyHitWithoutDoors(glm::vec3 pointA, glm::vec3 pointB) {
        return AnyHit(g_withoutDoorsSceneBvhId, pointA, pointB);
    }

    bool AnyHitWithDoors(glm::vec3 pointA, glm::vec3 pointB) {
        return AnyHit(g_withDoorsSceneBvhId, pointA, pointB);
    }

    BvhRayResult ClosestHitWithoutDoors(glm::vec3 rayOrigin, glm::vec3 rayDir, float maxRayDistance) {
        return ClosestHit(g_withoutDoorsSceneBvhId, rayOrigin, rayDir, maxRayDistance);
    }

    BvhRayResult ClosestHitWithDoors(glm::vec3 rayOrigin, glm::vec3 rayDir, float maxRayDistance) {
        return ClosestHit(g_withDoorsSceneBvhId, rayOrigin, rayDir, maxRayDistance);
    }
}
