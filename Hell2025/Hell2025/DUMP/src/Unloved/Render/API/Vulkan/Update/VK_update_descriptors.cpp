#include "Unloved/Render/API/Vulkan/VK_renderer_internal.h"

#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/ResourceManagement/Types/Texture.h"
#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/Render/API/Vulkan/Types/vk_allocated_image.h"
#include "Hell/Render/API/Vulkan/Types/vk_buffer.h"
#include "Hell/Render/API/Vulkan/Types/vk_cube_map_array.h"
#include "Hell/Render/API/Vulkan/Types/vk_descriptor_set.h"
#include "Hell/Render/API/Vulkan/Types/vk_sampler.h"
#include "Hell/Render/API/Vulkan/Types/vk_texture.h"
#include "Unloved/Render/API/Vulkan/VK_descriptor_indices.h"

#include <algorithm>
#include <string>

namespace VulkanRenderer {

    namespace {
        enum class SampledImageViewType {
            Default,
            DepthOnly
        };

        bool TryWriteSampledImage(VulkanDescriptorSet& descriptorSet, uint32_t binding, const std::string& imageName, uint32_t arrayElement, SampledImageViewType viewType = SampledImageViewType::Default) {
            if (!VulkanResourceManager::AllocatedImageExists(imageName)) return false;

            AllocatedImage* image = VulkanResourceManager::GetAllocatedImage(imageName);
            if (!image) return false;

            VkImageView imageView = viewType == SampledImageViewType::DepthOnly
                ? image->GetSampledDepthOnlyImageView()
                : image->GetSampledImageView();
            if (imageView == VK_NULL_HANDLE) return false;

            descriptorSet.WriteImage(binding, imageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, arrayElement);
            return true;
        }

        bool TryWriteStorageImage(VulkanDescriptorSet& descriptorSet, uint32_t binding, const std::string& imageName, uint32_t arrayElement) {
            if (!VulkanResourceManager::AllocatedImageExists(imageName)) return false;

            AllocatedImage* image = VulkanResourceManager::GetAllocatedImage(imageName);
            if (!image) return false;

            VkImageView imageView = image->GetImageView();
            if (imageView == VK_NULL_HANDLE) return false;

            descriptorSet.WriteImage(binding, imageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, arrayElement);
            return true;
        }

        bool TryWritePointShadowMap(VulkanDescriptorSet& descriptorSet, const std::string& imageName, uint32_t arrayElement) {
            if (!VulkanResourceManager::CubeMapArrayExists(imageName)) return false;

            VulkanCubeMapArray* image = VulkanResourceManager::GetCubeMapArray(imageName);
            if (!image || image->GetImageView() == VK_NULL_HANDLE || image->GetSampler() == VK_NULL_HANDLE) return false;

            descriptorSet.WriteImage(DESC_IDX_TEXTURE_CUBE_ARRAYS_DEPTH, image->GetImageView(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, arrayElement);
            descriptorSet.WriteImage(DESC_IDX_SHADOW_SAMPLERS, VK_NULL_HANDLE, image->GetSampler(), VK_IMAGE_LAYOUT_UNDEFINED, VK_DESCRIPTOR_TYPE_SAMPLER, arrayElement);
            return true;
        }

        bool TryWriteStorageImageMips(VulkanDescriptorSet& descriptorSet, uint32_t binding, const std::string& imageName, uint32_t firstArrayElement, uint32_t descriptorCount) {
            if (!VulkanResourceManager::AllocatedImageExists(imageName)) return false;

            AllocatedImage* image = VulkanResourceManager::GetAllocatedImage(imageName);
            if (!image || image->GetMipLevelCount() == 0) return false;

            const uint32_t lastMip = image->GetMipLevelCount() - 1;
            bool dirty = false;

            for (uint32_t descriptorIndex = 0; descriptorIndex < descriptorCount; descriptorIndex++) {
                const uint32_t mipLevel = std::min(descriptorIndex, lastMip);
                const VkImageView mipImageView = image->GetMipImageView(mipLevel);
                if (mipImageView == VK_NULL_HANDLE) continue;

                descriptorSet.WriteImage(binding, mipImageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, firstArrayElement + descriptorIndex);
                dirty = true;
            }

            return dirty;
        }
    }

