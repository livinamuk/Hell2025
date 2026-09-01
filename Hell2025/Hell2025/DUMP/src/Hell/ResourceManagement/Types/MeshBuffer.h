#pragma once
#include "Hell/AssetFormats/AssetData.h"
#include "Hell/Common.h"
#include "Hell/Render/VertexAttributes.h"
#include "Hell/ResourceManagement/Types/Mesh.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace Hell {

struct MeshMorphTargetMetadata {
    std::string name;
    uint32_t positionDeltaOffset = 0;
    uint32_t positionDeltaCount = 0;
    uint32_t normalDeltaOffset = 0;
    uint32_t normalDeltaCount = 0;
    uint32_t tangentDeltaOffset = 0;
    uint32_t tangentDeltaCount = 0;
};

struct SkinnedMeshMetadata {
    int32_t baseVertexWeight = -1;
    int32_t nonDeformingBoneIndex = -1;
    bool requiresSkinning = false;
    std::vector<MeshMorphTargetMetadata> morphTargets;
};

struct MeshBuffer {
    MeshBuffer() = default;
    MeshBuffer(const std::string& name);
    MeshBuffer(const MeshBuffer&) = delete;
    MeshBuffer& operator=(const MeshBuffer&) = delete;
    MeshBuffer(MeshBuffer&&) noexcept = default;
    MeshBuffer& operator=(MeshBuffer&&) noexcept = default;
    ~MeshBuffer() = default;

    void PreAllocate(size_t maxVertices, size_t maxIndices, size_t maxVertexWeights = 0, size_t maxMorphDeltas = 0);
    void RemoveMesh(uint32_t meshId);
    void Reset();
    void CleanUp();
    void PrintDebugInfo();

    uint32_t AddMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, const std::string& name = UNDEFINED_STRING);
    uint32_t AddSkinnedMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, const std::vector<VertexWeight>& vertexWeights, const std::vector<MorphTargetData>& morphTargets, SkinnedMeshMetadata metadata, const std::string& name = UNDEFINED_STRING);

    void SetCreateVulkanBlasForNewMeshes(bool value) { m_createVulkanBlasForNewMeshes = value; }

    Mesh* GetMeshById(uint32_t meshId);
    Mesh* GetMeshByName(const std::string& name);
    uint32_t GetMeshIdByName(const std::string& name);
    uint32_t GetBaseVertexByMeshId(uint32_t meshId);
    uint32_t GetBaseIndexByMeshId(uint32_t meshId);
    SkinnedMeshMetadata* GetSkinnedMeshMetadataByMeshId(uint32_t meshId);
    bool HasSkinnedMeshMetadata(uint32_t meshId) const;
    const std::string& GetMeshNameByMeshId(uint32_t meshId);
    std::span<Vertex> GetMeshVertexSpan(uint32_t meshId);
    std::span<uint32_t> GetMeshIndexSpan(uint32_t meshId);
    std::span<VertexWeight> GetMeshVertexWeightSpan(uint32_t meshId);

    size_t GetCPUAllocatedByteCount() const;
    size_t GetGPUAllocatedByteCount() const;

    size_t GetMeshCount() const                   { return m_meshes.size(); }
    size_t GetSkinnedMeshMetadataCount() const    { return m_skinnedMeshMetadata.size(); }
    size_t GetAllocatedVertexCount() const        { return m_vertices.size(); }
    size_t GetAllocatedIndexCount() const         { return m_indices.size(); }
    size_t GetAllocatedVertexWeightCount() const  { return m_vertexWeights.size(); }
    size_t GetAllocatedMorphDeltaCount() const    { return m_morphDeltas.size(); }
    std::vector<Vertex>& GetVertices()            { return m_vertices; }
    std::vector<uint32_t>& GetIndices()           { return m_indices; }
    std::vector<VertexWeight>& GetVertexWeights() { return m_vertexWeights; }
    std::vector<MorphTargetVertexDelta>& GetMorphDeltas() { return m_morphDeltas; }

    uint64_t GetOpenGLId() const { return m_openGLId; }
    uint64_t GetVulkanId() const { return m_vulkanId; }
    uint64_t GetVersion() const  { return m_version; }

private:
    struct MemoryBlock {
        size_t begin = 0;
        size_t end = 0;
        size_t GetSize() const { return end - begin; }
    };

    void Initialize();
    int32_t AllocateExtraVertexSpace(size_t vertexCount);
    int32_t AllocateExtraIndexSpace(size_t indexCount);
    int32_t AddVertices(const std::vector<Vertex>& newVertices);
    int32_t AddIndices(const std::vector<uint32_t>& newIndices);
    int32_t AddVertexWeights(const std::vector<VertexWeight>& newVertexWeights);
    int32_t AddMorphDeltas(const std::vector<MorphTargetVertexDelta>& newMorphDeltas);
    size_t CalculateNewCapacity(size_t requiredCount, size_t currentCapacity);
    void CreateVulkanBlas(Mesh& mesh);
    void DestroyVulkanBlas(Mesh& mesh);
    void DestroyAllVulkanBlas();

    std::string m_name = UNDEFINED_STRING;
    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;
    std::vector<VertexWeight> m_vertexWeights;
    std::vector<MorphTargetVertexDelta> m_morphDeltas;
    std::unordered_map<uint32_t, Mesh> m_meshes;
    std::unordered_map<std::string, uint32_t> m_meshIdsByName;
    std::unordered_map<uint32_t, SkinnedMeshMetadata> m_skinnedMeshMetadata;
    std::vector<MemoryBlock> m_freeVertexMemoryBlocks;
    std::vector<MemoryBlock> m_freeIndexMemoryBlocks;

    uint32_t m_nextMeshId = 0;
    size_t m_vertexCapacity = 0;
    size_t m_indexCapacity = 0;
    size_t m_vertexWeightCapacity = 0;
    size_t m_morphDeltaCapacity = 0;
    size_t m_minCapacity = 1024;
    bool m_initialized = false;
    bool m_createVulkanBlasForNewMeshes = true;
    float m_growthMultiplier = 1.0f;

    uint64_t m_openGLId = 0;
    uint64_t m_vulkanId = 0;
    uint64_t m_version = 0;
};

}
