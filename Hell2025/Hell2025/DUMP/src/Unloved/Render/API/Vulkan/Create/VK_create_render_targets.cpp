#include "Unloved/Render/API/Vulkan/VK_renderer_internal.h"

#include "Hell/Render/API/Vulkan/Managers/vk_device_manager.h"
#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/Render/API/Vulkan/Managers/vk_swapchain_manager.h"
#include "Unloved/Config/Config.h"

#include <algorithm>

namespace VulkanRenderer {

    void CreateRenderTargets() {
        const Resolutions& resolutions = Config::GetResolutions();

        VkExtent2D gBufferExtent = { static_cast<uint32_t>(resolutions.gBuffer.x), static_cast<uint32_t>(resolutions.gBuffer.y) };
        VkExtent2D halfResExtent = { static_cast<uint32_t>(resolutions.gBufferHalfRes.x), static_cast<uint32_t>(resolutions.gBufferHalfRes.y) };
        constexpr uint32_t emissiveBloomMipAlignment = 4;
        VkExtent2D emissiveBloomExtent = { std::max((gBufferExtent.width + 1) / 2, 1u), std::max((gBufferExtent.height + 1) / 2, 1u) };
        emissiveBloomExtent.width = ((emissiveBloomExtent.width + emissiveBloomMipAlignment - 1) / emissiveBloomMipAlignment) * emissiveBloomMipAlignment;
        emissiveBloomExtent.height = ((emissiveBloomExtent.height + emissiveBloomMipAlignment - 1) / emissiveBloomMipAlignment) * emissiveBloomMipAlignment;
        VkExtent2D indirectSpecularAMDAverageExtent = { (gBufferExtent.width + 7) / 8, (gBufferExtent.height + 7) / 8 };
        VkExtent2D finalImageExtent = { static_cast<uint32_t>(resolutions.finalImage.x), static_cast<uint32_t>(resolutions.finalImage.y) };
        VkFormat finalImageFormat = VulkanSwapchainManager::GetSwapchainImageFormat();

        const VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        const VkImageUsageFlags hairUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        const VkImageUsageFlags depthUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        // GBuffer
        VulkanResourceManager::CreateAllocatedImage("BaseColorMetallic", gBufferExtent.width, gBufferExtent.height, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_UNORM, usage);
        VulkanResourceManager::CreateAllocatedImage("NormalXYRoughnessMisc", gBufferExtent.width, gBufferExtent.height, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_A2B10G10R10_UNORM_PACK32, usage);
        VulkanResourceManager::CreateAllocatedImage("VelocityXYOcclusionSubSurface", gBufferExtent.width, gBufferExtent.height, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R16G16B16A16_SFLOAT, usage);
        VulkanResourceManager::CreateAllocatedImage("IndirectSpecularAMDMaterialRoughness", gBufferExtent.width, gBufferExtent.height, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8_UNORM, usage);
        VulkanResourceManager::CreateAllocatedImage("Visibility", gBufferExtent.width, gBufferExtent.height, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R32G32_UINT, usage);
        VulkanResourceManager::CreateAllocatedImage("Lighting", gBufferExtent.width, gBufferExtent.height, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R16G16B16A16_SFLOAT, usage);
        VulkanResourceManager::CreateAllocatedImage("Emissive", gBufferExtent.width, gBufferExtent.height, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_UNORM, usage);
        VulkanResourceManager::CreateAllocatedImage("Depth", gBufferExtent.width, gBufferExtent.height, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_D32_SFLOAT_S8_UINT, depthUsage);

        // One viewport-local pyramid is reused after each viewport has been completely composited.
        VulkanResourceManager::CreateAllocatedImage("EmissiveBloomA", emissiveBloomExtent.width, emissiveBloomExtent.height, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_B10G11R11_UFLOAT_PACK32, usage, true);
        VulkanResourceManager::CreateAllocatedImage("EmissiveBloomB", emissiveBloomExtent.width, emissiveBloomExtent.height, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_B10G11R11_UFLOAT_PACK32, usage, true);

        // Reverse-Z hierarchical depth buffer. It is generated at the end of
        // the frame and consumed as the next frame's occlusion structure.
        VulkanResourceManager::CreateAllocatedImage("HiZ", gBufferExtent.width, gBufferExtent.height, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R32_SFLOAT, usage, true);
        VulkanResourceManager::CreateBuffer("HiZAtomicCounter", sizeof(uint32_t), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

        // AMD Reflection DNSR's intersection contract is RGB incident radiance
        // with the reflection-ray length in alpha.
        VulkanResourceManager::CreateAllocatedImage("IndirectSpecularAMDRayInput", gBufferExtent.width, gBufferExtent.height, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R16G16B16A16_SFLOAT, usage);
        VulkanResourceManager::CreateAllocatedImage("IndirectSpecularAMDInput", gBufferExtent.width, gBufferExtent.height, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R16G16B16A16_SFLOAT, usage);

        // FidelityFX Reflection DNSR resources are render-resolution resources.
        VulkanResourceManager::CreateAllocatedImage("IndirectSpecularAMDExtractedRoughness", gBufferExtent.width, gBufferExtent.height, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8_UNORM, usage);
        VulkanResourceManager::CreateAllocatedImage("IndirectSpecularAMDNormalHistory", gBufferExtent.width, gBufferExtent.height, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R16G16B16A16_SFLOAT, usage);
        VulkanResourceManager::CreateAllocatedImage("IndirectSpecularAMDRoughnessHistory", gBufferExtent.width, gBufferExtent.height, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8_UNORM, usage);
        VulkanResourceManager::CreateAllocatedImage("IndirectSpecularAMDDepthHistory", gBufferExtent.width, gBufferExtent.height, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R32_SFLOAT, usage);
        VulkanResourceManager::CreateAllocatedImage("IndirectSpecularAMDVariance", gBufferExtent.width, gBufferExtent.height, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R16_SFLOAT, usage);
        VulkanResourceManager::CreateAllocatedImage("IndirectSpecularAMDVarianceHistory", gBufferExtent.width, gBufferExtent.height, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R16_SFLOAT, usage);
        VulkanResourceManager::CreateAllocatedImage("IndirectSpecularAMDSampleCount", gBufferExtent.width, gBufferExtent.height, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R16_SFLOAT, usage);
        VulkanResourceManager::CreateAllocatedImage("IndirectSpecularAMDSampleCountHistory", gBufferExtent.width, gBufferExtent.height, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R16_SFLOAT, usage);
        VulkanResourceManager::CreateAllocatedImage("IndirectSpecularAMDPrefilteredVariance", gBufferExtent.width, gBufferExtent.height, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R16_SFLOAT, usage);
        VulkanResourceManager::CreateAllocatedImage("IndirectSpecularAMDReprojected", gBufferExtent.width, gBufferExtent.height, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R16G16B16A16_SFLOAT, usage);
        VulkanResourceManager::CreateAllocatedImage("IndirectSpecularAMDAverage", indirectSpecularAMDAverageExtent.width, indirectSpecularAMDAverageExtent.height, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_B10G11R11_UFLOAT_PACK32, usage);
        VulkanResourceManager::CreateAllocatedImage("IndirectSpecularAMDAverageHistory", indirectSpecularAMDAverageExtent.width, indirectSpecularAMDAverageExtent.height, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_B10G11R11_UFLOAT_PACK32, usage);
        VulkanResourceManager::CreateAllocatedImage("IndirectSpecularAMDFiltered", gBufferExtent.width, gBufferExtent.height, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R16G16B16A16_SFLOAT, usage);
        VulkanResourceManager::CreateAllocatedImage("IndirectSpecularAMDTemporal", gBufferExtent.width, gBufferExtent.height, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R16G16B16A16_SFLOAT, usage);
        VulkanResourceManager::CreateAllocatedImage("IndirectSpecularAMDHistory", gBufferExtent.width, gBufferExtent.height, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R16G16B16A16_SFLOAT, usage);

        const VkDeviceSize amdReflectionPixelCount = static_cast<VkDeviceSize>(gBufferExtent.width) * gBufferExtent.height;
        VulkanResourceManager::CreateBuffer("IndirectSpecularAMDRayCounter", sizeof(uint32_t) * 4, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
        VulkanResourceManager::CreateBuffer("IndirectSpecularAMDDenoiserTileList", amdReflectionPixelCount * sizeof(uint32_t), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
        VulkanResourceManager::CreateBuffer("IndirectSpecularAMDIndirectArgs", sizeof(uint32_t) * 6, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

        // DDGI
        VulkanResourceManager::CreateAllocatedImage("IndirectDiffuse", halfResExtent.width, halfResExtent.height, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R16G16B16A16_SFLOAT, usage);
        VulkanResourceManager::CreateAllocatedImage("IndirectDiffuseSurface", halfResExtent.width, halfResExtent.height, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R16G16B16A16_SFLOAT, usage);

        // Hair
        VulkanResourceManager::CreateAllocatedImage("HairLighting", gBufferExtent.width, gBufferExtent.height, VK_SAMPLE_COUNT_4_BIT, VK_FORMAT_R16G16B16A16_SFLOAT, hairUsage);
        VulkanResourceManager::CreateAllocatedImage("HairDepth", gBufferExtent.width, gBufferExtent.height, VK_SAMPLE_COUNT_4_BIT, VK_FORMAT_D32_SFLOAT_S8_UINT, depthUsage);

        // Final image
        VulkanResourceManager::CreateAllocatedImage("FinalImage", finalImageExtent.width, finalImageExtent.height, VK_SAMPLE_COUNT_1_BIT, finalImageFormat, usage);

        UpdateBindlessRenderTargetDescriptors();
        ResetIndirectSpecularAMDHistory();
    }

    void CreatePresentRenderTarget() {
        const Resolutions& resolutions = Config::GetResolutions();
        const VkExtent2D extent = { static_cast<uint32_t>(resolutions.ui.x), static_cast<uint32_t>(resolutions.ui.y) };
        VkFormat format = VulkanSwapchainManager::GetSwapchainImageFormat();
        bool needsCreate = !VulkanResourceManager::AllocatedImageExists("Present");

        if (!needsCreate) {
            AllocatedImage* presentImage = VulkanResourceManager::GetAllocatedImage("Present");
            if (presentImage) {
                VkExtent2D presentExtent = presentImage->GetExtent2D();
                needsCreate = presentExtent.width != extent.width || presentExtent.height != extent.height || presentImage->GetFormat() != format || presentImage->GetSampleCount() != VK_SAMPLE_COUNT_1_BIT;
            }
            else {
                needsCreate = true;
            }
        }

        if (!needsCreate) return;

        if (VulkanResourceManager::AllocatedImageExists("Present")) {
            vkDeviceWaitIdle(VulkanDeviceManager::GetDevice());
        }

        VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        VulkanResourceManager::CreateAllocatedImage("Present", extent.width, extent.height, VK_SAMPLE_COUNT_1_BIT, format, usage);
        UpdateBindlessRenderTargetDescriptors();
    }
}
