#include "Unloved/Render/API/Vulkan/VK_renderer.h"

#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/Render/API/Vulkan/Types/vk_allocated_image.h"
#include "Hell/Render/API/Vulkan/Types/vk_pipeline.h"
#include "Hell/Render/API/Vulkan/Types/vk_timer.h"

namespace VulkanRenderer {

    void PresentPass(VkCommandBuffer commandBuffer, VkImageView imageView, VkExtent2D extent) {
        ProfilerVulkanZoneFunction();

        AllocatedImage* presentImage = VulkanResourceManager::GetAllocatedImage("Present");
        VulkanPipeline* pipeline = VulkanResourceManager::GetPipeline("Present");
        VulkanDescriptorSet* staticDescriptorSet = VulkanResourceManager::GetDescriptorSet("StaticDescriptorSet");
        if (!presentImage) return;
        if (!pipeline || !staticDescriptorSet) return;

        presentImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);

        VkRenderingAttachmentInfo colorAttachment{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
        colorAttachment.imageView = imageView;
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
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
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetLayout(), 0, 1, staticDescriptorSet->GetHandlePtr(), 0, nullptr);
        vkCmdDraw(commandBuffer, 3, 1, 0, 0);

        vkCmdEndRendering(commandBuffer);
    }
}
