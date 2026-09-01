#include "vk_device_manager.h"
#include "vk_instance_manager.h"

#include "Hell/Logging.h"

#include <vector>
#include <set>
#include <cstring>
#include <stdexcept>
#include <iostream>

namespace VulkanDeviceManager {
    VkPhysicalDevice g_physicalDevice = VK_NULL_HANDLE;
    VkDevice g_device = VK_NULL_HANDLE;

    VkQueue g_graphicsQueue = VK_NULL_HANDLE;
    uint32_t g_graphicsQueueFamily = UINT32_MAX;

    VkQueue g_presentQueue = VK_NULL_HANDLE;
    uint32_t g_presentQueueFamily = UINT32_MAX;

    // Properties
    VkPhysicalDeviceProperties g_properties = {};
    VkPhysicalDeviceProperties2 g_properties2 = {};
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR g_rayTracingPipeline = {};
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR g_rayTracingPipelineProperties = {};
    VkPhysicalDeviceAccelerationStructurePropertiesKHR g_accelerationStructureProperties = {};
    VkPhysicalDeviceMemoryProperties g_memoryProperties = {};

    // Features
    VkPhysicalDeviceFeatures2 g_features2 = {};
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR g_rayTracingPipelineFeatures = {};
    VkPhysicalDeviceAccelerationStructureFeaturesKHR g_accelerationStructureFeatures = {};
    VkPhysicalDeviceRayQueryFeaturesKHR g_rayQueryFeatures = {};
    VkPhysicalDeviceUnifiedImageLayoutsFeaturesKHR g_unifiedLayoutFeatures = {};

    bool Init() {
        VkInstance instance = VulkanInstanceManager::GetInstance();
        VkSurfaceKHR surface = VulkanInstanceManager::GetSurface();

        // Enumerate devices
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        if (deviceCount == 0) {
            throw std::runtime_error("No Vulkan devices found");
        }
        std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data());

        // Required extensions
        std::vector<const char*> requiredExtensions = {
            VK_KHR_MAINTENANCE_4_EXTENSION_NAME,
            VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
            VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
            VK_KHR_RAY_QUERY_EXTENSION_NAME,
            VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
            VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
            VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
            VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
            VK_KHR_SPIRV_1_4_EXTENSION_NAME,
            VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME,
            VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME,
            VK_NV_DEVICE_DIAGNOSTICS_CONFIG_EXTENSION_NAME,
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_KHR_UNIFIED_IMAGE_LAYOUTS_EXTENSION_NAME
        };

