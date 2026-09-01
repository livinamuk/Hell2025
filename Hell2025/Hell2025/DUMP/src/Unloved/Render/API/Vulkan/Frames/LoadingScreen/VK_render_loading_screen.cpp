#include "Unloved/Render/API/Vulkan/VK_renderer_internal.h"

#include "Hell/Render/API/Vulkan/Types/vk_allocated_image.h"

namespace VulkanRenderer {

    void RenderBlackFrame() {
        SwapchainFrame frame;
        if (!BeginSwapchainFrame(frame)) return;

        frame.presentImage->Sync(frame.commandBuffer, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT);

        VkClearColorValue clearValue{};
        VkImageSubresourceRange range{};
        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        range.levelCount = 1;
        range.layerCount = 1;
        vkCmdClearColorImage(frame.commandBuffer, frame.presentImage->GetImage(), VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &range);

        PresentPass(frame.commandBuffer, frame.swapchainImageView, frame.extent);
        EndSwapchainFrame(frame);
    }

    void RenderLoadingScreen() {
        SwapchainFrame frame;
        if (!BeginSwapchainFrame(frame)) return;

        frame.presentImage->Sync(frame.commandBuffer, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);

        LoadingScreenPass(frame.commandBuffer, frame.presentImage->GetImageView(), frame.presentImage->GetExtent2D());

        if (!UpdateBuffersUI() || !UpdateFrameAddressTable()) {
            EndSwapchainFrame(frame);
            return;
        }

        RenderGameUIPass(frame.commandBuffer);

        PresentPass(frame.commandBuffer, frame.swapchainImageView, frame.extent);
        EndSwapchainFrame(frame);
    }
}
