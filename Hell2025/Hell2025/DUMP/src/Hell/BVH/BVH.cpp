#include "BVH.h"

#include <unordered_map>

namespace Hell::Bvh {
    namespace {
        uint64_t g_nextBvhId = 1;
        std::unordered_map<uint64_t, MeshBvh> g_meshBvhs;
        std::unordered_map<uint64_t, SceneBvh> g_sceneBvhs;
    }

    uint64_t CreateMeshBvhFromMeshBvh(MeshBvh& sourceMeshBvh) {
        const uint64_t meshBvhId = g_nextBvhId++;

        MeshBvh& targetMeshBvh = g_meshBvhs[meshBvhId];
        targetMeshBvh.m_nodes.swap(sourceMeshBvh.m_nodes);
        targetMeshBvh.m_triangles.swap(sourceMeshBvh.m_triangles);

        return meshBvhId;
    }

    uint64_t CreateMeshBvhFromVertexData(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices) {
        const uint64_t meshBvhId = g_nextBvhId++;
        g_meshBvhs[meshBvhId] = BuildMeshBvh(vertices, indices);
        return meshBvhId;
    }

    void DestroyMeshBvh(uint64_t meshBvhId) {
        auto it = g_meshBvhs.find(meshBvhId);
        if (it != g_meshBvhs.end()) {
            g_meshBvhs.erase(it);
        }
    }

    bool MeshBvhExists(uint64_t meshBvhId) {
        return g_meshBvhs.find(meshBvhId) != g_meshBvhs.end();
    }

    MeshBvh* GetMeshBvhById(uint64_t meshBvhId) {
        auto it = g_meshBvhs.find(meshBvhId);
        if (it == g_meshBvhs.end()) return nullptr;
        return &it->second;
    }

    const std::unordered_map<uint64_t, MeshBvh>& GetMeshBvhs() {
        return g_meshBvhs;
    }

    uint64_t CreateSceneBvh() {
        const uint64_t sceneBvhId = g_nextBvhId++;
        g_sceneBvhs[sceneBvhId] = SceneBvh();
        return sceneBvhId;
    }

    void DestroySceneBvh(uint64_t sceneBvhId) {
        auto it = g_sceneBvhs.find(sceneBvhId);
        if (it != g_sceneBvhs.end()) {
            g_sceneBvhs.erase(it);
        }
    }

    bool SceneBvhExists(uint64_t sceneBvhId) {
        return g_sceneBvhs.find(sceneBvhId) != g_sceneBvhs.end();
    }

    SceneBvh* GetSceneBvhById(uint64_t sceneBvhId) {
        auto it = g_sceneBvhs.find(sceneBvhId);
        if (it == g_sceneBvhs.end()) return nullptr;
        return &it->second;
    }

    bool AddMeshBvhToSceneBvh(uint64_t sceneBvhId, uint64_t meshBvhId) {
        SceneBvh* sceneBvh = GetSceneBvhById(sceneBvhId);
        if (!sceneBvh) return false;

        if (sceneBvh->HasMeshBvh(meshBvhId)) {
            return true;
        }

        return sceneBvh->AddMeshBvh(meshBvhId, GetMeshBvhById(meshBvhId));
    }

    bool AddInstanceMeshBvhsToSceneBvh(uint64_t sceneBvhId, const std::vector<PrimitiveInstance>& instances) {
        SceneBvh* sceneBvh = GetSceneBvhById(sceneBvhId);
        if (!sceneBvh) return false;

        for (const PrimitiveInstance& instance : instances) {
            if (sceneBvh->HasMeshBvh(instance.meshBvhId)) {
                continue;
            }

            if (!sceneBvh->AddMeshBvh(instance.meshBvhId, GetMeshBvhById(instance.meshBvhId))) {
                return false;
            }
        }

        return true;
    }
}
