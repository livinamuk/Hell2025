#include "vk_instance_manager.h"
#include "Hell/Backend/Integration/GLFW.h"
#include <GLFW/glfw3.h>
#include <vector>
#include <cstring>
#include <stdexcept>
#include <iostream>

namespace VulkanInstanceManager {
    VkInstance g_instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT g_debugMessenger = VK_NULL_HANDLE;
    VkSurfaceKHR g_surface = VK_NULL_HANDLE;
    bool g_validationEnabled = true;

    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData) {

        std::cerr << "Validation Layer: " << callbackData->pMessage << std::endl;
        return VK_FALSE;
    }

    bool Init() {
        // Kill the OBS layer warning before doing anything else
#ifdef _WIN32
        _putenv_s("DISABLE_VULKAN_OBS_CAPTURE", "1");
#else
        setenv("DISABLE_VULKAN_OBS_CAPTURE", "1", 1);
#endif

        if (volkInitialize() != VK_SUCCESS) {
            std::cerr << "Failed to initialize volk\n";
            return false;
        }

        std::vector<const char*> instanceExtensions = Hell::BackEnd::GLFW::GetRequiredInstanceExtensions();

        if (g_validationEnabled) {
            instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        // Verify and enable validation layers
        std::vector<const char*> instanceLayers;
        if (g_validationEnabled) {
            uint32_t layerCount;
            vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
            std::vector<VkLayerProperties> availableLayers(layerCount);
            vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

            bool found = false;
            for (const auto& layer : availableLayers) {
                if (strcmp(layer.layerName, "VK_LAYER_KHRONOS_validation") == 0) {
                    instanceLayers.push_back("VK_LAYER_KHRONOS_validation");
                    found = true;
                    break;
                }
            }

            if (!found) {
                std::cerr << "Validation layers requested but not available\n";
                g_validationEnabled = false;
            }
        }

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Unloved";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "Unloved";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_4;

        // Configure messenger for instance creation and destruction
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        if (g_validationEnabled) {
            debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            debugCreateInfo.pfnUserCallback = DebugCallback;
        }

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
        createInfo.ppEnabledExtensionNames = instanceExtensions.data();
        createInfo.enabledLayerCount = static_cast<uint32_t>(instanceLayers.size());
        createInfo.ppEnabledLayerNames = instanceLayers.data();
        createInfo.pNext = g_validationEnabled ? &debugCreateInfo : nullptr;

        if (vkCreateInstance(&createInfo, nullptr, &g_instance) != VK_SUCCESS) {
            return false;
        }

        volkLoadInstance(g_instance);

        // Create the persistent debug messenger
        if (g_validationEnabled) {
            auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(g_instance, "vkCreateDebugUtilsMessengerEXT");
            if (func) {
                if (func(g_instance, &debugCreateInfo, nullptr, &g_debugMessenger) != VK_SUCCESS) {
                    std::cerr << "Failed to set up debug messenger\n";
                }
            }
        }

        GLFWwindow* window = static_cast<GLFWwindow*>(Hell::BackEnd::GLFW::GetWindowPointer());
        if (glfwCreateWindowSurface(g_instance, window, nullptr, &g_surface) != VK_SUCCESS) {
            return false;
        }

        return true;
    }

    void Cleanup() {
        if (g_surface != VK_NULL_HANDLE) {
            vkDestroySurfaceKHR(g_instance, g_surface, nullptr);
            g_surface = VK_NULL_HANDLE;
        }

        if (g_debugMessenger != VK_NULL_HANDLE) {
            auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(g_instance, "vkDestroyDebugUtilsMessengerEXT");
            if (func) func(g_instance, g_debugMessenger, nullptr);
            g_debugMessenger = VK_NULL_HANDLE;
        }

        if (g_instance != VK_NULL_HANDLE) {
            vkDestroyInstance(g_instance, nullptr);
            g_instance = VK_NULL_HANDLE;
        }
    }

    VkInstance GetInstance() { return g_instance; }
    VkDebugUtilsMessengerEXT GetDebugMessenger() { return g_debugMessenger; }
    VkSurfaceKHR GetSurface() { return g_surface; }
    bool ValidationEnabled() { return g_validationEnabled; }
}
