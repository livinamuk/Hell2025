#include "VK_texture_uploader.h"

#include "Hell/Logging.h"
#include "Hell/Render/API/Vulkan/Managers/vk_command_manager.h"
#include "Hell/Render/API/Vulkan/Managers/vk_device_manager.h"
#include "Hell/Render/API/Vulkan/Managers/vk_memory_manager.h"
#include "Hell/Render/API/Vulkan/vk_tools.h"

#include <array>
#include <algorithm>
#include <cstring>
#include <deque>
#include <limits>
#include <utility>

namespace Vulkan::TextureUploader {

    namespace {
        constexpr size_t MAX_IN_FLIGHT_UPLOAD_COUNT = 8;

        struct UploadSlot {
            VkCommandPool commandPool = VK_NULL_HANDLE;
            VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
            VkFence fence = VK_NULL_HANDLE;
            VkBuffer stagingBuffer = VK_NULL_HANDLE;
            VmaAllocation stagingAllocation = VK_NULL_HANDLE;
            Texture* texture = nullptr;
        };

        std::array<UploadSlot, MAX_IN_FLIGHT_UPLOAD_COUNT> g_uploadSlots;
        std::deque<Texture*> g_uploadQueue;
        std::vector<Texture*> g_completedUploads;

        size_t AlignUploadOffset(size_t offset) {
            constexpr size_t alignment = 4;
            return (offset + alignment - 1) & ~(alignment - 1);
        }

        VkFormat ImageFormatToVkFormat(ImageFormat format) {
            switch (format) {
            case ImageFormat::R8_UNORM:          return VK_FORMAT_R8_UNORM;
            case ImageFormat::RG8_UNORM:         return VK_FORMAT_R8G8_UNORM;
            case ImageFormat::RGB8_UNORM:        return VK_FORMAT_R8G8B8_UNORM;
            case ImageFormat::RGBA8_UNORM:       return VK_FORMAT_R8G8B8A8_UNORM;
            case ImageFormat::RGB8_SRGB:         return VK_FORMAT_R8G8B8_SRGB;
            case ImageFormat::RGBA8_SRGB:        return VK_FORMAT_R8G8B8A8_SRGB;
            case ImageFormat::R16_UNORM:         return VK_FORMAT_R16_UNORM;
            case ImageFormat::R16_SFLOAT:        return VK_FORMAT_R16_SFLOAT;
            case ImageFormat::RG16_SFLOAT:       return VK_FORMAT_R16G16_SFLOAT;
            case ImageFormat::RGB16_SFLOAT:      return VK_FORMAT_R16G16B16_SFLOAT;
            case ImageFormat::RGBA16_SFLOAT:     return VK_FORMAT_R16G16B16A16_SFLOAT;
            case ImageFormat::R32_SFLOAT:        return VK_FORMAT_R32_SFLOAT;
            case ImageFormat::RG32_SFLOAT:       return VK_FORMAT_R32G32_SFLOAT;
            case ImageFormat::RGB32_SFLOAT:      return VK_FORMAT_R32G32B32_SFLOAT;
            case ImageFormat::RGBA32_SFLOAT:     return VK_FORMAT_R32G32B32A32_SFLOAT;
            case ImageFormat::BC1_RGB_UNORM:     return VK_FORMAT_BC1_RGB_UNORM_BLOCK;
            case ImageFormat::BC1_RGBA_UNORM:    return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
            case ImageFormat::BC1_RGB_SRGB:      return VK_FORMAT_BC1_RGB_SRGB_BLOCK;
            case ImageFormat::BC1_RGBA_SRGB:     return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
            case ImageFormat::BC2_RGBA_UNORM:    return VK_FORMAT_BC2_UNORM_BLOCK;
            case ImageFormat::BC2_RGBA_SRGB:     return VK_FORMAT_BC2_SRGB_BLOCK;
            case ImageFormat::BC3_RGBA_UNORM:    return VK_FORMAT_BC3_UNORM_BLOCK;
            case ImageFormat::BC3_RGBA_SRGB:     return VK_FORMAT_BC3_SRGB_BLOCK;
            case ImageFormat::BC4_R_UNORM:       return VK_FORMAT_BC4_UNORM_BLOCK;
            case ImageFormat::BC5_RG_UNORM:      return VK_FORMAT_BC5_UNORM_BLOCK;
            case ImageFormat::BC6H_RGB_UFLOAT:   return VK_FORMAT_BC6H_UFLOAT_BLOCK;
            case ImageFormat::BC6H_RGB_SFLOAT:   return VK_FORMAT_BC6H_SFLOAT_BLOCK;
            case ImageFormat::BC7_RGBA_UNORM:    return VK_FORMAT_BC7_UNORM_BLOCK;
            case ImageFormat::BC7_RGBA_SRGB:     return VK_FORMAT_BC7_SRGB_BLOCK;
            default:                             return VK_FORMAT_UNDEFINED;
            }
        }

