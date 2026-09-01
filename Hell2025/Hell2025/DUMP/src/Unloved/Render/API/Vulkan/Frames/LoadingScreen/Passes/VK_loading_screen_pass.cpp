#include "Unloved/Render/API/Vulkan/VK_renderer.h"

#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/Render/API/Vulkan/Types/vk_pipeline.h"
#include "Hell/Render/API/Vulkan/Types/vk_timer.h"

namespace VulkanRenderer {

    void LoadingScreenPass(VkCommandBuffer commandBuffer, VkImageView imageView, VkExtent2D extent) {
        ProfilerVulkanZoneFunction();

        VulkanPipeline* pipeline = VulkanResourceManager::GetPipeline("LoadingScreen");
        if (!pipeline) return;

        VkClearValue clearValue{};
        clearValue.color.float32[3] = 1.0f;

        VkRenderingAttachmentInfo colorAttachment{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
        colorAttachment.imageView = imageView;
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.clearValue = clearValue;

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
        vkCmdDraw(commandBuffer, 3, 1, 0, 0);

        vkCmdEndRendering(commandBuffer);
    }
}
