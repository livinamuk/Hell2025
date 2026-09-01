#include "Unloved/Render/API/Vulkan/VK_renderer_internal.h"

#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/Render/API/Vulkan/Types/vk_allocated_image.h"
#include "Hell/Render/API/Vulkan/Types/vk_buffer.h"
#include "Hell/Render/API/Vulkan/Types/vk_descriptor_set.h"
#include "Hell/Render/API/Vulkan/Types/vk_pipeline.h"
#include "Hell/Render/DrawCommandTypes.h"
#include "Hell/Time.h"

#include "Unloved/Render/API/Vulkan/VK_descriptor_indices.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Common/Constants.h"
#include "Unloved/Systems/DDGI/DDGIManager.h"
#include "Hell/Math/AABB.h"
#include "Unloved/Viewport/ViewportManager.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <unordered_map>
#include <vector>

using namespace Unloved;

namespace VulkanRenderer {
namespace {
    constexpr VkBufferUsageFlags DDGI_POINT_CLOUD_BUFFER_USAGE = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    constexpr VmaAllocationCreateFlags DDGI_POINT_CLOUD_BUFFER_VMA_FLAGS = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
    constexpr const char* DDGI_PROBE_STATES_BUFFER_NAME = "ProbeStates";
    constexpr const char* DDGI_REFLECTION_VOLUME_DATA_BUFFER_PREFIX = "DDGIReflectionVolumeData_";
    constexpr uint32_t DDGI_IRRADIANCE_OCTA_SIZE = 8;
    constexpr VkImageUsageFlags DDGI_PROBE_ATLAS_IMAGE_USAGE = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    constexpr uint32_t DDGI_PROBE_ATLAS_STORAGE_IMAGE_INDEX_LIMIT = 100;

    struct ProbeAtlasBindlessImages {
        uint32_t storageImageIndex = 0;
        VkImageView distanceImageView = VK_NULL_HANDLE;
        VkImageView irradianceImageView = VK_NULL_HANDLE;
    };

    uint64_t g_uploadedProbeResetVersion = std::numeric_limits<uint64_t>::max();
    float g_probeInitializationTime = 0.0f;
    bool g_ddgiReflectionVolumeDataReady = false;
    std::vector<uint64_t> g_updatedDDGIVolumeIds;
    std::unordered_map<uint64_t, ProbeAtlasBindlessImages> g_probeAtlasBindlessImages;
    std::unordered_map<uint64_t, uint64_t> g_probeAtlasResetVersions;
    uint32_t g_nextProbeAtlasStorageImageIndex = VULKAN_STORAGE_IMAGE_IDX_FIRST_DYNAMIC;

    VulkanBuffer* GetOrCreateDDGIBuffer(const std::string& name, VkDeviceSize size) {
        const VkDeviceSize nonZeroSize = std::max(size, static_cast<VkDeviceSize>(sizeof(uint32_t)));
        if (!VulkanResourceManager::BufferExists(name)) {
            VulkanResourceManager::CreateBuffer(name, nonZeroSize, DDGI_POINT_CLOUD_BUFFER_USAGE, VMA_MEMORY_USAGE_AUTO, DDGI_POINT_CLOUD_BUFFER_VMA_FLAGS);
        }

        VulkanBuffer* buffer = VulkanResourceManager::GetBuffer(name);
        if (!buffer) return nullptr;
        if (!EnsureBufferSize(buffer, nonZeroSize)) return nullptr;
        return buffer;
    }

    std::string GetDDGIReflectionVolumeDataBufferName() {
        return std::string(DDGI_REFLECTION_VOLUME_DATA_BUFFER_PREFIX) + std::to_string(GetCurrentFrameIndex());
    }

    bool UploadDDGIBuffer(const std::string& name, const void* data, VkDeviceSize size, VulkanBuffer*& buffer) {
        buffer = GetOrCreateDDGIBuffer(name, size);
        if (!buffer) return false;
        if (data && size > 0) {
            return UpdateBuffer(buffer, data, size);
        }
        return true;
    }

    void RecordBufferBarrier(VkCommandBuffer commandBuffer, VulkanBuffer* buffer, VkAccessFlags srcAccess, VkAccessFlags dstAccess, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage) {
        if (!buffer || buffer->GetBuffer() == VK_NULL_HANDLE) return;

        VkBufferMemoryBarrier barrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
        barrier.srcAccessMask = srcAccess;
        barrier.dstAccessMask = dstAccess;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.buffer = buffer->GetBuffer();
        barrier.offset = 0;
        barrier.size = buffer->GetSize();
        vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage, 0, 0, nullptr, 1, &barrier, 0, nullptr);
    }

