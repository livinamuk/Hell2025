#pragma once
#include "Hell/Render/API/Vulkan/vk_common.h"
#include <vector>

namespace VulkanSyncManager {
    bool Init();
    void Cleanup();

    VkSemaphore GetPresentSemaphore(uint32_t frameIndex);
    VkSemaphore GetRenderFinishedSemaphore(uint32_t frameIndex, uint32_t swapchainImageIndex);
    VkFence GetRenderFence(uint32_t frameIndex);
    VkFence GetUploadFence();

    VkResult WaitForRenderFence(uint32_t frameIndex);
    VkResult ResetRenderFence(uint32_t frameIndex);
    VkResult WaitForUploadFence();
    VkResult ResetUploadFence();
}
