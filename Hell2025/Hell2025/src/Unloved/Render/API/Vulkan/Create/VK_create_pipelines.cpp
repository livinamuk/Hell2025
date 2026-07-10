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

    void CreateDebugTileView() {
        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("DebugTileView");
        pipeline.SetShader("DebugTileView");
        pipeline.AddDescriptorSetLayout("StaticDescriptorSet");
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
        pipeline.AddPushConstant(sizeof(PushConstantsFrameResources), VK_SHADER_STAGE_COMPUTE_BIT);
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

    //void CreateHairDepthPrepPipeline() {
    //    VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("HairDepthPrep");
    //    pipeline.SetShader("HairDepthPrep");
    //    pipeline.AddDescriptorSetLayout("StaticDescriptorSet");
    //    pipeline.SetRenderState("HairDepthPrep");
    //    pipeline.SetSampleCount(VK_SAMPLE_COUNT_4_BIT);
    //    pipeline.SetCullMode(VK_CULL_MODE_NONE);
    //    pipeline.Build();
    //}
    //
    //void CreateHairDepthPrePassPipeline() {
    //    VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("HairDepthPrePass");
    //    pipeline.SetShader("HairDepthPrePass");
    //    pipeline.AddDescriptorSetLayout("StaticDescriptorSet");
    //    pipeline.AddPushConstant(sizeof(PushConstantsVisibility), VK_SHADER_STAGE_VERTEX_BIT);
    //    pipeline.SetRenderState("HairDepthPrePass");
    //    pipeline.SetSampleCount(VK_SAMPLE_COUNT_4_BIT);
    //    pipeline.SetVertexDescription(Vertex::GetPositionUVLayout());
    //    pipeline.Build();
    //}
    //
    //void CreateHairLightingPipeline() {
    //    VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("HairLighting");
    //    pipeline.SetShader("HairLighting");
    //    pipeline.AddDescriptorSetLayout("StaticDescriptorSet");
    //    pipeline.AddDescriptorSetLayout("RayQueryDescriptorSet");
    //    pipeline.AddPushConstant(sizeof(PushConstantsHair), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
    //    pipeline.SetRenderState("HairLighting");
    //    pipeline.SetSampleCount(VK_SAMPLE_COUNT_4_BIT);
    //    pipeline.SetVertexDescription<Vertex>();
    //    pipeline.Build();
    //}
    //
    //void CreateHairCompositePipeline() {
    //    VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("HairComposite");
    //    pipeline.SetShader("HairComposite");
    //    pipeline.AddDescriptorSetLayout("StaticDescriptorSet");
    //    pipeline.Build();
    //}

}

namespace VulkanRenderer {

    void CreatePipelines() {

        // Debug
        CreateDebugTileView();
        CreateDebugViewPipeline();

        // Loading screen
        CreateLoadingScreenPipeline();

        // Game
        CreateComputeSkinningPipeline();
        CreateVisibilityPipeline();
        CreateVisibilityAlphaDiscardPipeline();
        CreateMaterialResolvePipeline();
        CreateLightingDeferredPipeline();
        CreateLightingForwardBlendedPipeline();
        CreateSkyboxPipeline();

        // Misc
        CreateComputeRedTestPipeline();

        // Post Processing
        CreatePostProcessingPipeline();

        // Final present
        CreatePresentPipeline();

        // CreateHairDepthPrepPipeline();
        // CreateHairDepthPrePassPipeline();
        // CreateHairLightingPipeline();
        // CreateHairCompositePipeline();

        CreateUIPipeline();
    }
}
