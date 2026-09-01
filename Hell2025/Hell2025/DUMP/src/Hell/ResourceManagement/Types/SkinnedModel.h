#pragma once
#include "Hell/AssetFormats/AssetData.h"
#include "Hell/File/FileInfo.h"
#include "Hell/Transform.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <unordered_set>
#include <vector>

struct SkinnedModel {
    SkinnedModel() = default;

    void BuildRuntimeData();
    void AddMeshIndex(uint32_t meshId);
    void SetFileInfo(FileInfo fileInfo);
    void SetName(std::string name);
    void SetSkinnedModelId(uint32_t skinnedModelId);
    void SetVertexCount(uint32_t vertexCount);

    bool BoneExists(const std::string& boneName);
    bool MorphTargetExists(const std::string& morphTargetName) const;
    const FileInfo& GetFileInfo();
    uint32_t GetSkinnedModelId() const;
    const std::string& GetName() const;
    std::vector<uint32_t>& GetMeshIndices();
    uint32_t GetMeshCount();
    uint32_t GetVertexCount();
    uint32_t GetBoneCount();
    uint32_t GetNodeCount();
    int32_t GetBoneIndex(const std::string& boneName);
    int32_t GetNodeIndex(const std::string& nodeName);
    size_t GetCPUAllocatedByteCount() const;
    const glm::mat4& GetBoneOffset(const std::string& boneName);
    const glm::mat4& GetLocalBindTransform(const std::string& nodeName);
    const std::vector<glm::mat4>& GetGlobalBindPoseMatrices() const;
    const std::vector<glm::mat4>& GetBindPoseBoneSkinningMatrices() const;

    void PrintNodeInfo();
    void PrintBoneInfo();

public:
    std::vector<Node> m_nodes;
    std::vector<glm::mat4> m_boneOffsets;
    std::map<std::string, unsigned int> m_boneMapping;
    std::map<std::string, unsigned int> m_nodeMapping;
    std::vector<int> m_boneNodeIndices;
    std::vector<Hell::QuatTransform> m_bindPose;
    std::vector<glm::mat4> m_globalBindPoseMatrices;
    std::vector<glm::mat4> m_bindPoseBoneSkinningMatrices;
    SkinnedModelData m_skinnedModelData;

private:
    FileInfo m_fileInfo;
    uint32_t m_skinnedModelId = 0;
    std::string m_name = "undefined";
    uint32_t m_vertexCount = 0;
    uint32_t m_indexCount = 0;
    std::vector<uint32_t> m_meshIndices;
    std::unordered_set<std::string> m_morphTargetNames;
};