    void UpdateBindlessRenderTargetDescriptors() {
        VulkanDescriptorSet* staticDescriptorSet = VulkanResourceManager::GetDescriptorSet("StaticDescriptorSet");
        if (!staticDescriptorSet) return;

        bool dirty = false;

        dirty |= TryWriteSampledImage(*staticDescriptorSet, DESC_IDX_TEXTURES, "Present", VULKAN_TEXTURE_IDX_PRESENT);
        dirty |= TryWriteSampledImage(*staticDescriptorSet, DESC_IDX_UINT_TEXTURES, "Visibility", VULKAN_UINT_TEXTURE_IDX_GBUFFER_VISIBILITY);
        dirty |= TryWriteSampledImage(*staticDescriptorSet, DESC_IDX_TEXTURES, "Depth", VULKAN_TEXTURE_IDX_GBUFFER_DEPTH, SampledImageViewType::DepthOnly);
        dirty |= TryWriteSampledImage(*staticDescriptorSet, DESC_IDX_TEXTURES, "BaseColorMetallic", VULKAN_TEXTURE_IDX_BASE_COLOR_METALLIC);
        dirty |= TryWriteSampledImage(*staticDescriptorSet, DESC_IDX_TEXTURES, "NormalXYRoughnessMisc", VULKAN_TEXTURE_IDX_NORMAL_XY_ROUGHNESS_MISC);
        dirty |= TryWriteSampledImage(*staticDescriptorSet, DESC_IDX_TEXTURES, "VelocityXYOcclusionSubSurface", VULKAN_TEXTURE_IDX_VELOCITY_XY_OCCLUSION_SUBSURFACE);
        dirty |= TryWriteSampledImage(*staticDescriptorSet, DESC_IDX_TEXTURES, "IndirectDiffuse", VULKAN_TEXTURE_IDX_INDIRECT_DIFFUSE);
        dirty |= TryWriteSampledImage(*staticDescriptorSet, DESC_IDX_TEXTURES, "IndirectDiffuseSurface", VULKAN_TEXTURE_IDX_INDIRECT_DIFFUSE_SURFACE);
        dirty |= TryWriteSampledImage(*staticDescriptorSet, DESC_IDX_TEXTURES, "IndirectSpecularAMDRayInput", VULKAN_TEXTURE_IDX_INDIRECT_SPECULAR_AMD_RAY_INPUT);
        dirty |= TryWriteSampledImage(*staticDescriptorSet, DESC_IDX_TEXTURES, "IndirectSpecularAMDInput", VULKAN_TEXTURE_IDX_INDIRECT_SPECULAR_AMD_INPUT);
        dirty |= TryWriteSampledImage(*staticDescriptorSet, DESC_IDX_TEXTURES, "IndirectSpecularAMDNormalHistory", VULKAN_TEXTURE_IDX_INDIRECT_SPECULAR_AMD_NORMAL_HISTORY);
        dirty |= TryWriteSampledImage(*staticDescriptorSet, DESC_IDX_TEXTURES, "IndirectSpecularAMDDepthHistory", VULKAN_TEXTURE_IDX_INDIRECT_SPECULAR_AMD_DEPTH_HISTORY);
        dirty |= TryWriteSampledImage(*staticDescriptorSet, DESC_IDX_TEXTURES, "IndirectSpecularAMDVarianceHistory", VULKAN_TEXTURE_IDX_INDIRECT_SPECULAR_AMD_VARIANCE_HISTORY);
        dirty |= TryWriteSampledImage(*staticDescriptorSet, DESC_IDX_TEXTURES, "IndirectSpecularAMDSampleCountHistory", VULKAN_TEXTURE_IDX_INDIRECT_SPECULAR_AMD_SAMPLE_COUNT_HISTORY);
        dirty |= TryWriteSampledImage(*staticDescriptorSet, DESC_IDX_TEXTURES, "IndirectSpecularAMDVariance", VULKAN_TEXTURE_IDX_INDIRECT_SPECULAR_AMD_VARIANCE);
        dirty |= TryWriteSampledImage(*staticDescriptorSet, DESC_IDX_TEXTURES, "IndirectSpecularAMDSampleCount", VULKAN_TEXTURE_IDX_INDIRECT_SPECULAR_AMD_SAMPLE_COUNT);
        dirty |= TryWriteSampledImage(*staticDescriptorSet, DESC_IDX_TEXTURES, "IndirectSpecularAMDPrefilteredVariance", VULKAN_TEXTURE_IDX_INDIRECT_SPECULAR_AMD_PREFILTERED_VARIANCE);
        dirty |= TryWriteSampledImage(*staticDescriptorSet, DESC_IDX_TEXTURES, "IndirectSpecularAMDMaterialRoughness", VULKAN_TEXTURE_IDX_INDIRECT_SPECULAR_AMD_MATERIAL_ROUGHNESS);
        dirty |= TryWriteSampledImage(*staticDescriptorSet, DESC_IDX_TEXTURES, "IndirectSpecularAMDExtractedRoughness", VULKAN_TEXTURE_IDX_INDIRECT_SPECULAR_AMD_EXTRACTED_ROUGHNESS);
        dirty |= TryWriteSampledImage(*staticDescriptorSet, DESC_IDX_TEXTURES, "IndirectSpecularAMDRoughnessHistory", VULKAN_TEXTURE_IDX_INDIRECT_SPECULAR_AMD_ROUGHNESS_HISTORY);
        dirty |= TryWriteSampledImage(*staticDescriptorSet, DESC_IDX_TEXTURES, "IndirectSpecularAMDHistory", VULKAN_TEXTURE_IDX_INDIRECT_SPECULAR_AMD_HISTORY);
        dirty |= TryWriteSampledImage(*staticDescriptorSet, DESC_IDX_TEXTURES, "IndirectSpecularAMDReprojected", VULKAN_TEXTURE_IDX_INDIRECT_SPECULAR_AMD_REPROJECTED);
        dirty |= TryWriteSampledImage(*staticDescriptorSet, DESC_IDX_TEXTURES, "IndirectSpecularAMDAverage", VULKAN_TEXTURE_IDX_INDIRECT_SPECULAR_AMD_AVERAGE);
        dirty |= TryWriteSampledImage(*staticDescriptorSet, DESC_IDX_TEXTURES, "IndirectSpecularAMDAverageHistory", VULKAN_TEXTURE_IDX_INDIRECT_SPECULAR_AMD_AVERAGE_HISTORY);
        dirty |= TryWriteSampledImage(*staticDescriptorSet, DESC_IDX_TEXTURES, "IndirectSpecularAMDFiltered", VULKAN_TEXTURE_IDX_INDIRECT_SPECULAR_AMD_PREFILTERED);
        dirty |= TryWriteSampledImage(*staticDescriptorSet, DESC_IDX_TEXTURES, "IndirectSpecularAMDTemporal", VULKAN_TEXTURE_IDX_INDIRECT_SPECULAR_AMD_TEMPORAL);
        dirty |= TryWriteSampledImage(*staticDescriptorSet, DESC_IDX_TEXTURES, "HiZ", VULKAN_TEXTURE_IDX_HIZ);
        dirty |= TryWriteSampledImage(*staticDescriptorSet, DESC_IDX_TEXTURES, "HairLighting", VULKAN_TEXTURE_IDX_HAIR_LIGHTING);
        dirty |= TryWriteSampledImage(*staticDescriptorSet, DESC_IDX_TEXTURES, "Emissive", VULKAN_TEXTURE_IDX_EMISSIVE);
        dirty |= TryWriteSampledImage(*staticDescriptorSet, DESC_IDX_TEXTURES, "EmissiveBloomA", VULKAN_TEXTURE_IDX_EMISSIVE_BLOOM_A);
        dirty |= TryWriteSampledImage(*staticDescriptorSet, DESC_IDX_TEXTURES, "EmissiveBloomB", VULKAN_TEXTURE_IDX_EMISSIVE_BLOOM_B);

        dirty |= TryWritePointShadowMap(*staticDescriptorSet, "PointShadowHiRes", VULKAN_POINT_SHADOW_IDX_HIGH_RES);
        dirty |= TryWritePointShadowMap(*staticDescriptorSet, "PointShadowLowRes", VULKAN_POINT_SHADOW_IDX_LOW_RES);

        dirty |= TryWriteStorageImage(*staticDescriptorSet, DESC_IDX_STORAGE_IMAGES_RGBA16F, "Lighting", VULKAN_STORAGE_IMAGE_IDX_GBUFFER_LIGHTING);
        dirty |= TryWriteStorageImage(*staticDescriptorSet, DESC_IDX_STORAGE_IMAGES_RGBA16F, "IndirectDiffuse", VULKAN_STORAGE_IMAGE_IDX_INDIRECT_DIFFUSE);
        dirty |= TryWriteStorageImage(*staticDescriptorSet, DESC_IDX_STORAGE_IMAGES_RGBA16F, "IndirectDiffuseSurface", VULKAN_STORAGE_IMAGE_IDX_INDIRECT_DIFFUSE_SURFACE);
        dirty |= TryWriteStorageImage(*staticDescriptorSet, DESC_IDX_STORAGE_IMAGES_RGBA16F, "IndirectSpecularAMDInput", VULKAN_STORAGE_IMAGE_IDX_INDIRECT_SPECULAR_AMD_INPUT);
        dirty |= TryWriteStorageImage(*staticDescriptorSet, DESC_IDX_STORAGE_IMAGES_RGBA16F, "IndirectSpecularAMDReprojected", VULKAN_STORAGE_IMAGE_IDX_INDIRECT_SPECULAR_AMD_REPROJECTED);
        dirty |= TryWriteStorageImage(*staticDescriptorSet, DESC_IDX_STORAGE_IMAGES_RGBA16F, "IndirectSpecularAMDNormalHistory", VULKAN_STORAGE_IMAGE_IDX_INDIRECT_SPECULAR_AMD_NORMAL_HISTORY);
        dirty |= TryWriteStorageImage(*staticDescriptorSet, DESC_IDX_STORAGE_IMAGES_RGBA16F, "IndirectSpecularAMDFiltered", VULKAN_STORAGE_IMAGE_IDX_INDIRECT_SPECULAR_AMD_PREFILTERED);
        dirty |= TryWriteStorageImage(*staticDescriptorSet, DESC_IDX_STORAGE_IMAGES_RGBA16F, "IndirectSpecularAMDTemporal", VULKAN_STORAGE_IMAGE_IDX_INDIRECT_SPECULAR_AMD_TEMPORAL);
        dirty |= TryWriteStorageImage(*staticDescriptorSet, DESC_IDX_STORAGE_IMAGES_R16F, "IndirectSpecularAMDVariance", VULKAN_STORAGE_IMAGE_R16F_IDX_INDIRECT_SPECULAR_AMD_VARIANCE);
        dirty |= TryWriteStorageImage(*staticDescriptorSet, DESC_IDX_STORAGE_IMAGES_R16F, "IndirectSpecularAMDSampleCount", VULKAN_STORAGE_IMAGE_R16F_IDX_INDIRECT_SPECULAR_AMD_SAMPLE_COUNT);
        dirty |= TryWriteStorageImage(*staticDescriptorSet, DESC_IDX_STORAGE_IMAGES_R16F, "IndirectSpecularAMDVarianceHistory", VULKAN_STORAGE_IMAGE_R16F_IDX_INDIRECT_SPECULAR_AMD_VARIANCE_HISTORY);
        dirty |= TryWriteStorageImage(*staticDescriptorSet, DESC_IDX_STORAGE_IMAGES_R16F, "IndirectSpecularAMDSampleCountHistory", VULKAN_STORAGE_IMAGE_R16F_IDX_INDIRECT_SPECULAR_AMD_SAMPLE_COUNT_HISTORY);
        dirty |= TryWriteStorageImage(*staticDescriptorSet, DESC_IDX_STORAGE_IMAGES_R16F, "IndirectSpecularAMDPrefilteredVariance", VULKAN_STORAGE_IMAGE_R16F_IDX_INDIRECT_SPECULAR_AMD_PREFILTERED_VARIANCE);
        dirty |= TryWriteStorageImage(*staticDescriptorSet, DESC_IDX_STORAGE_IMAGES_R32F, "IndirectSpecularAMDDepthHistory", VULKAN_STORAGE_IMAGE_R32F_IDX_INDIRECT_SPECULAR_AMD_DEPTH_HISTORY);
        dirty |= TryWriteStorageImage(*staticDescriptorSet, DESC_IDX_STORAGE_IMAGES_R8, "IndirectSpecularAMDExtractedRoughness", VULKAN_STORAGE_IMAGE_R8_IDX_INDIRECT_SPECULAR_AMD_EXTRACTED_ROUGHNESS);
        dirty |= TryWriteStorageImage(*staticDescriptorSet, DESC_IDX_STORAGE_IMAGES_R11G11B10F, "IndirectSpecularAMDAverage", VULKAN_STORAGE_IMAGE_R11G11B10F_IDX_INDIRECT_SPECULAR_AMD_AVERAGE);
        dirty |= TryWriteStorageImage(*staticDescriptorSet, DESC_IDX_STORAGE_IMAGES_R11G11B10F, "IndirectSpecularAMDAverageHistory", VULKAN_STORAGE_IMAGE_R11G11B10F_IDX_INDIRECT_SPECULAR_AMD_AVERAGE_HISTORY);
        dirty |= TryWriteStorageImage(*staticDescriptorSet, DESC_IDX_STORAGE_IMAGES_RGBA8, "Emissive", VULKAN_STORAGE_IMAGE_RGBA8_IDX_EMISSIVE);

        dirty |= TryWriteStorageImageMips(*staticDescriptorSet, DESC_IDX_STORAGE_IMAGES_R32F, "HiZ", VULKAN_STORAGE_IMAGE_R32F_IDX_HIZ_MIP_0, VULKAN_STORAGE_IMAGE_R32F_HIZ_MIP_COUNT);
        dirty |= TryWriteStorageImageMips(*staticDescriptorSet, DESC_IDX_STORAGE_IMAGES_R11G11B10F, "EmissiveBloomA", VULKAN_STORAGE_IMAGE_R11G11B10F_IDX_BLOOM_A_MIP_0, VULKAN_STORAGE_IMAGE_R11G11B10F_BLOOM_MIP_COUNT);
        dirty |= TryWriteStorageImageMips(*staticDescriptorSet, DESC_IDX_STORAGE_IMAGES_R11G11B10F, "EmissiveBloomB", VULKAN_STORAGE_IMAGE_R11G11B10F_IDX_BLOOM_B_MIP_0, VULKAN_STORAGE_IMAGE_R11G11B10F_BLOOM_MIP_COUNT);

        VulkanBuffer* hiZAtomicCounter = VulkanResourceManager::GetBuffer("HiZAtomicCounter");
        if (hiZAtomicCounter) {
            staticDescriptorSet->WriteBuffer(
                DESC_IDX_SSBOS,
                hiZAtomicCounter->GetBuffer(),
                sizeof(uint32_t),
                VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                VULKAN_SSBO_IDX_HIZ_ATOMIC_COUNTER);
            dirty = true;
        }

        const auto writeStorageBuffer = [&](const char* name, VkDeviceSize size, uint32_t index) {
            VulkanBuffer* buffer = VulkanResourceManager::GetBuffer(name);
            if (!buffer) return;
            staticDescriptorSet->WriteBuffer(DESC_IDX_SSBOS, buffer->GetBuffer(), size, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, index);
            dirty = true;
        };
        writeStorageBuffer("IndirectSpecularAMDRayCounter", sizeof(uint32_t) * 4, VULKAN_SSBO_IDX_INDIRECT_SPECULAR_AMD_RAY_COUNTER);
        AllocatedImage* depthImage = VulkanResourceManager::GetAllocatedImage("Depth");
        const VkDeviceSize tileListSize = depthImage
            ? static_cast<VkDeviceSize>(depthImage->GetExtent2D().width) * depthImage->GetExtent2D().height * sizeof(uint32_t)
            : 0;
        if (tileListSize > 0) {
            writeStorageBuffer("IndirectSpecularAMDDenoiserTileList", tileListSize, VULKAN_SSBO_IDX_INDIRECT_SPECULAR_AMD_DENOISER_TILE_LIST);
        }
        writeStorageBuffer("IndirectSpecularAMDIndirectArgs", sizeof(uint32_t) * 6, VULKAN_SSBO_IDX_INDIRECT_SPECULAR_AMD_INDIRECT_ARGS);

        if (dirty) {
            staticDescriptorSet->Update();
        }
    }

