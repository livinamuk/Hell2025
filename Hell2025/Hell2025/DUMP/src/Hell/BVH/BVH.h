#pragma once

#include "Hell/BVH/Types.h"
#include "Hell/BVH/Types/MeshBvh.h"
#include "Hell/BVH/Types/SceneBvh.h"

#include <cstdint>
#include <unordered_map>
#include <vector>

struct Vertex;

namespace Hell::Bvh {
    uint64_t CreateMeshBvhFromVertexData(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
    uint64_t CreateMeshBvhFromMeshBvh(MeshBvh& sourceMeshBvh);
    void DestroyMeshBvh(uint64_t meshBvhId);
    bool MeshBvhExists(uint64_t meshBvhId);
    MeshBvh* GetMeshBvhById(uint64_t meshBvhId);
    const std::unordered_map<uint64_t, MeshBvh>& GetMeshBvhs();

    uint64_t CreateSceneBvh();
    void DestroySceneBvh(uint64_t sceneBvhId);
    bool SceneBvhExists(uint64_t sceneBvhId);
    SceneBvh* GetSceneBvhById(uint64_t sceneBvhId);
    bool AddMeshBvhToSceneBvh(uint64_t sceneBvhId, uint64_t meshBvhId);
    bool AddInstanceMeshBvhsToSceneBvh(uint64_t sceneBvhId, const std::vector<PrimitiveInstance>& instances);
}
