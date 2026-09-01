#include "Unloved/Render/API/Vulkan/VK_renderer.h"
#include "Unloved/Render/API/Vulkan/VK_renderer_internal.h"

#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/Render/API/Vulkan/Types/vk_allocated_image.h"
#include "Hell/Render/API/Vulkan/Types/vk_buffer.h"
#include "Hell/Render/API/Vulkan/Types/vk_pipeline.h"

#include "Unloved/Render/API/Vulkan/VK_draw.h"
#include "Unloved/Render/API/Vulkan/VK_push_constants.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Render/RendererConstants.h"
#include "Unloved/Viewport/ViewportManager.h"

using namespace Unloved;

namespace VulkanRenderer {
    namespace {

    void RenderVisibilityPassOpaque(VkCommandBuffer commandBuffer, VkExtent2D extent) {
        ProfilerVulkanZoneFunction();

        const DrawCommandsSet& drawInfoSet = RenderDataManager::GetDrawInfoSet();
        const VulkanFrameData& frameData = GetCurrentFrameData();

        VulkanMeshBuffer* assetGeometry = VulkanResourceManager::GetMeshBuffer("AssetGeometry");
        VulkanMeshBuffer* proceduralGeometry = VulkanResourceManager::GetMeshBuffer("Procedural");
        VulkanRenderState* renderState = VulkanResourceManager::GetRenderState("Visibility");
        VulkanPipeline* pipeline = VulkanResourceManager::GetPipeline("Visibility");
        VulkanBuffer* skinnedVertexBuffer = frameData.buffers.skinnedVertices != 0 ? VulkanResourceManager::GetBuffer(frameData.buffers.skinnedVertices) : nullptr;

        if (!assetGeometry) return;
        if (!proceduralGeometry) return;
        if (!renderState) return;
        if (!pipeline) return;
        if (!skinnedVertexBuffer) return;
        if (!assetGeometry->GetVertexBuffer()) return;
        if (!assetGeometry->GetIndexBuffer()) return;
        if (!proceduralGeometry->GetVertexBuffer()) return;
        if (!proceduralGeometry->GetIndexBuffer()) return;

        std::array<VulkanDrawCommandBatch, 4> standardCommands = WriteDrawCommandsByViewport(drawInfoSet.standard);
        std::array<VulkanDrawCommandBatch, 4> proceduralCommands = WriteDrawCommandsByViewport(drawInfoSet.procedural);
        std::array<VulkanDrawCommandBatch, 4> skinnedStandardCommands = WriteDrawCommandsByViewport(drawInfoSet.skinnedStandard);
        std::array<VulkanDrawCommandBatch, 4> skinnedNonDeformingStandardCommands = WriteDrawCommandsByViewport(drawInfoSet.skinnedNonDeformingStandard);
        std::array<VulkanDrawCommandBatch, 4> viewWeaponStandardCommands = WriteDrawCommandsByViewport(drawInfoSet.viewWeaponStandard);
        std::array<VulkanDrawCommandBatch, 4> skinnedViewWeaponStandardCommands = WriteDrawCommandsByViewport(drawInfoSet.skinnedViewWeaponStandard);
        std::array<VulkanDrawCommandBatch, 4> skinnedNonDeformingViewWeaponStandardCommands = WriteDrawCommandsByViewport(drawInfoSet.skinnedNonDeformingViewWeaponStandard);

        if (!BeginRenderState(commandBuffer, *renderState, extent)) return;

        for (uint32_t i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (viewport->IsVisible()) {

                SetGameViewportAndScissor(commandBuffer, *viewport, extent);

                PushConstantsVisibility pushConstants{};
                pushConstants.frameAddressTableDeviceAddress = GetFrameAddressTableDeviceAddress();
                pushConstants.skinnedVerticesDeviceAddress = skinnedVertexBuffer->GetDeviceAddress();
                pushConstants.viewportIndex = i;

                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetHandle());
                vkCmdPushConstants(commandBuffer, pipeline->GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushConstants), &pushConstants);

                BindVertexBuffer(commandBuffer, assetGeometry->GetVertexBuffer());
                BindIndexBuffer(commandBuffer, assetGeometry->GetIndexBuffer());
                SetStencilReference(commandBuffer, STENCIL_VERTEX_BUFFER_ASSET | STENCIL_BIT_WORLD_LIGHTING);
                MultiDrawIndexedCommands(commandBuffer, standardCommands[i]);
                MultiDrawIndexedCommands(commandBuffer, skinnedNonDeformingStandardCommands[i]);

                SetStencilReference(commandBuffer, STENCIL_VERTEX_BUFFER_ASSET | STENCIL_BIT_VIEW_WEAPON_LIGHTING);
                MultiDrawIndexedCommands(commandBuffer, viewWeaponStandardCommands[i]);
                MultiDrawIndexedCommands(commandBuffer, skinnedNonDeformingViewWeaponStandardCommands[i]);

                BindVertexBuffer(commandBuffer, proceduralGeometry->GetVertexBuffer());
                BindIndexBuffer(commandBuffer, proceduralGeometry->GetIndexBuffer());
                SetStencilReference(commandBuffer, STENCIL_VERTEX_BUFFER_PROCEDURAL | STENCIL_BIT_WORLD_LIGHTING);
                MultiDrawIndexedCommands(commandBuffer, proceduralCommands[i]);

                BindVertexBuffer(commandBuffer, skinnedVertexBuffer);
                BindIndexBuffer(commandBuffer, assetGeometry->GetIndexBuffer());
                SetStencilReference(commandBuffer, STENCIL_VERTEX_BUFFER_SKINNED | STENCIL_BIT_WORLD_LIGHTING);
                MultiDrawIndexedCommands(commandBuffer, skinnedStandardCommands[i]);

                SetStencilReference(commandBuffer, STENCIL_VERTEX_BUFFER_SKINNED | STENCIL_BIT_VIEW_WEAPON_LIGHTING);
                MultiDrawIndexedCommands(commandBuffer, skinnedViewWeaponStandardCommands[i]);
            }
        }

