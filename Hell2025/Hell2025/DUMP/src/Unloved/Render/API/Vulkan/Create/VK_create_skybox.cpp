#include "Unloved/Render/API/Vulkan/VK_renderer_internal.h"

#include "Hell/Logging.h"
#include "Hell/Render/API/Vulkan/Managers/vk_command_manager.h"
#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/Render/API/Vulkan/Types/vk_cubemap.h"
#include "Hell/Render/API/Vulkan/Types/vk_texture.h"
#include "Hell/Render/API/Vulkan/vk_tools.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/ResourceManagement/Types/Texture.h"
#include "Unloved/Render/API/Vulkan/VK_descriptor_indices.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <string>
#include <unordered_map>

namespace {
    constexpr uint32_t SKYBOX_FACE_COUNT = 6;
    constexpr const char* SKYBOX_CUBEMAP_NAME = "SkyboxNightSky";

    constexpr std::array<const char*, SKYBOX_FACE_COUNT> SKYBOX_FACE_TEXTURE_NAMES = {
        "px",
        "nx",
        "py",
        "ny",
        "pz",
        "nz"
    };

    struct SkyboxFace {
        Texture* texture = nullptr;
        VulkanTexture* vulkanTexture = nullptr;
    };

    Texture* FindTexture(const char* name) {
        std::unordered_map<std::string, Texture>& textures = Hell::ResourceManager::GetTextures();
        auto it = textures.find(name);
        return it != textures.end() ? &it->second : nullptr;
    }

    bool GetSkyboxFaces(std::array<SkyboxFace, SKYBOX_FACE_COUNT>& faces) {
        for (size_t i = 0; i < SKYBOX_FACE_COUNT; i++) {
            Texture* texture = FindTexture(SKYBOX_FACE_TEXTURE_NAMES[i]);
            if (!texture || texture->GetUploadState() != UploadState::UPLOADED) {
                return false;
            }

            VulkanTexture& vulkanTexture = texture->GetVKTexture();
            if (vulkanTexture.GetImage() == VK_NULL_HANDLE) {
                return false;
            }

            faces[i].texture = texture;
            faces[i].vulkanTexture = &vulkanTexture;
        }

        return true;
    }

    bool ValidateSkyboxFaces(const std::array<SkyboxFace, SKYBOX_FACE_COUNT>& faces, uint32_t& sizeOut, VkFormat& formatOut, uint32_t& mipmapLevelCountOut) {
        const VulkanTexture* firstFace = faces[0].vulkanTexture;
        sizeOut = firstFace->GetWidth();
        formatOut = firstFace->GetFormat();
        mipmapLevelCountOut = firstFace->GetMipmapLevelCount();

        if (sizeOut == 0 || firstFace->GetHeight() == 0 || sizeOut != firstFace->GetHeight()) {
            Logging::Error() << "VulkanRenderer::CreateSkybox() failed because texture '" << SKYBOX_FACE_TEXTURE_NAMES[0] << "' is not square\n";
            return false;
        }

        if (formatOut == VK_FORMAT_UNDEFINED || mipmapLevelCountOut == 0) {
            Logging::Error() << "VulkanRenderer::CreateSkybox() failed because texture '" << SKYBOX_FACE_TEXTURE_NAMES[0] << "' has invalid Vulkan texture state\n";
            return false;
        }

        for (size_t i = 1; i < SKYBOX_FACE_COUNT; i++) {
            const VulkanTexture* face = faces[i].vulkanTexture;

            if (face->GetWidth() != sizeOut || face->GetHeight() != sizeOut) {
                Logging::Error() << "VulkanRenderer::CreateSkybox() failed because texture '" << SKYBOX_FACE_TEXTURE_NAMES[i] << "' does not match the first face dimensions\n";
                return false;
            }

            if (face->GetFormat() != formatOut) {
                Logging::Error() << "VulkanRenderer::CreateSkybox() failed because texture '" << SKYBOX_FACE_TEXTURE_NAMES[i] << "' does not match the first face format\n";
                return false;
            }

            if (face->GetMipmapLevelCount() != mipmapLevelCountOut) {
                Logging::Error() << "VulkanRenderer::CreateSkybox() failed because texture '" << SKYBOX_FACE_TEXTURE_NAMES[i] << "' does not match the first face mip count\n";
                return false;
            }
        }

        return true;
    }

