#include "Unloved/Render/API/Vulkan/VK_renderer_internal.h"

#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/Render/API/Vulkan/Types/vk_allocated_image.h"

namespace VulkanRenderer {

    void BlitImage(VkCommandBuffer commandBuffer, const std::string& srcName, const std::string& dstName, VkFilter filter) {

        AllocatedImage* srcImage = VulkanResourceManager::GetAllocatedImage(srcName);
        AllocatedImage* dstImage = VulkanResourceManager::GetAllocatedImage(dstName);
        if (!srcImage || !dstImage) return;

        VkExtent2D srcExtent = srcImage->GetExtent2D();
        VkExtent2D dstExtent = dstImage->GetExtent2D();

        srcImage->Sync(commandBuffer, VK_ACCESS_2_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT);
        dstImage->Sync(commandBuffer, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT);

        VkImageBlit blit{};
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.layerCount = 1;
        blit.srcOffsets[1].x = static_cast<int32_t>(srcExtent.width);
        blit.srcOffsets[1].y = static_cast<int32_t>(srcExtent.height);
        blit.srcOffsets[1].z = 1;
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.layerCount = 1;
        blit.dstOffsets[1].x = static_cast<int32_t>(dstExtent.width);
        blit.dstOffsets[1].y = static_cast<int32_t>(dstExtent.height);
        blit.dstOffsets[1].z = 1;

        vkCmdBlitImage(commandBuffer, srcImage->GetImage(), VK_IMAGE_LAYOUT_GENERAL, dstImage->GetImage(), VK_IMAGE_LAYOUT_GENERAL, 1, &blit, filter);
    }
}
