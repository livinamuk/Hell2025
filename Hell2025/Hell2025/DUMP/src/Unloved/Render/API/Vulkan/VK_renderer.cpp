#include "VK_renderer.h"
#include "Unloved/Render/API/Vulkan/VK_renderer_internal.h"

#include "Hell/Logging.h"
#include "Hell/Profiling/CPUProfiler.h"
#include "Hell/Render/API/Vulkan/Managers/vk_command_manager.h"
#include "Hell/Render/API/Vulkan/Managers/vk_deletion_queue.h"
#include "Hell/Render/API/Vulkan/Managers/vk_device_manager.h"
#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/Render/API/Vulkan/Managers/vk_swapchain_manager.h"
#include "Hell/Render/API/Vulkan/Managers/vk_sync_manager.h"
#include "Hell/Render/API/Vulkan/vk_tools.h"
#include "Hell/Render/API/Vulkan/vk_types.h"
#include "Unloved/Debug/Debug.h"
#include "Unloved/Render/API/Vulkan/VK_draw.h"

#include <array>
#include <iostream>
#include <vector>

namespace VulkanRenderer {
    uint32_t g_frameIndex = 0;
    std::array<VulkanFrameData, FRAME_OVERLAP> g_frameData;
    std::vector<VkImageLayout> g_swapchainImageLayouts;
    bool g_staticSamplersUploaded = false;

    VkResult WaitForRenderFence(uint32_t frameIndex) {
        ProfilerCPUZoneFunction();

        return VulkanSyncManager::WaitForRenderFence(frameIndex);
    }

    VkResult AcquireSwapchainImage(SwapchainFrame& frame) {
        ProfilerCPUZoneFunction();

        return vkAcquireNextImageKHR(
            VulkanDeviceManager::GetDevice(),
            VulkanSwapchainManager::GetSwapchain(),
            UINT64_MAX,
            VulkanSyncManager::GetPresentSemaphore(frame.frameIndex),
            VK_NULL_HANDLE,
            &frame.imageIndex
        );
    }

    VkResult SubmitSwapchainFrame(const SwapchainFrame& frame) {
        ProfilerCPUZoneFunction();

        VkSemaphore waitSemaphore = VulkanSyncManager::GetPresentSemaphore(frame.frameIndex);
        VkSemaphore signalSemaphore = VulkanSyncManager::GetRenderFinishedSemaphore(frame.frameIndex, frame.imageIndex);
        VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &waitSemaphore;
        submitInfo.pWaitDstStageMask = &waitStage;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &frame.commandBuffer;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &signalSemaphore;

        return vkQueueSubmit(
            VulkanDeviceManager::GetGraphicsQueue(),
            1,
            &submitInfo,
            VulkanSyncManager::GetRenderFence(frame.frameIndex)
        );
    }

