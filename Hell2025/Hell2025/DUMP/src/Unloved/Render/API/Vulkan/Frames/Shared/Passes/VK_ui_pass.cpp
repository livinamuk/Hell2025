#include "Unloved/Render/API/Vulkan/VK_renderer.h"

#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/Render/API/Vulkan/Types/vk_allocated_image.h"
#include "Hell/Render/API/Vulkan/Types/vk_pipeline.h"
#include "Hell/Render/API/Vulkan/Types/vk_timer.h"
#include "Hell/UI/UIBackEnd.h"
#include "Unloved/Render/API/Vulkan/VK_draw.h"
#include "Unloved/Render/API/Vulkan/VK_push_constants.h"
#include "Unloved/Render/API/Vulkan/VK_renderer_internal.h"
#include "Unloved/Render/RenderDataManager.h"

namespace VulkanRenderer {
    namespace {
        void DrawUICanvas(VkCommandBuffer commandBuffer, UICanvas canvas, const std::vector<DrawIndexedIndirectCommand>& drawCommands, VkImageView imageView, VkImageLayout imageLayout, VkExtent2D extent, bool flipViewportY) {
            if (drawCommands.empty()) return;

            VulkanFrameData& frameData = GetCurrentFrameData();
            VulkanGenericMesh* vulkanMesh = VulkanResourceManager::GetGenericMesh(frameData.genericMeshes.ui);
            if (!vulkanMesh || vulkanMesh->GetIndexCount() == 0) return;

            VulkanBuffer* vertexBuffer = vulkanMesh->GetVertexBuffer();
            VulkanBuffer* indexBuffer = vulkanMesh->GetIndexBuffer();
            if (!vertexBuffer || !indexBuffer) return;

            VulkanPipeline* pipeline = VulkanResourceManager::GetPipeline("UI");
            VulkanDescriptorSet* staticDescriptorSet = VulkanResourceManager::GetDescriptorSet("StaticDescriptorSet");
            if (!pipeline || !staticDescriptorSet) return;

            VulkanDrawCommandBatch drawCommandBatch = WriteDrawCommands(drawCommands);
            if (drawCommandBatch.count == 0) return;

            VkRenderingAttachmentInfo colorAttachment{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
            colorAttachment.imageView = imageView;
            colorAttachment.imageLayout = imageLayout;
            colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
            colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

            VkRenderingInfo renderingInfo{ VK_STRUCTURE_TYPE_RENDERING_INFO };
            renderingInfo.renderArea.extent = extent;
            renderingInfo.layerCount = 1;
            renderingInfo.colorAttachmentCount = 1;
            renderingInfo.pColorAttachments = &colorAttachment;

            vkCmdBeginRendering(commandBuffer, &renderingInfo);

            VkViewport viewport{};
            viewport.y = flipViewportY ? static_cast<float>(extent.height) : 0.0f;
            viewport.width = static_cast<float>(extent.width);
            viewport.height = flipViewportY ? -static_cast<float>(extent.height) : static_cast<float>(extent.height);
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

            VkRect2D scissor{};
            scissor.extent = extent;
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

            const glm::uvec2 resolution = UIBackEnd::GetCanvasResolution(canvas);
            PushConstantsUI pushConstants{};
            pushConstants.frameAddressTableDeviceAddress = GetFrameAddressTableDeviceAddress();
            pushConstants.renderTargetWidth = static_cast<float>(resolution.x);
            pushConstants.renderTargetHeight = static_cast<float>(resolution.y);

            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetHandle());
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetLayout(), 0, 1, staticDescriptorSet->GetHandlePtr(), 0, nullptr);
            BindVertexBuffer(commandBuffer, vertexBuffer);
            BindIndexBuffer(commandBuffer, indexBuffer);
            vkCmdPushConstants(commandBuffer, pipeline->GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushConstants), &pushConstants);
            MultiDrawIndexedCommands(commandBuffer, drawCommandBatch);

            vkCmdEndRendering(commandBuffer);
        }

        void SynchronizeNativeTarget(VkCommandBuffer commandBuffer, VkImage image) {
            VkImageMemoryBarrier2 barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
            barrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
            barrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
            barrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
            barrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
            barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = image;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.layerCount = 1;

            VkDependencyInfo dependencyInfo{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
            dependencyInfo.imageMemoryBarrierCount = 1;
            dependencyInfo.pImageMemoryBarriers = &barrier;
            vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
        }
    }

    void RenderGameUIPass(VkCommandBuffer commandBuffer) {
        ProfilerVulkanZoneFunction();

        const std::vector<DrawIndexedIndirectCommand>& drawCommands = Unloved::RenderDataManager::GetDrawCommandsUI(UICanvas::INTERNAL);
        if (drawCommands.empty()) return;

        AllocatedImage* presentImage = VulkanResourceManager::GetAllocatedImage("Present");
        if (!presentImage) return;

        presentImage->Sync(commandBuffer, VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
        DrawUICanvas(commandBuffer, UICanvas::INTERNAL, drawCommands, presentImage->GetImageView(), VK_IMAGE_LAYOUT_GENERAL, presentImage->GetExtent2D(), false);
    }

    void RenderEditorUIPass(VkCommandBuffer commandBuffer, VkImage image, VkImageView imageView, VkExtent2D extent) {
        ProfilerVulkanZoneFunction();

        const std::vector<DrawIndexedIndirectCommand>& drawCommands = Unloved::RenderDataManager::GetDrawCommandsUI(UICanvas::NATIVE);
        if (drawCommands.empty()) return;

        SynchronizeNativeTarget(commandBuffer, image);
        DrawUICanvas(commandBuffer, UICanvas::NATIVE, drawCommands, imageView, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, extent, true);
    }
}