    void UpdateBindlessTextureDescriptors() {
        VulkanDescriptorSet* staticDescriptorSet = VulkanResourceManager::GetDescriptorSet("StaticDescriptorSet");
        VulkanSampler* linearSampler = VulkanResourceManager::GetSampler("Linear");
        VulkanSampler* nearestSampler = VulkanResourceManager::GetSampler("Nearest");
        VulkanSampler* clampBorderLinearSampler = VulkanResourceManager::GetSampler("ClampBorderLinear");

        if (!staticDescriptorSet || !linearSampler || !nearestSampler || !clampBorderLinearSampler) return;

        if (!g_staticSamplersUploaded) {
            staticDescriptorSet->WriteImage(DESC_IDX_SAMPLERS, VK_NULL_HANDLE, linearSampler->GetSampler(), VK_IMAGE_LAYOUT_UNDEFINED, VK_DESCRIPTOR_TYPE_SAMPLER, VULKAN_SAMPLER_IDX_LINEAR);
            staticDescriptorSet->WriteImage(DESC_IDX_SAMPLERS, VK_NULL_HANDLE, nearestSampler->GetSampler(), VK_IMAGE_LAYOUT_UNDEFINED, VK_DESCRIPTOR_TYPE_SAMPLER, VULKAN_SAMPLER_IDX_NEAREST);
            staticDescriptorSet->WriteImage(DESC_IDX_SAMPLERS, VK_NULL_HANDLE, clampBorderLinearSampler->GetSampler(), VK_IMAGE_LAYOUT_UNDEFINED, VK_DESCRIPTOR_TYPE_SAMPLER, VULKAN_SAMPLER_IDX_CLAMP_BORDER_LINEAR);
            g_staticSamplersUploaded = true;
        }

        for (auto& [textureName, texture] : Hell::ResourceManager::GetTextures()) {
            (void)textureName;
            if (texture.GetUploadState() != UploadState::UPLOADED) continue;
            if (texture.GetBindlessIndex() < 0) continue;
            if (texture.GetVulkanId() == 0) continue;

            VulkanTexture* vulkanTexture = VulkanResourceManager::GetTexturePtr(texture.GetVulkanId());
            if (!vulkanTexture || vulkanTexture->GetImageView() == VK_NULL_HANDLE) continue;
            if (vulkanTexture->GetSampler() == VK_NULL_HANDLE) continue;

            uint32_t textureIndex = static_cast<uint32_t>(texture.GetBindlessIndex());
            staticDescriptorSet->WriteImage(DESC_IDX_TEXTURES, vulkanTexture->GetImageView(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, textureIndex);
            staticDescriptorSet->WriteImage(DESC_IDX_TEXTURE_SAMPLERS, VK_NULL_HANDLE, vulkanTexture->GetSampler(), VK_IMAGE_LAYOUT_UNDEFINED, VK_DESCRIPTOR_TYPE_SAMPLER, textureIndex);

        }

        staticDescriptorSet->Update();
    }
}
