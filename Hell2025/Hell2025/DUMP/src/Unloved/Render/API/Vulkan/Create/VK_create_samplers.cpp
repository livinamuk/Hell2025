#include "Unloved/Render/API/Vulkan/VK_renderer_internal.h"

#include "Hell/Render/API/Vulkan/Managers/vk_device_manager.h"
#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"

namespace VulkanRenderer {
    void CreateSamplers() {
        const VkPhysicalDeviceProperties& properties = VulkanDeviceManager::GetProperties();
        const float maxAnisotropy = properties.limits.maxSamplerAnisotropy;

        VulkanResourceManager::CreateSampler("Linear", VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, maxAnisotropy);
        VulkanResourceManager::CreateSampler("Nearest", VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
        VulkanResourceManager::CreateSampler("ClampBorderLinear", VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);
    }
}
