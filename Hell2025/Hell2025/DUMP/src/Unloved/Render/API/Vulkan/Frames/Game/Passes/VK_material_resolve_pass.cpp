#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/Render/API/Vulkan/Types/vk_allocated_image.h"
#include "Hell/Render/API/Vulkan/Types/vk_mesh_buffer.h"
#include "Hell/Render/API/Vulkan/Types/vk_pipeline.h"

#include "Unloved/Render/API/Vulkan/VK_renderer.h"
#include "Unloved/Render/API/Vulkan/VK_renderer_internal.h"
#include "Unloved/Render/API/Vulkan/VK_push_constants.h"
#include "Unloved/Render/RendererConstants.h"

namespace VulkanRenderer {
    void MaterialResolvePass(VkCommandBuffer commandBuffer) {
        ProfilerVulkanZoneFunction();

        const VulkanFrameData& frameData = GetCurrentFrameData();

        AllocatedImage* baseColorImage = VulkanResourceManager::GetAllocatedImage("BaseColorMetallic");
        AllocatedImage* normalImage = VulkanResourceManager::GetAllocatedImage("NormalXYRoughnessMisc");
        AllocatedImage* velocityImage = VulkanResourceManager::GetAllocatedImage("VelocityXYOcclusionSubSurface");
        AllocatedImage* amdMaterialRoughnessImage = VulkanResourceManager::GetAllocatedImage("IndirectSpecularAMDMaterialRoughness");
        VulkanDescriptorSet* staticDescriptorSet = VulkanResourceManager::GetDescriptorSet("StaticDescriptorSet");
        VulkanMeshBuffer* assetGeometry = VulkanResourceManager::GetMeshBuffer("AssetGeometry");
        VulkanMeshBuffer* proceduralGeometry = VulkanResourceManager::GetMeshBuffer("Procedural");
        VulkanBuffer* skinnedVertexBuffer = frameData.buffers.skinnedVertices != 0 ? VulkanResourceManager::GetBuffer(frameData.buffers.skinnedVertices) : nullptr;
        VulkanBuffer* previousSkinnedPositionBuffer = frameData.buffers.previousSkinnedPositions != 0 ? VulkanResourceManager::GetBuffer(frameData.buffers.previousSkinnedPositions) : nullptr;
        VulkanRenderState* renderState = VulkanResourceManager::GetRenderState("MaterialResolve");
        VulkanPipeline* pipeline = VulkanResourceManager::GetPipeline("MaterialResolve");

        if (!baseColorImage) return;
        if (!normalImage) return;
        if (!velocityImage) return;
        if (!amdMaterialRoughnessImage) return;
        if (!staticDescriptorSet) return;
        if (!assetGeometry) return;
        if (!proceduralGeometry) return;
        if (!skinnedVertexBuffer) return;
        if (!previousSkinnedPositionBuffer) return;
        if (!renderState) return;
        if (!pipeline) return;
        if (!assetGeometry->GetVertexBuffer()) return;
        if (!assetGeometry->GetIndexBuffer()) return;
        if (!proceduralGeometry->GetVertexBuffer()) return;
        if (!proceduralGeometry->GetIndexBuffer()) return;

        // Begin
        VkExtent2D extent = baseColorImage->GetExtent2D();
        if (!BeginRenderState(commandBuffer, *renderState, extent)) return;

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetHandle());
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetLayout(), 0, 1, staticDescriptorSet->GetHandlePtr(), 0, nullptr);

        VkViewport viewport{};
        viewport.width = static_cast<float>(extent.width);
        viewport.height = static_cast<float>(extent.height);
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.extent = extent;
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        PushConstantsMaterialResolve pushConstants{};
        pushConstants.frameAddressTableDeviceAddress = GetFrameAddressTableDeviceAddress();

        // Static
        pushConstants.vertexBufferDeviceAddress = assetGeometry->GetVertexBufferAddress();
        pushConstants.indexBufferDeviceAddress = assetGeometry->GetIndexBufferAddress();
        pushConstants.previousSkinnedPositionsDeviceAddress = 0;
        pushConstants.vertexCount = static_cast<uint32_t>(assetGeometry->GetVertexBuffer()->GetSize() / sizeof(Vertex));
        pushConstants.indexCount = static_cast<uint32_t>(assetGeometry->GetIndexBuffer()->GetSize() / sizeof(uint32_t));
        pushConstants.hasPreviousSkinnedPositions = 0;

        vkCmdPushConstants(commandBuffer, pipeline->GetLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstants), &pushConstants);
        SetStencilReference(commandBuffer, STENCIL_VERTEX_BUFFER_ASSET);
        vkCmdDraw(commandBuffer, 3, 1, 0, 0);

        // Procedural
        pushConstants.vertexBufferDeviceAddress = proceduralGeometry->GetVertexBufferAddress();
        pushConstants.indexBufferDeviceAddress = proceduralGeometry->GetIndexBufferAddress();
        pushConstants.vertexCount = static_cast<uint32_t>(proceduralGeometry->GetVertexBuffer()->GetSize() / sizeof(Vertex));
        pushConstants.indexCount = static_cast<uint32_t>(proceduralGeometry->GetIndexBuffer()->GetSize() / sizeof(uint32_t));

        vkCmdPushConstants(commandBuffer, pipeline->GetLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstants), &pushConstants);
        SetStencilReference(commandBuffer, STENCIL_VERTEX_BUFFER_PROCEDURAL);
        vkCmdDraw(commandBuffer, 3, 1, 0, 0);

        // Skinned
        pushConstants.vertexBufferDeviceAddress = skinnedVertexBuffer->GetDeviceAddress();
        pushConstants.indexBufferDeviceAddress = assetGeometry->GetIndexBufferAddress();
        pushConstants.previousSkinnedPositionsDeviceAddress = previousSkinnedPositionBuffer->GetDeviceAddress();
        pushConstants.vertexCount = static_cast<uint32_t>(skinnedVertexBuffer->GetSize() / sizeof(Vertex));
        pushConstants.indexCount = static_cast<uint32_t>(assetGeometry->GetIndexBuffer()->GetSize() / sizeof(uint32_t));
        pushConstants.hasPreviousSkinnedPositions = 1;

        vkCmdPushConstants(commandBuffer, pipeline->GetLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstants), &pushConstants);
        SetStencilReference(commandBuffer, STENCIL_VERTEX_BUFFER_SKINNED);
        vkCmdDraw(commandBuffer, 3, 1, 0, 0);

        // End
        EndRenderState(commandBuffer);

        baseColorImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_TRANSFER_BIT);
        normalImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);
        velocityImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);
        amdMaterialRoughnessImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);
    }
}