    bool ClearDDGICounter(VkCommandBuffer commandBuffer, VulkanBuffer* buffer) {
        if (!buffer || buffer->GetBuffer() == VK_NULL_HANDLE) return false;
        vkCmdFillBuffer(commandBuffer, buffer->GetBuffer(), 0, sizeof(uint32_t), 0);
        RecordBufferBarrier(commandBuffer, buffer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        return true;
    }

    VulkanBuffer* GetDDGIBufferIfReady(const std::string& name) {
        if (!VulkanResourceManager::BufferExists(name)) return nullptr;
        VulkanBuffer* buffer = VulkanResourceManager::GetBuffer(name);
        if (!buffer || buffer->GetDeviceAddress() == 0) return nullptr;
        return buffer;
    }

    void ClearColorImage(VkCommandBuffer commandBuffer, AllocatedImage* image) {
        if (!image) return;

        image->Sync(commandBuffer, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT);

        VkClearColorValue clearValue{};
        VkImageSubresourceRange range{};
        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel = 0;
        range.levelCount = 1;
        range.baseArrayLayer = 0;
        range.layerCount = image->GetArrayLayerCount();

        vkCmdClearColorImage(commandBuffer, image->GetImage(), VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &range);
        image->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
    }

    bool CreateOrResizeProbeAtlasImage(VkCommandBuffer commandBuffer, const std::string& name, uint32_t width, uint32_t height, uint32_t layerCount, VkFormat format, AllocatedImage*& image) {
        image = VulkanResourceManager::AllocatedImageExists(name) ? VulkanResourceManager::GetAllocatedImage(name) : nullptr;
        const bool needsResize = !image || image->GetWidth() != static_cast<int32_t>(width) || image->GetHeight() != static_cast<int32_t>(height) || image->GetArrayLayerCount() != layerCount || image->GetFormat() != format;

        if (needsResize) {
            image = &VulkanResourceManager::CreateAllocatedImageArray(name, width, height, layerCount, VK_SAMPLE_COUNT_1_BIT, format, DDGI_PROBE_ATLAS_IMAGE_USAGE);
            ClearColorImage(commandBuffer, image);
        }
        else if (image) {
            image->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        }

        return image != nullptr;
    }

    bool CreateOrResizeProbeAtlases(VkCommandBuffer commandBuffer, DDGIVolume& ddgiVolume, AllocatedImage*& distanceAtlas, AllocatedImage*& irradianceAtlas) {
        const uint32_t probeCountX = static_cast<uint32_t>(ddgiVolume.GetProbeCountX());
        const uint32_t probeCountY = static_cast<uint32_t>(ddgiVolume.GetProbeCountY());
        const uint32_t probeCountZ = static_cast<uint32_t>(ddgiVolume.GetProbeCountZ());
        if (probeCountX == 0 || probeCountY == 0 || probeCountZ == 0) return false;

        const uint32_t distanceWidth = probeCountX * PROBE_DISTANCE_OCTA_SIZE;
        const uint32_t distanceHeight = probeCountZ * PROBE_DISTANCE_OCTA_SIZE;
        const uint32_t irradianceWidth = probeCountX * DDGI_IRRADIANCE_OCTA_SIZE;
        const uint32_t irradianceHeight = probeCountZ * DDGI_IRRADIANCE_OCTA_SIZE;

        if (!CreateOrResizeProbeAtlasImage(commandBuffer, ddgiVolume.GetProbeDistanceTextureArrayName(), distanceWidth, distanceHeight, probeCountY, VK_FORMAT_R16G16_SFLOAT, distanceAtlas)) return false;
        if (!CreateOrResizeProbeAtlasImage(commandBuffer, ddgiVolume.GetProbeIrradianceTextureArrayName(), irradianceWidth, irradianceHeight, probeCountY, VK_FORMAT_R16G16B16A16_SFLOAT, irradianceAtlas)) return false;

        const uint64_t volumeId = ddgiVolume.GetObjectId();
        const uint64_t resetVersion = DDGIManager::GetProbeResetVersion();
        auto [resetVersionIt, inserted] = g_probeAtlasResetVersions.try_emplace(volumeId, resetVersion);
        if (inserted || resetVersionIt->second != resetVersion) {
            ClearColorImage(commandBuffer, irradianceAtlas);
            resetVersionIt->second = resetVersion;
        }
        return true;
    }

    bool BindProbeAtlasImages(DDGIVolume& ddgiVolume, AllocatedImage* distanceAtlas, AllocatedImage* irradianceAtlas, VulkanDescriptorSet* staticDescriptorSet, uint32_t& probeAtlasImageIndex) {
        if (!distanceAtlas || !irradianceAtlas || !staticDescriptorSet) return false;

        const uint64_t volumeId = ddgiVolume.GetObjectId();
        const VkImageView distanceImageView = distanceAtlas->GetImageView();
        const VkImageView irradianceImageView = irradianceAtlas->GetImageView();
        if (distanceImageView == VK_NULL_HANDLE || irradianceImageView == VK_NULL_HANDLE) return false;

        auto [it, inserted] = g_probeAtlasBindlessImages.try_emplace(volumeId);
        ProbeAtlasBindlessImages& bindlessImages = it->second;

        if (inserted) {
            if (g_nextProbeAtlasStorageImageIndex >= DDGI_PROBE_ATLAS_STORAGE_IMAGE_INDEX_LIMIT) {
                g_probeAtlasBindlessImages.erase(it);
                return false;
            }
            bindlessImages.storageImageIndex = g_nextProbeAtlasStorageImageIndex++;
        }

        if (bindlessImages.storageImageIndex == 0) return false;

        if (bindlessImages.distanceImageView != distanceImageView || bindlessImages.irradianceImageView != irradianceImageView) {
            staticDescriptorSet->WriteImage(DESC_IDX_STORAGE_IMAGES_RG16F, distanceImageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, bindlessImages.storageImageIndex);
            staticDescriptorSet->WriteImage(DESC_IDX_STORAGE_IMAGES_RGBA16F, irradianceImageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, bindlessImages.storageImageIndex);
            staticDescriptorSet->WriteImage(DESC_IDX_TEXTURE_ARRAYS_RG16F, distanceImageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, bindlessImages.storageImageIndex);
            staticDescriptorSet->WriteImage(DESC_IDX_TEXTURE_ARRAYS_RGBA16F, irradianceImageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, bindlessImages.storageImageIndex);
            staticDescriptorSet->Update();

            bindlessImages.distanceImageView = distanceImageView;
            bindlessImages.irradianceImageView = irradianceImageView;
        }

        probeAtlasImageIndex = bindlessImages.storageImageIndex;
        return true;
    }

    bool UploadDDGIReflectionVolumeData(
        VkCommandBuffer commandBuffer,
        VulkanBuffer* probeStatesBuffer,
        const std::vector<DDGIReflectionVolumeGPU>& reflectionVolumes) {

        if (!probeStatesBuffer || probeStatesBuffer->GetDeviceAddress() == 0 || reflectionVolumes.empty()) return false;

        DDGIReflectionVolumeBufferHeaderGPU header{};
        header.probeStatesDeviceAddress = probeStatesBuffer->GetDeviceAddress();
        header.volumeCount = static_cast<uint32_t>(reflectionVolumes.size());

        const VkDeviceSize volumeDataSize = sizeof(DDGIReflectionVolumeGPU) * reflectionVolumes.size();
        const VkDeviceSize uploadSize = sizeof(header) + volumeDataSize;
        std::vector<uint8_t> uploadData(static_cast<size_t>(uploadSize));
        std::memcpy(uploadData.data(), &header, sizeof(header));
        std::memcpy(uploadData.data() + sizeof(header), reflectionVolumes.data(), static_cast<size_t>(volumeDataSize));

        VulkanBuffer* reflectionVolumeDataBuffer = nullptr;
        if (!UploadDDGIBuffer(
                GetDDGIReflectionVolumeDataBufferName(),
                uploadData.data(),
                uploadSize,
                reflectionVolumeDataBuffer)) {
            return false;
        }

        RecordBufferBarrier(
            commandBuffer,
            reflectionVolumeDataBuffer,
            VK_ACCESS_HOST_WRITE_BIT,
            VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_HOST_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        RecordBufferBarrier(
            commandBuffer,
            probeStatesBuffer,
            VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
            VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        return true;
    }

    bool UploadProbeStateBuffer(VkCommandBuffer commandBuffer) {
        const uint32_t totalProbeCount = DDGIManager::GetTotalProbeCount();
        if (totalProbeCount == 0) return false;

        const VkDeviceSize probeStatesSize = sizeof(ProbeState) * totalProbeCount;
        const uint64_t resetVersion = DDGIManager::GetProbeResetVersion();
        VulkanBuffer* existingBuffer = VulkanResourceManager::BufferExists(DDGI_PROBE_STATES_BUFFER_NAME) ? VulkanResourceManager::GetBuffer(DDGI_PROBE_STATES_BUFFER_NAME) : nullptr;
        const bool needsReset = !existingBuffer || existingBuffer->GetSize() < std::max(probeStatesSize, static_cast<VkDeviceSize>(sizeof(uint32_t))) || g_uploadedProbeResetVersion != resetVersion;

        VulkanBuffer* probeStatesBuffer = GetOrCreateDDGIBuffer(DDGI_PROBE_STATES_BUFFER_NAME, probeStatesSize);
        if (!probeStatesBuffer) return false;

        if (needsReset) {
            std::vector<ProbeState> probeStates(totalProbeCount);
            Hell::SlotMap<DDGIVolume>& ddgiVolumes = DDGIManager::GetVolumes();

            for (DDGIVolume& ddgiVolume : ddgiVolumes) {
                const uint32_t probeOffset = ddgiVolume.GetProbeOffset();
                const uint32_t probeCount = ddgiVolume.GetTotalProbeCount();

                for (uint32_t i = 0; i < probeCount; i++) {
                    ProbeState& probeState = probeStates[probeOffset + i];
                    probeState.isActive = true;
                    probeState.isRelevant = false;
                    probeState.distanceCooldown = PROBE_MAX_DISTANCE_COOLDOWN;
                    probeState.irradianceCooldown = PROBE_MAX_IRRADIANCE_COOLDOWN;
                    probeState.relocationOffset = glm::vec3(0.0f);
                    probeState.padding = 0;
                }
            }

            if (!UpdateBuffer(probeStatesBuffer, probeStates.data(), probeStatesSize)) return false;

            RecordBufferBarrier(commandBuffer, probeStatesBuffer, VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
            g_probeInitializationTime = 0.0f;
            g_uploadedProbeResetVersion = resetVersion;
        }

        return true;
    }

    bool UploadPointCloudBuffers(VkCommandBuffer commandBuffer, DDGIVolume& ddgiVolume, VulkanBuffer*& pointCloudBuffer, VulkanBuffer*& pointCloudTextureInfoBuffer, VulkanBuffer*& pointCloudDirtyFlagsBuffer) {
        const PointCloud& pointCloud = ddgiVolume.GetPointClound();
        const std::vector<CloudPoint>& points = pointCloud.GetPoints();
        const std::vector<CloudPointTextureInfo>& textureInfo = pointCloud.GetTextureInfo();
        const std::vector<uint32_t>& gridCellOffsets = pointCloud.GetGridCellOffsets();
        const std::vector<uint32_t>& gridCellCounts = pointCloud.GetGridCellCounts();

        const VkDeviceSize pointCloudSize = sizeof(CloudPoint) * points.size();
        const VkDeviceSize textureInfoSize = sizeof(CloudPointTextureInfo) * textureInfo.size();
        const VkDeviceSize gridCellOffsetsSize = sizeof(uint32_t) * gridCellOffsets.size();
        const VkDeviceSize gridCellCountsSize = sizeof(uint32_t) * gridCellCounts.size();
        const VkDeviceSize dirtyFlagsSize = sizeof(uint32_t) * points.size();

        VulkanBuffer* gridCellOffsetsBuffer = nullptr;
        VulkanBuffer* gridCellCountsBuffer = nullptr;
        if (!UploadDDGIBuffer(ddgiVolume.GetPointCloudSSBOName(), points.data(), pointCloudSize, pointCloudBuffer)) return false;
        if (!UploadDDGIBuffer(ddgiVolume.GetPointCloudTextureInfoSSBOName(), textureInfo.data(), textureInfoSize, pointCloudTextureInfoBuffer)) return false;
        if (!UploadDDGIBuffer(ddgiVolume.GetPointCloudGridOffsetsSSBOName(), gridCellOffsets.data(), gridCellOffsetsSize, gridCellOffsetsBuffer)) return false;
        if (!UploadDDGIBuffer(ddgiVolume.GetPointCloudGridCountsSSBOName(), gridCellCounts.data(), gridCellCountsSize, gridCellCountsBuffer)) return false;

        pointCloudDirtyFlagsBuffer = GetOrCreateDDGIBuffer(ddgiVolume.GetPointCloudDirtyFlagsSSBOName(), dirtyFlagsSize);
        if (!pointCloudDirtyFlagsBuffer) return false;

        RecordBufferBarrier(commandBuffer, pointCloudBuffer, VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        RecordBufferBarrier(commandBuffer, pointCloudTextureInfoBuffer, VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        RecordBufferBarrier(commandBuffer, gridCellOffsetsBuffer, VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        RecordBufferBarrier(commandBuffer, gridCellCountsBuffer, VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        return true;
    }

    bool DispatchProbePointIndices(VkCommandBuffer commandBuffer, VulkanFrameData::DDGI& ddgi, DDGIVolume& ddgiVolume, VulkanBuffer* pointCloudBuffer) {
        VulkanPipeline* pipeline = VulkanResourceManager::GetPipeline("DDGIProbePointIndices");
        if (!pipeline || !pointCloudBuffer) return false;

        const PointCloud& pointCloud = ddgiVolume.GetPointClound();
        const uint32_t totalProbes = ddgiVolume.GetTotalProbeCount();
        if (totalProbes == 0) return false;

        VulkanBuffer* gridOffsetsBuffer = GetDDGIBufferIfReady(ddgiVolume.GetPointCloudGridOffsetsSSBOName());
        VulkanBuffer* gridCountsBuffer = GetDDGIBufferIfReady(ddgiVolume.GetPointCloudGridCountsSSBOName());
        if (!gridOffsetsBuffer || !gridCountsBuffer) return false;

        VulkanBuffer* probePointIndicesBuffer = GetOrCreateDDGIBuffer(ddgiVolume.GetProbePointIndicesSSBOName(), sizeof(uint32_t) * ddgiVolume.GetProbePointIndexPoolSize());
        VulkanBuffer* probePointOffsetsBuffer = GetOrCreateDDGIBuffer(ddgiVolume.GetProbePointOffsetsSSBOName(), sizeof(uint32_t) * totalProbes);
        VulkanBuffer* probePointCountsBuffer = GetOrCreateDDGIBuffer(ddgiVolume.GetProbePointCountsSSBOName(), sizeof(uint32_t) * totalProbes);
        if (!EnsureBufferSize(ddgi.probeIndexCounter, sizeof(uint32_t))) return false;

        VulkanBuffer* probeIndexCounterBuffer = VulkanResourceManager::GetBuffer(ddgi.probeIndexCounter);
        if (!probePointIndicesBuffer || !probePointOffsetsBuffer || !probePointCountsBuffer || !probeIndexCounterBuffer) return false;
        if (!ClearDDGICounter(commandBuffer, probeIndexCounterBuffer)) return false;

        const DDGIVolumeGPU volume = ddgiVolume.GetGPUData();
        PushConstantsDDGIProbePointIndices pushConstants{};
        pushConstants.pointCloudDeviceAddress = pointCloudBuffer->GetDeviceAddress();
        pushConstants.pointCloudGridOffsetsDeviceAddress = gridOffsetsBuffer->GetDeviceAddress();
        pushConstants.pointCloudGridCountsDeviceAddress = gridCountsBuffer->GetDeviceAddress();
        pushConstants.probePointIndicesDeviceAddress = probePointIndicesBuffer->GetDeviceAddress();
        pushConstants.probePointOffsetsDeviceAddress = probePointOffsetsBuffer->GetDeviceAddress();
        pushConstants.probePointCountsDeviceAddress = probePointCountsBuffer->GetDeviceAddress();
        pushConstants.probeIndexCounterDeviceAddress = probeIndexCounterBuffer->GetDeviceAddress();
        pushConstants.volumeOrigin = volume.origin;
        pushConstants.probeSpacing = volume.probeSpacing;
        pushConstants.probeCounts = volume.probeCounts;
        pushConstants.probeOffset = volume.probeOffset;
        pushConstants.gridMin = ddgiVolume.GetBoundsMin();
        pushConstants.gridCellSize = pointCloud.GetGridCellSize();
        pushConstants.gridDimensions = pointCloud.GetGridDimensions();
        pushConstants.totalProbes = totalProbes;

        if (pushConstants.pointCloudDeviceAddress == 0 || pushConstants.pointCloudGridOffsetsDeviceAddress == 0 || pushConstants.pointCloudGridCountsDeviceAddress == 0) return false;
        if (pushConstants.probePointIndicesDeviceAddress == 0 || pushConstants.probePointOffsetsDeviceAddress == 0 || pushConstants.probePointCountsDeviceAddress == 0 || pushConstants.probeIndexCounterDeviceAddress == 0) return false;

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->GetHandle());
        vkCmdPushConstants(commandBuffer, pipeline->GetLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pushConstants), &pushConstants);
        vkCmdDispatch(commandBuffer, totalProbes, 1, 1);

        RecordBufferBarrier(commandBuffer, probePointIndicesBuffer, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        RecordBufferBarrier(commandBuffer, probePointOffsetsBuffer, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        RecordBufferBarrier(commandBuffer, probePointCountsBuffer, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        return true;
    }

    bool DispatchPointCloudBaseColor(VkCommandBuffer commandBuffer, DDGIVolume& ddgiVolume, VulkanBuffer* pointCloudBuffer, VulkanBuffer* pointCloudTextureInfoBuffer) {
        VulkanPipeline* pipeline = VulkanResourceManager::GetPipeline("DDGIPointCloudBaseColor");
        VulkanDescriptorSet* staticDescriptorSet = VulkanResourceManager::GetDescriptorSet("StaticDescriptorSet");
        if (!pipeline || !staticDescriptorSet) return false;
        if (!pointCloudBuffer || !pointCloudTextureInfoBuffer) return false;

        const uint32_t pointCount = ddgiVolume.GetPointCloudCount();
        if (pointCount == 0) return false;

        PushConstantsDDGIPointCloudBaseColor pushConstants{};
        pushConstants.pointCloudDeviceAddress = pointCloudBuffer->GetDeviceAddress();
        pushConstants.pointCloudTextureInfoDeviceAddress = pointCloudTextureInfoBuffer->GetDeviceAddress();
        pushConstants.pointCount = pointCount;
        pushConstants.textureInfoCount = static_cast<uint32_t>(ddgiVolume.GetPointCloudTextureInfo().size());
        if (pushConstants.pointCloudDeviceAddress == 0 || pushConstants.pointCloudTextureInfoDeviceAddress == 0) return false;

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->GetHandle());
        VkDescriptorSet descriptorSet = staticDescriptorSet->GetHandle();
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->GetLayout(), 0, 1, &descriptorSet, 0, nullptr);
        vkCmdPushConstants(commandBuffer, pipeline->GetLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pushConstants), &pushConstants);
        vkCmdDispatch(commandBuffer, (pointCount + 127) / 128, 1, 1);

        RecordBufferBarrier(commandBuffer, pointCloudBuffer, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        return true;
    }

    VulkanDescriptorSet* GetOrCreateRayQueryDescriptorSet(DDGIVolume& ddgiVolume) {
        const std::string& descriptorSetName = ddgiVolume.GetRayQueryDescriptorSetName();

        if (!VulkanResourceManager::DescriptorSetExists(descriptorSetName)) {
            std::vector<VkDescriptorSetLayoutBinding> bindings = {
                { 0, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1, VK_SHADER_STAGE_COMPUTE_BIT }
            };

            VkDescriptorSetLayoutCreateInfo layoutInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
            layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
            layoutInfo.pBindings = bindings.data();

            VulkanResourceManager::CreateDescriptorSet(descriptorSetName, layoutInfo, DescriptorSetLifetime::PER_FRAME);
        }

        VulkanDescriptorSetResource* descriptorSetResource = VulkanResourceManager::GetDescriptorSetResource(descriptorSetName);
        return descriptorSetResource ? &descriptorSetResource->GetSet(GetCurrentFrameIndex()) : nullptr;
    }

    bool DispatchPointCloudLighting(VkCommandBuffer commandBuffer, DDGIVolume& ddgiVolume, VulkanBuffer* pointCloudBuffer, VulkanBuffer* pointCloudDirtyFlagsBuffer, bool forceUpdate) {
        ProfilerVulkanZoneLightGreen("ComputePointCloudLighting");

        VulkanPipeline* pipeline = VulkanResourceManager::GetPipeline("DDGIPointCloudLighting");
        VulkanDescriptorSet* staticDescriptorSet = VulkanResourceManager::GetDescriptorSet("StaticDescriptorSet");
        VulkanDescriptorSet* ddgiRayQueryDescriptorSet = GetOrCreateRayQueryDescriptorSet(ddgiVolume);
        if (!pipeline || !staticDescriptorSet || !ddgiRayQueryDescriptorSet) return false;
        if (!pointCloudBuffer || !pointCloudDirtyFlagsBuffer) return false;
        if (ddgiVolume.GetPointCloudCount() == 0) return false;
        if (!BuildDDGIRayQueryScene(commandBuffer, ddgiVolume, ddgiRayQueryDescriptorSet)) return false;

        PushConstantsDDGIPointCloudLighting pushConstants{};
        pushConstants.frameAddressTableDeviceAddress = GetFrameAddressTableDeviceAddress();
        pushConstants.pointCloudDeviceAddress = pointCloudBuffer->GetDeviceAddress();
        pushConstants.pointCloudDirtyFlagsDeviceAddress = pointCloudDirtyFlagsBuffer->GetDeviceAddress();
        pushConstants.pointCount = ddgiVolume.GetPointCloudCount();
        pushConstants.lightCount = static_cast<uint32_t>(RenderDataManager::GetGPULights().size());
        pushConstants.forceUpdate = forceUpdate ? 1u : 0u;
        if (pushConstants.frameAddressTableDeviceAddress == 0) return false;
        if (pushConstants.pointCloudDeviceAddress == 0 || pushConstants.pointCloudDirtyFlagsDeviceAddress == 0) return false;

        VkDescriptorSet descriptorSets[] = {
            staticDescriptorSet->GetHandle(),
            ddgiRayQueryDescriptorSet->GetHandle()
        };

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->GetHandle());
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->GetLayout(), 0, 2, descriptorSets, 0, nullptr);
        vkCmdPushConstants(commandBuffer, pipeline->GetLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pushConstants), &pushConstants);
        vkCmdDispatch(commandBuffer, (pushConstants.pointCount + 127) / 128, 1, 1);

        RecordBufferBarrier(commandBuffer, pointCloudBuffer, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        RecordBufferBarrier(commandBuffer, pointCloudDirtyFlagsBuffer, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        return true;
    }

    bool DispatchProbeStateUpdate(VkCommandBuffer commandBuffer, DDGIVolume& ddgiVolume, VulkanBuffer* probeStatesBuffer, VulkanBuffer* dirtyDoorAABBsBuffer, uint32_t dirtyDoorAABBCount) {
        ProfilerVulkanZoneLightGreen("UpdateProbeStates");

        VulkanPipeline* pipeline = VulkanResourceManager::GetPipeline("DDGIProbeStateUpdate");
        if (!pipeline || !probeStatesBuffer || !dirtyDoorAABBsBuffer) return false;

        const uint32_t totalProbes = ddgiVolume.GetTotalProbeCount();
        if (totalProbes == 0) return false;

        const DDGIVolumeGPU volume = ddgiVolume.GetGPUData();
        PushConstantsDDGIProbeStateUpdate pushConstants{};
        pushConstants.probeStatesDeviceAddress = probeStatesBuffer->GetDeviceAddress();
        pushConstants.dirtyDoorAABBsDeviceAddress = dirtyDoorAABBsBuffer->GetDeviceAddress();
        pushConstants.volumeOrigin = volume.origin;
        pushConstants.probeSpacing = volume.probeSpacing;
        pushConstants.probeCounts = volume.probeCounts;
        pushConstants.probeOffset = volume.probeOffset;
        pushConstants.totalProbes = totalProbes;
        pushConstants.dirtyDoorAABBCount = dirtyDoorAABBCount;
        g_probeInitializationTime += Hell::Time::DeltaTime();
        pushConstants.time = g_probeInitializationTime;
        if (pushConstants.probeStatesDeviceAddress == 0 || pushConstants.dirtyDoorAABBsDeviceAddress == 0) return false;

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->GetHandle());
        vkCmdPushConstants(commandBuffer, pipeline->GetLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pushConstants), &pushConstants);
        vkCmdDispatch(commandBuffer, (totalProbes + 63) / 64, 1, 1);

        RecordBufferBarrier(commandBuffer, probeStatesBuffer, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);
        return true;
    }

    bool DispatchProbeRelevance(VkCommandBuffer commandBuffer, DDGIVolume& ddgiVolume, VulkanBuffer* probeStatesBuffer) {
        ProfilerVulkanZoneLightGreen("ComputeProbeRelevance");

        VulkanPipeline* pipeline = VulkanResourceManager::GetPipeline("DDGIProbeRelevance");
        VulkanDescriptorSet* staticDescriptorSet = VulkanResourceManager::GetDescriptorSet("StaticDescriptorSet");
        if (!pipeline || !staticDescriptorSet || !probeStatesBuffer) return false;

        AllocatedImage* normalImage = VulkanResourceManager::GetAllocatedImage("NormalXYRoughnessMisc");
        AllocatedImage* depthImage = VulkanResourceManager::GetAllocatedImage("Depth");
        if (!normalImage || !depthImage) return false;

        const uint32_t totalProbes = ddgiVolume.GetTotalProbeCount();
        if (totalProbes == 0) return false;

        normalImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        depthImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);

        const DDGIVolumeGPU volume = ddgiVolume.GetGPUData();
        PushConstantsDDGIProbeRelevance pushConstants{};
        pushConstants.frameAddressTableDeviceAddress = GetFrameAddressTableDeviceAddress();
        pushConstants.probeStatesDeviceAddress = probeStatesBuffer->GetDeviceAddress();
        pushConstants.volumeOrigin = volume.origin;
        pushConstants.probeSpacing = volume.probeSpacing;
        pushConstants.probeCounts = volume.probeCounts;
        pushConstants.probeOffset = volume.probeOffset;
        pushConstants.totalProbes = totalProbes;
        if (pushConstants.frameAddressTableDeviceAddress == 0 || pushConstants.probeStatesDeviceAddress == 0) return false;

        const VkExtent2D extent = normalImage->GetExtent2D();
        VkDescriptorSet descriptorSet = staticDescriptorSet->GetHandle();

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->GetHandle());
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->GetLayout(), 0, 1, &descriptorSet, 0, nullptr);
        vkCmdPushConstants(commandBuffer, pipeline->GetLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pushConstants), &pushConstants);
        vkCmdDispatch(commandBuffer, ((extent.width + 3) / 4 + 7) / 8, ((extent.height + 3) / 4 + 7) / 8, 1);

        RecordBufferBarrier(commandBuffer, probeStatesBuffer, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);
        return true;
    }