        EndRenderState(commandBuffer);
    }

    void RenderVisibilityAlphaDiscardPass(VkCommandBuffer commandBuffer, VkExtent2D extent) {
        ProfilerVulkanZoneFunction();

        const DrawCommandsSet& drawInfoSet = RenderDataManager::GetDrawInfoSet();
        const VulkanFrameData& frameData = GetCurrentFrameData();

        VulkanDescriptorSet* staticDescriptorSet = VulkanResourceManager::GetDescriptorSet("StaticDescriptorSet");
        VulkanMeshBuffer* meshBuffer = VulkanResourceManager::GetMeshBuffer("AssetGeometry");
        VulkanRenderState* renderState = VulkanResourceManager::GetRenderState("VisibilityAlphaDiscard");
        VulkanPipeline* pipeline = VulkanResourceManager::GetPipeline("VisibilityAlphaDiscard");
        VulkanBuffer* skinnedVertexBuffer = frameData.buffers.skinnedVertices != 0 ? VulkanResourceManager::GetBuffer(frameData.buffers.skinnedVertices) : nullptr;

        if (!staticDescriptorSet) return;
        if (!meshBuffer) return;
        if (!renderState) return;
        if (!pipeline) return;
        if (!skinnedVertexBuffer) return;
        if (!meshBuffer->GetVertexBuffer()) return;
        if (!meshBuffer->GetIndexBuffer()) return;

        std::array<VulkanDrawCommandBatch, 4> alphaDiscardCommands = WriteDrawCommandsByViewport(drawInfoSet.alphaDiscard);
        std::array<VulkanDrawCommandBatch, 4> hairCommands = WriteDrawCommandsByViewport(drawInfoSet.hair);
        std::array<VulkanDrawCommandBatch, 4> skinnedAlphaDiscardCommands = WriteDrawCommandsByViewport(drawInfoSet.skinnedAlphaDiscard);
        std::array<VulkanDrawCommandBatch, 4> skinnedHairCommands = WriteDrawCommandsByViewport(drawInfoSet.skinnedHair);
        std::array<VulkanDrawCommandBatch, 4> skinnedNonDeformingAlphaDiscardCommands = WriteDrawCommandsByViewport(drawInfoSet.skinnedNonDeformingAlphaDiscard);
        std::array<VulkanDrawCommandBatch, 4> skinnedNonDeformingHairCommands = WriteDrawCommandsByViewport(drawInfoSet.skinnedNonDeformingHair);
        std::array<VulkanDrawCommandBatch, 4> viewWeaponAlphaDiscardCommands = WriteDrawCommandsByViewport(drawInfoSet.viewWeaponAlphaDiscard);
        std::array<VulkanDrawCommandBatch, 4> skinnedViewWeaponAlphaDiscardCommands = WriteDrawCommandsByViewport(drawInfoSet.skinnedViewWeaponAlphaDiscard);
        std::array<VulkanDrawCommandBatch, 4> skinnedNonDeformingViewWeaponAlphaDiscardCommands = WriteDrawCommandsByViewport(drawInfoSet.skinnedNonDeformingViewWeaponAlphaDiscard);

        if (!BeginRenderState(commandBuffer, *renderState, extent)) return;

        for (uint32_t i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (viewport->IsVisible()) {

                SetGameViewportAndScissor(commandBuffer, *viewport, extent);

                PushConstantsVisibility pushConstants{};
                pushConstants.frameAddressTableDeviceAddress = GetFrameAddressTableDeviceAddress();
                pushConstants.skinnedVerticesDeviceAddress = skinnedVertexBuffer->GetDeviceAddress();
                pushConstants.viewportIndex = i;
                pushConstants.useDepthOffset = 0;

                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetHandle());
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetLayout(), 0, 1, staticDescriptorSet->GetHandlePtr(), 0, nullptr);
                vkCmdPushConstants(commandBuffer, pipeline->GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushConstants), &pushConstants);

                BindVertexBuffer(commandBuffer, meshBuffer->GetVertexBuffer());
                BindIndexBuffer(commandBuffer, meshBuffer->GetIndexBuffer());
                SetStencilReference(commandBuffer, STENCIL_VERTEX_BUFFER_ASSET | STENCIL_BIT_WORLD_LIGHTING);
                MultiDrawIndexedCommands(commandBuffer, alphaDiscardCommands[i]);
                MultiDrawIndexedCommands(commandBuffer, skinnedNonDeformingAlphaDiscardCommands[i]);

                SetStencilReference(commandBuffer, STENCIL_VERTEX_BUFFER_ASSET | STENCIL_BIT_VIEW_WEAPON_LIGHTING);
                MultiDrawIndexedCommands(commandBuffer, viewWeaponAlphaDiscardCommands[i]);
                MultiDrawIndexedCommands(commandBuffer, skinnedNonDeformingViewWeaponAlphaDiscardCommands[i]);

                pushConstants.useDepthOffset = 1;
                vkCmdPushConstants(commandBuffer, pipeline->GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushConstants), &pushConstants);
                SetStencilReference(commandBuffer, STENCIL_VERTEX_BUFFER_ASSET | STENCIL_BIT_HAIR);
                MultiDrawIndexedCommands(commandBuffer, hairCommands[i]);
                MultiDrawIndexedCommands(commandBuffer, skinnedNonDeformingHairCommands[i]);

                BindVertexBuffer(commandBuffer, skinnedVertexBuffer);
                BindIndexBuffer(commandBuffer, meshBuffer->GetIndexBuffer());
                pushConstants.useDepthOffset = 0;
                vkCmdPushConstants(commandBuffer, pipeline->GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushConstants), &pushConstants);
                SetStencilReference(commandBuffer, STENCIL_VERTEX_BUFFER_SKINNED | STENCIL_BIT_WORLD_LIGHTING);
                MultiDrawIndexedCommands(commandBuffer, skinnedAlphaDiscardCommands[i]);

                SetStencilReference(commandBuffer, STENCIL_VERTEX_BUFFER_SKINNED | STENCIL_BIT_VIEW_WEAPON_LIGHTING);
                MultiDrawIndexedCommands(commandBuffer, skinnedViewWeaponAlphaDiscardCommands[i]);

                pushConstants.useDepthOffset = 1;
                vkCmdPushConstants(commandBuffer, pipeline->GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushConstants), &pushConstants);
                SetStencilReference(commandBuffer, STENCIL_VERTEX_BUFFER_SKINNED | STENCIL_BIT_HAIR);
                MultiDrawIndexedCommands(commandBuffer, skinnedHairCommands[i]);
            }
        }

        EndRenderState(commandBuffer);
    }

    }

    void VisibilityPass(VkCommandBuffer commandBuffer) {
        ProfilerVulkanZoneFunction();

        AllocatedImage* visibilityImage = VulkanResourceManager::GetAllocatedImage("Visibility");
        AllocatedImage* depthImage = VulkanResourceManager::GetAllocatedImage("Depth");
        if (!visibilityImage || !depthImage) return;

        VkExtent2D extent = visibilityImage->GetExtent2D();

        RenderVisibilityPassOpaque(commandBuffer, extent);
        RenderVisibilityAlphaDiscardPass(commandBuffer, extent);

        visibilityImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);
        depthImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);
    }
}
