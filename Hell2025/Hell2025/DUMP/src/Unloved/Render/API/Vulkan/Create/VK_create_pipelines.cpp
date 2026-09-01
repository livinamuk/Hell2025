#include "Unloved/Render/API/Vulkan/VK_renderer_internal.h"

#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/Render/API/Vulkan/Managers/vk_swapchain_manager.h"
#include "Hell/Render/API/Vulkan/Types/vk_pipeline.h"
#include "Hell/Render/VertexAttributes.h"
#include "Unloved/Render/API/Vulkan/VK_push_constants.h"
#include "Unloved/Render/RendererConstants.h"

namespace {

    // Compute Skinning

    void CreateComputeSkinningPipeline() {
        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("ComputeSkinning");
        pipeline.SetShader("ComputeSkinning");
        pipeline.AddPushConstant(sizeof(PushConstantsSkinning), VK_SHADER_STAGE_COMPUTE_BIT);
        pipeline.Build();
    }

    // Debug

    void CreateComputeRedTestPipeline() {
        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("ComputeRedTest");
        pipeline.SetShader("ComputeRedTest");
        pipeline.AddDescriptorSetLayout("StaticDescriptorSet");
        pipeline.Build();
    }

    void CreateDebugTileView() {
        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("DebugTileView");
        pipeline.SetShader("DebugTileView");
        pipeline.AddPushConstant(sizeof(PushConstantsDebugTileView), VK_SHADER_STAGE_COMPUTE_BIT);
        pipeline.AddDescriptorSetLayout("StaticDescriptorSet");
        pipeline.Build();
    }

    void CreateDDGIRaytraceScenePipeline() {
        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("DDGIRaytraceScene");
        pipeline.SetShader("DDGIRaytraceScene");
        pipeline.AddDescriptorSetLayout("StaticDescriptorSet");
        pipeline.AddDescriptorSetLayout("DDGIRayQueryDescriptorSet");
        pipeline.AddPushConstant(sizeof(PushConstantsDDGIRaytraceScene), VK_SHADER_STAGE_COMPUTE_BIT);
        pipeline.Build();
    }

    void CreateDDGIPointCloudBaseColorPipeline() {
        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("DDGIPointCloudBaseColor");
        pipeline.SetShader("DDGIPointCloudBaseColor");
        pipeline.AddDescriptorSetLayout("StaticDescriptorSet");
        pipeline.AddPushConstant(sizeof(PushConstantsDDGIPointCloudBaseColor), VK_SHADER_STAGE_COMPUTE_BIT);
        pipeline.Build();
    }

    void CreateDDGIPointCloudLightingPipeline() {
        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("DDGIPointCloudLighting");
        pipeline.SetShader("DDGIPointCloudLighting");
        pipeline.AddDescriptorSetLayout("StaticDescriptorSet");
        pipeline.AddDescriptorSetLayout("DDGIRayQueryDescriptorSet");
        pipeline.AddPushConstant(sizeof(PushConstantsDDGIPointCloudLighting), VK_SHADER_STAGE_COMPUTE_BIT);
        pipeline.Build();
    }

    void CreateDDGIProbePointIndicesPipeline() {
        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("DDGIProbePointIndices");
        pipeline.SetShader("DDGIProbePointIndices");
        pipeline.AddPushConstant(sizeof(PushConstantsDDGIProbePointIndices), VK_SHADER_STAGE_COMPUTE_BIT);
        pipeline.Build();
    }

    void CreateDDGIProbeStateUpdatePipeline() {
        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("DDGIProbeStateUpdate");
        pipeline.SetShader("DDGIProbeStateUpdate");
        pipeline.AddPushConstant(sizeof(PushConstantsDDGIProbeStateUpdate), VK_SHADER_STAGE_COMPUTE_BIT);
        pipeline.Build();
    }

    void CreateDDGIProbeRelevancePipeline() {
        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("DDGIProbeRelevance");
        pipeline.SetShader("DDGIProbeRelevance");
        pipeline.AddDescriptorSetLayout("StaticDescriptorSet");
        pipeline.AddPushConstant(sizeof(PushConstantsDDGIProbeRelevance), VK_SHADER_STAGE_COMPUTE_BIT);
        pipeline.Build();
    }

