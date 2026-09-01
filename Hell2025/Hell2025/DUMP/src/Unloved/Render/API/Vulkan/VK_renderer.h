#pragma once
#include "Unloved/Render/API/Vulkan/VK_frame_data.h"
#include "Hell/Render/API/Vulkan/vk_common.h"

#include <string>

namespace VulkanRenderer {

    void Init();
    void InitMain();
    void WaitIdle();
    void CleanUp();
    void HotloadShaders();
    void RenderLoadingScreen();
    void RenderBlackFrame();
    void RenderGame();

    VulkanFrameData& GetCurrentFrameData();
    VulkanFrameData& GetFrameDataByIndex(uint32_t frameIndex);
    uint32_t GetCurrentFrameIndex();

    void UpdateBindlessTextureDescriptors();
    void DestroyDDGIRayQueryScene(uint64_t volumeId);

    const std::string& GetZoneNames();
    const std::string& GetZoneGPUTimings();
    const std::string& GetZoneCPUTimings();
    const std::string& GetTotalGPUTime();
    const std::string& GetTotalCPUTime();
    float GetTotalGPUTimeFloat();

    void RenderGameUIPass(VkCommandBuffer commandBuffer);
    void RenderEditorUIPass(VkCommandBuffer commandBuffer, VkImage image, VkImageView imageView, VkExtent2D extent);
}
