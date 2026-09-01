#define VMA_IMPLEMENTATION
#include "Hell/Render/API/Vulkan/vk_common.h"

#include "vk_memory_manager.h"
#include "vk_device_manager.h"
#include "vk_instance_manager.h"

#include <vector>
#include <set>
#include <cstring>
#include <stdexcept>
#include <iostream>

namespace VulkanMemoryManager {
    VmaAllocator g_allocator = VK_NULL_HANDLE;
    VkDescriptorPool g_descriptorPool = VK_NULL_HANDLE;

    bool CreateAllocator();
    bool CreateDescriptorPool();

    bool Init() {
        if (!CreateAllocator())      return false;
        if (!CreateDescriptorPool()) return false;
        return true;
    }

    bool CreateAllocator() {
        VkInstance instance = VulkanInstanceManager::GetInstance();
        VkDevice device = VulkanDeviceManager::GetDevice();
        VkPhysicalDevice physicalDevice = VulkanDeviceManager::GetPhysicalDevice();

        VmaVulkanFunctions vulkanFunctions{};
        vulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
        vulkanFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

        VmaAllocatorCreateInfo allocatorInfo{};
        allocatorInfo.instance = instance;
        allocatorInfo.physicalDevice = physicalDevice;
        allocatorInfo.device = device;
        allocatorInfo.pVulkanFunctions = &vulkanFunctions;
        allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

        VkResult result = vmaCreateAllocator(&allocatorInfo, &g_allocator);
        return result == VK_SUCCESS;
    }

    bool CreateDescriptorPool() {
        VkDevice device = VulkanDeviceManager::GetDevice();

        // Define pool sizes
        //std::vector<VkDescriptorPoolSize> sizes = {
        //    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100 },
        //    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 100 },
        //    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100 },
        //    { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 100 },
        //    { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        //    { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        //    { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 10000 },
        //    { VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 100 }
        //};

        uint32_t sets = 4; // Buffer for a few versions/overlays

        std::vector<VkDescriptorPoolSize> sizes = {
            // Static samplers + texture samplers + shadow samplers
            { VK_DESCRIPTOR_TYPE_SAMPLER, (16 + 10000 + 16) * sets + 100 },

            // From sampled textures, uint textures, sampled texture arrays, and depth cube arrays
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, (10000 + 128 + 100 + 100 + 16) * sets },

            // From Binding 2
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 128 * sets + 100 },

            // From Binding 3 + (Old 5/6 legacy)
            // Note: You have a typo in your bindings 5/6, they should be STORAGE_BUFFER type
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, (1024 * sets) + 200 },

            // Seven bindless storage-image format bindings (RGBA32F, RGBA16F,
            // RGBA8, RG16F, R32F, RG8, and R16F).
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, (100 * 7) * sets + 100 },

            // Misc/Legacy/Raytracing
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 100 },
            { VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 100 }
        };

        VkDescriptorPoolCreateInfo poolInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
        poolInfo.maxSets = 1000;
        poolInfo.poolSizeCount = (uint32_t)sizes.size();
        poolInfo.pPoolSizes = sizes.data();

        if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &g_descriptorPool) != VK_SUCCESS) {
            return false;
        }
        return true;
    }

    void Cleanup() {
        VkDevice device = VulkanDeviceManager::GetDevice();

        if (g_allocator != VK_NULL_HANDLE) {
            vmaDestroyAllocator(g_allocator);
            g_allocator = VK_NULL_HANDLE;
        }

        if (g_descriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(device, g_descriptorPool, nullptr);
            g_descriptorPool = VK_NULL_HANDLE;
        }
    }

    VmaAllocator GetAllocator() { 
        return g_allocator; 
    }

    VkDescriptorPool GetDescriptorPool() { 
        return g_descriptorPool; 
    }
}