        size_t GetUploadMipCount(Texture& texture) {
            return texture.MipmapsAreRequested() ? texture.GetImageData().mips.size() : 1;
        }

        bool ValidateTexture(Texture& texture) {
            const ImageData& imageData = texture.GetImageData();

            if (imageData.mips.empty()) {
                Logging::Error() << "Vulkan::TextureUploader::ValidateTexture(..) failed because texture '" << texture.GetFileName() << "' has no image data\n";
                return false;
            }

            if (ImageFormatToVkFormat(imageData.format) == VK_FORMAT_UNDEFINED) {
                Logging::Error() << "Vulkan::TextureUploader::ValidateTexture(..) failed because texture '" << texture.GetFileName() << "' has an unsupported image format\n";
                return false;
            }

            const size_t uploadMipCount = GetUploadMipCount(texture);
            for (size_t mipIndex = 0; mipIndex < uploadMipCount; mipIndex++) {
                const TextureMip& mip = imageData.mips[mipIndex];
                if (mip.width == 0 || mip.height == 0 || mip.data.empty()) {
                    Logging::Error() << "Vulkan::TextureUploader::ValidateTexture(..) failed because texture '" << texture.GetFileName() << "' has invalid mip " << mipIndex << "\n";
                    return false;
                }
            }

            return true;
        }

        void MarkUploadInProgress(Texture& texture) {
            texture.SetUploadState(UploadState::UPLOADING);

            for (int mipIndex = 0; mipIndex < texture.GetTextureDataCount(); mipIndex++) {
                texture.SetTextureDataLevelBakeState(mipIndex, BakeState::BAKING_IN_PROGRESS);
            }
        }

        void MarkUploadComplete(Texture& texture) {
            texture.SetUploadState(UploadState::UPLOADED);

            for (int mipIndex = 0; mipIndex < texture.GetTextureDataCount(); mipIndex++) {
                texture.SetTextureDataLevelBakeState(mipIndex, BakeState::BAKE_COMPLETE);
            }

            texture.CheckForBakeCompletion();
            g_completedUploads.push_back(&texture);
        }

        void DestroyStagingBuffer(UploadSlot& slot) {
            if (slot.stagingBuffer != VK_NULL_HANDLE) {
                vmaDestroyBuffer(VulkanMemoryManager::GetAllocator(), slot.stagingBuffer, slot.stagingAllocation);
                slot.stagingBuffer = VK_NULL_HANDLE;
                slot.stagingAllocation = VK_NULL_HANDLE;
            }
        }

        bool EnsureSlotResources(UploadSlot& slot) {
            VkDevice device = VulkanDeviceManager::GetDevice();

            if (slot.commandPool == VK_NULL_HANDLE) {
                VkCommandPoolCreateInfo poolInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
                poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
                poolInfo.queueFamilyIndex = VulkanDeviceManager::GetGraphicsQueueFamily();

                if (vkCreateCommandPool(device, &poolInfo, nullptr, &slot.commandPool) != VK_SUCCESS) {
                    Logging::Error() << "Vulkan::TextureUploader::EnsureSlotResources(..) failed to create command pool\n";
                    return false;
                }
            }

            if (slot.commandBuffer == VK_NULL_HANDLE) {
                VkCommandBufferAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
                allocInfo.commandPool = slot.commandPool;
                allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
                allocInfo.commandBufferCount = 1;

                if (vkAllocateCommandBuffers(device, &allocInfo, &slot.commandBuffer) != VK_SUCCESS) {
                    Logging::Error() << "Vulkan::TextureUploader::EnsureSlotResources(..) failed to allocate command buffer\n";
                    return false;
                }
            }

            if (slot.fence == VK_NULL_HANDLE) {
                VkFenceCreateInfo fenceInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };

                if (vkCreateFence(device, &fenceInfo, nullptr, &slot.fence) != VK_SUCCESS) {
                    Logging::Error() << "Vulkan::TextureUploader::EnsureSlotResources(..) failed to create fence\n";
                    return false;
                }
            }

            return true;
        }

