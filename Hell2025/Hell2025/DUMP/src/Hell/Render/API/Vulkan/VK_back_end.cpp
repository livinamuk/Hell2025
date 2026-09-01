#include "VK_back_end.h"

#include "Hell/Logging.h"
#include "Hell/Render/API/Vulkan/Managers/vk_command_manager.h"
#include "Hell/Render/API/Vulkan/Managers/vk_device_manager.h"
#include "Hell/Render/API/Vulkan/Managers/vk_instance_manager.h"
#include "Hell/Render/API/Vulkan/Managers/vk_memory_manager.h"
#include "Hell/Render/API/Vulkan/Managers/vk_swapchain_manager.h"
#include "Hell/Render/API/Vulkan/Managers/vk_sync_manager.h"

namespace Vulkan::BackEnd {

    bool Init() {
        if (!VulkanInstanceManager::Init())  return false;
        if (!VulkanDeviceManager::Init())    return false;
        if (!VulkanMemoryManager::Init())    return false;
        if (!VulkanSwapchainManager::Init()) return false;
        if (!VulkanCommandManager::Init())   return false;
        if (!VulkanSyncManager::Init())      return false;

        Logging::Init() << "Vulkan::BackEnd::Init()\n";
        return true;
    }

    void BeginFrame() {
    }

    void CleanUp() {
        if (VulkanDeviceManager::GetDevice() != VK_NULL_HANDLE) {
            vkDeviceWaitIdle(VulkanDeviceManager::GetDevice());
        }

        VulkanSyncManager::Cleanup();
        VulkanCommandManager::Cleanup();
        VulkanSwapchainManager::Cleanup();
        VulkanMemoryManager::Cleanup();
        VulkanDeviceManager::Cleanup();
        VulkanInstanceManager::Cleanup();
    }
}
