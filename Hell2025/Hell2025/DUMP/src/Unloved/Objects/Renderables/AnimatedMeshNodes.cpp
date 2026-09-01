#include "AnimatedMeshNodes.h"

#include "Hell/Common/Bit.h"
#include "Hell/Logging.h"

#include "Hell/ResourceManagement/ResourceManager.h"
#include "../../../../res/shaders/common/flags.glsl"
#include "Unloved/Render/RendererUtil.h"

namespace Unloved {

void AnimatedMeshNodes::Init(uint64_t parentId, const std::string& modelName, const std::vector<AnimatedMeshNodeCreateInfo>& createInfoSet) {

}

void AnimatedMeshNodes::SetSkinnedModel(uint64_t parentId, std::string name) {
    m_parentId = parentId;

    SkinnedModel* ptr = Hell::ResourceManager::GetSkinnedModelByName(name);
    if (ptr) {
        //std::cout << "SetSkinnedModel() " << name << " mesh count: " << m_skinnedModel->GetMeshCount() << "\n";

        m_skinnedModel = ptr;
        m_nodes.clear();
        m_woundMaskArrayIndices.resize(m_skinnedModel->GetMeshCount());

        int meshCount = m_skinnedModel->GetMeshCount();
        Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("AssetGeometry");

        for (int i = 0; i < meshCount; i++) {
            uint32_t meshId = m_skinnedModel->GetMeshIndices()[i];
            Mesh* mesh = meshBuffer.GetMeshById(meshId);
            Hell::SkinnedMeshMetadata* metadata = meshBuffer.GetSkinnedMeshMetadataByMeshId(meshId);
            if (!mesh || !metadata) continue;

            AnimatedMeshNode& node = m_nodes.emplace_back();
            node.meshId = meshId;
            node.meshName = mesh->name;
            node.deforming = metadata->requiresSkinning;
            node.vertexCount = mesh->vertexCount;
            node.indexCount = mesh->indexCount;
            node.baseVertex = mesh->baseVertex;
            node.baseIndex = mesh->baseIndex;
            node.excludeFromVulkanTLAS = m_excludeFromVulkanTLAS;
            node.meshId = meshId;
            node.baseVertexWeight = metadata->baseVertexWeight;

            m_woundMaskArrayIndices[i] = -1;
        }
    }
    else {
        Logging::Error() << "AnimatedMeshNodes::SetSkinnedModel(..) failed '" << name << "' does not exist\n";
    }
}

void AnimatedMeshNodes::UpdateRenderItems(const glm::mat4& modelMatrix, const std::vector<glm::mat4>& boneSkinningMatrices) {
    m_deformingRenderItems.clear();
    m_nonDeformingRenderItems.clear();
    m_nonDeformingRenderItemsDepthPeeledTransparent.clear();

    if (!m_renderingEnabled) return;

    for (int i = 0; i < m_nodes.size(); i++) {
        if (m_nodes[i].blendingMode == BlendingMode::DO_NOT_RENDER) continue;

        AnimatedMeshNode& node = m_nodes[i];

        RenderItem& renderItem = node.renderItem;
        Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("AssetGeometry");
        Mesh* mesh = meshBuffer.GetMeshById(node.meshId);
        Hell::SkinnedMeshMetadata* metadata = meshBuffer.GetSkinnedMeshMetadataByMeshId(node.meshId);
        if (!mesh || !metadata) continue;

        renderItem.materialIndex = node.materialIndex;
        renderItem.woundMaterialIndex = node.woundMaterialIndex;

        renderItem.prevModelMatrix = renderItem.modelMatrix; // TODO: write logic for on the first frame where this is identity
        renderItem.modelMatrix = modelMatrix;
        renderItem.inverseModelMatrix = glm::inverse(renderItem.modelMatrix);
        renderItem.meshId = m_skinnedModel->GetMeshIndices()[i];
        renderItem.ignoredViewportIndex = m_ignoredViewportIndex;
        renderItem.exclusiveViewportIndex = m_exclusiveViewportIndex;
        renderItem.baseVertexWeight = metadata->baseVertexWeight;
        renderItem.woundMaskTextureIndex = m_woundMaskArrayIndices[i];
        renderItem.baseVertex = mesh->baseVertex;
        renderItem.baseIndex = mesh->baseIndex;
        renderItem.vertexCount = mesh->vertexCount;
        renderItem.indexCount = mesh->indexCount;
        renderItem.blendingMode = static_cast<uint32_t>(node.blendingMode);

        Hell::Bit::SetState(renderItem.miscFlags, MISC_FLAG_DYNAMIC_OBJECT, true);
        Hell::Bit::SetState(renderItem.miscFlags, MISC_FLAG_RESEVERED, false);
        renderItem.shadowFlags = SHADOW_FLAG_POINT_LIGHT;

        Hell::Bit::PackUint64(m_parentId, renderItem.objectIdLowerBit, renderItem.objectIdUpperBit);

        // Put it where it belongs
        if (metadata->requiresSkinning) {
            renderItem.prevModelMatrix = renderItem.modelMatrix; // Hack because you are compute skinning and can't rely on shit here. FIGURE THIS OUT
            m_deformingRenderItems.push_back(renderItem);
        }
        else {
            // Update the model matrix to include the animated bone transform
            int boneIndex = metadata->nonDeformingBoneIndex;

            if (boneIndex >= 0 && boneIndex < boneSkinningMatrices.size()) {
                renderItem.modelMatrix = modelMatrix * boneSkinningMatrices[boneIndex];
                renderItem.inverseModelMatrix = glm::inverse(renderItem.modelMatrix);
                RendererUtil::UpdateRenderItemAABB(renderItem);

                if (mesh->name == "P90_Magazine") {
                    m_nonDeformingRenderItemsDepthPeeledTransparent.push_back(renderItem);
                }
                else {
                    m_nonDeformingRenderItems.push_back(renderItem);
                }
            }
            else {
                Logging::Error() << "AnimatedMeshNodes::UpdateRenderItems(..) wants to access boneSkinningMatrices[" << boneIndex << "] but size is " << boneSkinningMatrices.size() << "\n";
            }
        }
    }
}

void AnimatedMeshNodes::SetMeshWoundMaskArrayIndex(const std::string& meshName, int32_t woundMaskTextureIndex) {
    std::vector<uint32_t>& meshIndices = m_skinnedModel->GetMeshIndices();
    Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("AssetGeometry");

    for (int i = 0; i < meshIndices.size(); i++) {
        uint32_t meshId = meshIndices[i];
        Mesh* skinnedMesh = meshBuffer.GetMeshById(meshId);
        if (skinnedMesh && skinnedMesh->name == meshName) {
            m_woundMaskArrayIndices[i] = woundMaskTextureIndex;
            return;
        }
    }
}

void AnimatedMeshNodes::SetBlendingModeByMeshName(const std::string& meshName, BlendingMode blendingMode) {
    for (AnimatedMeshNode& node : m_nodes) {
        if (node.meshName == meshName) {
            node.blendingMode = blendingMode;
        }
    }
}

void AnimatedMeshNodes::SetMeshMaterialByMeshName(const std::string& meshName, const std::string& materialName, BlendingMode blendingMode) {
    int materialIndex = Hell::ResourceManager::GetMaterialIndexByName(materialName);

    if (materialIndex == -1) {
        Logging::Error() << "AnimatedMeshNodes::SetMeshMaterialByMeshName(..) failed because '" << materialName << "' was not found\n";
        return;
    }

    for (AnimatedMeshNode& node : m_nodes) {
        if (node.meshName == meshName) {
            node.materialIndex = materialIndex;
            node.blendingMode = blendingMode;
        }
    }
}

void AnimatedMeshNodes::SetMeshMaterialByMeshIndex(int meshIndex, const std::string& materialName) {
    if (meshIndex >= 0 && meshIndex < m_nodes.size()) {
        m_nodes[meshIndex].materialIndex = Hell::ResourceManager::GetMaterialIndexByName(materialName);
    }
}

void AnimatedMeshNodes::SetMeshWoundMaterialByMeshName(const std::string& meshName, const std::string& textureName) {
    for (AnimatedMeshNode& node : m_nodes) {
        if (node.meshName == meshName) {
            node.woundMaterialIndex = Hell::ResourceManager::GetMaterialIndexByName(textureName);
        }
    }
}

void AnimatedMeshNodes::SetAllMeshMaterials(const std::string& materialName) {
    for (AnimatedMeshNode& node : m_nodes) {
        node.materialIndex = Hell::ResourceManager::GetMaterialIndexByName(materialName);
    }
}

void AnimatedMeshNodes::SetAllMeshBlendingModes(BlendingMode blendingMode) {
    for (AnimatedMeshNode& node : m_nodes) {
        node.blendingMode = blendingMode;
    }
}

void AnimatedMeshNodes::SetExcludeFromVulkanTLAS(bool exclude) {
    m_excludeFromVulkanTLAS = exclude;

    for (AnimatedMeshNode& node : m_nodes) {
        node.excludeFromVulkanTLAS = exclude;
    }
}

void AnimatedMeshNodes::SetExclusiveViewportIndex(int index) {
    m_exclusiveViewportIndex = index;
}

void AnimatedMeshNodes::SetIgnoredViewportIndex(int index) {
    m_ignoredViewportIndex = index;
}

void AnimatedMeshNodes::PrintMeshNames() {
    std::string message = m_skinnedModel->GetName() + "\n";
    for (int i = 0; i < m_nodes.size(); i++) {
        message += "-" + std::to_string(i) + " " + m_nodes[i].meshName + "\n";
    }

    Logging::Debug() << message;
}

void AnimatedMeshNodes::EnableRendering() {
    m_renderingEnabled = true;
}

void AnimatedMeshNodes::DisableRendering() {
    m_renderingEnabled = false;
}

}