    void CreateDDGIProbeDistanceListPipeline() {
        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("DDGIProbeDistanceList");
        pipeline.SetShader("DDGIProbeDistanceList");
        pipeline.AddPushConstant(sizeof(PushConstantsDDGIProbeDistanceList), VK_SHADER_STAGE_COMPUTE_BIT);
        pipeline.Build();
    }

    void CreateDDGIProbeDistanceDispatchArgsPipeline() {
        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("DDGIProbeDistanceDispatchArgs");
        pipeline.SetShader("DDGIProbeDistanceDispatchArgs");
        pipeline.AddPushConstant(sizeof(PushConstantsDDGIProbeDistanceDispatchArgs), VK_SHADER_STAGE_COMPUTE_BIT);
        pipeline.Build();
    }

    void CreateDDGIProbeDistancePipeline() {
        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("DDGIProbeDistance");
        pipeline.SetShader("DDGIProbeDistance");
        pipeline.AddDescriptorSetLayout("StaticDescriptorSet");
        pipeline.AddDescriptorSetLayout("DDGIRayQueryDescriptorSet");
        pipeline.AddPushConstant(sizeof(PushConstantsDDGIProbeDistance), VK_SHADER_STAGE_COMPUTE_BIT);
        pipeline.Build();
    }

    void CreateDDGIProbeDistanceBorderPipeline() {
        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("DDGIProbeDistanceBorder");
        pipeline.SetShader("DDGIProbeDistanceBorder");
        pipeline.AddDescriptorSetLayout("StaticDescriptorSet");
        pipeline.AddPushConstant(sizeof(PushConstantsDDGIProbeBorder), VK_SHADER_STAGE_COMPUTE_BIT);
        pipeline.Build();
    }

    void CreateDDGIProbeIrradianceDirtyPointCheckPipeline() {
        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("DDGIProbeIrradianceDirtyPointCheck");
        pipeline.SetShader("DDGIProbeIrradianceDirtyPointCheck");
        pipeline.AddPushConstant(sizeof(PushConstantsDDGIProbeIrradianceDirtyPointCheck), VK_SHADER_STAGE_COMPUTE_BIT);
        pipeline.Build();
    }

    void CreateDDGIProbeIrradianceListPipeline() {
        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("DDGIProbeIrradianceList");
        pipeline.SetShader("DDGIProbeIrradianceList");
        pipeline.AddPushConstant(sizeof(PushConstantsDDGIProbeIrradianceList), VK_SHADER_STAGE_COMPUTE_BIT);
        pipeline.Build();
    }

    void CreateDDGIProbeIrradianceDispatchArgsPipeline() {
        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("DDGIProbeIrradianceDispatchArgs");
        pipeline.SetShader("DDGIProbeIrradianceDispatchArgs");
        pipeline.AddPushConstant(sizeof(PushConstantsDDGIProbeIrradianceDispatchArgs), VK_SHADER_STAGE_COMPUTE_BIT);
        pipeline.Build();
    }

    void CreateDDGIProbeIrradiancePipeline() {
        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("DDGIProbeIrradiance");
        pipeline.SetShader("DDGIProbeIrradiance");
        pipeline.AddDescriptorSetLayout("StaticDescriptorSet");
        pipeline.AddDescriptorSetLayout("DDGIRayQueryDescriptorSet");
        pipeline.AddPushConstant(sizeof(PushConstantsDDGIProbeIrradiance), VK_SHADER_STAGE_COMPUTE_BIT);
        pipeline.Build();
    }

    void CreateDDGIProbeIrradianceBorderPipeline() {
        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("DDGIProbeIrradianceBorder");
        pipeline.SetShader("DDGIProbeIrradianceBorder");
        pipeline.AddDescriptorSetLayout("StaticDescriptorSet");
        pipeline.AddPushConstant(sizeof(PushConstantsDDGIProbeBorder), VK_SHADER_STAGE_COMPUTE_BIT);
        pipeline.Build();
    }

    void CreateDDGIProbeIrradianceTexturePipeline() {
        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("DDGIProbeIrradianceTexture");
        pipeline.SetShader("DDGIProbeIrradianceTexture");
        pipeline.AddDescriptorSetLayout("StaticDescriptorSet");
        pipeline.AddPushConstant(sizeof(PushConstantsDDGIProbeIrradianceTexture), VK_SHADER_STAGE_COMPUTE_BIT);
        pipeline.Build();
    }

