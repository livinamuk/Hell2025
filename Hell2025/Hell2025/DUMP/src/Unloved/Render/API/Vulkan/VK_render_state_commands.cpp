#include "Unloved/Render/API/Vulkan/VK_renderer_internal.h"

#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/Render/API/Vulkan/Types/vk_allocated_image.h"

#include <array>

namespace VulkanRenderer {

    bool BeginRenderState(VkCommandBuffer commandBuffer, const VulkanRenderState& state, VkExtent2D extent) {
        if (state.colorTargetCount == 0 && !state.hasDepthTarget) return false;

        std::array<VkRenderingAttachmentInfo, VulkanRenderState::MAX_RENDER_TARGET_COUNT> colorAttachments{};

        for (uint32_t i = 0; i < state.colorTargetCount; i++) {
            const VulkanRenderTargetInfo& target = state.colorTargets[i];
            AllocatedImage* image = VulkanResourceManager::GetAllocatedImage(target.imageName);
            if (!image) return false;

            image->Sync(commandBuffer, VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);

            VkRenderingAttachmentInfo& attachment = colorAttachments[i];
            attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            attachment.imageView = image->GetImageView();
            attachment.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            attachment.loadOp = target.loadOp;
            attachment.storeOp = target.storeOp;
            attachment.clearValue = target.clearValue;
        }

        bool useDepthAttachment = state.hasDepthTarget && (state.rasterizer.depthTestEnabled || state.rasterizer.depthWriteEnabled);
        bool useStencilAttachment = state.hasDepthTarget && state.rasterizer.stencilTestEnabled;

        VkRenderingAttachmentInfo depthAttachment{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };

        if (state.hasDepthTarget) {
            AllocatedImage* image = VulkanResourceManager::GetAllocatedImage(state.depthTarget.imageName);
            if (!image) return false;

            VkAccessFlags2 accessFlags = VK_ACCESS_2_SHADER_READ_BIT;
            VkPipelineStageFlags2 stageFlags = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;

            if (useDepthAttachment || useStencilAttachment) {
                accessFlags |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
                stageFlags |= VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
            }

            if (state.rasterizer.depthWriteEnabled || state.rasterizer.stencilWriteMask != 0) {
                accessFlags |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            }

            image->Sync(commandBuffer, accessFlags, stageFlags);

            depthAttachment.imageView = useDepthAttachment && !useStencilAttachment ? image->GetDepthOnlyImageView() : image->GetImageView();
            depthAttachment.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            depthAttachment.loadOp = state.depthTarget.loadOp;
            depthAttachment.storeOp = state.depthTarget.storeOp;
            depthAttachment.clearValue = state.depthTarget.clearValue;
        }

        VkRenderingInfo renderingInfo{ VK_STRUCTURE_TYPE_RENDERING_INFO };
        renderingInfo.renderArea.extent = extent;
        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = state.colorTargetCount;
        renderingInfo.pColorAttachments = state.colorTargetCount ? colorAttachments.data() : nullptr;

        renderingInfo.pDepthAttachment = useDepthAttachment ? &depthAttachment : nullptr;
        renderingInfo.pStencilAttachment = useStencilAttachment ? &depthAttachment : nullptr;

        vkCmdBeginRendering(commandBuffer, &renderingInfo);
        if (useStencilAttachment) {
            vkCmdSetStencilReference(commandBuffer, VK_STENCIL_FACE_FRONT_AND_BACK, state.rasterizer.stencilRef);
        }

        return true;
    }

    void EndRenderState(VkCommandBuffer commandBuffer) {
        vkCmdEndRendering(commandBuffer);
    }
}
