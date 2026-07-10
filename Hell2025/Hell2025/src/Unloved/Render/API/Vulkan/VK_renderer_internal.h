#pragma once
#include "Unloved/Render/API/Vulkan/VK_renderer.h"
#include "Hell/Render/API/Vulkan/Types/VK_render_state.h"
#include "Hell/Render/API/Vulkan/Types/vk_timer.h"
#include "Unloved/Render/API/Vulkan/VK_push_constants.h"

#include <array>
#include <string>
#include <vector>

struct AllocatedImage;
struct VulkanBuffer;

namespace VulkanRenderer {
    struct SwapchainFrame {
        uint32_t frameIndex = 0;
        uint32_t imageIndex = 0;
        VkExtent2D extent = {};
        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
        VkImage swapchainImage = VK_NULL_HANDLE;
        VkImageView swapchainImageView = VK_NULL_HANDLE;
        AllocatedImage* presentImage = nullptr;
    };

    extern uint32_t g_frameIndex;
    extern std::array<VulkanFrameData, FRAME_OVERLAP> g_frameData;
    extern std::vector<VkImageLayout> g_swapchainImageLayouts;
    extern bool g_staticSamplersUploaded;

    void CreateFrameData();
    void CreateSamplers();
    void CreateStaticDescriptorSet();
    void CreateRayQueryDescriptorSet();
    PushConstantsFrameResources CreatePushConstantsFrameResources();

    void CreatePipelines();
    void CreateRenderStates();
    void CreateRenderTargets();
    void CreatePresentRenderTarget(VkExtent2D extent);
    void CreateShaders();
    void CreateSkybox();

    void UpdateBindlessRenderTargetDescriptors();

    void UpdateBuffers();
    void UpdateBuffersUI();

    bool UpdateBuffer(VkBuffer* buffer, const void* data, VkDeviceSize size);
    bool UpdateBuffer(VulkanBuffer* buffer, const void* data, VkDeviceSize size);
    bool EnsureBufferSize(uint64_t id, VkDeviceSize size);
    bool EnsureBufferSize(VulkanBuffer* buffer, VkDeviceSize size);

    bool BeginSwapchainFrame(SwapchainFrame& frame);
    void EndSwapchainFrame(SwapchainFrame& frame);
    void BlitImage(VkCommandBuffer commandBuffer, const std::string& srcName, const std::string& dstName, VkFilter filter);
    void BindVertexBuffer(VkCommandBuffer commandBuffer, VulkanBuffer* vertexBuffer);
    void BindIndexBuffer(VkCommandBuffer commandBuffer, VulkanBuffer* indexBuffer);
    void SetStencilReference(VkCommandBuffer commandBuffer, uint32_t stencilReference);

    // Compute passes
    void ComputeSkinningPass(VkCommandBuffer commandBuffer);
    void ComputeRedTestPass(VkCommandBuffer commandBuffer);

    // Loading screen passes
    void LoadingScreenPass(VkCommandBuffer commandBuffer, VkImageView imageView, VkExtent2D extent);

    // Game passes
    void VisibilityPass(VkCommandBuffer commandBuffer);
    void MaterialResolvePass(VkCommandBuffer commandBuffer);
    void UpdateRayTracing(VkCommandBuffer commandBuffer);
    void LightingPass(VkCommandBuffer commandBuffer);
    void LightingForwardBlendedPass(VkCommandBuffer commandBuffer);
    void SkyboxPass(VkCommandBuffer commandBuffer);
    void DebugViewPass(VkCommandBuffer commandBuffer);
    void ComputeDebugTileViewPass(VkCommandBuffer commandBuffer);
    void HairPass(VkCommandBuffer commandBuffer);
    void PostProcessingPass(VkCommandBuffer commandBuffer);

    // Tile culling passes
    void ComputeTileWorldBounds(VkCommandBuffer commandBuffer);
    void LightCullingPass(VkCommandBuffer commandBuffer);

    // Present Pass
    void PresentPass(VkCommandBuffer commandBuffer, VkImageView imageView);

    bool BeginRenderState(VkCommandBuffer commandBuffer, const VulkanRenderState& state, VkExtent2D extent);
    void EndRenderState(VkCommandBuffer commandBuffer);
}
