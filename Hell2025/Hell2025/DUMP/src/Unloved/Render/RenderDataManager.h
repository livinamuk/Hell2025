#pragma once
#include "Unloved/Common/Types.h"
#include "Unloved/Objects/Renderables/AnimatedMeshNodes.h"
#include "Unloved/Objects/Renderables/MeshNodes.h"
#include "Unloved/Objects/Renderables/SkinnedGameObject.h"
#include "Unloved/Render/DrawCommandSets.h"
#include "Unloved/Render/RendererTypes.h"
#include "Unloved/Systems/Bullets/Bullet.h"
#include "Hell/UI/UITypes.h"

#include <cstddef>
#include <vector>

struct DecalPaintingInfo {
    glm::vec3 rayOrigin;
    glm::vec3 rayDirection;
    int textureArrayIndex = 0;
};

namespace Unloved::RenderDataManager {
    using namespace Unloved;

    void BeginFrame();
    void Update();
    void UpdateDrawCommandsUI();

    const std::vector<DrawIndexedIndirectCommand>& GetDrawCommandsUI();
    const std::vector<DrawIndexedIndirectCommand>& GetDrawCommandsUI(UICanvas canvas);

    inline std::vector<glm::mat4> skinningTransforms;

    // Submissions
    void SubmitAnimatedMeshNodes(const AnimatedMeshNodes& animatedMeshNodes);
    void SubmitMeshNodes(const MeshNodes& meshNodes, bool viewWeapon = false);

    void SubmitRenderItem(const RenderItem& renderItem, bool viewWeapon = false);
    void SubmitRenderItems(const std::vector<RenderItem>& renderItems, bool viewWeapon = false);
    void SubmitRenderItemPhysicsShape(const RenderItem& renderItem, bool outline = false);
    void SubmitSpriteSheetRenderItem(const SpriteSheetRenderItem& renderItem);

    // House submissions
    void SubmitRenderItemProcedural(const RenderItem& renderItem);

    void SubmitDecalPaintingInfo(DecalPaintingInfo decalPaintingInfo);

    const std::vector<SkinningJob>& GetSkinningJobs();
    const std::vector<SkinningMorphJob>& GetSkinningMorphJobs();
    const std::vector<SkinningMorphTarget>& GetSkinningMorphTargets();
    const std::vector<SkinningDispatchGroup>& GetSkinningDispatchGroups();
    const std::vector<std::vector<uint32_t>>& GetTransientRayQueryRenderItemGroups();
    const std::vector<uint32_t>& GetProceduralRayQueryRenderItemIndices();
    const std::vector<uint32_t>& GetPersistentRayQueryRenderItemIndices();

    const RendererData& GetRendererData();
    const std::vector<glm::mat4>& GetSkinningTransforms();
    const std::vector<glm::mat4>& GetPreviousSkinningTransforms();
    const std::vector<GPULight>& GetGPULights();
    const std::vector<GPUSpotLight>& GetGPUSpotLights();
    const std::vector<DecalPaintingInfo>& GetDecalPaintingInfo();
    const std::vector<RenderItem>& GetSceneRenderItems();
    const std::vector<uint32_t>& GetDrawRenderItemIndices();
    const std::vector<GlassLightRange>& GetGlassLightRanges();
    const std::vector<uint32_t>& GetGlassLightIndices();
    const std::vector<GlassLightRange>& GetGlassSpotLightRanges();
    const std::vector<uint32_t>& GetGlassSpotLightIndices();
    const std::vector<SpriteSheetRenderItem>& GetSpriteSheetInstanceData();
    const std::vector<uint32_t>& GetCombinedSkinnedRenderItemIndices();
    const std::vector<uint32_t>& GetRenderItemIndicesOutline();
    const std::vector<uint32_t>& GetRenderItemIndicesOutlinePhysicsShapes();
    const std::vector<uint32_t>& GetRenderItemIndicesOutlineProcedural();
    const std::vector<uint32_t>& GetRenderItemIndicesOutlineSkinned();
    const std::vector<uint32_t>& GetRenderItemIndicesPlastic();
    const std::vector<uint32_t>& GetRenderItemIndicesProcedural();
    std::size_t GetRenderItemCount(BlendingMode blendingMode);
    std::size_t GetSkinnedRenderItemCount(BlendingMode blendingMode);
    uint32_t GetRequiredSkinnedVertexCount();

    const std::vector<BloodDecalInstanceData>& GetBloodScreenSpaceDecalInstanceData();
    const std::vector<ViewportData>& GetViewportData();
    const DrawCommandsSet& GetDrawInfoSet();
    const FlashLightShadowMapDrawInfo& GetFlashLightShadowMapDrawInfo();

    const std::vector<RenderItem>& GetNonDeformingSkinnedMeshRenderItemsDepthPeeledTransparent();
}
