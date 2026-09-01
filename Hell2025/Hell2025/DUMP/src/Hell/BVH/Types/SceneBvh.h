#pragma once

#include "Hell/BVH/Types.h"

#include <cstdint>
#include <unordered_map>
#include <vector>

struct MeshBvh;

struct SceneBvhMeshInput {
    uint64_t meshBvhId = 0;
    const MeshBvh* meshBvh = nullptr;
};

struct SceneBvh {
    std::vector<BvhNode> m_nodes;
    std::vector<PrimitiveInstance> m_instances;
    std::vector<GpuPrimitiveInstance> m_gpuInstances;

    std::vector<uint64_t> m_meshBvhIds;
    std::unordered_map<uint64_t, uint32_t> m_meshRootNodeOffsets;
    std::vector<BvhNode> m_meshNodes;
    std::vector<BVHTriangle> m_triangles;
    std::vector<uint32_t> m_leafInstanceIndices;
    std::vector<uint32_t> m_refitNodeOrder;

    bool SetMeshBvhs(const std::vector<SceneBvhMeshInput>& meshBvhs);
    bool AddMeshBvh(uint64_t meshBvhId, const MeshBvh* meshBvh);
    bool AddMeshBvhs(const std::vector<SceneBvhMeshInput>& meshBvhs);
    bool UpdateInstances(const std::vector<PrimitiveInstance>& instances);
    bool RebuildInstances(const std::vector<PrimitiveInstance>& instances);
    bool RefitInstances(const std::vector<PrimitiveInstance>& instances);
    bool HasMeshBvh(uint64_t meshBvhId) const;
    BvhRayResult AnyHit(glm::vec3 rayOrigin, glm::vec3 rayDir, float maxDistance) const;
    BvhRayResult ClosestHit(glm::vec3 rayOrigin, glm::vec3 rayDir, float maxDistance) const;

private:
    bool BuildRefitNodeOrder();
    bool CanRefitInstances(const std::vector<PrimitiveInstance>& instances) const;
};
