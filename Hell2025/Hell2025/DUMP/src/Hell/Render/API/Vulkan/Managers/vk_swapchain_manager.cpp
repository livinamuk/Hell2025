#include "vk_swapchain_manager.h"
#include "vk_device_manager.h"
#include "vk_instance_manager.h"

#include "Hell/Backend/BackEnd.h"
#include "Hell/Logging.h"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <iostream>
#include <limits>

namespace VulkanSwapchainManager {
    VkSwapchainKHR g_swapchain = VK_NULL_HANDLE;
    VkExtent2D g_swapchainExtent = {};
    VkFormat g_swachainImageFormat;
    std::vector<VkImage> g_swapchainImages;
    std::vector<VkImageView> g_swapchainImageViews;

    bool Init() {
	    CreateSwapchain();
        Logging::Init() << "VulkanSwapchainManager::Init()\n";
	    return true;
    }

    void RecreateSwapchain() {
        while (Hell::BackEnd::WindowIsMinimized()) {
            glfwWaitEvents();
        }

	    vkDeviceWaitIdle(VulkanDeviceManager::GetDevice());

	    // Clean up old resources
	    for (int i = 0; i < g_swapchainImages.size(); i++) {
		    vkDestroyImageView(VulkanDeviceManager::GetDevice(), g_swapchainImageViews[i], nullptr);
	    }

	    if (g_swapchain != VK_NULL_HANDLE) {
		    vkDestroySwapchainKHR(VulkanDeviceManager::GetDevice(), g_swapchain, nullptr);
	    }

	    // Recreate
        CreateSwapchain();
        Logging::Init() << "VulkanSwapchainManager::RecreateSwapchain()\n";
    }

    void CreateSwapchain() {
        VkDevice device = VulkanDeviceManager::GetDevice();
        VkPhysicalDevice physicalDevice = VulkanDeviceManager::GetPhysicalDevice();
        VkSurfaceKHR surface = VulkanInstanceManager::GetSurface();

        // Query Surface Capabilities
        VkSurfaceCapabilitiesKHR capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities);

        // Resolve Extent
        VkExtent2D extent = capabilities.currentExtent;
        if (extent.width == std::numeric_limits<uint32_t>::max()) {
            extent.width = static_cast<uint32_t>(std::max(1, Hell::BackEnd::GetDrawableWidth()));
            extent.height = static_cast<uint32_t>(std::max(1, Hell::BackEnd::GetDrawableHeight()));
            extent.width = std::clamp(extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            extent.height = std::clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        }
        g_swapchainExtent = extent;

        // Choose Format
        g_swachainImageFormat = VK_FORMAT_R8G8B8A8_UNORM;
        VkColorSpaceKHR colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

        // Determine Image Count
        uint32_t imageCount = capabilities.minImageCount + 1;
        if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
            imageCount = capabilities.maxImageCount;
        }

        // Configure Creation Info
        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = g_swachainImageFormat;
        createInfo.imageColorSpace = colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        uint32_t queueFamilyIndices[] = {
            VulkanDeviceManager::GetGraphicsQueueFamily(),
            VulkanDeviceManager::GetPresentQueueFamily()
        };

        if (queueFamilyIndices[0] == queueFamilyIndices[1]) {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }
        else {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }

        createInfo.preTransform = capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR; // Standard fallback
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &g_swapchain) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create swapchain!");
        }

        // Retrieve Images
        uint32_t actualImageCount;
        vkGetSwapchainImagesKHR(device, g_swapchain, &actualImageCount, nullptr);
        g_swapchainImages.resize(actualImageCount);
        vkGetSwapchainImagesKHR(device, g_swapchain, &actualImageCount, g_swapchainImages.data());

        // Create Image Views
        g_swapchainImageViews.resize(g_swapchainImages.size());
        for (size_t i = 0; i < g_swapchainImages.size(); i++) {
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = g_swapchainImages[i];
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = g_swachainImageFormat;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(device, &viewInfo, nullptr, &g_swapchainImageViews[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create image views!");
            }
        }
    }

    void Cleanup() {
        for (auto imageView : g_swapchainImageViews) {
            vkDestroyImageView(VulkanDeviceManager::GetDevice(), imageView, nullptr);
        }
        vkDestroySwapchainKHR(VulkanDeviceManager::GetDevice(), g_swapchain, nullptr);
    }

    VkSwapchainKHR GetSwapchain() { return g_swapchain; }
    VkExtent2D GetSwapchainExtent() { return g_swapchainExtent; }
    VkFormat GetSwapchainImageFormat() { return g_swachainImageFormat; }
    std::vector<VkImage>& GetSwapchainImages() { return g_swapchainImages; }
    std::vector<VkImageView>& GetSwapchainImageViews() { return g_swapchainImageViews; }
}