    VkResult PresentSwapchainFrame(const SwapchainFrame& frame) {
        ProfilerCPUZoneFunction();

        VkSwapchainKHR swapchain = VulkanSwapchainManager::GetSwapchain();
        VkSemaphore waitSemaphore = VulkanSyncManager::GetRenderFinishedSemaphore(frame.frameIndex, frame.imageIndex);

        VkPresentInfoKHR presentInfo{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &waitSemaphore;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapchain;
        presentInfo.pImageIndices = &frame.imageIndex;

        return vkQueuePresentKHR(VulkanDeviceManager::GetPresentQueue(), &presentInfo);
    }

    void Init() {
        CreateShaders();
        CreateSamplers();
        CreateStaticDescriptorSet();
        CreateRayQueryDescriptorSet();
        CreateDDGIRayQueryDescriptorSet();
        CreateFrameData();
        CreateRenderTargets();
        CreatePointShadowMaps();
        CreatePresentRenderTarget();
        CreateRenderStates();
        CreatePipelines();
        UpdateBindlessTextureDescriptors();
    }

    void InitMain() {
        UpdateBindlessTextureDescriptors();
        CreateSkybox();
    }

    void WaitIdle() {
        if (VulkanDeviceManager::GetDevice() != VK_NULL_HANDLE) {
            vkDeviceWaitIdle(VulkanDeviceManager::GetDevice());
        }
    }

    void CleanUp() {
        WaitIdle();

        ProfilerVulkanReset();
        VulkanDeletionQueue::FlushAll();
        CleanUpDDGIRayQueryScenes();
        CleanUpDDGIProbeAtlasBindlessImages();
        VulkanResourceManager::Cleanup();
    }

    void HotloadShaders() {
        VkDevice device = VulkanDeviceManager::GetDevice();
        if (device == VK_NULL_HANDLE) return;

        vkDeviceWaitIdle(device);

        std::string failedShaders = "FAILED TO HOTLOAD";
        if (!VulkanResourceManager::HotloadShaders(failedShaders)) {
            Debug::BlitQuickDebugMessage(failedShaders);
            return;
        }

        VulkanResourceManager::CleanUpPipelines();
        CreatePipelines();

        std::cout << "Hotloaded shaders\n";
        Debug::BlitQuickDebugMessage("HOTLOADED SHADERS");
    }

    bool UpdateBuffer(VulkanBuffer* buffer, const void* data, VkDeviceSize size) {
        if (!buffer) return false;

        if (size > buffer->GetSize()) {
            Logging::Error() << "VulkanRenderer::UpdateBuffer() data exceeded the current Vulkan buffer capacity\n";
            return false;
        }

        return buffer->UpdateData(data, size);
    }

    bool EnsureBufferSize(uint64_t id, VkDeviceSize size) {
        VulkanBuffer* buffer = VulkanResourceManager::GetBuffer(id);

        return EnsureBufferSize(buffer, size);
    }

    bool EnsureBufferSize(VulkanBuffer* buffer, VkDeviceSize size) {
        if (!buffer) return false;

        return buffer->EnsureSize(size);
    }

    VulkanFrameData& GetCurrentFrameData() {
        return g_frameData[g_frameIndex % FRAME_OVERLAP];
    }

    VulkanFrameData& GetFrameDataByIndex(uint32_t frameIndex) {
        return g_frameData[frameIndex % FRAME_OVERLAP];
    }

    uint32_t GetCurrentFrameIndex() {
        return g_frameIndex % FRAME_OVERLAP;
    }

    const std::string& GetZoneNames() {
        return ProfilerVulkanZoneNames();
    }

    const std::string& GetZoneGPUTimings() {
        return ProfilerVulkanGpuTimings();
    }

    const std::string& GetZoneCPUTimings() {
        return ProfilerVulkanCpuTimings();
    }

    const std::string& GetTotalGPUTime() {
        return ProfilerVulkanTotalGPU();
    }

    const std::string& GetTotalCPUTime() {
        return ProfilerVulkanTotalCPU();
    }

    float GetTotalGPUTimeFloat() {
        return ProfilerVulkanTotalGPUFloat();
    }

    bool BeginSwapchainFrame(SwapchainFrame& frame) {
        VkDevice device = VulkanDeviceManager::GetDevice();
        std::vector<VkImage>& swapchainImages = VulkanSwapchainManager::GetSwapchainImages();
        std::vector<VkImageView>& swapchainImageViews = VulkanSwapchainManager::GetSwapchainImageViews();

        if (g_swapchainImageLayouts.size() != swapchainImages.size()) {
            g_swapchainImageLayouts.assign(swapchainImages.size(), VK_IMAGE_LAYOUT_UNDEFINED);
        }

        frame.frameIndex = g_frameIndex % FRAME_OVERLAP;

        VkResult waitResult = WaitForRenderFence(frame.frameIndex);
        if (waitResult != VK_SUCCESS) {
            Logging::Error() << "VulkanRenderer::BeginSwapchainFrame() failed while waiting for render fence: " << static_cast<int>(waitResult) << "\n";
            return false;
        }

        VulkanDeletionQueue::Flush(frame.frameIndex);
        VulkanDeletionQueue::SetFrameIndex(frame.frameIndex);

        VkResult acquireResult = AcquireSwapchainImage(frame);

        if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR) {
            VulkanSwapchainManager::RecreateSwapchain();
            g_swapchainImageLayouts.clear();
            CreatePresentRenderTarget();
            return false;
        }
        if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR) {
            Logging::Error() << "VulkanRenderer::BeginSwapchainFrame() failed to acquire swapchain image\n";
            return false;
        }

