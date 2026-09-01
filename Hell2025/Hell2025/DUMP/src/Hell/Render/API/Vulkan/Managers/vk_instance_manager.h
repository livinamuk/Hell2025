#pragma once
#include "Hell/Render/API/Vulkan/vk_common.h"

struct GLFWwindow;

namespace VulkanInstanceManager {
    bool Init();
    void Cleanup();

    VkDebugUtilsMessengerEXT GetDebugMessenger();
    VkInstance GetInstance();
    VkSurfaceKHR GetSurface();

    bool ValidationEnabled();
}