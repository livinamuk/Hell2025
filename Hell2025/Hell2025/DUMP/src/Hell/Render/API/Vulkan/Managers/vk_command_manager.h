#pragma once
#include "Hell/Render/API/Vulkan/vk_common.h"
#include <functional>

namespace VulkanCommandManager {
    bool Init();
    void Cleanup();

    VkCommandPool GetGraphicsCommandPool(uint32_t frameIndex);
    VkCommandBuffer GetGraphicsCommandBuffer(uint32_t frameIndex);

    VkCommandPool GetUploadCommandPool();
    VkCommandBuffer GetUploadCommandBuffer();

    void SubmitImmediate(std::function<void(VkCommandBuffer cmd)>&& function);
}