        // Select first suitable device
        for (VkPhysicalDevice physicalDevice : physicalDevices) {
            // Check extensions
            uint32_t extCount = 0;
            vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extCount, nullptr);
            std::vector<VkExtensionProperties> available(extCount);
            vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extCount, available.data());

            bool allFound = true;
            for (auto req : requiredExtensions) {
                bool found = false;
                for (auto& av : available) {
                    if (strcmp(req, av.extensionName) == 0) { found = true; break; }
                }
                if (!found) { allFound = false; break; }
            }
            if (!allFound) continue;

            // Check required feature support.
            VkPhysicalDeviceVulkan11Features supportedFeatures11{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES };
            VkPhysicalDeviceVulkan12Features supportedFeatures12{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
            VkPhysicalDeviceUnifiedImageLayoutsFeaturesKHR unifiedFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_UNIFIED_IMAGE_LAYOUTS_FEATURES_KHR };
            VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR };
            VkPhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR };
            VkPhysicalDeviceFeatures2 checkFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
            checkFeatures.pNext = &supportedFeatures11;
            supportedFeatures11.pNext = &supportedFeatures12;
            supportedFeatures12.pNext = &unifiedFeatures;
            unifiedFeatures.pNext = &accelerationStructureFeatures;
            accelerationStructureFeatures.pNext = &rayQueryFeatures;
            vkGetPhysicalDeviceFeatures2(physicalDevice, &checkFeatures);

            // Scalar block layout is required by the compact 44-byte Vertex
            // used through PhysicalStorageBuffer buffer references.
            if (!checkFeatures.features.shaderInt64 ||
                !checkFeatures.features.drawIndirectFirstInstance ||
                !checkFeatures.features.multiDrawIndirect ||
                !checkFeatures.features.geometryShader ||
                !checkFeatures.features.imageCubeArray ||
                !checkFeatures.features.shaderClipDistance ||
                !checkFeatures.features.shaderStorageImageArrayDynamicIndexing ||
                !supportedFeatures11.shaderDrawParameters ||
                !supportedFeatures12.descriptorBindingStorageImageUpdateAfterBind ||
                !supportedFeatures12.shaderStorageImageArrayNonUniformIndexing ||
                !supportedFeatures12.bufferDeviceAddress ||
                !supportedFeatures12.scalarBlockLayout ||
                !supportedFeatures12.shaderOutputLayer ||
                !unifiedFeatures.unifiedImageLayouts ||
                !accelerationStructureFeatures.accelerationStructure ||
                !accelerationStructureFeatures.descriptorBindingAccelerationStructureUpdateAfterBind ||
                !rayQueryFeatures.rayQuery) {
                continue;
            }

            // Find queue families
            uint32_t qCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &qCount, nullptr);
            std::vector<VkQueueFamilyProperties> qProps(qCount);
            vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &qCount, qProps.data());

            int graphicsFam = -1, presentFam = -1;
            for (uint32_t i = 0; i < qCount; ++i) {
                if (qProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) graphicsFam = i;
                VkBool32 presentSupport = VK_FALSE;
                vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
                if (presentSupport) presentFam = i;
                if (graphicsFam != -1 && presentFam != -1) break;
            }
            if (graphicsFam == -1 || presentFam != -1) { /* graphicsFam check sufficient for loop break logic */ }
            if (graphicsFam == -1 || presentFam == -1) continue;

            // This device is usable
            g_physicalDevice = physicalDevice;
            g_graphicsQueueFamily = graphicsFam;
            g_presentQueueFamily = presentFam;

            // Enable features
            VkPhysicalDeviceFeatures features{};
            features.samplerAnisotropy = VK_TRUE;
            features.shaderInt64 = VK_TRUE;
            features.drawIndirectFirstInstance = VK_TRUE;
            features.multiDrawIndirect = VK_TRUE;
            features.geometryShader = VK_TRUE;
            features.imageCubeArray = VK_TRUE;
            features.shaderClipDistance = VK_TRUE;
            features.shaderStorageImageArrayDynamicIndexing = VK_TRUE;
            features.shaderStorageImageExtendedFormats = VK_TRUE;

            VkPhysicalDeviceVulkan11Features features11{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES };
            features11.shaderDrawParameters = VK_TRUE;

            VkPhysicalDeviceVulkan12Features features12{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
            features12.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
            features12.shaderStorageImageArrayNonUniformIndexing = VK_TRUE;
            features12.runtimeDescriptorArray = VK_TRUE;
            features12.descriptorBindingVariableDescriptorCount = VK_TRUE;
            features12.descriptorBindingPartiallyBound = VK_TRUE;
            features12.descriptorIndexing = VK_TRUE;
            features12.bufferDeviceAddress = VK_TRUE;
            features12.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
            features12.descriptorBindingStorageImageUpdateAfterBind = VK_TRUE;
            features12.scalarBlockLayout = VK_TRUE;
            features12.shaderOutputLayer = VK_TRUE;

            VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtPipeline{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR };
            rtPipeline.rayTracingPipeline = VK_TRUE;

            VkPhysicalDeviceRayQueryFeaturesKHR rayQuery{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR };
            rayQuery.rayQuery = VK_TRUE;
            rayQuery.pNext = &rtPipeline;

            VkPhysicalDeviceAccelerationStructureFeaturesKHR accel{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR };
            accel.accelerationStructure = VK_TRUE;
            accel.descriptorBindingAccelerationStructureUpdateAfterBind = VK_TRUE;
            accel.pNext = &rayQuery;

            VkPhysicalDeviceVulkan13Features features13{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
            features13.maintenance4 = VK_TRUE;
            features13.dynamicRendering = VK_TRUE;
            features13.synchronization2 = VK_TRUE;
            features13.pNext = &accel;

            VkPhysicalDeviceUnifiedImageLayoutsFeaturesKHR deviceUnifiedFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_UNIFIED_IMAGE_LAYOUTS_FEATURES_KHR };
            deviceUnifiedFeatures.unifiedImageLayouts = VK_TRUE;
            deviceUnifiedFeatures.pNext = &features13;

            VkPhysicalDeviceFeatures2 features2{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
            features2.features = features;
            features2.pNext = &features11;
            features11.pNext = &features12;
            features12.pNext = &deviceUnifiedFeatures;

            float priority = 1.0f;
            std::vector<VkDeviceQueueCreateInfo> qInfos;
            std::set<uint32_t> uniqueQ = { (uint32_t)graphicsFam, (uint32_t)presentFam };
            for (uint32_t fam : uniqueQ) {
                VkDeviceQueueCreateInfo qci{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
                qci.queueFamilyIndex = fam;
                qci.queueCount = 1;
                qci.pQueuePriorities = &priority;
                qInfos.push_back(qci);
            }

            VkDeviceCreateInfo dci{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
            dci.queueCreateInfoCount = (uint32_t)qInfos.size();
            dci.pQueueCreateInfos = qInfos.data();
            dci.enabledExtensionCount = (uint32_t)requiredExtensions.size();
            dci.ppEnabledExtensionNames = requiredExtensions.data();
            dci.pNext = &features2;

            // Properties
            g_memoryProperties = {};
            vkGetPhysicalDeviceMemoryProperties(g_physicalDevice, &g_memoryProperties);

            g_properties = {};
            vkGetPhysicalDeviceProperties(g_physicalDevice, &g_properties);

            g_rayTracingPipelineProperties = {};
            g_rayTracingPipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
            g_accelerationStructureProperties = {};
            g_accelerationStructureProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR;
            g_accelerationStructureProperties.pNext = &g_rayTracingPipelineProperties;

            g_properties2 = {};
            g_properties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
            g_properties2.pNext = &g_accelerationStructureProperties;
            vkGetPhysicalDeviceProperties2(g_physicalDevice, &g_properties2);

            // Features
            g_accelerationStructureFeatures = {};
            g_accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
            g_rayQueryFeatures = {};
            g_rayQueryFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;

            g_features2 = {};
            g_features2.sType = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
            g_features2.pNext = &g_accelerationStructureFeatures;
            g_accelerationStructureFeatures.pNext = &g_rayQueryFeatures;
            vkGetPhysicalDeviceFeatures2(g_physicalDevice, &g_features2);

            if (vkCreateDevice(g_physicalDevice, &dci, nullptr, &g_device) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create logical device");
            }

            vkGetDeviceQueue(g_device, graphicsFam, 0, &g_graphicsQueue);
            vkGetDeviceQueue(g_device, presentFam, 0, &g_presentQueue);

            volkLoadDevice(GetDevice());

            Logging::Init() << "VulkanDeviceManager::Init()\n";
            return true; // Done, first suitable device
        }

        Logging::Fatal() << "VulkanDeviceManager::Init() No suitable Vulkan device found\n";
        return false;
    }

    void Cleanup() {
        if (g_device != VK_NULL_HANDLE) {
            vkDestroyDevice(g_device, nullptr);
            g_device = VK_NULL_HANDLE;
            g_physicalDevice = VK_NULL_HANDLE;

            // Reset queue handles and families
            g_graphicsQueue = VK_NULL_HANDLE;
            g_presentQueue = VK_NULL_HANDLE;
            g_graphicsQueueFamily = UINT32_MAX;
            g_presentQueueFamily = UINT32_MAX;
        }
    }

    VkPhysicalDevice GetPhysicalDevice() { return g_physicalDevice; }
    VkDevice GetDevice() { return g_device; }
    
    VkQueue GetGraphicsQueue() { return g_graphicsQueue; }
    uint32_t GetGraphicsQueueFamily() { return g_graphicsQueueFamily; }
    
    VkQueue GetPresentQueue() { return g_presentQueue; }
    uint32_t GetPresentQueueFamily() { return g_presentQueueFamily; }
    
    const VkPhysicalDeviceProperties& GetProperties() { return g_properties; }
    const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& GetRayTracingPipelineProperties() { return g_rayTracingPipelineProperties; }
    const VkPhysicalDeviceAccelerationStructurePropertiesKHR& GetAccelerationStructureProperties() { return g_accelerationStructureProperties; }
    const VkPhysicalDeviceAccelerationStructureFeaturesKHR& GetAccelerationStructureFeatures() { return g_accelerationStructureFeatures; }
    const VkPhysicalDeviceMemoryProperties& GetMemoryProperties() { return g_memoryProperties; }
}