    void CreateDDGIPointCloudDebugPipeline() {
        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("DDGIPointCloudDebug");
        pipeline.SetShader("DDGIPointCloudDebug");
        pipeline.SetRenderState("Debug3D");
        pipeline.AddPushConstant(sizeof(PushConstantsDDGIPointCloudDebug), VK_SHADER_STAGE_VERTEX_BIT);
        pipeline.SetTopology(VK_PRIMITIVE_TOPOLOGY_POINT_LIST);
        pipeline.SetCullMode(VK_CULL_MODE_NONE);
        pipeline.Build();
    }

    void CreateDDGIProbeDebugPipeline() {
        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("DDGIProbeDebug");
        pipeline.SetShader("DDGIProbeDebug");
        pipeline.SetRenderState("DDGIProbeDebug");
        pipeline.AddPushConstant(sizeof(PushConstantsDDGIProbeDebug), VK_SHADER_STAGE_VERTEX_BIT);
        pipeline.SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        pipeline.SetCullMode(VK_CULL_MODE_NONE);
        pipeline.SetVertexDescription<Vertex>();
        pipeline.Build();
    }

    void CreateDebugViewPipeline() {
        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("DebugView");
        pipeline.SetShader("DebugView");
        pipeline.AddDescriptorSetLayout("StaticDescriptorSet");
        pipeline.AddPushConstant(sizeof(PushConstantsDebugView), VK_SHADER_STAGE_FRAGMENT_BIT);
        pipeline.AddColorAttachmentFormat(VK_FORMAT_R16G16B16A16_SFLOAT);
        pipeline.SetDepthTest(false, false);
        pipeline.SetCullMode(VK_CULL_MODE_NONE);
        pipeline.Build();
    }

