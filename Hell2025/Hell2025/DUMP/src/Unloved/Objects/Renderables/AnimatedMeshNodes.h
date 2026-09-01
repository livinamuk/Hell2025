#pragma once
#include "Hell/ResourceManagement/Types/SkinnedModel.h"

#include "Unloved/Common/CreateInfo.h"
#include "Unloved/Common/Constants.h"
#include "Unloved/Common/Enums.h"
#include "Unloved/Common/Types.h"
#include "Unloved/Render/RendererTypes.h"

#include <cstdint>
#include <string>
#include <vector>

namespace Unloved {

struct AnimatedMeshNodeCreateInfo {
    std::string meshName;
    std::string materialName = UNDEFINED_STRING;
    BlendingMode blendingMode = BlendingMode::DEFAULT;
    bool castShadows = true;
};

struct AnimatedMeshNode {
    std::string meshName;
    BlendingMode blendingMode = BlendingMode::DEFAULT;
    RenderItem renderItem;
    int32_t materialIndex = 0;
    int32_t woundMaterialIndex = -1;
    uint32_t meshId = 0;
    uint32_t baseVertex = 0;
    uint32_t baseIndex = 0;
    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;
    uint32_t baseVertexWeight = 0;
    bool deforming = true;
    bool excludeFromVulkanTLAS = false;
};

struct AnimatedMeshNodes {
    void Init(uint64_t parentId, const std::string& modelName, const std::vector<AnimatedMeshNodeCreateInfo>& createInfoSet);
    void UpdateRenderItems(const glm::mat4& modelMatrix, const std::vector<glm::mat4>& boneSkinningMatrices);

    void SetSkinnedModel(uint64_t parentId, std::string name); // temp

    void SetBlendingModeByMeshName(const std::string& meshName, BlendingMode blendingMode);
    void SetMeshMaterialByMeshName(const std::string& meshName, const std::string& materialName, BlendingMode blendingMode = BlendingMode::DEFAULT);
    void SetMeshMaterialByMeshIndex(int meshIndex, const std::string& materialName);
    void SetMeshWoundMaskArrayIndex(const std::string& meshName, int32_t woundMaskArrayIndex);
    void SetMeshWoundMaterialByMeshName(const std::string& meshName, const std::string& textureName);
    void SetAllMeshMaterials(const std::string& materialName);
    void SetAllMeshBlendingModes(BlendingMode blendingMode);
    void SetExcludeFromVulkanTLAS(bool exclude);
    void SetExclusiveViewportIndex(int index);
    void SetIgnoredViewportIndex(int index);
    void PrintMeshNames();
    void EnableRendering();
    void DisableRendering();

    bool RenderingEnabled() const { return m_renderingEnabled; }

    const int32_t& GetIgnoredViewportIndex() const        { return m_ignoredViewportIndex; };
    const int32_t& GetExclusiveViewportIndex() const      { return m_exclusiveViewportIndex; };
    const std::vector<AnimatedMeshNode>& GetNodes() const { return m_nodes; }

    uint64_t m_parentId = 0;
    int32_t m_ignoredViewportIndex = -1;
    int32_t m_exclusiveViewportIndex = -1;

    std::vector<int32_t> m_woundMaskArrayIndices;

    std::vector<RenderItem> m_deformingRenderItems;
    std::vector<RenderItem> m_nonDeformingRenderItems;
    std::vector<RenderItem> m_nonDeformingRenderItemsDepthPeeledTransparent;

    SkinnedModel* m_skinnedModel = nullptr;
    bool m_renderingEnabled = true;
    bool m_excludeFromVulkanTLAS = false;

private:
    std::vector<AnimatedMeshNode> m_nodes;
};
}
