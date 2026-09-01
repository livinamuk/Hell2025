#pragma once
#include "Hell/Render/API/Vulkan/vk_common.h"

namespace VulkanDeviceManager {
    bool Init();
    void Cleanup();
    
    VkPhysicalDevice GetPhysicalDevice();
    VkDevice GetDevice();
    VkQueue GetGraphicsQueue();
    uint32_t GetGraphicsQueueFamily();
    VkQueue GetPresentQueue();
    uint32_t GetPresentQueueFamily();
    const VkPhysicalDeviceProperties& GetProperties();
    const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& GetRayTracingPipelineProperties();
    const VkPhysicalDeviceAccelerationStructurePropertiesKHR& GetAccelerationStructureProperties();
    const VkPhysicalDeviceAccelerationStructureFeaturesKHR& GetAccelerationStructureFeatures();
    const VkPhysicalDeviceMemoryProperties& GetMemoryProperties();
}