    bool DispatchProbeDistanceList(VkCommandBuffer commandBuffer, VulkanFrameData::DDGI& ddgi, DDGIVolume& ddgiVolume, VulkanBuffer* probeStatesBuffer) {
        ProfilerVulkanZoneLightGreen("ComputeProbeDistanceList");

        VulkanPipeline* pipeline = VulkanResourceManager::GetPipeline("DDGIProbeDistanceList");
        if (!pipeline || !probeStatesBuffer) return false;

        const uint32_t totalProbes = ddgiVolume.GetTotalProbeCount();
        if (totalProbes == 0) return false;

        if (!EnsureBufferSize(ddgi.probeDistanceCounter, sizeof(uint32_t))) return false;
        if (!EnsureBufferSize(ddgi.probeDistanceIndices, sizeof(uint32_t) * DDGIManager::GetTotalProbeCount())) return false;

        VulkanBuffer* distanceCounterBuffer = VulkanResourceManager::GetBuffer(ddgi.probeDistanceCounter);
        VulkanBuffer* distanceIndicesBuffer = VulkanResourceManager::GetBuffer(ddgi.probeDistanceIndices);
        if (!distanceCounterBuffer || !distanceIndicesBuffer) return false;
        if (!ClearDDGICounter(commandBuffer, distanceCounterBuffer)) return false;

        PushConstantsDDGIProbeDistanceList pushConstants{};
        pushConstants.probeStatesDeviceAddress = probeStatesBuffer->GetDeviceAddress();
        pushConstants.probeDistanceCounterDeviceAddress = distanceCounterBuffer->GetDeviceAddress();
        pushConstants.probeDistanceIndicesDeviceAddress = distanceIndicesBuffer->GetDeviceAddress();
        pushConstants.totalProbes = totalProbes;
        pushConstants.probeOffset = ddgiVolume.GetProbeOffset();
        if (pushConstants.probeStatesDeviceAddress == 0 || pushConstants.probeDistanceCounterDeviceAddress == 0 || pushConstants.probeDistanceIndicesDeviceAddress == 0) return false;

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->GetHandle());
        vkCmdPushConstants(commandBuffer, pipeline->GetLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pushConstants), &pushConstants);
        vkCmdDispatch(commandBuffer, (totalProbes + 63) / 64, 1, 1);

        RecordBufferBarrier(commandBuffer, distanceCounterBuffer, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        RecordBufferBarrier(commandBuffer, distanceIndicesBuffer, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        return true;
    }

    bool DispatchProbeDistanceDispatchArgs(VkCommandBuffer commandBuffer, VulkanFrameData::DDGI& ddgi) {
        VulkanPipeline* pipeline = VulkanResourceManager::GetPipeline("DDGIProbeDistanceDispatchArgs");
        if (!pipeline) return false;

        if (!EnsureBufferSize(ddgi.probeDistanceDispatchArgs, sizeof(DispatchIndirectCommand))) return false;

        VulkanBuffer* distanceCounterBuffer = VulkanResourceManager::GetBuffer(ddgi.probeDistanceCounter);
        VulkanBuffer* distanceDispatchArgsBuffer = VulkanResourceManager::GetBuffer(ddgi.probeDistanceDispatchArgs);
        if (!distanceCounterBuffer || !distanceDispatchArgsBuffer) return false;

        RecordBufferBarrier(commandBuffer, distanceDispatchArgsBuffer, VK_ACCESS_INDIRECT_COMMAND_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

        PushConstantsDDGIProbeDistanceDispatchArgs pushConstants{};
        pushConstants.probeDistanceCounterDeviceAddress = distanceCounterBuffer->GetDeviceAddress();
        pushConstants.probeDistanceDispatchArgsDeviceAddress = distanceDispatchArgsBuffer->GetDeviceAddress();
        if (pushConstants.probeDistanceCounterDeviceAddress == 0 || pushConstants.probeDistanceDispatchArgsDeviceAddress == 0) return false;

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->GetHandle());
        vkCmdPushConstants(commandBuffer, pipeline->GetLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pushConstants), &pushConstants);
        vkCmdDispatch(commandBuffer, 1, 1, 1);

        RecordBufferBarrier(commandBuffer, distanceDispatchArgsBuffer, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_INDIRECT_COMMAND_READ_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT);
        return true;
    }

    bool DispatchProbeDistance(VkCommandBuffer commandBuffer, VulkanFrameData::DDGI& ddgi, DDGIVolume& ddgiVolume, VulkanBuffer* probeStatesBuffer, VulkanDescriptorSet* staticDescriptorSet, VulkanDescriptorSet* rayQueryDescriptorSet, AllocatedImage* distanceAtlas, uint32_t distanceAtlasStorageImageIndex) {
        ProfilerVulkanZoneLightGreen("ComputeProbeDistance");

        VulkanPipeline* pipeline = VulkanResourceManager::GetPipeline("DDGIProbeDistance");
        if (!pipeline || !probeStatesBuffer || !staticDescriptorSet || !rayQueryDescriptorSet || !distanceAtlas) return false;

        const uint32_t totalProbes = ddgiVolume.GetTotalProbeCount();
        if (totalProbes == 0) return false;

        VulkanBuffer* distanceCounterBuffer = VulkanResourceManager::GetBuffer(ddgi.probeDistanceCounter);
        VulkanBuffer* distanceIndicesBuffer = VulkanResourceManager::GetBuffer(ddgi.probeDistanceIndices);
        VulkanBuffer* distanceDispatchArgsBuffer = VulkanResourceManager::GetBuffer(ddgi.probeDistanceDispatchArgs);
        if (!distanceCounterBuffer || !distanceIndicesBuffer || !distanceDispatchArgsBuffer) return false;

        const DDGIVolumeGPU volume = ddgiVolume.GetGPUData();
        PushConstantsDDGIProbeDistance pushConstants{};
        pushConstants.probeStatesDeviceAddress = probeStatesBuffer->GetDeviceAddress();
        pushConstants.probeDistanceCounterDeviceAddress = distanceCounterBuffer->GetDeviceAddress();
        pushConstants.probeDistanceIndicesDeviceAddress = distanceIndicesBuffer->GetDeviceAddress();
        pushConstants.volumeOrigin = volume.origin;
        pushConstants.probeSpacing = volume.probeSpacing;
        pushConstants.probeCounts = volume.probeCounts;
        pushConstants.probeOffset = volume.probeOffset;
        pushConstants.totalProbes = totalProbes;
        pushConstants.distanceAtlasStorageImageIndex = distanceAtlasStorageImageIndex;
        if (pushConstants.probeStatesDeviceAddress == 0 || pushConstants.probeDistanceCounterDeviceAddress == 0 || pushConstants.probeDistanceIndicesDeviceAddress == 0) return false;

        VkDescriptorSet descriptorSets[] = {
            staticDescriptorSet->GetHandle(),
            rayQueryDescriptorSet->GetHandle()
        };

        distanceAtlas->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->GetHandle());
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->GetLayout(), 0, 2, descriptorSets, 0, nullptr);
        vkCmdPushConstants(commandBuffer, pipeline->GetLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pushConstants), &pushConstants);
        vkCmdDispatchIndirect(commandBuffer, distanceDispatchArgsBuffer->GetBuffer(), 0);

        RecordBufferBarrier(commandBuffer, probeStatesBuffer, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);
        distanceAtlas->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        return true;
    }

    bool DispatchProbeDistanceBorder(VkCommandBuffer commandBuffer, DDGIVolume& ddgiVolume, VulkanDescriptorSet* staticDescriptorSet, AllocatedImage* distanceAtlas, uint32_t distanceAtlasStorageImageIndex) {
        ProfilerVulkanZoneLightGreen("ComputeProbeDistanceBorder");

        VulkanPipeline* pipeline = VulkanResourceManager::GetPipeline("DDGIProbeDistanceBorder");
        if (!pipeline || !staticDescriptorSet || !distanceAtlas) return false;

        const uint32_t totalProbes = ddgiVolume.GetTotalProbeCount();
        if (totalProbes == 0) return false;

        const DDGIVolumeGPU volume = ddgiVolume.GetGPUData();
        PushConstantsDDGIProbeBorder pushConstants{};
        pushConstants.volumeOrigin = volume.origin;
        pushConstants.probeSpacing = volume.probeSpacing;
        pushConstants.probeCounts = volume.probeCounts;
        pushConstants.totalProbes = totalProbes;
        pushConstants.probeOffset = volume.probeOffset;
        pushConstants.distanceAtlasStorageImageIndex = distanceAtlasStorageImageIndex;
        VkDescriptorSet descriptorSet = staticDescriptorSet->GetHandle();

        distanceAtlas->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->GetHandle());
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->GetLayout(), 0, 1, &descriptorSet, 0, nullptr);
        vkCmdPushConstants(commandBuffer, pipeline->GetLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pushConstants), &pushConstants);
        vkCmdDispatch(commandBuffer, (totalProbes + 63) / 64, 1, 1);

        distanceAtlas->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        return true;
    }

    bool DispatchProbeIrradianceDirtyPointCheck(VkCommandBuffer commandBuffer, DDGIVolume& ddgiVolume, VulkanBuffer* probeStatesBuffer, VulkanBuffer* pointCloudDirtyFlagsBuffer) {
        ProfilerVulkanZoneLightGreen("ComputeIrradianceDirtyPointCheck");

        VulkanPipeline* pipeline = VulkanResourceManager::GetPipeline("DDGIProbeIrradianceDirtyPointCheck");
        if (!pipeline || !probeStatesBuffer || !pointCloudDirtyFlagsBuffer) return false;

        const uint32_t totalProbes = ddgiVolume.GetTotalProbeCount();
        if (totalProbes == 0) return false;

        VulkanBuffer* probePointIndicesBuffer = GetDDGIBufferIfReady(ddgiVolume.GetProbePointIndicesSSBOName());
        VulkanBuffer* probePointOffsetsBuffer = GetDDGIBufferIfReady(ddgiVolume.GetProbePointOffsetsSSBOName());
        VulkanBuffer* probePointCountsBuffer = GetDDGIBufferIfReady(ddgiVolume.GetProbePointCountsSSBOName());
        if (!probePointIndicesBuffer || !probePointOffsetsBuffer || !probePointCountsBuffer) return false;

        PushConstantsDDGIProbeIrradianceDirtyPointCheck pushConstants{};
        pushConstants.probeStatesDeviceAddress = probeStatesBuffer->GetDeviceAddress();
        pushConstants.probePointIndicesDeviceAddress = probePointIndicesBuffer->GetDeviceAddress();
        pushConstants.probePointOffsetsDeviceAddress = probePointOffsetsBuffer->GetDeviceAddress();
        pushConstants.probePointCountsDeviceAddress = probePointCountsBuffer->GetDeviceAddress();
        pushConstants.pointCloudDirtyFlagsDeviceAddress = pointCloudDirtyFlagsBuffer->GetDeviceAddress();
        pushConstants.totalProbes = totalProbes;
        pushConstants.probeOffset = ddgiVolume.GetProbeOffset();
        if (pushConstants.probeStatesDeviceAddress == 0 || pushConstants.probePointIndicesDeviceAddress == 0 || pushConstants.probePointOffsetsDeviceAddress == 0 || pushConstants.probePointCountsDeviceAddress == 0 || pushConstants.pointCloudDirtyFlagsDeviceAddress == 0) return false;

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->GetHandle());
        vkCmdPushConstants(commandBuffer, pipeline->GetLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pushConstants), &pushConstants);
        vkCmdDispatch(commandBuffer, (totalProbes + 31) / 32, 1, 1);

        RecordBufferBarrier(commandBuffer, probeStatesBuffer, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);
        return true;
    }

    bool DispatchProbeIrradianceList(VkCommandBuffer commandBuffer, VulkanFrameData::DDGI& ddgi, DDGIVolume& ddgiVolume, VulkanBuffer* probeStatesBuffer) {
        ProfilerVulkanZoneLightGreen("ComputeProbeIrradianceList");

        VulkanPipeline* pipeline = VulkanResourceManager::GetPipeline("DDGIProbeIrradianceList");
        if (!pipeline || !probeStatesBuffer) return false;

        const uint32_t totalProbes = ddgiVolume.GetTotalProbeCount();
        if (totalProbes == 0) return false;

        if (!EnsureBufferSize(ddgi.probeIrradianceCounter, sizeof(uint32_t))) return false;
        if (!EnsureBufferSize(ddgi.probeIrradianceIndices, sizeof(uint32_t) * DDGIManager::GetTotalProbeCount())) return false;

        VulkanBuffer* irradianceCounterBuffer = VulkanResourceManager::GetBuffer(ddgi.probeIrradianceCounter);
        VulkanBuffer* irradianceIndicesBuffer = VulkanResourceManager::GetBuffer(ddgi.probeIrradianceIndices);
        if (!irradianceCounterBuffer || !irradianceIndicesBuffer) return false;
        if (!ClearDDGICounter(commandBuffer, irradianceCounterBuffer)) return false;

        PushConstantsDDGIProbeIrradianceList pushConstants{};
        pushConstants.probeStatesDeviceAddress = probeStatesBuffer->GetDeviceAddress();
        pushConstants.probeIrradianceCounterDeviceAddress = irradianceCounterBuffer->GetDeviceAddress();
        pushConstants.probeIrradianceIndicesDeviceAddress = irradianceIndicesBuffer->GetDeviceAddress();
        pushConstants.totalProbes = totalProbes;
        pushConstants.probeOffset = ddgiVolume.GetProbeOffset();
        if (pushConstants.probeStatesDeviceAddress == 0 || pushConstants.probeIrradianceCounterDeviceAddress == 0 || pushConstants.probeIrradianceIndicesDeviceAddress == 0) return false;

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->GetHandle());
        vkCmdPushConstants(commandBuffer, pipeline->GetLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pushConstants), &pushConstants);
        vkCmdDispatch(commandBuffer, (totalProbes + 63) / 64, 1, 1);

        RecordBufferBarrier(commandBuffer, irradianceCounterBuffer, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        RecordBufferBarrier(commandBuffer, irradianceIndicesBuffer, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        return true;
    }

    bool DispatchProbeIrradianceDispatchArgs(VkCommandBuffer commandBuffer, VulkanFrameData::DDGI& ddgi) {
        ProfilerVulkanZoneLightGreen("ComputeProbeIrradianceDispatchArgs");

        VulkanPipeline* pipeline = VulkanResourceManager::GetPipeline("DDGIProbeIrradianceDispatchArgs");
        if (!pipeline) return false;

        if (!EnsureBufferSize(ddgi.probeIrradianceDispatchArgs, sizeof(DispatchIndirectCommand))) return false;

        VulkanBuffer* irradianceCounterBuffer = VulkanResourceManager::GetBuffer(ddgi.probeIrradianceCounter);
        VulkanBuffer* irradianceDispatchArgsBuffer = VulkanResourceManager::GetBuffer(ddgi.probeIrradianceDispatchArgs);
        if (!irradianceCounterBuffer || !irradianceDispatchArgsBuffer) return false;

        RecordBufferBarrier(commandBuffer, irradianceDispatchArgsBuffer, VK_ACCESS_INDIRECT_COMMAND_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

        PushConstantsDDGIProbeIrradianceDispatchArgs pushConstants{};
        pushConstants.probeIrradianceCounterDeviceAddress = irradianceCounterBuffer->GetDeviceAddress();
        pushConstants.probeIrradianceDispatchArgsDeviceAddress = irradianceDispatchArgsBuffer->GetDeviceAddress();
        if (pushConstants.probeIrradianceCounterDeviceAddress == 0 || pushConstants.probeIrradianceDispatchArgsDeviceAddress == 0) return false;

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->GetHandle());
        vkCmdPushConstants(commandBuffer, pipeline->GetLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pushConstants), &pushConstants);
        vkCmdDispatch(commandBuffer, 1, 1, 1);

        RecordBufferBarrier(commandBuffer, irradianceDispatchArgsBuffer, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_INDIRECT_COMMAND_READ_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT);
        return true;
    }

    bool DispatchProbeIrradiance(VkCommandBuffer commandBuffer, VulkanFrameData::DDGI& ddgi, DDGIVolume& ddgiVolume, VulkanBuffer* probeStatesBuffer, VulkanDescriptorSet* staticDescriptorSet, VulkanDescriptorSet* rayQueryDescriptorSet, AllocatedImage* irradianceAtlas, uint32_t irradianceAtlasStorageImageIndex) {
        ProfilerVulkanZoneLightGreen("ComputeProbeIrradiance");

        VulkanPipeline* pipeline = VulkanResourceManager::GetPipeline("DDGIProbeIrradiance");
        if (!pipeline || !probeStatesBuffer || !staticDescriptorSet || !rayQueryDescriptorSet || !irradianceAtlas) return false;

        const uint32_t totalProbes = ddgiVolume.GetTotalProbeCount();
        if (totalProbes == 0) return false;

        VulkanBuffer* pointCloudBuffer = GetDDGIBufferIfReady(ddgiVolume.GetPointCloudSSBOName());
        VulkanBuffer* irradianceCounterBuffer = VulkanResourceManager::GetBuffer(ddgi.probeIrradianceCounter);
        VulkanBuffer* irradianceIndicesBuffer = VulkanResourceManager::GetBuffer(ddgi.probeIrradianceIndices);
        VulkanBuffer* irradianceDispatchArgsBuffer = VulkanResourceManager::GetBuffer(ddgi.probeIrradianceDispatchArgs);
        VulkanBuffer* probePointIndicesBuffer = GetDDGIBufferIfReady(ddgiVolume.GetProbePointIndicesSSBOName());
        VulkanBuffer* probePointOffsetsBuffer = GetDDGIBufferIfReady(ddgiVolume.GetProbePointOffsetsSSBOName());
        VulkanBuffer* probePointCountsBuffer = GetDDGIBufferIfReady(ddgiVolume.GetProbePointCountsSSBOName());
        if (!pointCloudBuffer || !irradianceCounterBuffer || !irradianceIndicesBuffer || !irradianceDispatchArgsBuffer || !probePointIndicesBuffer || !probePointOffsetsBuffer || !probePointCountsBuffer) return false;

        const DDGIVolumeGPU volume = ddgiVolume.GetGPUData();
        PushConstantsDDGIProbeIrradiance pushConstants{};
        pushConstants.pointCloudDeviceAddress = pointCloudBuffer->GetDeviceAddress();
        pushConstants.probeStatesDeviceAddress = probeStatesBuffer->GetDeviceAddress();
        pushConstants.probeIrradianceCounterDeviceAddress = irradianceCounterBuffer->GetDeviceAddress();
        pushConstants.probeIrradianceIndicesDeviceAddress = irradianceIndicesBuffer->GetDeviceAddress();
        pushConstants.probePointIndicesDeviceAddress = probePointIndicesBuffer->GetDeviceAddress();
        pushConstants.probePointOffsetsDeviceAddress = probePointOffsetsBuffer->GetDeviceAddress();
        pushConstants.probePointCountsDeviceAddress = probePointCountsBuffer->GetDeviceAddress();
        pushConstants.volumeOrigin = volume.origin;
        pushConstants.probeSpacing = volume.probeSpacing;
        pushConstants.probeCounts = volume.probeCounts;
        pushConstants.probeOffset = volume.probeOffset;
        pushConstants.totalProbes = totalProbes;
        pushConstants.pointCloudSpacing = ddgiVolume.GetPointCloudSpacing();
        pushConstants.irradianceAtlasStorageImageIndex = irradianceAtlasStorageImageIndex;
        if (pushConstants.pointCloudDeviceAddress == 0 || pushConstants.probeStatesDeviceAddress == 0 || pushConstants.probeIrradianceCounterDeviceAddress == 0 || pushConstants.probeIrradianceIndicesDeviceAddress == 0) return false;
        if (pushConstants.probePointIndicesDeviceAddress == 0 || pushConstants.probePointOffsetsDeviceAddress == 0 || pushConstants.probePointCountsDeviceAddress == 0) return false;

        VkDescriptorSet descriptorSets[] = {
            staticDescriptorSet->GetHandle(),
            rayQueryDescriptorSet->GetHandle()
        };

        irradianceAtlas->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->GetHandle());
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->GetLayout(), 0, 2, descriptorSets, 0, nullptr);
        vkCmdPushConstants(commandBuffer, pipeline->GetLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pushConstants), &pushConstants);
        vkCmdDispatchIndirect(commandBuffer, irradianceDispatchArgsBuffer->GetBuffer(), 0);

        RecordBufferBarrier(commandBuffer, probeStatesBuffer, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);
        irradianceAtlas->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        return true;
    }

    bool DispatchProbeIrradianceBorder(VkCommandBuffer commandBuffer, DDGIVolume& ddgiVolume, VulkanDescriptorSet* staticDescriptorSet, AllocatedImage* irradianceAtlas, uint32_t irradianceAtlasStorageImageIndex) {
        ProfilerVulkanZoneLightGreen("ComputeProbeIrradianceBorder");

        VulkanPipeline* pipeline = VulkanResourceManager::GetPipeline("DDGIProbeIrradianceBorder");
        if (!pipeline || !staticDescriptorSet || !irradianceAtlas) return false;

        const uint32_t totalProbes = ddgiVolume.GetTotalProbeCount();
        if (totalProbes == 0) return false;

        const DDGIVolumeGPU volume = ddgiVolume.GetGPUData();
        PushConstantsDDGIProbeBorder pushConstants{};
        pushConstants.volumeOrigin = volume.origin;
        pushConstants.probeSpacing = volume.probeSpacing;
        pushConstants.probeCounts = volume.probeCounts;
        pushConstants.totalProbes = totalProbes;
        pushConstants.probeOffset = volume.probeOffset;
        pushConstants.irradianceAtlasStorageImageIndex = irradianceAtlasStorageImageIndex;
        VkDescriptorSet descriptorSet = staticDescriptorSet->GetHandle();

        irradianceAtlas->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->GetHandle());
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->GetLayout(), 0, 1, &descriptorSet, 0, nullptr);
        vkCmdPushConstants(commandBuffer, pipeline->GetLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pushConstants), &pushConstants);
        vkCmdDispatch(commandBuffer, (totalProbes + 63) / 64, 1, 1);

        irradianceAtlas->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        return true;
    }

    bool DispatchProbeIrradianceTexture(
        VkCommandBuffer commandBuffer,
        DDGIVolume& ddgiVolume,
        VulkanBuffer* probeStatesBuffer,
        AllocatedImage* distanceAtlas,
        AllocatedImage* irradianceAtlas,
        uint32_t probeAtlasImageIndex) {
        ProfilerVulkanZoneLightGreen("ComputeIrradianceTexture");

        VulkanPipeline* pipeline = VulkanResourceManager::GetPipeline("DDGIProbeIrradianceTexture");
        VulkanDescriptorSet* staticDescriptorSet = VulkanResourceManager::GetDescriptorSet("StaticDescriptorSet");
        if (!pipeline || !staticDescriptorSet || !probeStatesBuffer) return false;

        AllocatedImage* normalImage = VulkanResourceManager::GetAllocatedImage("NormalXYRoughnessMisc");
        AllocatedImage* depthImage = VulkanResourceManager::GetAllocatedImage("Depth");
        AllocatedImage* indirectDiffuseImage = VulkanResourceManager::GetAllocatedImage("IndirectDiffuse");
        AllocatedImage* indirectDiffuseSurfaceImage = VulkanResourceManager::GetAllocatedImage("IndirectDiffuseSurface");
        if (!normalImage || !depthImage || !indirectDiffuseImage || !indirectDiffuseSurfaceImage) return false;

        if (!distanceAtlas || !irradianceAtlas || probeAtlasImageIndex == 0) return false;

        normalImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        depthImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        distanceAtlas->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        irradianceAtlas->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        indirectDiffuseImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        indirectDiffuseSurfaceImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);

        const DDGIVolumeGPU volume = ddgiVolume.GetGPUData();
        PushConstantsDDGIProbeIrradianceTexture pushConstants{};
        pushConstants.frameAddressTableDeviceAddress = GetFrameAddressTableDeviceAddress();
        pushConstants.probeStatesDeviceAddress = probeStatesBuffer->GetDeviceAddress();
        pushConstants.volumeOrigin = volume.origin;
        pushConstants.probeSpacing = volume.probeSpacing;
        pushConstants.probeCounts = volume.probeCounts;
        pushConstants.totalProbes = ddgiVolume.GetTotalProbeCount();
        pushConstants.worldBoundsMin = volume.worldBoundsMin;
        pushConstants.probeOffset = volume.probeOffset;
        pushConstants.worldBoundsMax = volume.worldBoundsMax;
        pushConstants.probeAtlasImageIndex = probeAtlasImageIndex;
        pushConstants.indirectDiffuseStorageImageIndex = VULKAN_STORAGE_IMAGE_IDX_INDIRECT_DIFFUSE;
        pushConstants.indirectDiffuseSurfaceStorageImageIndex = VULKAN_STORAGE_IMAGE_IDX_INDIRECT_DIFFUSE_SURFACE;
        if (pushConstants.frameAddressTableDeviceAddress == 0 || pushConstants.probeStatesDeviceAddress == 0) return false;

        VkDescriptorSet descriptorSet = staticDescriptorSet->GetHandle();
        const VkExtent2D extent = indirectDiffuseImage->GetExtent2D();

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->GetHandle());
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->GetLayout(), 0, 1, &descriptorSet, 0, nullptr);
        vkCmdPushConstants(commandBuffer, pipeline->GetLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pushConstants), &pushConstants);
        vkCmdDispatch(commandBuffer, (extent.width + TILE_SIZE - 1) / TILE_SIZE, (extent.height + TILE_SIZE - 1) / TILE_SIZE, 1);

        indirectDiffuseImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        indirectDiffuseSurfaceImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        return true;
    }

    bool VolumePointCloudBuffersMissing(DDGIVolume& ddgiVolume) {
        return !VulkanResourceManager::BufferExists(ddgiVolume.GetPointCloudSSBOName()) ||
               !VulkanResourceManager::BufferExists(ddgiVolume.GetPointCloudDirtyFlagsSSBOName()) ||
               !VulkanResourceManager::BufferExists(ddgiVolume.GetPointCloudTextureInfoSSBOName()) ||
               !VulkanResourceManager::BufferExists(ddgiVolume.GetPointCloudGridOffsetsSSBOName()) ||
               !VulkanResourceManager::BufferExists(ddgiVolume.GetPointCloudGridCountsSSBOName()) ||
               !VulkanResourceManager::BufferExists(ddgiVolume.GetProbePointIndicesSSBOName()) ||
               !VulkanResourceManager::BufferExists(ddgiVolume.GetProbePointOffsetsSSBOName()) ||
               !VulkanResourceManager::BufferExists(ddgiVolume.GetProbePointCountsSSBOName());
    }

    bool UpdateVolumePointCloud(VkCommandBuffer commandBuffer, VulkanFrameData::DDGI& ddgi, DDGIVolume& ddgiVolume) {
        if (ddgiVolume.GetPointCloudCount() == 0) {
            ddgiVolume.MarkPointCloudAsUploaded();
            return true;
        }

        VulkanBuffer* pointCloudBuffer = nullptr;
        VulkanBuffer* pointCloudTextureInfoBuffer = nullptr;
        VulkanBuffer* pointCloudDirtyFlagsBuffer = nullptr;
        bool forcePointCloudLighting = false;

        if (ddgiVolume.PointCloudNeedsGPUUpload() || VolumePointCloudBuffersMissing(ddgiVolume)) {
            forcePointCloudLighting = true;
            if (!UploadPointCloudBuffers(commandBuffer, ddgiVolume, pointCloudBuffer, pointCloudTextureInfoBuffer, pointCloudDirtyFlagsBuffer)) return false;
            if (!DispatchPointCloudBaseColor(commandBuffer, ddgiVolume, pointCloudBuffer, pointCloudTextureInfoBuffer)) return false;
            if (!DispatchProbePointIndices(commandBuffer, ddgi, ddgiVolume, pointCloudBuffer)) return false;
        }
        else {
            const std::string& pointCloudBufferName = ddgiVolume.GetPointCloudSSBOName();
            const std::string& pointCloudDirtyFlagsBufferName = ddgiVolume.GetPointCloudDirtyFlagsSSBOName();
            pointCloudBuffer = VulkanResourceManager::BufferExists(pointCloudBufferName) ? VulkanResourceManager::GetBuffer(pointCloudBufferName) : nullptr;
            pointCloudDirtyFlagsBuffer = VulkanResourceManager::BufferExists(pointCloudDirtyFlagsBufferName) ? VulkanResourceManager::GetBuffer(pointCloudDirtyFlagsBufferName) : nullptr;
        }

        if (!pointCloudBuffer || !pointCloudDirtyFlagsBuffer) return false;
        if (!DispatchPointCloudLighting(commandBuffer, ddgiVolume, pointCloudBuffer, pointCloudDirtyFlagsBuffer, forcePointCloudLighting)) return false;

        if (forcePointCloudLighting) {
            ddgiVolume.MarkPointCloudAsUploaded();
        }

        return true;
    }
}