    VkImageSubresourceRange GetColorRange(uint32_t mipmapLevelCount, uint32_t baseArrayLayer, uint32_t layerCount) {
        VkImageSubresourceRange range{};
        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel = 0;
        range.levelCount = mipmapLevelCount;
        range.baseArrayLayer = baseArrayLayer;
        range.layerCount = layerCount;
        return range;
    }

    void WriteSkyboxBindlessDescriptors(VulkanCubemap* cubemap) {
        if (!cubemap) return;
        if (cubemap->GetImageView() == VK_NULL_HANDLE) return;
        if (cubemap->GetSampler() == VK_NULL_HANDLE) return;

        VulkanDescriptorSet* descriptorSet = VulkanResourceManager::GetDescriptorSet("StaticDescriptorSet");
        if (!descriptorSet) return;

        descriptorSet->WriteImage(DESC_IDX_TEXTURES, cubemap->GetImageView(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VULKAN_TEXTURE_IDX_SKYBOX_NIGHT_SKY);
        descriptorSet->WriteImage(DESC_IDX_TEXTURE_SAMPLERS, VK_NULL_HANDLE, cubemap->GetSampler(), VK_IMAGE_LAYOUT_UNDEFINED, VK_DESCRIPTOR_TYPE_SAMPLER, VULKAN_TEXTURE_IDX_SKYBOX_NIGHT_SKY);
        descriptorSet->Update();
    }
}

namespace VulkanRenderer {

    void CreateSkybox() {
        if (VulkanResourceManager::CubemapExists(SKYBOX_CUBEMAP_NAME)) {
            VulkanCubemap* cubemap = VulkanResourceManager::GetCubemap(SKYBOX_CUBEMAP_NAME);
            if (cubemap && cubemap->GetImage() != VK_NULL_HANDLE) {
                WriteSkyboxBindlessDescriptors(cubemap);
                return;
            }
        }

        std::array<SkyboxFace, SKYBOX_FACE_COUNT> faces{};
        if (!GetSkyboxFaces(faces)) {
            return;
        }

        uint32_t cubemapSize = 0;
        uint32_t mipmapLevelCount = 0;
        VkFormat cubemapFormat = VK_FORMAT_UNDEFINED;
        if (!ValidateSkyboxFaces(faces, cubemapSize, cubemapFormat, mipmapLevelCount)) {
            return;
        }

        VulkanCubemap& skybox = VulkanResourceManager::CreateCubemap(SKYBOX_CUBEMAP_NAME);
        skybox.Create(cubemapSize, cubemapFormat, mipmapLevelCount, SKYBOX_CUBEMAP_NAME);
        skybox.CreateSampler(TextureWrapMode::CLAMP_TO_EDGE, TextureFilter::LINEAR, TextureFilter::LINEAR, glm::vec4(0.0f));

        VulkanCommandManager::SubmitImmediate([&](VkCommandBuffer commandBuffer) {
            VkImage cubemapImage = skybox.GetImage();
            VkImageSubresourceRange cubemapRange = GetColorRange(mipmapLevelCount, 0, SKYBOX_FACE_COUNT);

            vktools::setImageLayout(commandBuffer, cubemapImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, cubemapRange, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

            for (uint32_t faceIndex = 0; faceIndex < SKYBOX_FACE_COUNT; faceIndex++) {
                VulkanTexture* face = faces[faceIndex].vulkanTexture;
                VkImage faceImage = face->GetImage();
                VkImageSubresourceRange faceRange = GetColorRange(mipmapLevelCount, 0, 1);

                vktools::setImageLayout(commandBuffer, faceImage, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, faceRange, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

                for (uint32_t mipIndex = 0; mipIndex < mipmapLevelCount; mipIndex++) {
                    uint32_t mipSize = std::max(1u, cubemapSize >> mipIndex);

                    VkImageCopy copy{};
                    copy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    copy.srcSubresource.mipLevel = mipIndex;
                    copy.srcSubresource.baseArrayLayer = 0;
                    copy.srcSubresource.layerCount = 1;
                    copy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    copy.dstSubresource.mipLevel = mipIndex;
                    copy.dstSubresource.baseArrayLayer = faceIndex;
                    copy.dstSubresource.layerCount = 1;
                    copy.extent = { mipSize, mipSize, 1 };

                    vkCmdCopyImage(commandBuffer, faceImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, cubemapImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);
                }

                vktools::setImageLayout(commandBuffer, faceImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, faceRange, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
            }

            vktools::setImageLayout(
                commandBuffer,
                cubemapImage,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                cubemapRange,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        });

        WriteSkyboxBindlessDescriptors(&skybox);
    }
}
