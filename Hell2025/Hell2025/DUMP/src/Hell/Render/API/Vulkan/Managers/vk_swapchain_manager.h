#pragma once
#include "Hell/Render/API/Vulkan/vk_common.h"
#include <vector>

namespace VulkanSwapchainManager {
    bool Init();
    void Cleanup();

    void CreateSwapchain();
    void RecreateSwapchain();

    VkSwapchainKHR GetSwapchain();
    VkExtent2D GetSwapchainExtent();
    VkFormat GetSwapchainImageFormat();
    std::vector<VkImage>& GetSwapchainImages();
    std::vector<VkImageView>& GetSwapchainImageViews();
}