    void CreateDebugVertex3DLinePipeline() {
        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("DebugVertex3DLine");
        pipeline.SetShader("DebugVertex3D");
        pipeline.SetRenderState("Debug3D");
        pipeline.AddPushConstant(sizeof(PushConstantsDebug3D), VK_SHADER_STAGE_VERTEX_BIT);
        pipeline.SetTopology(VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
        pipeline.SetCullMode(VK_CULL_MODE_NONE);
        pipeline.SetVertexDescription<DebugVertex3D>();
        pipeline.Build();
    }

    void CreateDebugVertex3DPointPipeline() {
        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("DebugVertex3DPoint");
        pipeline.SetShader("DebugVertex3D");
        pipeline.SetRenderState("Debug3D");
        pipeline.AddPushConstant(sizeof(PushConstantsDebug3D), VK_SHADER_STAGE_VERTEX_BIT);
        pipeline.SetTopology(VK_PRIMITIVE_TOPOLOGY_POINT_LIST);
        pipeline.SetCullMode(VK_CULL_MODE_NONE);
        pipeline.SetVertexDescription<DebugVertex3D>();
        pipeline.Build();
    }

    void CreateDebugVertex2DLinePipeline() {
        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("DebugVertex2DLine");
        pipeline.SetShader("DebugVertex2D");
        pipeline.SetRenderState("Debug2D");
        pipeline.AddPushConstant(sizeof(PushConstantsDebug2D), VK_SHADER_STAGE_VERTEX_BIT);
        pipeline.SetTopology(VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
        pipeline.SetCullMode(VK_CULL_MODE_NONE);
        pipeline.SetVertexDescription<DebugVertex2D>();
        pipeline.Build();
    }

    void CreateDebugVertex2DPointPipeline() {
        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("DebugVertex2DPoint");
        pipeline.SetShader("DebugVertex2D");
        pipeline.SetRenderState("Debug2D");
        pipeline.AddPushConstant(sizeof(PushConstantsDebug2D), VK_SHADER_STAGE_VERTEX_BIT);
        pipeline.SetTopology(VK_PRIMITIVE_TOPOLOGY_POINT_LIST);
        pipeline.SetCullMode(VK_CULL_MODE_NONE);
        pipeline.SetVertexDescription<DebugVertex2D>();
        pipeline.Build();
    }

    // Material Resolve

    void CreateMaterialResolvePipeline() {
        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("MaterialResolve");
        pipeline.SetShader("MaterialResolve");
        pipeline.SetRenderState("MaterialResolve");
        pipeline.AddDescriptorSetLayout("StaticDescriptorSet");
        pipeline.AddPushConstant(sizeof(PushConstantsMaterialResolve), VK_SHADER_STAGE_FRAGMENT_BIT);
        pipeline.Build();
    }

    // Emissive bloom

    void CreateEmissiveForwardPipeline() {
        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("EmissiveForward");
        pipeline.SetShader("EmissiveForward");
        pipeline.SetRenderState("EmissiveForward");
        pipeline.AddDescriptorSetLayout("StaticDescriptorSet");
        pipeline.AddPushConstant(sizeof(PushConstantsEmissive), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
        pipeline.SetVertexDescription<Vertex>();
        pipeline.Build();
    }

    void CreateEmissiveBloomFilterPipeline() {
        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("EmissiveBloomFilter");
        pipeline.SetShader("EmissiveBloomFilter");
        pipeline.AddDescriptorSetLayout("StaticDescriptorSet");
        pipeline.AddPushConstant(sizeof(PushConstantsEmissiveBloomFilter), VK_SHADER_STAGE_COMPUTE_BIT);
        pipeline.Build();
    }

    void CreateEmissiveBloomCompositePipeline() {
        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("EmissiveBloomComposite");
        pipeline.SetShader("EmissiveBloomComposite");
        pipeline.AddDescriptorSetLayout("StaticDescriptorSet");
        pipeline.AddPushConstant(sizeof(PushConstantsEmissiveBloomComposite), VK_SHADER_STAGE_COMPUTE_BIT);
        pipeline.Build();
    }

    // Hierarchical depth buffer

    void CreateHiZPipeline() {
        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("HiZ");
        pipeline.SetShader("HiZ");
        pipeline.AddDescriptorSetLayout("StaticDescriptorSet");
        pipeline.Build();
    }

    // Indirect specular

    void CreateIndirectSpecularAMDInputPipeline() {
        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("IndirectSpecularAMDInput");
        pipeline.SetShader("IndirectSpecularAMDInput");
        pipeline.AddDescriptorSetLayout("StaticDescriptorSet");
        pipeline.AddDescriptorSetLayout("RayQueryDescriptorSet");
        pipeline.AddPushConstant(sizeof(PushConstantsIndirectSpecularAMDInput), VK_SHADER_STAGE_FRAGMENT_BIT);
        pipeline.SetRenderState("IndirectSpecularAMDRayInput");
        pipeline.Build();
    }

    void CreateIndirectSpecularAMDResolveRayInputPipeline() {
        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("IndirectSpecularAMDResolveRayInput");
        pipeline.SetShader("IndirectSpecularAMDResolveRayInput");
        pipeline.AddDescriptorSetLayout("StaticDescriptorSet");
        pipeline.Build();
    }

    void CreateIndirectSpecularAMDClassifyTilesPipeline() {
        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("IndirectSpecularAMDClassifyTiles");
        pipeline.SetShader("IndirectSpecularAMDClassifyTiles");
        pipeline.AddDescriptorSetLayout("StaticDescriptorSet");
        pipeline.Build();
    }

    void CreateIndirectSpecularAMDPrepareIndirectArgsPipeline() {
        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("IndirectSpecularAMDPrepareIndirectArgs");
        pipeline.SetShader("IndirectSpecularAMDPrepareIndirectArgs");
        pipeline.AddDescriptorSetLayout("StaticDescriptorSet");
        pipeline.Build();
    }

    void CreateIndirectSpecularAMDReprojectPipeline() {
        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("IndirectSpecularAMDReproject");
        pipeline.SetShader("IndirectSpecularAMDReproject");
        pipeline.AddDescriptorSetLayout("StaticDescriptorSet");
        pipeline.AddPushConstant(sizeof(PushConstantsIndirectSpecularAMDReproject), VK_SHADER_STAGE_COMPUTE_BIT);
        pipeline.Build();
    }

    void CreateIndirectSpecularAMDPrefilterPipeline() {
        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("IndirectSpecularAMDPrefilter");
        pipeline.SetShader("IndirectSpecularAMDPrefilter");
        pipeline.AddDescriptorSetLayout("StaticDescriptorSet");
        pipeline.AddPushConstant(sizeof(PushConstantsIndirectSpecularAMDPrefilter), VK_SHADER_STAGE_COMPUTE_BIT);
        pipeline.Build();
    }

    void CreateIndirectSpecularAMDResolveTemporalPipeline() {
        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("IndirectSpecularAMDResolveTemporal");
        pipeline.SetShader("IndirectSpecularAMDResolveTemporal");
        pipeline.AddDescriptorSetLayout("StaticDescriptorSet");
        pipeline.Build();
    }

    void CreateIndirectSpecularAMDStoreHistoryPipeline() {
        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("IndirectSpecularAMDStoreHistory");
        pipeline.SetShader("IndirectSpecularAMDStoreHistory");
        pipeline.AddDescriptorSetLayout("StaticDescriptorSet");
        pipeline.Build();
    }

    // Lighting

    void CreateLightingDeferredPipeline() {
        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("LightingDeferred");
        pipeline.SetShader("LightingDeferred");
        pipeline.AddDescriptorSetLayout("StaticDescriptorSet");
        pipeline.AddDescriptorSetLayout("RayQueryDescriptorSet");
        pipeline.AddPushConstant(sizeof(PushConstantsDeferredLighting), VK_SHADER_STAGE_FRAGMENT_BIT);
        pipeline.SetRenderState("LightingDeferred");
        pipeline.Build();
    }

    void CreateLightingForwardBlendedPipeline() {
        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("LightingForwardBlended");
        pipeline.SetShader("LightingForward");
        pipeline.AddDescriptorSetLayout("StaticDescriptorSet");
        pipeline.AddDescriptorSetLayout("RayQueryDescriptorSet");
        pipeline.AddPushConstant(sizeof(PushConstantsDeferredLighting), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
        pipeline.SetRenderState("LightingForwardBlended");
        pipeline.SetVertexDescription<Vertex>();
        pipeline.Build();
    }

    void CreatePointShadowPipeline(const std::string& name, const std::string& shaderName, VkCullModeFlags cullMode, bool useStaticDescriptorSet) {
        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline(name);
        pipeline.SetShader(shaderName);
        if (useStaticDescriptorSet) pipeline.AddDescriptorSetLayout("StaticDescriptorSet");
        pipeline.AddPushConstant(sizeof(PushConstantsPointShadow), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
        pipeline.SetDepthAttachmentFormat(VK_FORMAT_D16_UNORM);
        pipeline.SetDepthTest(true, true);
        pipeline.SetDepthCompareOp(VK_COMPARE_OP_LESS);
        pipeline.SetFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE);
        pipeline.SetCullMode(cullMode);
        pipeline.SetVertexDescription(Vertex::GetPositionUVLayout());
        pipeline.Build();
    }

    void CreatePointShadowPipelines() {
        CreatePointShadowPipeline("PointShadowOpaque", "PointShadow", VK_CULL_MODE_FRONT_BIT, false);
        CreatePointShadowPipeline("PointShadowAlphaDiscard", "PointShadowAlphaDiscard", VK_CULL_MODE_NONE, true);
    }

    void CreateSpriteSheetPipeline() {
        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("SpriteSheet");
        pipeline.SetShader("SpriteSheet");
        pipeline.SetRenderState("SpriteSheet");
        pipeline.AddDescriptorSetLayout("StaticDescriptorSet");
        pipeline.AddPushConstant(sizeof(PushConstantsSpriteSheet), VK_SHADER_STAGE_VERTEX_BIT);
        pipeline.SetVertexDescription(Vertex::GetPositionUVLayout());
        pipeline.Build();
    }

    // Loading Screen

    void CreateLoadingScreenPipeline() {
        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("LoadingScreen");
        pipeline.SetShader("FullscreenTriangle");
        pipeline.AddColorAttachmentFormat(VulkanSwapchainManager::GetSwapchainImageFormat());
        pipeline.SetDepthTest(false, false);
        pipeline.SetCullMode(VK_CULL_MODE_NONE);
        pipeline.Build();
    }

    // Present

    void CreatePresentPipeline() {
        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("Present");
        pipeline.SetShader("Present");
        pipeline.AddDescriptorSetLayout("StaticDescriptorSet");
        pipeline.AddColorAttachmentFormat(VulkanSwapchainManager::GetSwapchainImageFormat());
        pipeline.SetDepthTest(false, false);
        pipeline.SetCullMode(VK_CULL_MODE_NONE);
        pipeline.Build();
    }

    // Post processing

    void CreatePostProcessingPipeline() {
        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("PostProcessing");
        pipeline.SetShader("PostProcessing");
        pipeline.AddDescriptorSetLayout("StaticDescriptorSet");
        pipeline.AddPushConstant(sizeof(PushConstantsPostProcessing), VK_SHADER_STAGE_COMPUTE_BIT);
        pipeline.Build();
    }

    // Skybox

    void CreateSkyboxPipeline() {
        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("Skybox");
        pipeline.SetShader("Skybox");
        pipeline.SetRenderState("Skybox");
        pipeline.AddDescriptorSetLayout("StaticDescriptorSet");
        pipeline.AddPushConstant(sizeof(PushConstantsSkybox), VK_SHADER_STAGE_FRAGMENT_BIT);
        pipeline.Build();
    }

    // Tile culling

    void CreateTileWorldBoundsPipeline() {
        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("TileWorldBounds");
        pipeline.SetShader("TileWorldBounds");
        pipeline.AddDescriptorSetLayout("StaticDescriptorSet");
        pipeline.AddPushConstant(sizeof(PushConstantsTileWorldBounds), VK_SHADER_STAGE_COMPUTE_BIT);
        pipeline.Build();
    }

    void CreateTileLightCullingPipeline() {
        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("TileLightCulling");
        pipeline.SetShader("TileLightCulling");
        pipeline.AddDescriptorSetLayout("StaticDescriptorSet");
        pipeline.AddPushConstant(sizeof(PushConstantsTileLightCulling), VK_SHADER_STAGE_COMPUTE_BIT);
        pipeline.Build();
    }

    // Visibility

    void CreateVisibilityPipeline() {
        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("Visibility");
        pipeline.SetShader("Visibility");
        pipeline.SetRenderState("Visibility");
        pipeline.AddPushConstant(sizeof(PushConstantsVisibility), VK_SHADER_STAGE_VERTEX_BIT);
        pipeline.SetVertexDescription(Vertex::GetPositionUVLayout());
        pipeline.Build();
    }

    void CreateVisibilityAlphaDiscardPipeline() {
        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("VisibilityAlphaDiscard");
        pipeline.SetShader("VisibilityAlphaDiscard");
        pipeline.SetRenderState("VisibilityAlphaDiscard");
        pipeline.AddDescriptorSetLayout("StaticDescriptorSet");
        pipeline.AddPushConstant(sizeof(PushConstantsVisibility), VK_SHADER_STAGE_VERTEX_BIT);
        pipeline.SetVertexDescription(Vertex::GetPositionUVLayout());
        pipeline.Build();
    }

    // UI

    void CreateUIPipeline() {
        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("UI");
        pipeline.SetShader("UI");
        pipeline.AddDescriptorSetLayout("StaticDescriptorSet");
        pipeline.AddPushConstant(sizeof(PushConstantsUI), VK_SHADER_STAGE_VERTEX_BIT);
        pipeline.AddColorAttachmentFormat(VulkanSwapchainManager::GetSwapchainImageFormat());
        pipeline.SetDepthTest(false, false);
        pipeline.SetCullMode(VK_CULL_MODE_NONE);
        pipeline.SetColorBlending(true);
        pipeline.SetVertexDescription<Vertex2D>();
        pipeline.Build();
    }

    void CreateHairDepthPrepPipeline() {
        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("HairDepthPrep");
        pipeline.SetShader("HairDepthPrep");
        pipeline.AddDescriptorSetLayout("StaticDescriptorSet");
        pipeline.SetRenderState("HairDepthPrep");
        pipeline.SetSampleCount(VK_SAMPLE_COUNT_4_BIT);
        pipeline.SetCullMode(VK_CULL_MODE_NONE);
        pipeline.Build();
    }

    void CreateHairDepthPrePassPipeline() {
        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("HairDepthPrePass");
        pipeline.SetShader("HairDepthPrePass");
        pipeline.AddDescriptorSetLayout("StaticDescriptorSet");
        pipeline.AddPushConstant(sizeof(PushConstantsVisibility), VK_SHADER_STAGE_VERTEX_BIT);
        pipeline.SetRenderState("HairDepthPrePass");
        pipeline.SetSampleCount(VK_SAMPLE_COUNT_4_BIT);
        pipeline.SetVertexDescription(Vertex::GetPositionUVLayout());
        pipeline.Build();
    }

    void CreateHairLightingPipeline() {
        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("HairLighting");
        pipeline.SetShader("HairLighting");
        pipeline.AddDescriptorSetLayout("StaticDescriptorSet");
        pipeline.AddPushConstant(sizeof(PushConstantsHair), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
        pipeline.SetRenderState("HairLighting");
        pipeline.SetSampleCount(VK_SAMPLE_COUNT_4_BIT);
        pipeline.SetVertexDescription<Vertex>();
        pipeline.Build();
    }

    void CreateHairSurfaceLightingPipeline() {
        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("HairSurfaceLighting");
        pipeline.SetShader("HairSurfaceLighting");
        pipeline.AddDescriptorSetLayout("StaticDescriptorSet");
        pipeline.AddPushConstant(sizeof(PushConstantsHair), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
        pipeline.SetRenderState("HairLighting");
        pipeline.SetSampleCount(VK_SAMPLE_COUNT_4_BIT);
        pipeline.SetVertexDescription<Vertex>();
        pipeline.Build();
    }

    void CreateHairCompositePipeline() {
        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("HairComposite");
        pipeline.SetShader("HairComposite");
        pipeline.AddDescriptorSetLayout("StaticDescriptorSet");
        pipeline.Build();
    }

}

namespace VulkanRenderer {

    void CreatePipelines() {
        // Debug
        CreateDebugTileView();
        CreateDDGIRaytraceScenePipeline();
        CreateDDGIPointCloudBaseColorPipeline();
        CreateDDGIPointCloudLightingPipeline();
        CreateDDGIProbePointIndicesPipeline();
        CreateDDGIProbeStateUpdatePipeline();
        CreateDDGIProbeRelevancePipeline();
        CreateDDGIProbeDistanceListPipeline();
        CreateDDGIProbeDistanceDispatchArgsPipeline();
        CreateDDGIProbeDistancePipeline();
        CreateDDGIProbeDistanceBorderPipeline();
        CreateDDGIProbeIrradianceDirtyPointCheckPipeline();
        CreateDDGIProbeIrradianceListPipeline();
        CreateDDGIProbeIrradianceDispatchArgsPipeline();
        CreateDDGIProbeIrradiancePipeline();
        CreateDDGIProbeIrradianceBorderPipeline();
        CreateDDGIProbeIrradianceTexturePipeline();
        CreateDDGIPointCloudDebugPipeline();
        CreateDDGIProbeDebugPipeline();
        CreateDebugViewPipeline();
        CreateDebugVertex3DLinePipeline();
        CreateDebugVertex3DPointPipeline();
        CreateDebugVertex2DLinePipeline();
        CreateDebugVertex2DPointPipeline();

        // Loading screen
        CreateLoadingScreenPipeline();

        // Game
        CreateComputeSkinningPipeline();
        CreateVisibilityPipeline();
        CreateVisibilityAlphaDiscardPipeline();
        CreateMaterialResolvePipeline();
        CreateEmissiveForwardPipeline();
        CreateEmissiveBloomFilterPipeline();
        CreateEmissiveBloomCompositePipeline();
        CreateLightingDeferredPipeline();
        CreateLightingForwardBlendedPipeline();
        CreatePointShadowPipelines();
        CreateSkyboxPipeline();
        CreateSpriteSheetPipeline();

        // Misc
        CreateComputeRedTestPipeline();

        // Post Processing
        CreatePostProcessingPipeline();

        // Tile culling
        CreateTileWorldBoundsPipeline();
        CreateTileLightCullingPipeline();

        // Hierarchical depth buffer
        CreateHiZPipeline();

        // Indirect specular
        CreateIndirectSpecularAMDInputPipeline();
        CreateIndirectSpecularAMDResolveRayInputPipeline();
        CreateIndirectSpecularAMDClassifyTilesPipeline();
        CreateIndirectSpecularAMDPrepareIndirectArgsPipeline();
        CreateIndirectSpecularAMDReprojectPipeline();
        CreateIndirectSpecularAMDPrefilterPipeline();
        CreateIndirectSpecularAMDResolveTemporalPipeline();
        CreateIndirectSpecularAMDStoreHistoryPipeline();

        // Final present
        CreatePresentPipeline();

        CreateHairDepthPrepPipeline();
        CreateHairDepthPrePassPipeline();
        CreateHairLightingPipeline();
        CreateHairSurfaceLightingPipeline();
        CreateHairCompositePipeline();

        CreateUIPipeline();
    }
}
