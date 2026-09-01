#include "SkinnedModel.h"
#include <mutex>
#include "Hell/Logging.h"

#include <cstddef>
#include <iostream> // TODO clean up logging
#include <utility>

namespace {
    size_t StringAllocatedByteCount(const std::string& value) {
        return value.capacity() + 1;
    }

    size_t FileInfoAllocatedByteCount(const FileInfo& fileInfo) {
        return StringAllocatedByteCount(fileInfo.path) +
               StringAllocatedByteCount(fileInfo.name) +
               StringAllocatedByteCount(fileInfo.ext) +
               StringAllocatedByteCount(fileInfo.dir);
    }

    size_t NodeAllocatedByteCount(const Node& node) {
        return StringAllocatedByteCount(node.name);
    }

    size_t SkinnedMeshDataAllocatedByteCount(const SkinnedMeshData& meshData) {
        size_t byteCount = StringAllocatedByteCount(meshData.name) +
                           meshData.vertices.capacity() * sizeof(Vertex) +
                           meshData.vertexWeights.capacity() * sizeof(VertexWeight) +
                           meshData.indices.capacity() * sizeof(uint32_t) +
                           meshData.morphTargets.capacity() * sizeof(MorphTargetData);

        for (const MorphTargetData& morphTarget : meshData.morphTargets) {
            byteCount += StringAllocatedByteCount(morphTarget.name);
            byteCount += morphTarget.positionDeltas.capacity() * sizeof(MorphTargetVertexDelta);
            byteCount += morphTarget.normalDeltas.capacity() * sizeof(MorphTargetVertexDelta);
            byteCount += morphTarget.tangentDeltas.capacity() * sizeof(MorphTargetVertexDelta);
        }

        return byteCount;
    }

    size_t BoneMappingAllocatedByteCount(const std::map<std::string, unsigned int>& mapping) {
        size_t byteCount = mapping.size() * sizeof(std::pair<const std::string, unsigned int>);

        for (const auto& [name, index] : mapping) {
            byteCount += StringAllocatedByteCount(name);
        }

        return byteCount;
    }

    size_t SkinnedModelDataAllocatedByteCount(const SkinnedModelData& skinnedModelData) {
        size_t byteCount = StringAllocatedByteCount(skinnedModelData.name);
        byteCount += skinnedModelData.meshes.capacity() * sizeof(SkinnedMeshData);
        byteCount += skinnedModelData.boneOffsets.capacity() * sizeof(glm::mat4);
        byteCount += skinnedModelData.nodes.capacity() * sizeof(Node);
        byteCount += BoneMappingAllocatedByteCount(skinnedModelData.boneMapping);

        for (const SkinnedMeshData& meshData : skinnedModelData.meshes) {
            byteCount += SkinnedMeshDataAllocatedByteCount(meshData);
        }

        for (const Node& node : skinnedModelData.nodes) {
            byteCount += NodeAllocatedByteCount(node);
        }

        return byteCount;
    }
}

void SkinnedModel::BuildRuntimeData() {
    m_vertexCount = m_skinnedModelData.vertexCount;
    m_indexCount = m_skinnedModelData.indexCount;
    m_boneOffsets = m_skinnedModelData.boneOffsets;
    m_nodes = m_skinnedModelData.nodes;
    m_boneMapping = m_skinnedModelData.boneMapping;
    m_nodeMapping.clear();
    m_morphTargetNames.clear();

    for (const SkinnedMeshData& meshData : m_skinnedModelData.meshes) {
        for (const MorphTargetData& morphTarget : meshData.morphTargets) {
            m_morphTargetNames.insert(morphTarget.name);
        }
    }

    // Build node mappings and cache the bind pose
    m_boneNodeIndices.assign(m_boneMapping.size(), -1);
    m_bindPose.resize(m_nodes.size());
    m_globalBindPoseMatrices.resize(m_nodes.size());
    for (int nodeIdx = 0; nodeIdx < GetNodeCount(); ++nodeIdx) {
        const auto& name = m_nodes[nodeIdx].name;
        auto it = m_boneMapping.find(name);
        if (it != m_boneMapping.end()) {
            m_boneNodeIndices[it->second] = nodeIdx;
        }
        m_nodeMapping[name] = nodeIdx;
        m_bindPose[nodeIdx] = Hell::QuatTransform(m_nodes[nodeIdx].localBindTransform);

        const int32_t parentIndex = m_nodes[nodeIdx].parentIndex;
        if (parentIndex == -1) m_globalBindPoseMatrices[nodeIdx] = m_nodes[nodeIdx].localBindTransform;
        else m_globalBindPoseMatrices[nodeIdx] = m_globalBindPoseMatrices[parentIndex] * m_nodes[nodeIdx].localBindTransform;
    }

    // Cache matrices in the bone index order expected by skinning
    m_bindPoseBoneSkinningMatrices.resize(GetBoneCount());
    for (uint32_t boneIndex = 0; boneIndex < GetBoneCount(); boneIndex++) {
        m_bindPoseBoneSkinningMatrices[boneIndex] = m_globalBindPoseMatrices[m_boneNodeIndices[boneIndex]] * m_boneOffsets[boneIndex];
    }
}