void DDGIPointCloudPass(VkCommandBuffer commandBuffer) {
    g_updatedDDGIVolumeIds.clear();

    Hell::SlotMap<DDGIVolume>& ddgiVolumes = DDGIManager::GetVolumes();
    if (ddgiVolumes.empty()) return;
    if (!UploadProbeStateBuffer(commandBuffer)) return;

    VulkanFrameData::DDGI& ddgi = GetCurrentFrameData().ddgi;

    std::vector<uint64_t> volumeIds = DDGIManager::GetProbeUpdateVolumeIds();
    for (DDGIVolume& ddgiVolume : ddgiVolumes) {
        if (ddgiVolume.PointCloudNeedsGPUUpload() || VolumePointCloudBuffersMissing(ddgiVolume)) {
            const uint64_t volumeId = ddgiVolume.GetObjectId();
            if (std::find(volumeIds.begin(), volumeIds.end(), volumeId) == volumeIds.end()) {
                volumeIds.push_back(volumeId);
            }
        }
    }

    for (uint64_t volumeId : volumeIds) {
        DDGIVolume* ddgiVolume = DDGIManager::GetVolumeByObjectId(volumeId);
        if (!ddgiVolume) continue;
        if (UpdateVolumePointCloud(commandBuffer, ddgi, *ddgiVolume)) {
            if (std::find(g_updatedDDGIVolumeIds.begin(), g_updatedDDGIVolumeIds.end(), volumeId) == g_updatedDDGIVolumeIds.end()) {
                g_updatedDDGIVolumeIds.push_back(volumeId);
            }
        }
    }
}

