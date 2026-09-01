#include "Unloved/Render/API/Vulkan/VK_renderer.h"
#include "Unloved/Render/API/Vulkan/VK_renderer_internal.h"

#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/Render/API/Vulkan/Types/vk_allocated_image.h"
#include "Hell/Render/API/Vulkan/Types/vk_buffer.h"
#include "Hell/Render/API/Vulkan/Types/vk_descriptor_set.h"
#include "Hell/Render/API/Vulkan/Types/vk_mesh_buffer.h"
#include "Hell/Render/API/Vulkan/Types/vk_pipeline.h"
#include "Unloved/Render/API/Vulkan/VK_draw.h"
#include "Unloved/Render/API/Vulkan/VK_push_constants.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Viewport/ViewportManager.h"

#include <array>

using namespace Unloved;

namespace VulkanRenderer {
    void LightingForwardBlendedPass(VkCommandBuffer commandBuffer) {
        ProfilerVulkanZoneFunction();

        const DrawCommandsSet& drawInfoSet = RenderDataManager::GetDrawInfoSet();
        const VulkanFrameData& frameData = GetCurrentFrameData();

        AllocatedImage* lightingImage = VulkanResourceManager::GetAllocatedImage("Lighting");
        AllocatedImage* depthImage = VulkanResourceManager::GetAllocatedImage("Depth");
        AllocatedImage* indirectDiffuseImage = VulkanResourceManager::GetAllocatedImage("IndirectDiffuse");
        AllocatedImage* indirectDiffuseSurfaceImage = VulkanResourceManager::GetAllocatedImage("IndirectDiffuseSurface");
        VulkanPipeline* pipeline = VulkanResourceManager::GetPipeline("LightingForwardBlended");
        VulkanRenderState* renderState = VulkanResourceManager::GetRenderState("LightingForwardBlended");
        VulkanDescriptorSet* staticDescriptorSet = VulkanResourceManager::GetDescriptorSet("StaticDescriptorSet");
        VulkanDescriptorSetResource* rayQueryDescriptorSetResource = VulkanResourceManager::GetDescriptorSetResource("RayQueryDescriptorSet");
        VulkanDescriptorSet* rayQueryDescriptorSet = rayQueryDescriptorSetResource ? &rayQueryDescriptorSetResource->GetSet(GetCurrentFrameIndex()) : nullptr;
        VulkanMeshBuffer* meshBuffer = VulkanResourceManager::GetMeshBuffer("AssetGeometry");
        VulkanBuffer* rayQueryBLASDataBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.rayQueryBLASData);
        VulkanBuffer* rayQuerySceneRenderItemIndexBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.rayQuerySceneRenderItemIndices);
        VulkanBuffer* skinnedVertexBuffer = frameData.buffers.skinnedVertices != 0 ? VulkanResourceManager::GetBuffer(frameData.buffers.skinnedVertices) : nullptr;

        if (!lightingImage) return;
        if (!depthImage) return;
        if (!indirectDiffuseImage) return;
        if (!indirectDiffuseSurfaceImage) return;
        if (!pipeline) return;
        if (!renderState) return;
        if (!staticDescriptorSet) return;
        if (!rayQueryDescriptorSet) return;
        if (!meshBuffer) return;
        if (!rayQueryBLASDataBuffer) return;
        if (!rayQuerySceneRenderItemIndexBuffer) return;
        if (!skinnedVertexBuffer) return;
        if (!meshBuffer->GetVertexBuffer()) return;
        if (!meshBuffer->GetIndexBuffer()) return;

        std::array<VulkanDrawCommandBatch, 4> blendedCommands = WriteDrawCommandsByViewport(drawInfoSet.blended);
        std::array<VulkanDrawCommandBatch, 4> skinnedBlendedCommands = WriteDrawCommandsByViewport(drawInfoSet.skinnedBlended);
        std::array<VulkanDrawCommandBatch, 4> skinnedNonDeformingBlendedCommands = WriteDrawCommandsByViewport(drawInfoSet.skinnedNonDeformingBlended);

        VkExtent2D extent = lightingImage->GetExtent2D();
        indirectDiffuseImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);
        indirectDiffuseSurfaceImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);

        if (!BeginRenderState(commandBuffer, *renderState, extent)) return;

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetHandle());
        VkDescriptorSet descriptorSets[] = { staticDescriptorSet->GetHandle(), rayQueryDescriptorSet->GetHandle() };
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetLayout(), 0, 2, descriptorSets, 0, nullptr);

        PushConstantsDeferredLighting pushConstants{};
        pushConstants.frameAddressTableDeviceAddress = GetFrameAddressTableDeviceAddress();
        pushConstants.rayQueryBLASDataDeviceAddress = rayQueryBLASDataBuffer->GetDeviceAddress();
        pushConstants.rayQuerySceneRenderItemIndicesDeviceAddress = rayQuerySceneRenderItemIndexBuffer->GetDeviceAddress();
        vkCmdPushConstants(commandBuffer, pipeline->GetLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstants), &pushConstants);

        BindVertexBuffer(commandBuffer, meshBuffer->GetVertexBuffer());
        BindIndexBuffer(commandBuffer, meshBuffer->GetIndexBuffer());

        // Static and Procedural
        for (uint32_t i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            SetGameViewportAndScissor(commandBuffer, *viewport, extent);
            pushConstants.viewportIndex = i;
            vkCmdPushConstants(commandBuffer, pipeline->GetLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstants), &pushConstants);

            MultiDrawIndexedCommands(commandBuffer, blendedCommands[i]);
            MultiDrawIndexedCommands(commandBuffer, skinnedNonDeformingBlendedCommands[i]);
        }

        // Skinned
        BindVertexBuffer(commandBuffer, skinnedVertexBuffer);
        BindIndexBuffer(commandBuffer, meshBuffer->GetIndexBuffer());

        for (uint32_t i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            SetGameViewportAndScissor(commandBuffer, *viewport, extent);
            pushConstants.viewportIndex = i;
            vkCmdPushConstants(commandBuffer, pipeline->GetLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstants), &pushConstants);

            MultiDrawIndexedCommands(commandBuffer, skinnedBlendedCommands[i]);
        }

        EndRenderState(commandBuffer);
    }
}