void SkinnedModel::PrintNodeInfo() {
    std::cout << "\n" << m_fileInfo.name.c_str() << " nodes\n";
    for (size_t i = 0; i < m_nodes.size(); ++i) {
        const Node& node = m_nodes[i];
        std::cout << " " << i << ": " << node.name.c_str() << " (" << node.parentIndex << ")\n";
    }
}

void SkinnedModel::PrintBoneInfo() {
    std::cout << "\n" << m_fileInfo.name.c_str() << " bones\n";

    for (const auto& pair : m_boneMapping) {
        std::cout << pair.first << ": " << pair.second << "\n";
    }
}

void SkinnedModel::SetFileInfo(FileInfo fileInfo) {
    m_fileInfo = fileInfo;
    if (m_name == "undefined") {
        m_name = fileInfo.name;
    }
}

bool SkinnedModel::BoneExists(const std::string& boneName) {
    return m_boneMapping.find(boneName) != m_boneMapping.end();
}

bool SkinnedModel::MorphTargetExists(const std::string& morphTargetName) const {
    return m_morphTargetNames.contains(morphTargetName);
}

const FileInfo& SkinnedModel::GetFileInfo() {
    return m_fileInfo;
}

void SkinnedModel::SetVertexCount(uint32_t vertexCount) {
    m_vertexCount = vertexCount;
}

void SkinnedModel::SetName(std::string name) {
    m_name = name;
}

void SkinnedModel::SetSkinnedModelId(uint32_t skinnedModelId) {
    m_skinnedModelId = skinnedModelId;
}

uint32_t SkinnedModel::GetSkinnedModelId() const {
    return m_skinnedModelId;
}

const std::string& SkinnedModel::GetName() const {
    return m_name;
}

void SkinnedModel::AddMeshIndex(uint32_t meshId) {
    m_meshIndices.push_back(meshId);
}

uint32_t SkinnedModel::GetMeshCount() {
    return m_meshIndices.size();
}

std::vector<uint32_t>& SkinnedModel::GetMeshIndices() {
    return m_meshIndices;
}

uint32_t SkinnedModel::GetVertexCount() { 
    return m_vertexCount; 
}

uint32_t SkinnedModel::GetBoneCount() {
    return m_boneMapping.size();
}

uint32_t SkinnedModel::GetNodeCount() {
    return m_nodes.size();
}

int32_t SkinnedModel::GetBoneIndex(const std::string& boneName) {
    auto it = m_boneMapping.find(boneName);
    return (it != m_boneMapping.end()) ? it->second : -1;
}

int32_t SkinnedModel::GetNodeIndex(const std::string& nodeName) {
    auto it = m_nodeMapping.find(nodeName);
    return (it != m_nodeMapping.end()) ? it->second : -1;
}

size_t SkinnedModel::GetCPUAllocatedByteCount() const {
    size_t byteCount = FileInfoAllocatedByteCount(m_fileInfo);
    byteCount += StringAllocatedByteCount(m_name);
    byteCount += m_nodes.capacity() * sizeof(Node);
    byteCount += m_boneOffsets.capacity() * sizeof(glm::mat4);
    byteCount += BoneMappingAllocatedByteCount(m_boneMapping);
    byteCount += BoneMappingAllocatedByteCount(m_nodeMapping);
    byteCount += m_boneNodeIndices.capacity() * sizeof(int);
    byteCount += m_bindPose.capacity() * sizeof(Hell::QuatTransform);
    byteCount += m_globalBindPoseMatrices.capacity() * sizeof(glm::mat4);
    byteCount += m_bindPoseBoneSkinningMatrices.capacity() * sizeof(glm::mat4);
    byteCount += m_meshIndices.capacity() * sizeof(uint32_t);
    byteCount += SkinnedModelDataAllocatedByteCount(m_skinnedModelData);

    for (const Node& node : m_nodes) {
        byteCount += NodeAllocatedByteCount(node);
    }

    return byteCount;
}

const glm::mat4& SkinnedModel::GetBoneOffset(const std::string& boneName) {
    const int boneIndex = GetBoneIndex(boneName);
    if (boneIndex >= 0 && boneIndex < (int)m_boneOffsets.size()) {
        return m_boneOffsets[boneIndex];
    }
    static const glm::mat4 identity(1.0f);
    return identity;
}

const glm::mat4& SkinnedModel::GetLocalBindTransform(const std::string& nodeName) {
    for (int i = 0; i < m_nodes.size(); i++) {
        if (m_nodes[i].name == nodeName) {
            return m_nodes[i].localBindTransform;
        }
    }

    Logging::Error() << "SkinnedModel::GetLocalBindTransform(..) failed to find '" << nodeName << "'";

    const static glm::mat4 identity(1.0f);
    return identity;
}

const std::vector<glm::mat4>& SkinnedModel::GetGlobalBindPoseMatrices() const {
    return m_globalBindPoseMatrices;
}

const std::vector<glm::mat4>& SkinnedModel::GetBindPoseBoneSkinningMatrices() const {
    return m_bindPoseBoneSkinningMatrices;
}