        frame.extent = VulkanSwapchainManager::GetSwapchainExtent();
        frame.swapchainImage = swapchainImages[frame.imageIndex];
        frame.swapchainImageView = swapchainImageViews[frame.imageIndex];
        frame.presentImage = VulkanResourceManager::GetAllocatedImage("Present");
        if (!frame.presentImage) return false;

        VkResult resetFenceResult = VulkanSyncManager::ResetRenderFence(frame.frameIndex);
        if (resetFenceResult != VK_SUCCESS) {
            Logging::Error() << "VulkanRenderer::BeginSwapchainFrame() failed to reset render fence: " << static_cast<int>(resetFenceResult) << "\n";
            return false;
        }

        VkCommandPool commandPool = VulkanCommandManager::GetGraphicsCommandPool(frame.frameIndex);
        frame.commandBuffer = VulkanCommandManager::GetGraphicsCommandBuffer(frame.frameIndex);

        VkResult resetCommandPoolResult = vkResetCommandPool(device, commandPool, 0);
        if (resetCommandPoolResult != VK_SUCCESS) {
            Logging::Error() << "VulkanRenderer::BeginSwapchainFrame() failed to reset command pool: " << static_cast<int>(resetCommandPoolResult) << "\n";
            return false;
        }

        VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        VkResult beginCommandBufferResult = vkBeginCommandBuffer(frame.commandBuffer, &beginInfo);
        if (beginCommandBufferResult != VK_SUCCESS) {
            Logging::Error() << "VulkanRenderer::BeginSwapchainFrame() failed to begin command buffer: " << static_cast<int>(beginCommandBufferResult) << "\n";
            return false;
        }

        ProfilerVulkanBeginFrame(frame.commandBuffer, frame.frameIndex);

        ResetDrawCommandOffset();

        vktools::setImageLayout(frame.commandBuffer, frame.swapchainImage, VK_IMAGE_ASPECT_COLOR_BIT, g_swapchainImageLayouts[frame.imageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        g_swapchainImageLayouts[frame.imageIndex] = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        return true;
    }

    void EndSwapchainFrame(SwapchainFrame& frame) {
        vktools::setImageLayout(frame.commandBuffer, frame.swapchainImage, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
        g_swapchainImageLayouts[frame.imageIndex] = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        ProfilerVulkanEndFrame(frame.commandBuffer);

        VkResult endCommandBufferResult = vkEndCommandBuffer(frame.commandBuffer);
        if (endCommandBufferResult != VK_SUCCESS) {
            Logging::Error() << "VulkanRenderer::EndSwapchainFrame() failed to end command buffer: " << static_cast<int>(endCommandBufferResult) << "\n";
            return;
        }

        VkResult submitResult = SubmitSwapchainFrame(frame);
        if (submitResult != VK_SUCCESS) {
            Logging::Error() << "VulkanRenderer::EndSwapchainFrame() failed to submit command buffer: " << static_cast<int>(submitResult) << "\n";
            return;
        }

        VkResult presentResult = PresentSwapchainFrame(frame);
        if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR) {
            VulkanSwapchainManager::RecreateSwapchain();
            g_swapchainImageLayouts.clear();
            CreatePresentRenderTarget();
        }
        else if (presentResult != VK_SUCCESS) {
            Logging::Error() << "VulkanRenderer::EndSwapchainFrame() failed to present swapchain image\n";
        }

        ++g_frameIndex;
    }

}