void DDGIProbeUpdatePass(VkCommandBuffer commandBuffer) {
    if (g_updatedDDGIVolumeIds.empty()) return;

    VulkanBuffer* probeStatesBuffer = GetDDGIBufferIfReady(DDGI_PROBE_STATES_BUFFER_NAME);
    if (!probeStatesBuffer) return;

    VulkanFrameData::DDGI& ddgi = GetCurrentFrameData().ddgi;

    VulkanBuffer* dirtyDoorAABBsBuffer = VulkanResourceManager::GetBuffer(ddgi.dirtyDoorAABBs);
    if (!dirtyDoorAABBsBuffer) return;

    RecordBufferBarrier(commandBuffer, dirtyDoorAABBsBuffer, VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

    for (uint64_t volumeId : g_updatedDDGIVolumeIds) {
        DDGIVolume* ddgiVolume = DDGIManager::GetVolumeByObjectId(volumeId);
        if (!ddgiVolume) continue;

        VulkanBuffer* pointCloudDirtyFlagsBuffer = GetDDGIBufferIfReady(ddgiVolume->GetPointCloudDirtyFlagsSSBOName());
        if (!pointCloudDirtyFlagsBuffer) continue;

        AllocatedImage* distanceAtlas = nullptr;
        AllocatedImage* irradianceAtlas = nullptr;
        if (!CreateOrResizeProbeAtlases(commandBuffer, *ddgiVolume, distanceAtlas, irradianceAtlas)) continue;

        VulkanDescriptorSet* staticDescriptorSet = VulkanResourceManager::GetDescriptorSet("StaticDescriptorSet");
        VulkanDescriptorSet* rayQueryDescriptorSet = GetOrCreateRayQueryDescriptorSet(*ddgiVolume);
        if (!staticDescriptorSet || !rayQueryDescriptorSet) continue;

        uint32_t probeAtlasImageIndex = 0;
        if (!BindProbeAtlasImages(*ddgiVolume, distanceAtlas, irradianceAtlas, staticDescriptorSet, probeAtlasImageIndex)) continue;

        if (!DispatchProbeStateUpdate(commandBuffer, *ddgiVolume, probeStatesBuffer, dirtyDoorAABBsBuffer, ddgi.dirtyDoorAABBCount)) continue;
        if (!DispatchProbeRelevance(commandBuffer, *ddgiVolume, probeStatesBuffer)) continue;
        if (!DispatchProbeDistanceList(commandBuffer, ddgi, *ddgiVolume, probeStatesBuffer)) continue;
        if (!DispatchProbeDistanceDispatchArgs(commandBuffer, ddgi)) continue;
        if (!DispatchProbeDistance(commandBuffer, ddgi, *ddgiVolume, probeStatesBuffer, staticDescriptorSet, rayQueryDescriptorSet, distanceAtlas, probeAtlasImageIndex)) continue;
        if (!DispatchProbeDistanceBorder(commandBuffer, *ddgiVolume, staticDescriptorSet, distanceAtlas, probeAtlasImageIndex)) continue;
        if (!DispatchProbeIrradianceDirtyPointCheck(commandBuffer, *ddgiVolume, probeStatesBuffer, pointCloudDirtyFlagsBuffer)) continue;
        if (!DispatchProbeIrradianceList(commandBuffer, ddgi, *ddgiVolume, probeStatesBuffer)) continue;
        if (!DispatchProbeIrradianceDispatchArgs(commandBuffer, ddgi)) continue;
        if (!DispatchProbeIrradiance(commandBuffer, ddgi, *ddgiVolume, probeStatesBuffer, staticDescriptorSet, rayQueryDescriptorSet, irradianceAtlas, probeAtlasImageIndex)) continue;
        if (!DispatchProbeIrradianceBorder(commandBuffer, *ddgiVolume, staticDescriptorSet, irradianceAtlas, probeAtlasImageIndex)) continue;

        ddgiVolume->MarkProbesUpdated();
    }
}

void DDGIIrradianceTexturePass(VkCommandBuffer commandBuffer) {
    g_ddgiReflectionVolumeDataReady = false;

    AllocatedImage* indirectDiffuseImage = VulkanResourceManager::GetAllocatedImage("IndirectDiffuse");
    AllocatedImage* indirectDiffuseSurfaceImage = VulkanResourceManager::GetAllocatedImage("IndirectDiffuseSurface");
    if (!indirectDiffuseImage || !indirectDiffuseSurfaceImage) return;

    ClearColorImage(commandBuffer, indirectDiffuseImage);
    ClearColorImage(commandBuffer, indirectDiffuseSurfaceImage);

    Hell::SlotMap<DDGIVolume>& ddgiVolumes = DDGIManager::GetVolumes();
    if (ddgiVolumes.empty()) return;

    VulkanBuffer* probeStatesBuffer = GetDDGIBufferIfReady(DDGI_PROBE_STATES_BUFFER_NAME);
    if (!probeStatesBuffer) return;

    VulkanDescriptorSet* staticDescriptorSet = VulkanResourceManager::GetDescriptorSet("StaticDescriptorSet");
    if (!staticDescriptorSet) return;

    std::vector<DDGIReflectionVolumeGPU> reflectionVolumes;
    reflectionVolumes.reserve(ddgiVolumes.size());

    for (DDGIVolume& ddgiVolume : ddgiVolumes) {
        AllocatedImage* distanceAtlas = nullptr;
        AllocatedImage* irradianceAtlas = nullptr;
        if (!CreateOrResizeProbeAtlases(commandBuffer, ddgiVolume, distanceAtlas, irradianceAtlas)) continue;

        uint32_t probeAtlasImageIndex = 0;
        if (!BindProbeAtlasImages(ddgiVolume, distanceAtlas, irradianceAtlas, staticDescriptorSet, probeAtlasImageIndex)) continue;

        const AABB volumeBounds(ddgiVolume.GetBoundsMin(), ddgiVolume.GetBoundsMax());

        bool isVisible = false;
        for (uint32_t viewportIndex = 0; viewportIndex < MAX_VIEWPORT_COUNT; viewportIndex++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(viewportIndex);
            if (!viewport || !viewport->IsVisible()) continue;
            if (viewport->GetFrustum().IntersectsAABBFast(volumeBounds)) {
                isVisible = true;
                break;
            }
        }

        if (isVisible) {
            DispatchProbeIrradianceTexture(
                commandBuffer,
                ddgiVolume,
                probeStatesBuffer,
                distanceAtlas,
                irradianceAtlas,
                probeAtlasImageIndex);
        }

        distanceAtlas->Sync(commandBuffer, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);
        irradianceAtlas->Sync(commandBuffer, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);

        DDGIReflectionVolumeGPU reflectionVolume{};
        reflectionVolume.volume = ddgiVolume.GetGPUData();
        reflectionVolume.probeAtlasImageIndex = probeAtlasImageIndex;
        reflectionVolumes.push_back(reflectionVolume);
    }

    g_ddgiReflectionVolumeDataReady = UploadDDGIReflectionVolumeData(
        commandBuffer,
        probeStatesBuffer,
        reflectionVolumes);
}

uint64_t GetDDGIReflectionVolumeDataDeviceAddress() {
    const std::string bufferName = GetDDGIReflectionVolumeDataBufferName();
    if (!g_ddgiReflectionVolumeDataReady || !VulkanResourceManager::BufferExists(bufferName)) {
        return 0;
    }

    VulkanBuffer* buffer = VulkanResourceManager::GetBuffer(bufferName);
    return buffer ? buffer->GetDeviceAddress() : 0;
}

void CleanUpDDGIProbeAtlasBindlessImages() {
    g_probeInitializationTime = 0.0f;
    g_ddgiReflectionVolumeDataReady = false;
    g_probeAtlasBindlessImages.clear();
    g_probeAtlasResetVersions.clear();
    g_nextProbeAtlasStorageImageIndex = VULKAN_STORAGE_IMAGE_IDX_FIRST_DYNAMIC;
}

}
