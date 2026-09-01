#pragma once
#include "Unloved/Render/API/Vulkan/VK_renderer.h"
#include "Hell/Render/API/Vulkan/Types/VK_render_state.h"
#include "Hell/Render/API/Vulkan/Types/vk_timer.h"
#include "Unloved/Render/API/Vulkan/VK_push_constants.h"

#include <array>
#include <string>
#include <vector>

struct AllocatedImage;
struct VulkanDescriptorSet;
struct VulkanBuffer;

namespace Unloved {
    struct DDGIVolume;
}

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
    void CreateDDGIRayQueryDescriptorSet();
    bool UpdateFrameAddressTable();
    uint64_t GetFrameAddressTableDeviceAddress();

    void CreatePipelines();
    void CreateRenderStates();
    void CreateRenderTargets();
    void CreatePresentRenderTarget();
    void CreateShaders();
    void CreateSkybox();
    void CreatePointShadowMaps();

    void UpdateBindlessRenderTargetDescriptors();

    bool UpdateBuffers();
    bool UpdateBuffersUI();

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
    void PointLightShadowPass(VkCommandBuffer commandBuffer);
    void MaterialResolvePass(VkCommandBuffer commandBuffer);
    void UpdateRayTracing(VkCommandBuffer commandBuffer);
    void LightingPass(VkCommandBuffer commandBuffer);
    void LightingForwardBlendedPass(VkCommandBuffer commandBuffer);
    void SkyboxPass(VkCommandBuffer commandBuffer);
    void DebugPass(VkCommandBuffer commandBuffer);
    void DebugViewPass(VkCommandBuffer commandBuffer);
    void DebugTileViewPass(VkCommandBuffer commandBuffer);
    void DDGIPointCloudDebugPass(VkCommandBuffer commandBuffer);
    void DDGIProbeDebugPass(VkCommandBuffer commandBuffer);
    bool BuildDDGIRayQueryScene(VkCommandBuffer commandBuffer, Unloved::DDGIVolume& ddgiVolume, VulkanDescriptorSet* descriptorSet);
    void DestroyDDGIRayQueryScene(uint64_t volumeId);
    void CleanUpDDGIRayQueryScenes();
    void CleanUpDDGIProbeAtlasBindlessImages();
    void DDGIPointCloudPass(VkCommandBuffer commandBuffer);
    void DDGIProbeUpdatePass(VkCommandBuffer commandBuffer);
    void DDGIIrradianceTexturePass(VkCommandBuffer commandBuffer);
    uint64_t GetDDGIReflectionVolumeDataDeviceAddress();
    void DDGIRaytraceScenePass(VkCommandBuffer commandBuffer);
    void EmissiveForwardPass(VkCommandBuffer commandBuffer);
    void EmissiveBloomPass(VkCommandBuffer commandBuffer);
    void HairPass(VkCommandBuffer commandBuffer);
    void PostProcessingPass(VkCommandBuffer commandBuffer);
    void ComputeTileWorldBounds(VkCommandBuffer commandBuffer);
    void LightCullingPass(VkCommandBuffer commandBuffer);
    void SpriteSheetPass(VkCommandBuffer commandBuffer);
    void HiZPass(VkCommandBuffer commandBuffer);

    void IndirectSpecularInputPass(VkCommandBuffer commandBuffer);
    void IndirectSpecularClassifyTilesPass(VkCommandBuffer commandBuffer);
    void IndirectSpecularReprojectPass(VkCommandBuffer commandBuffer);
    void IndirectSpecularPrefilterPass(VkCommandBuffer commandBuffer);
    void IndirectSpecularResolveTemporalPass(VkCommandBuffer commandBuffer);
    void ResetIndirectSpecularAMDHistory();

    void PresentPass(VkCommandBuffer commandBuffer, VkImageView imageView, VkExtent2D extent);

    bool BeginRenderState(VkCommandBuffer commandBuffer, const VulkanRenderState& state, VkExtent2D extent);
    void EndRenderState(VkCommandBuffer commandBuffer);
}