        bool CreateStagingBuffer(UploadSlot& slot, size_t requiredSize) {
            DestroyStagingBuffer(slot);

            VkBufferCreateInfo stagingInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
            stagingInfo.size = requiredSize;
            stagingInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

            VmaAllocationCreateInfo stagingAllocationInfo = {};
            stagingAllocationInfo.usage = VMA_MEMORY_USAGE_AUTO;
            stagingAllocationInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

            VmaAllocationInfo stagingAllocationResult = {};

            if (vmaCreateBuffer(VulkanMemoryManager::GetAllocator(), &stagingInfo, &stagingAllocationInfo, &slot.stagingBuffer, &slot.stagingAllocation, &stagingAllocationResult) != VK_SUCCESS) {
                Logging::Error() << "Vulkan::TextureUploader::CreateStagingBuffer(..) failed to create staging buffer\n";
                return false;
            }

            if (!stagingAllocationResult.pMappedData) {
                Logging::Error() << "Vulkan::TextureUploader::CreateStagingBuffer(..) failed to map staging buffer\n";
                DestroyStagingBuffer(slot);
                return false;
            }

            return true;
        }

        void RecordTextureUploadCommands(VkCommandBuffer commandBuffer, Texture& texture, VkBuffer stagingBuffer, const std::vector<size_t>& mipOffsets, bool generateMipmaps, uint32_t mipmapLevelCount) {
            const ImageData& imageData = texture.GetImageData();
            const TextureMip& baseMip = imageData.mips[0];
            const size_t uploadMipCount = GetUploadMipCount(texture);
            VkImage image = texture.GetVKTexture().GetImage();
            VkImageSubresourceRange allMips = { VK_IMAGE_ASPECT_COLOR_BIT, 0, mipmapLevelCount, 0, 1 };
            vktools::setImageLayout(commandBuffer, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, allMips, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

            std::vector<VkBufferImageCopy> copyRegions(uploadMipCount);
            for (size_t mipIndex = 0; mipIndex < uploadMipCount; mipIndex++) {
                const TextureMip& mip = imageData.mips[mipIndex];
                VkBufferImageCopy& copyRegion = copyRegions[mipIndex];
                copyRegion.bufferOffset = mipOffsets[mipIndex];
                copyRegion.bufferRowLength = 0;
                copyRegion.bufferImageHeight = 0;
                copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                copyRegion.imageSubresource.mipLevel = static_cast<uint32_t>(mipIndex);
                copyRegion.imageSubresource.baseArrayLayer = 0;
                copyRegion.imageSubresource.layerCount = 1;
                copyRegion.imageExtent = { mip.width, mip.height, 1 };
            }

            vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(copyRegions.size()), copyRegions.data());

            if (generateMipmaps) {
                int32_t mipWidth = static_cast<int32_t>(baseMip.width);
                int32_t mipHeight = static_cast<int32_t>(baseMip.height);

                for (uint32_t mipIndex = 1; mipIndex < mipmapLevelCount; mipIndex++) {
                    VkImageSubresourceRange previousMip = { VK_IMAGE_ASPECT_COLOR_BIT, mipIndex - 1, 1, 0, 1 };
                    vktools::setImageLayout(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, previousMip, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

                    VkImageBlit imageBlit = {};
                    imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    imageBlit.srcSubresource.mipLevel = mipIndex - 1;
                    imageBlit.srcSubresource.baseArrayLayer = 0;
                    imageBlit.srcSubresource.layerCount = 1;
                    imageBlit.srcOffsets[0] = { 0, 0, 0 };
                    imageBlit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
                    imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    imageBlit.dstSubresource.mipLevel = mipIndex;
                    imageBlit.dstSubresource.baseArrayLayer = 0;
                    imageBlit.dstSubresource.layerCount = 1;
                    imageBlit.dstOffsets[0] = { 0, 0, 0 };
                    imageBlit.dstOffsets[1] = { std::max(1, mipWidth / 2), std::max(1, mipHeight / 2), 1 };

                    vkCmdBlitImage(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit, VK_FILTER_LINEAR);
                    vktools::setImageLayout(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, previousMip, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
                    mipWidth = std::max(1, mipWidth / 2);
                    mipHeight = std::max(1, mipHeight / 2);
                }

                VkImageSubresourceRange lastMip = { VK_IMAGE_ASPECT_COLOR_BIT, mipmapLevelCount - 1, 1, 0, 1 };
                vktools::setImageLayout(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, lastMip, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
            }
            else {
                vktools::setImageLayout(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, allMips, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
            }
        }

        bool PrepareUpload(Texture& texture, std::vector<size_t>& mipOffsets, size_t& requiredSize, bool& generateMipmaps, uint32_t& mipmapLevelCount) {
            if (!ValidateTexture(texture)) {
                texture.SetUploadState(UploadState::FAILED);
                return false;
            }

            const ImageData& imageData = texture.GetImageData();
            const size_t uploadMipCount = GetUploadMipCount(texture);
            generateMipmaps = texture.MipmapsAreRequested() && imageData.mips.size() == 1 && !IsCompressedImageFormat(imageData.format);
            mipmapLevelCount = generateMipmaps ? static_cast<uint32_t>(texture.GetMipmapLevelCount()) : static_cast<uint32_t>(uploadMipCount);
            mipOffsets.resize(uploadMipCount);
            requiredSize = 0;

            for (size_t mipIndex = 0; mipIndex < uploadMipCount; mipIndex++) {
                requiredSize = AlignUploadOffset(requiredSize);
                mipOffsets[mipIndex] = requiredSize;

                if (imageData.mips[mipIndex].data.size() > std::numeric_limits<size_t>::max() - requiredSize) {
                    Logging::Error() << "Vulkan::TextureUploader::PrepareUpload(..) failed because texture '" << texture.GetFileName() << "' is too large\n";
                    texture.SetUploadState(UploadState::FAILED);
                    return false;
                }

                requiredSize += imageData.mips[mipIndex].data.size();
            }

            return true;
        }

        void CopyMipData(Texture& texture, std::byte* mappedData, const std::vector<size_t>& mipOffsets) {
            const ImageData& imageData = texture.GetImageData();
            const size_t uploadMipCount = GetUploadMipCount(texture);

            for (size_t mipIndex = 0; mipIndex < uploadMipCount; mipIndex++) {
                const TextureMip& mip = imageData.mips[mipIndex];
                std::memcpy(mappedData + mipOffsets[mipIndex], mip.data.data(), mip.data.size());
            }
        }

        void CreateGPUTexture(Texture& texture, uint32_t mipmapLevelCount) {
            const ImageData& imageData = texture.GetImageData();
            const TextureMip& baseMip = imageData.mips[0];
            const VkFormat format = ImageFormatToVkFormat(imageData.format);
            VulkanTexture& vulkanTexture = texture.GetVKTexture();
            vulkanTexture.Create(baseMip.width, baseMip.height, format, mipmapLevelCount);
            vulkanTexture.CreateSampler(texture.GetTextureWrapModeS(), texture.GetTextureWrapModeT(), texture.GetMinFilter(), texture.GetMagFilter(), texture.GetBorderColor());
        }

        bool SubmitUpload(UploadSlot& slot, Texture& texture) {
            std::vector<size_t> mipOffsets;
            size_t requiredSize = 0;
            bool generateMipmaps = false;
            uint32_t mipmapLevelCount = 0;

            if (!PrepareUpload(texture, mipOffsets, requiredSize, generateMipmaps, mipmapLevelCount) || !EnsureSlotResources(slot) || !CreateStagingBuffer(slot, requiredSize)) {
                texture.SetUploadState(UploadState::FAILED);
                return false;
            }

            VmaAllocationInfo stagingAllocationInfo = {};
            vmaGetAllocationInfo(VulkanMemoryManager::GetAllocator(), slot.stagingAllocation, &stagingAllocationInfo);
            CopyMipData(texture, static_cast<std::byte*>(stagingAllocationInfo.pMappedData), mipOffsets);
            CreateGPUTexture(texture, mipmapLevelCount);

            VkDevice device = VulkanDeviceManager::GetDevice();
            vkResetFences(device, 1, &slot.fence);
            vkResetCommandPool(device, slot.commandPool, 0);

            VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            vkBeginCommandBuffer(slot.commandBuffer, &beginInfo);
            RecordTextureUploadCommands(slot.commandBuffer, texture, slot.stagingBuffer, mipOffsets, generateMipmaps, mipmapLevelCount);
            vkEndCommandBuffer(slot.commandBuffer);

            VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &slot.commandBuffer;

            if (vkQueueSubmit(VulkanDeviceManager::GetGraphicsQueue(), 1, &submitInfo, slot.fence) != VK_SUCCESS) {
                Logging::Error() << "Vulkan::TextureUploader::SubmitUpload(..) failed to submit upload for texture '" << texture.GetFileName() << "'\n";
                DestroyStagingBuffer(slot);
                texture.SetUploadState(UploadState::FAILED);
                return false;
            }

            MarkUploadInProgress(texture);
            slot.texture = &texture;
            return true;
        }

        void FinishUpload(UploadSlot& slot) {
            DestroyStagingBuffer(slot);

            if (slot.texture) {
                MarkUploadComplete(*slot.texture);
                slot.texture = nullptr;
            }
        }

        void DestroySlot(UploadSlot& slot) {
            VkDevice device = VulkanDeviceManager::GetDevice();

            if (slot.texture && slot.fence != VK_NULL_HANDLE) {
                vkWaitForFences(device, 1, &slot.fence, VK_TRUE, UINT64_MAX);
                FinishUpload(slot);
            }

            DestroyStagingBuffer(slot);

            if (slot.fence != VK_NULL_HANDLE) {
                vkDestroyFence(device, slot.fence, nullptr);
                slot.fence = VK_NULL_HANDLE;
            }

            if (slot.commandPool != VK_NULL_HANDLE) {
                vkDestroyCommandPool(device, slot.commandPool, nullptr);
                slot.commandPool = VK_NULL_HANDLE;
                slot.commandBuffer = VK_NULL_HANDLE;
            }

            slot.texture = nullptr;
        }
    }

    bool ImmediateUpload(Texture& texture) {
        std::vector<size_t> mipOffsets;
        size_t requiredSize = 0;
        bool generateMipmaps = false;
        uint32_t mipmapLevelCount = 0;

        if (!PrepareUpload(texture, mipOffsets, requiredSize, generateMipmaps, mipmapLevelCount)) {
            return false;
        }

        VkBufferCreateInfo stagingInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        stagingInfo.size = requiredSize;
        stagingInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        VmaAllocationCreateInfo stagingAllocationInfo = {};
        stagingAllocationInfo.usage = VMA_MEMORY_USAGE_AUTO;
        stagingAllocationInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VkBuffer stagingBuffer = VK_NULL_HANDLE;
        VmaAllocation stagingAllocation = VK_NULL_HANDLE;
        VmaAllocationInfo stagingAllocationResult = {};
        if (vmaCreateBuffer(VulkanMemoryManager::GetAllocator(), &stagingInfo, &stagingAllocationInfo, &stagingBuffer, &stagingAllocation, &stagingAllocationResult) != VK_SUCCESS || !stagingAllocationResult.pMappedData) {
            if (stagingBuffer != VK_NULL_HANDLE) {
                vmaDestroyBuffer(VulkanMemoryManager::GetAllocator(), stagingBuffer, stagingAllocation);
            }

            Logging::Error() << "Vulkan::TextureUploader::ImmediateUpload(..) failed to create staging buffer for texture '" << texture.GetFileName() << "'\n";
            texture.SetUploadState(UploadState::FAILED);
            return false;
        }

        std::byte* mappedData = static_cast<std::byte*>(stagingAllocationResult.pMappedData);
        CopyMipData(texture, mappedData, mipOffsets);
        CreateGPUTexture(texture, mipmapLevelCount);

        VulkanCommandManager::SubmitImmediate([&](VkCommandBuffer commandBuffer) {
            RecordTextureUploadCommands(commandBuffer, texture, stagingBuffer, mipOffsets, generateMipmaps, mipmapLevelCount);
        });

        vmaDestroyBuffer(VulkanMemoryManager::GetAllocator(), stagingBuffer, stagingAllocation);
        MarkUploadComplete(texture);
        return true;
    }

    void QueueUpload(Texture& texture) {
        texture.SetUploadState(UploadState::QUEUED);
        g_uploadQueue.push_back(&texture);
    }

    void Update() {
        VkDevice device = VulkanDeviceManager::GetDevice();

        for (UploadSlot& slot : g_uploadSlots) {
            if (!slot.texture || slot.fence == VK_NULL_HANDLE) {
                continue;
            }

            const VkResult result = vkGetFenceStatus(device, slot.fence);
            if (result == VK_SUCCESS) {
                FinishUpload(slot);
            }
            else if (result != VK_NOT_READY) {
                Logging::Error() << "Vulkan::TextureUploader::Update(..) encountered a failed texture upload fence\n";
                slot.texture->SetUploadState(UploadState::FAILED);
                DestroyStagingBuffer(slot);
                slot.texture = nullptr;
            }
        }

        for (UploadSlot& slot : g_uploadSlots) {
            if (g_uploadQueue.empty()) {
                break;
            }

            if (slot.texture) {
                continue;
            }

            Texture* texture = g_uploadQueue.front();
            g_uploadQueue.pop_front();
            SubmitUpload(slot, *texture);
        }
    }

    std::vector<Texture*> ConsumeCompletedUploads() {
        std::vector<Texture*> completedUploads = std::move(g_completedUploads);
        g_completedUploads.clear();
        return completedUploads;
    }

    void CleanUp() {
        g_uploadQueue.clear();
        g_completedUploads.clear();

        for (UploadSlot& slot : g_uploadSlots) {
            DestroySlot(slot);
        }
    }
}
