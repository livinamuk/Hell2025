#pragma once
#include "Hell/AssetFormats/AssetData.h"
#include "Hell/File/FileInfo.h"
#include "Hell/ResourceManagement/Types/Mesh.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

struct Model {
    Model() = default;

    void SetFileInfo(FileInfo fileInfo);
    void AddMeshIndex(uint32_t meshId);
    void SetModelId(uint32_t modelId);
    void SetName(std::string modelName);
    void SetAABB(glm::vec3 aabbMin, glm::vec3 aabbMax);
    int32_t GetGlobalMeshIndexByMeshName(const std::string& meshName) const;
    const glm::mat4& GetBoneLocalMatrix(const std::string& boneName) const;

    uint32_t GetModelId() const                              { return m_modelId; }
    const FileInfo& GetFileInfo() const                       { return m_fileInfo; }
    const size_t GetMeshCount()  const                        { return m_meshIndices.size(); }
    const glm::vec3& GetAABBMin() const                       { return m_aabbMin; }
    const glm::vec3& GetAABBMax() const                       { return m_aabbMax; }
    glm::vec3 GetExtents() const                              { return m_aabbMax - m_aabbMin; }
    const std::string GetName() const                         { return m_name; }
    const std::vector<uint32_t>& GetMeshIndices() const       { return m_meshIndices; }
    size_t GetCPUAllocatedByteCount() const;

    std::vector<ArmatureData> m_armatures;
    ModelData m_modelData;
    ModelBvhData m_modelBvhData;

private:
    FileInfo m_fileInfo;
    uint32_t m_modelId = 0;
    glm::vec3 m_aabbMin = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 m_aabbMax = glm::vec3(-std::numeric_limits<float>::max());
    std::string m_name = "undefined";
    std::vector<uint32_t> m_meshIndices;
    std::unordered_map<std::string, uint32_t> m_meshNameToGlobalMeshIndexMap;
};
