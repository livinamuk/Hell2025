#include "Unloved/Render/API/Vulkan/VK_renderer.h"
#include "Unloved/Render/API/Vulkan/VK_renderer_internal.h"

#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/Render/API/Vulkan/Types/vk_allocated_image.h"
#include "Hell/Render/API/Vulkan/Types/vk_buffer.h"
#include "Hell/Render/API/Vulkan/Types/vk_descriptor_set.h"
#include "Hell/Render/API/Vulkan/Types/vk_pipeline.h"

#include "Unloved/Render/API/Vulkan/VK_push_constants.h"
#include "Unloved/Render/Renderer.h"

using namespace Unloved;

namespace VulkanRenderer {

    void DebugViewPass(VkCommandBuffer commandBuffer) {
        ProfilerVulkanZoneFunction();

        if (!Renderer::OverrideStateUsesDebugViewPass()) return;

        AllocatedImage* lightingImage = VulkanResourceManager::GetAllocatedImage("Lighting");
        AllocatedImage* baseColorImage = VulkanResourceManager::GetAllocatedImage("BaseColorMetallic");
        AllocatedImage* normalImage = VulkanResourceManager::GetAllocatedImage("NormalXYRoughnessMisc");
        AllocatedImage* velocityImage = VulkanResourceManager::GetAllocatedImage("VelocityXYOcclusionSubSurface");
        AllocatedImage* visibilityImage = VulkanResourceManager::GetAllocatedImage("Visibility");
        AllocatedImage* depthImage = VulkanResourceManager::GetAllocatedImage("Depth");
        VulkanPipeline* pipeline = VulkanResourceManager::GetPipeline("DebugView");
        VulkanDescriptorSet* staticDescriptorSet = VulkanResourceManager::GetDescriptorSet("StaticDescriptorSet");
        const VulkanFrameData& frameData = GetCurrentFrameData();
        VulkanBuffer* viewportDataBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.viewportData);
        VulkanBuffer* rendererDataBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.rendererData);

        if (!pipeline) return;
        if (!staticDescriptorSet) return;
        if (!lightingImage) return;
        if (!baseColorImage) return;
        if (!normalImage) return;
        if (!velocityImage) return;
        if (!visibilityImage) return;
        if (!depthImage) return;
        if (!viewportDataBuffer) return;
        if (!rendererDataBuffer) return;

        VkExtent2D extent = lightingImage->GetExtent2D();
        baseColorImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);
        normalImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);
        velocityImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);
        visibilityImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);
        depthImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);
        lightingImage->Sync(commandBuffer, VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);

        VkRenderingAttachmentInfo colorAttachment{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
        colorAttachment.imageView = lightingImage->GetImageView();
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        VkRenderingInfo renderingInfo{ VK_STRUCTURE_TYPE_RENDERING_INFO };
        renderingInfo.renderArea.extent = extent;
        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachments = &colorAttachment;

        vkCmdBeginRendering(commandBuffer, &renderingInfo);

        VkViewport viewport{};
        viewport.width = static_cast<float>(extent.width);
        viewport.height = static_cast<float>(extent.height);
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.extent = extent;
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetHandle());
        VkDescriptorSet descriptorSets[] = { staticDescriptorSet->GetHandle() };
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetLayout(), 0, 1, descriptorSets, 0, nullptr);

        PushConstantsDebugView pushConstants{};
        pushConstants.frame = CreatePushConstantsFrameResources();
        vkCmdPushConstants(commandBuffer, pipeline->GetLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstantsDebugView), &pushConstants);

        vkCmdDraw(commandBuffer, 3, 1, 0, 0);

        vkCmdEndRendering(commandBuffer);
    }
}
