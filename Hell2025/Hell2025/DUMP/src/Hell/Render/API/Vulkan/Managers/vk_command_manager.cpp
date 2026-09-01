#include "vk_command_manager.h"
#include "vk_device_manager.h"
#include "vk_sync_manager.h"

#include "Hell/Logging.h"
#include "Hell/Render/API/Vulkan/vk_types.h"

#include <iostream>

namespace VulkanCommandManager {
    struct CommandData {
        VkCommandPool graphicsPool;
        VkCommandBuffer graphicsBuffer;
    };

    CommandData g_frames[FRAME_OVERLAP];

    VkCommandPool g_uploadPool = VK_NULL_HANDLE;
    VkCommandBuffer g_uploadBuffer = VK_NULL_HANDLE;

    bool Init() {
        VkDevice device = VulkanDeviceManager::GetDevice();
        uint32_t graphicsFamily = VulkanDeviceManager::GetGraphicsQueueFamily();

        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = graphicsFamily;

        for (int i = 0; i < FRAME_OVERLAP; i++) {
            if (vkCreateCommandPool(device, &poolInfo, nullptr, &g_frames[i].graphicsPool) != VK_SUCCESS) {
                Logging::Fatal() << "VulkanCommandManager::Init() failed to create frame command pool\n";
                return false;
            }

            VkCommandBufferAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.commandPool = g_frames[i].graphicsPool;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandBufferCount = 1;

            if (vkAllocateCommandBuffers(device, &allocInfo, &g_frames[i].graphicsBuffer) != VK_SUCCESS) {
                Logging::Fatal() << "VulkanCommandManager::Init() failed to allocate frame command buffer\n";
                return false;
            }
        }

        VkCommandPoolCreateInfo uploadPoolInfo = {};
        uploadPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        uploadPoolInfo.queueFamilyIndex = graphicsFamily;

        if (vkCreateCommandPool(device, &uploadPoolInfo, nullptr, &g_uploadPool) != VK_SUCCESS) {
            Logging::Fatal() << "VulkanCommandManager::Init() failed to create upload command pool\n";
            return false;
        }

        VkCommandBufferAllocateInfo uploadAllocInfo = {};
        uploadAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        uploadAllocInfo.commandPool = g_uploadPool;
        uploadAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        uploadAllocInfo.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(device, &uploadAllocInfo, &g_uploadBuffer) != VK_SUCCESS) {
            Logging::Fatal() << "VulkanCommandManager::Init() failed to allocate upload command buffer\n";
            return false;
        }

        Logging::Init() << "VulkanCommandManager::Init()\n";
        return true;
    }

    void Cleanup() {
        VkDevice device = VulkanDeviceManager::GetDevice();

        for (int i = 0; i < FRAME_OVERLAP; i++) {
            vkDestroyCommandPool(device, g_frames[i].graphicsPool, nullptr);
        }
        vkDestroyCommandPool(device, g_uploadPool, nullptr);
    }

    VkCommandPool GetGraphicsCommandPool(uint32_t frameIndex) {
        return g_frames[frameIndex].graphicsPool;
    }

    VkCommandBuffer GetGraphicsCommandBuffer(uint32_t frameIndex) {
        return g_frames[frameIndex].graphicsBuffer;
    }

    VkCommandPool GetUploadCommandPool() {
        return g_uploadPool;
    }

    VkCommandBuffer GetUploadCommandBuffer() {
        return g_uploadBuffer;
    }

    void SubmitImmediate(std::function<void(VkCommandBuffer cmd)>&& function) {
        VkDevice device = VulkanDeviceManager::GetDevice();
        VkQueue graphicsQueue = VulkanDeviceManager::GetGraphicsQueue();
        VkFence uploadFence = VulkanSyncManager::GetUploadFence();
        VkCommandBuffer cmd = g_uploadBuffer;

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VkResult beginResult = vkBeginCommandBuffer(cmd, &beginInfo);
        if (beginResult != VK_SUCCESS) {
            Logging::Error() << "VulkanCommandManager::SubmitImmediate(..) failed to begin upload command buffer: " << static_cast<int>(beginResult) << "\n";
            return;
        }

        function(cmd);

        VkResult endResult = vkEndCommandBuffer(cmd);
        if (endResult != VK_SUCCESS) {
            Logging::Error() << "VulkanCommandManager::SubmitImmediate(..) failed to end upload command buffer: " << static_cast<int>(endResult) << "\n";
            return;
        }

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmd;

        VkResult resetFenceResult = VulkanSyncManager::ResetUploadFence();
        if (resetFenceResult != VK_SUCCESS) {
            Logging::Error() << "VulkanCommandManager::SubmitImmediate(..) failed to reset upload fence: " << static_cast<int>(resetFenceResult) << "\n";
            return;
        }

        VkResult submitResult = vkQueueSubmit(graphicsQueue, 1, &submitInfo, uploadFence);
        if (submitResult != VK_SUCCESS) {
            Logging::Error() << "VulkanCommandManager::SubmitImmediate(..) failed to submit upload command buffer: " << static_cast<int>(submitResult) << "\n";
            return;
        }

        VkResult waitResult = VulkanSyncManager::WaitForUploadFence();
        if (waitResult != VK_SUCCESS) {
            Logging::Error() << "VulkanCommandManager::SubmitImmediate(..) failed while waiting for upload fence: " << static_cast<int>(waitResult) << "\n";
            return;
        }

        VkResult resetCommandPoolResult = vkResetCommandPool(device, g_uploadPool, 0);
        if (resetCommandPoolResult != VK_SUCCESS) {
            Logging::Error() << "VulkanCommandManager::SubmitImmediate(..) failed to reset upload command pool: " << static_cast<int>(resetCommandPoolResult) << "\n";
        }
    }
}
