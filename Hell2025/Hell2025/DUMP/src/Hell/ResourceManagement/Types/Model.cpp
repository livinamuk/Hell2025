#include "Model.h"
#include "Hell/Logging.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include <cstddef>
#include <stack>
#include <utility>

#include <iostream> // TODO clean up logging

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

    size_t MeshDataAllocatedByteCount(const MeshData& meshData) {
        return StringAllocatedByteCount(meshData.name) +
               meshData.vertices.capacity() * sizeof(Vertex) +
               meshData.indices.capacity() * sizeof(uint32_t);
    }

    size_t ArmatureDataAllocatedByteCount(const ArmatureData& armatureData) {
        return StringAllocatedByteCount(armatureData.name) +
               armatureData.bones.capacity() * sizeof(Bone);
    }

    size_t ModelDataAllocatedByteCount(const ModelData& modelData) {
        size_t byteCount = StringAllocatedByteCount(modelData.name);
        byteCount += modelData.meshes.capacity() * sizeof(MeshData);
        byteCount += modelData.armatures.capacity() * sizeof(ArmatureData);

        for (const MeshData& meshData : modelData.meshes) {
            byteCount += MeshDataAllocatedByteCount(meshData);
        }

        for (const ArmatureData& armatureData : modelData.armatures) {
            byteCount += ArmatureDataAllocatedByteCount(armatureData);
        }

        return byteCount;
    }

    size_t ModelBvhDataAllocatedByteCount(const ModelBvhData& modelBvhData) {
        size_t byteCount = modelBvhData.bvhs.capacity() * sizeof(MeshBvh);

        for (const MeshBvh& meshBvh : modelBvhData.bvhs) {
            byteCount += meshBvh.m_nodes.capacity() * sizeof(BvhNode);
            byteCount += meshBvh.m_triangles.capacity() * sizeof(BVHTriangle);
        }

        return byteCount;
    }
}

void Model::SetFileInfo(FileInfo fileInfo) {
    m_fileInfo = fileInfo;
}

void Model::AddMeshIndex(uint32_t meshId) {
    m_meshIndices.push_back(meshId);
    
    // Map global mesh id to mesh name
    if (Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(meshId)) {
        m_meshNameToGlobalMeshIndexMap[mesh->name] = meshId;
    }
}

void Model::SetModelId(uint32_t modelId) {
    m_modelId = modelId;
}

void Model::SetName(std::string modelName) {
    m_name = modelName;
}

void Model::SetAABB(glm::vec3 aabbMin, glm::vec3 aabbMax) {
    m_aabbMin = aabbMin;
    m_aabbMax = aabbMax;
}

int32_t Model::GetGlobalMeshIndexByMeshName(const std::string& meshName) const {
    auto it = m_meshNameToGlobalMeshIndexMap.find(meshName);
    if (it == m_meshNameToGlobalMeshIndexMap.end()) return -1;
    return (int32_t)it->second;
}

size_t Model::GetCPUAllocatedByteCount() const {
    size_t byteCount = FileInfoAllocatedByteCount(m_fileInfo);
    byteCount += StringAllocatedByteCount(m_name);
    byteCount += m_meshIndices.capacity() * sizeof(uint32_t);
    byteCount += m_meshNameToGlobalMeshIndexMap.size() * sizeof(std::pair<const std::string, uint32_t>);
    byteCount += m_armatures.capacity() * sizeof(ArmatureData);
    byteCount += ModelDataAllocatedByteCount(m_modelData);
    byteCount += ModelBvhDataAllocatedByteCount(m_modelBvhData);

    for (const auto& [meshName, meshId] : m_meshNameToGlobalMeshIndexMap) {
        byteCount += StringAllocatedByteCount(meshName);
    }

    for (const ArmatureData& armatureData : m_armatures) {
        byteCount += ArmatureDataAllocatedByteCount(armatureData);
    }

    return byteCount;
}

const glm::mat4& Model::GetBoneLocalMatrix(const std::string& boneName) const {
    static const glm::mat4 identity(1.0f);

    for (const ArmatureData& armatureData : m_armatures) {
        for (const Bone& bone : armatureData.bones) {
            if (bone.name == boneName) {
                return bone.localRestPose;
            }
        }
    }

    Logging::Warning() << "Model::GetBoneLocalMatrix(..) failed: " << boneName << " not found in " << m_name;

    // Print bones
    for (const ArmatureData& armatureData : m_armatures) {
        for (const Bone& bone : armatureData.bones) {
            std::cout << bone.name << "\n";
        }
    }

    return identity;
}
