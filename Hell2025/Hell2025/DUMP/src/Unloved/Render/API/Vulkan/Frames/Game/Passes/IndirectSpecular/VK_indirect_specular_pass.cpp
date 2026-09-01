#include "Unloved/Render/API/Vulkan/VK_renderer.h"
#include "Unloved/Render/API/Vulkan/VK_renderer_internal.h"

#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/Render/API/Vulkan/Types/vk_allocated_image.h"
#include "Hell/Render/API/Vulkan/Types/vk_buffer.h"
#include "Hell/Render/API/Vulkan/Types/vk_descriptor_set.h"
#include "Hell/Render/API/Vulkan/Types/vk_pipeline.h"
#include "Hell/ResourceManagement/ResourceManager.h"

#include "Unloved/Render/API/Vulkan/VK_descriptor_indices.h"
#include "Unloved/Render/API/Vulkan/VK_push_constants.h"
#include "Unloved/Render/Renderer_settings.h"
#include "Unloved/Render/RendererConstants.h"
#include <array>

namespace {
    bool g_indirectSpecularAMDHistoryValid = false;
    bool g_indirectSpecularAMDClassificationValid = false;
    bool g_indirectSpecularAMDReprojectValid = false;
    bool g_indirectSpecularAMDPrefilterValid = false;
    uint32_t g_indirectSpecularAMDPreviousSamplesPerQuad = 0;
    bool g_indirectSpecularAMDPreviousDDGIReflectionsEnabled = false;

    void ClearAMDColorImage(VkCommandBuffer commandBuffer, AllocatedImage* image) {
        image->Sync(commandBuffer, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT);

        VkClearColorValue clearValue{};
        VkImageSubresourceRange range{};
        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel = 0;
        range.levelCount = 1;
        range.baseArrayLayer = 0;
        range.layerCount = image->GetArrayLayerCount();
        vkCmdClearColorImage(commandBuffer, image->GetImage(), VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &range);

        image->Sync(commandBuffer, VK_ACCESS_2_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
    }

    void CopyAMDColorImage(VkCommandBuffer commandBuffer, AllocatedImage* source, AllocatedImage* destination) {
        source->Sync(commandBuffer, VK_ACCESS_2_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT);
        destination->Sync(commandBuffer, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT);

        VkImageCopy copy{};
        copy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copy.srcSubresource.layerCount = 1;
        copy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copy.dstSubresource.layerCount = 1;
        copy.extent = source->GetExtent();
        vkCmdCopyImage(commandBuffer, source->GetImage(), VK_IMAGE_LAYOUT_GENERAL, destination->GetImage(), VK_IMAGE_LAYOUT_GENERAL, 1, &copy);
    }

    void AMDRecordBufferBarrier(VkCommandBuffer commandBuffer, VulkanBuffer* buffer, VkAccessFlags srcAccess, VkAccessFlags dstAccess, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage) {
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
}

namespace VulkanRenderer {

    void ResetIndirectSpecularAMDHistory() {
        g_indirectSpecularAMDHistoryValid = false;
        g_indirectSpecularAMDClassificationValid = false;
        g_indirectSpecularAMDReprojectValid = false;
        g_indirectSpecularAMDPrefilterValid = false;
    }

    void IndirectSpecularClassifyTilesPass(VkCommandBuffer commandBuffer) {
        ProfilerVulkanZoneFunction();

        g_indirectSpecularAMDClassificationValid = false;

        AllocatedImage* materialRoughnessImage = VulkanResourceManager::GetAllocatedImage("IndirectSpecularAMDMaterialRoughness");
        AllocatedImage* extractedRoughnessImage = VulkanResourceManager::GetAllocatedImage("IndirectSpecularAMDExtractedRoughness");
        AllocatedImage* depthImage = VulkanResourceManager::GetAllocatedImage("Depth");
        VulkanBuffer* rayCounterBuffer = VulkanResourceManager::GetBuffer("IndirectSpecularAMDRayCounter");
        VulkanBuffer* tileListBuffer = VulkanResourceManager::GetBuffer("IndirectSpecularAMDDenoiserTileList");
        VulkanBuffer* indirectArgsBuffer = VulkanResourceManager::GetBuffer("IndirectSpecularAMDIndirectArgs");
        VulkanPipeline* classifyPipeline = VulkanResourceManager::GetPipeline("IndirectSpecularAMDClassifyTiles");
        VulkanPipeline* prepareArgsPipeline = VulkanResourceManager::GetPipeline("IndirectSpecularAMDPrepareIndirectArgs");
        VulkanDescriptorSet* staticDescriptorSet = VulkanResourceManager::GetDescriptorSet("StaticDescriptorSet");

        if (!materialRoughnessImage || !extractedRoughnessImage || !depthImage ||
            !rayCounterBuffer || !tileListBuffer || !indirectArgsBuffer ||
            !classifyPipeline || classifyPipeline->GetHandle() == VK_NULL_HANDLE || classifyPipeline->GetLayout() == VK_NULL_HANDLE ||
            !prepareArgsPipeline || prepareArgsPipeline->GetHandle() == VK_NULL_HANDLE || prepareArgsPipeline->GetLayout() == VK_NULL_HANDLE ||
            !staticDescriptorSet) {
            g_indirectSpecularAMDHistoryValid = false;
            return;
        }

        const VkExtent2D extent = depthImage->GetExtent2D();
        if (materialRoughnessImage->GetExtent2D().width != extent.width ||
            materialRoughnessImage->GetExtent2D().height != extent.height ||
            extractedRoughnessImage->GetExtent2D().width != extent.width ||
            extractedRoughnessImage->GetExtent2D().height != extent.height) {
            g_indirectSpecularAMDHistoryValid = false;
            return;
        }

        AMDRecordBufferBarrier(commandBuffer, rayCounterBuffer, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
        AMDRecordBufferBarrier(commandBuffer, tileListBuffer, VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        AMDRecordBufferBarrier(commandBuffer, indirectArgsBuffer, VK_ACCESS_INDIRECT_COMMAND_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

        vkCmdFillBuffer(commandBuffer, rayCounterBuffer->GetBuffer(), 0, sizeof(uint32_t) * 4, 0u);
        AMDRecordBufferBarrier(commandBuffer, rayCounterBuffer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

        materialRoughnessImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        depthImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        extractedRoughnessImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, classifyPipeline->GetHandle());
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, classifyPipeline->GetLayout(), 0, 1, staticDescriptorSet->GetHandlePtr(), 0, nullptr);
        vkCmdDispatch(commandBuffer, (extent.width + 7) / 8, (extent.height + 7) / 8, 1);

        AMDRecordBufferBarrier(commandBuffer, rayCounterBuffer, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        AMDRecordBufferBarrier(commandBuffer, tileListBuffer, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, prepareArgsPipeline->GetHandle());
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, prepareArgsPipeline->GetLayout(), 0, 1, staticDescriptorSet->GetHandlePtr(), 0, nullptr);
        vkCmdDispatch(commandBuffer, 1, 1, 1);

        AMDRecordBufferBarrier(commandBuffer, indirectArgsBuffer, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_INDIRECT_COMMAND_READ_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT);
        extractedRoughnessImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        g_indirectSpecularAMDClassificationValid = true;
    }

    void IndirectSpecularInputPass(VkCommandBuffer commandBuffer) {
        ProfilerVulkanZoneFunction();

        const VulkanFrameData& frameData = GetCurrentFrameData();

        AllocatedImage* rayInputImage = VulkanResourceManager::GetAllocatedImage("IndirectSpecularAMDRayInput");
        AllocatedImage* indirectSpecularImage = VulkanResourceManager::GetAllocatedImage("IndirectSpecularAMDInput");
        AllocatedImage* normalImage = VulkanResourceManager::GetAllocatedImage("NormalXYRoughnessMisc");
        AllocatedImage* materialRoughnessImage = VulkanResourceManager::GetAllocatedImage("IndirectSpecularAMDMaterialRoughness");
        AllocatedImage* extractedRoughnessImage = VulkanResourceManager::GetAllocatedImage("IndirectSpecularAMDExtractedRoughness");
        AllocatedImage* depthImage = VulkanResourceManager::GetAllocatedImage("Depth");
        VulkanPipeline* pipeline = VulkanResourceManager::GetPipeline("IndirectSpecularAMDInput");
        VulkanPipeline* resolvePipeline = VulkanResourceManager::GetPipeline("IndirectSpecularAMDResolveRayInput");
        VulkanRenderState* renderState = VulkanResourceManager::GetRenderState("IndirectSpecularAMDRayInput");
        VulkanDescriptorSet* staticDescriptorSet = VulkanResourceManager::GetDescriptorSet("StaticDescriptorSet");
        VulkanDescriptorSetResource* rayQueryDescriptorSetResource = VulkanResourceManager::GetDescriptorSetResource("RayQueryDescriptorSet");
        VulkanDescriptorSet* rayQueryDescriptorSet = rayQueryDescriptorSetResource ? &rayQueryDescriptorSetResource->GetSet(GetCurrentFrameIndex()) : nullptr;
        const uint64_t frameAddressTableDeviceAddress = GetFrameAddressTableDeviceAddress();
        VulkanBuffer* rayQueryBLASDataBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.rayQueryBLASData);
        VulkanBuffer* rayQuerySceneRenderItemIndexBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.rayQuerySceneRenderItemIndices);
        const int32_t blueNoiseTextureIndex = Hell::ResourceManager::GetTextureBindlessIndexByName("BlueNoiseRG", true);
        const RendererSettings& rendererSettings = Unloved::Renderer::GetCurrentRendererSettings();
        const uint64_t ddgiReflectionVolumeDataDeviceAddress = GetDDGIReflectionVolumeDataDeviceAddress();
        const bool ddgiReflectionsEnabled = rendererSettings.enableDDGI && rendererSettings.enableDDGIReflections && ddgiReflectionVolumeDataDeviceAddress != 0;

        if (!pipeline) return;
        if (!resolvePipeline || resolvePipeline->GetHandle() == VK_NULL_HANDLE || resolvePipeline->GetLayout() == VK_NULL_HANDLE) return;
        if (!renderState) return;
        if (!staticDescriptorSet) return;
        if (!rayQueryDescriptorSet) return;
        if (!rayInputImage) return;
        if (!indirectSpecularImage) return;
        if (!normalImage) return;
        if (!materialRoughnessImage) return;
        if (!extractedRoughnessImage || !g_indirectSpecularAMDClassificationValid) return;
        if (!depthImage) return;
        if (frameAddressTableDeviceAddress == 0) return;
        if (!rayQueryBLASDataBuffer) return;
        if (!rayQuerySceneRenderItemIndexBuffer) return;
        if (blueNoiseTextureIndex < 0) return;

        VkExtent2D extent = indirectSpecularImage->GetExtent2D();

        const auto hasExtent = [extent](AllocatedImage* image) {
            const VkExtent2D imageExtent = image->GetExtent2D();
            return imageExtent.width == extent.width && imageExtent.height == extent.height;
        };

        if (!hasExtent(rayInputImage) || !hasExtent(normalImage) ||
            !hasExtent(materialRoughnessImage) ||
            !hasExtent(extractedRoughnessImage) || !hasExtent(depthImage)) {
            g_indirectSpecularAMDHistoryValid = false;
            return;
        }

        const uint32_t samplesPerQuad = Unloved::Renderer::GetIndirectSpecularRaysPerQuad();

        if (samplesPerQuad != g_indirectSpecularAMDPreviousSamplesPerQuad) {
            g_indirectSpecularAMDHistoryValid = false;
            g_indirectSpecularAMDPreviousSamplesPerQuad = samplesPerQuad;
        }

        if (ddgiReflectionsEnabled != g_indirectSpecularAMDPreviousDDGIReflectionsEnabled) {
            g_indirectSpecularAMDHistoryValid = false;
            g_indirectSpecularAMDPreviousDDGIReflectionsEnabled = ddgiReflectionsEnabled;
        }

        normalImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);
        materialRoughnessImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);
        depthImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);

        if (!BeginRenderState(commandBuffer, *renderState, extent)) return;

        VkViewport viewport{};
        viewport.width = static_cast<float>(extent.width);
        viewport.height = static_cast<float>(extent.height);
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.extent = extent;
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetHandle());
        VkDescriptorSet descriptorSets[] = { staticDescriptorSet->GetHandle(), rayQueryDescriptorSet->GetHandle() };
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetLayout(), 0, 2, descriptorSets, 0, nullptr);

        PushConstantsIndirectSpecularAMDInput pushConstants{};
        pushConstants.frameAddressTableDeviceAddress = frameAddressTableDeviceAddress;
        pushConstants.rayQueryBLASDataDeviceAddress = rayQueryBLASDataBuffer->GetDeviceAddress();
        pushConstants.rayQuerySceneRenderItemIndicesDeviceAddress = rayQuerySceneRenderItemIndexBuffer->GetDeviceAddress();
        pushConstants.blueNoiseTextureIndex = blueNoiseTextureIndex;
        pushConstants.frameIndex = g_frameIndex;
        pushConstants.samplesPerQuad = samplesPerQuad;
        pushConstants.ddgiReflectionVolumeDataDeviceAddress = ddgiReflectionVolumeDataDeviceAddress;
        pushConstants.enableDDGIReflections = ddgiReflectionsEnabled ? 1u : 0u;
        vkCmdPushConstants(commandBuffer, pipeline->GetLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstants), &pushConstants);

        vkCmdDraw(commandBuffer, 3, 1, 0, 0);

        EndRenderState(commandBuffer);

        rayInputImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        extractedRoughnessImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        indirectSpecularImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, resolvePipeline->GetHandle());
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, resolvePipeline->GetLayout(), 0, 1, staticDescriptorSet->GetHandlePtr(), 0, nullptr);
        vkCmdDispatch(commandBuffer, (extent.width + 7) / 8, (extent.height + 7) / 8, 1);

        indirectSpecularImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);
    }

    void IndirectSpecularReprojectPass(VkCommandBuffer commandBuffer) {
        ProfilerVulkanZoneFunction();

        g_indirectSpecularAMDReprojectValid = false;
        g_indirectSpecularAMDPrefilterValid = false;

        AllocatedImage* rawImage = VulkanResourceManager::GetAllocatedImage("IndirectSpecularAMDInput");
        AllocatedImage* reprojectedImage = VulkanResourceManager::GetAllocatedImage("IndirectSpecularAMDReprojected");
        AllocatedImage* averageImage = VulkanResourceManager::GetAllocatedImage("IndirectSpecularAMDAverage");
        AllocatedImage* averageHistoryImage = VulkanResourceManager::GetAllocatedImage("IndirectSpecularAMDAverageHistory");
        AllocatedImage* temporalImage = VulkanResourceManager::GetAllocatedImage("IndirectSpecularAMDTemporal");
        AllocatedImage* radianceHistoryImage = VulkanResourceManager::GetAllocatedImage("IndirectSpecularAMDHistory");
        AllocatedImage* normalHistoryImage = VulkanResourceManager::GetAllocatedImage("IndirectSpecularAMDNormalHistory");
        AllocatedImage* roughnessHistoryImage = VulkanResourceManager::GetAllocatedImage("IndirectSpecularAMDRoughnessHistory");
        AllocatedImage* depthHistoryImage = VulkanResourceManager::GetAllocatedImage("IndirectSpecularAMDDepthHistory");
        AllocatedImage* varianceImage = VulkanResourceManager::GetAllocatedImage("IndirectSpecularAMDVariance");
        AllocatedImage* varianceHistoryImage = VulkanResourceManager::GetAllocatedImage("IndirectSpecularAMDVarianceHistory");
        AllocatedImage* sampleCountImage = VulkanResourceManager::GetAllocatedImage("IndirectSpecularAMDSampleCount");
        AllocatedImage* sampleCountHistoryImage = VulkanResourceManager::GetAllocatedImage("IndirectSpecularAMDSampleCountHistory");
        AllocatedImage* roughnessImage = VulkanResourceManager::GetAllocatedImage("IndirectSpecularAMDExtractedRoughness");
        AllocatedImage* normalImage = VulkanResourceManager::GetAllocatedImage("NormalXYRoughnessMisc");
        AllocatedImage* depthImage = VulkanResourceManager::GetAllocatedImage("Depth");
        AllocatedImage* velocityImage = VulkanResourceManager::GetAllocatedImage("VelocityXYOcclusionSubSurface");
        VulkanBuffer* indirectArgsBuffer = VulkanResourceManager::GetBuffer("IndirectSpecularAMDIndirectArgs");
        VulkanPipeline* reprojectPipeline = VulkanResourceManager::GetPipeline("IndirectSpecularAMDReproject");
        VulkanDescriptorSet* staticDescriptorSet = VulkanResourceManager::GetDescriptorSet("StaticDescriptorSet");
        const uint64_t frameAddressTableDeviceAddress = GetFrameAddressTableDeviceAddress();

        // Keep a valid current-frame fallback until Resolve Temporal replaces it.
        if (rawImage && temporalImage) {
            const VkExtent2D rawExtent = rawImage->GetExtent2D();
            const VkExtent2D temporalExtent = temporalImage->GetExtent2D();
            if (rawExtent.width == temporalExtent.width && rawExtent.height == temporalExtent.height) {
                BlitImage(commandBuffer, "IndirectSpecularAMDInput", "IndirectSpecularAMDTemporal", VK_FILTER_NEAREST);
            }
        }

        if (temporalImage) temporalImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);

        if (!rawImage || !g_indirectSpecularAMDClassificationValid) {
            g_indirectSpecularAMDHistoryValid = false;
            return;
        }

        const VkExtent2D extent = rawImage->GetExtent2D();
        const VkExtent2D expectedAverageExtent = { (extent.width + 7) / 8, (extent.height + 7) / 8 };
        const auto hasExtent = [](AllocatedImage* image, VkExtent2D expected) {
            if (!image) return false;
            const VkExtent2D actual = image->GetExtent2D();
            return actual.width == expected.width && actual.height == expected.height;
        };

        const std::array<AllocatedImage*, 13> renderResolutionImages = {
            reprojectedImage,
            radianceHistoryImage,
            normalHistoryImage,
            roughnessHistoryImage,
            depthHistoryImage,
            varianceImage,
            varianceHistoryImage,
            sampleCountImage,
            sampleCountHistoryImage,
            temporalImage,
            roughnessImage,
            normalImage,
            velocityImage
        };

        for (AllocatedImage* image : renderResolutionImages) {
            if (!hasExtent(image, extent)) {
                g_indirectSpecularAMDHistoryValid = false;
                return;
            }
        }

        if (!hasExtent(averageImage, expectedAverageExtent) ||
            !hasExtent(averageHistoryImage, expectedAverageExtent) ||
            !depthImage || !hasExtent(depthImage, extent) || !indirectArgsBuffer ||
            !reprojectPipeline || reprojectPipeline->GetHandle() == VK_NULL_HANDLE || reprojectPipeline->GetLayout() == VK_NULL_HANDLE ||
            !staticDescriptorSet || frameAddressTableDeviceAddress == 0) {
            g_indirectSpecularAMDHistoryValid = false;
            return;
        }

        rawImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        roughnessImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        normalImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        depthImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        velocityImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);

        if (!g_indirectSpecularAMDHistoryValid) {
            // Match the SDK reset set. Geometry histories are intentionally not cleared
            // AMD initializes them by copying after the first Resolve Temporal dispatch
            ClearAMDColorImage(commandBuffer, radianceHistoryImage);
            ClearAMDColorImage(commandBuffer, varianceHistoryImage);
            ClearAMDColorImage(commandBuffer, sampleCountHistoryImage);
            ClearAMDColorImage(commandBuffer, reprojectedImage);
            ClearAMDColorImage(commandBuffer, averageImage);
            ClearAMDColorImage(commandBuffer, averageHistoryImage);
            ClearAMDColorImage(commandBuffer, varianceImage);
            ClearAMDColorImage(commandBuffer, sampleCountImage);
        }
        radianceHistoryImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        normalHistoryImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        roughnessHistoryImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        depthHistoryImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        varianceHistoryImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        sampleCountHistoryImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        averageHistoryImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        reprojectedImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        averageImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        varianceImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        sampleCountImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, reprojectPipeline->GetHandle());
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, reprojectPipeline->GetLayout(), 0, 1, staticDescriptorSet->GetHandlePtr(), 0, nullptr);

        PushConstantsIndirectSpecularAMDReproject pushConstants{};
        pushConstants.frameAddressTableDeviceAddress = frameAddressTableDeviceAddress;
        vkCmdPushConstants(commandBuffer, reprojectPipeline->GetLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pushConstants), &pushConstants);
        vkCmdDispatchIndirect(commandBuffer, indirectArgsBuffer->GetBuffer(), sizeof(uint32_t) * 3);

        g_indirectSpecularAMDReprojectValid = true;

        constexpr VkPipelineStageFlags2 consumerStages = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
        reprojectedImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, consumerStages);
        averageImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, consumerStages);
        varianceImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, consumerStages);
        sampleCountImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, consumerStages);
    }

    void IndirectSpecularPrefilterPass(VkCommandBuffer commandBuffer) {
        ProfilerVulkanZoneFunction();

        g_indirectSpecularAMDPrefilterValid = false;

        AllocatedImage* rawImage = VulkanResourceManager::GetAllocatedImage("IndirectSpecularAMDInput");
        AllocatedImage* averageHistoryImage = VulkanResourceManager::GetAllocatedImage("IndirectSpecularAMDAverageHistory");
        AllocatedImage* varianceImage = VulkanResourceManager::GetAllocatedImage("IndirectSpecularAMDVariance");
        AllocatedImage* varianceHistoryImage = VulkanResourceManager::GetAllocatedImage("IndirectSpecularAMDVarianceHistory");
        AllocatedImage* filteredImage = VulkanResourceManager::GetAllocatedImage("IndirectSpecularAMDFiltered");
        AllocatedImage* radianceHistoryImage = VulkanResourceManager::GetAllocatedImage("IndirectSpecularAMDHistory");
        AllocatedImage* prefilteredVarianceImage = VulkanResourceManager::GetAllocatedImage("IndirectSpecularAMDPrefilteredVariance");
        AllocatedImage* normalImage = VulkanResourceManager::GetAllocatedImage("NormalXYRoughnessMisc");
        AllocatedImage* roughnessImage = VulkanResourceManager::GetAllocatedImage("IndirectSpecularAMDExtractedRoughness");
        AllocatedImage* depthImage = VulkanResourceManager::GetAllocatedImage("Depth");
        VulkanBuffer* indirectArgsBuffer = VulkanResourceManager::GetBuffer("IndirectSpecularAMDIndirectArgs");
        VulkanPipeline* pipeline = VulkanResourceManager::GetPipeline("IndirectSpecularAMDPrefilter");
        VulkanDescriptorSet* staticDescriptorSet = VulkanResourceManager::GetDescriptorSet("StaticDescriptorSet");
        const uint64_t frameAddressTableDeviceAddress = GetFrameAddressTableDeviceAddress();

        if (!g_indirectSpecularAMDReprojectValid ||
            !rawImage || !averageHistoryImage || !varianceImage || !varianceHistoryImage ||
            !filteredImage || !radianceHistoryImage || !prefilteredVarianceImage ||
            !normalImage || !roughnessImage || !depthImage || !indirectArgsBuffer ||
            !pipeline || pipeline->GetHandle() == VK_NULL_HANDLE || pipeline->GetLayout() == VK_NULL_HANDLE ||
            !staticDescriptorSet || frameAddressTableDeviceAddress == 0) {
            return;
        }

        const VkExtent2D extent = rawImage->GetExtent2D();
        const VkExtent2D expectedAverageExtent = { (extent.width + 7) / 8, (extent.height + 7) / 8 };
        const auto hasExtent = [](AllocatedImage* image, VkExtent2D expected) {
            const VkExtent2D actual = image->GetExtent2D();
            return actual.width == expected.width && actual.height == expected.height;
        };
        if (!hasExtent(varianceImage, extent) ||
            !hasExtent(varianceHistoryImage, extent) ||
            !hasExtent(filteredImage, extent) ||
            !hasExtent(radianceHistoryImage, extent) ||
            !hasExtent(prefilteredVarianceImage, extent) ||
            !hasExtent(averageHistoryImage, expectedAverageExtent)) {
            return;
        }

        if (!hasExtent(normalImage, extent) ||
            !hasExtent(roughnessImage, extent) ||
            !hasExtent(depthImage, extent)) {
            return;
        }

        // AMD Prefilter writes into the previous radiance ping-pong image.
        // Its compact dispatch leaves skipped tiles holding resolved history,
        // which Resolve Temporal can read through its four-pixel neighborhood halo

        CopyAMDColorImage(commandBuffer, radianceHistoryImage, filteredImage);

        // AMD writes Prefilter variance into the previous variance side
        // Seed the separate adapter target so skipped tiles retain that exact side

        CopyAMDColorImage(commandBuffer, varianceHistoryImage, prefilteredVarianceImage);

        rawImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        averageHistoryImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        varianceImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        normalImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        roughnessImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        depthImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        filteredImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        prefilteredVarianceImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->GetHandle());
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->GetLayout(), 0, 1, staticDescriptorSet->GetHandlePtr(), 0, nullptr);

        PushConstantsIndirectSpecularAMDPrefilter pushConstants{};
        pushConstants.frameAddressTableDeviceAddress = frameAddressTableDeviceAddress;
        vkCmdPushConstants(commandBuffer, pipeline->GetLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pushConstants), &pushConstants);
        vkCmdDispatchIndirect(commandBuffer, indirectArgsBuffer->GetBuffer(), sizeof(uint32_t) * 3);

        constexpr VkPipelineStageFlags2 consumerStages = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
        filteredImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, consumerStages);
        prefilteredVarianceImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, consumerStages);
        g_indirectSpecularAMDPrefilterValid = true;
    }

    void IndirectSpecularResolveTemporalPass(VkCommandBuffer commandBuffer) {
        ProfilerVulkanZoneFunction();

        AllocatedImage* filteredImage = VulkanResourceManager::GetAllocatedImage("IndirectSpecularAMDFiltered");
        AllocatedImage* prefilteredVarianceImage = VulkanResourceManager::GetAllocatedImage("IndirectSpecularAMDPrefilteredVariance");
        AllocatedImage* reprojectedImage = VulkanResourceManager::GetAllocatedImage("IndirectSpecularAMDReprojected");
        AllocatedImage* averageImage = VulkanResourceManager::GetAllocatedImage("IndirectSpecularAMDAverage");
        AllocatedImage* averageHistoryImage = VulkanResourceManager::GetAllocatedImage("IndirectSpecularAMDAverageHistory");
        AllocatedImage* sampleCountImage = VulkanResourceManager::GetAllocatedImage("IndirectSpecularAMDSampleCount");
        AllocatedImage* normalImage = VulkanResourceManager::GetAllocatedImage("NormalXYRoughnessMisc");
        AllocatedImage* roughnessImage = VulkanResourceManager::GetAllocatedImage("IndirectSpecularAMDExtractedRoughness");
        AllocatedImage* temporalImage = VulkanResourceManager::GetAllocatedImage("IndirectSpecularAMDTemporal");
        AllocatedImage* varianceImage = VulkanResourceManager::GetAllocatedImage("IndirectSpecularAMDVariance");
        AllocatedImage* radianceHistoryImage = VulkanResourceManager::GetAllocatedImage("IndirectSpecularAMDHistory");
        AllocatedImage* varianceHistoryImage = VulkanResourceManager::GetAllocatedImage("IndirectSpecularAMDVarianceHistory");
        AllocatedImage* sampleCountHistoryImage = VulkanResourceManager::GetAllocatedImage("IndirectSpecularAMDSampleCountHistory");
        AllocatedImage* normalHistoryImage = VulkanResourceManager::GetAllocatedImage("IndirectSpecularAMDNormalHistory");
        AllocatedImage* roughnessHistoryImage = VulkanResourceManager::GetAllocatedImage("IndirectSpecularAMDRoughnessHistory");
        AllocatedImage* depthImage = VulkanResourceManager::GetAllocatedImage("Depth");
        AllocatedImage* depthHistoryImage = VulkanResourceManager::GetAllocatedImage("IndirectSpecularAMDDepthHistory");
        VulkanBuffer* tileListBuffer = VulkanResourceManager::GetBuffer("IndirectSpecularAMDDenoiserTileList");
        VulkanBuffer* indirectArgsBuffer = VulkanResourceManager::GetBuffer("IndirectSpecularAMDIndirectArgs");
        VulkanPipeline* pipeline = VulkanResourceManager::GetPipeline("IndirectSpecularAMDResolveTemporal");
        VulkanPipeline* storeHistoryPipeline = VulkanResourceManager::GetPipeline("IndirectSpecularAMDStoreHistory");
        VulkanDescriptorSet* staticDescriptorSet = VulkanResourceManager::GetDescriptorSet("StaticDescriptorSet");

        if (!g_indirectSpecularAMDReprojectValid || !g_indirectSpecularAMDPrefilterValid ||
            !filteredImage || !prefilteredVarianceImage || !reprojectedImage || !averageImage || !averageHistoryImage ||
            !sampleCountImage || !normalImage || !roughnessImage || !temporalImage || !varianceImage ||
            !radianceHistoryImage || !varianceHistoryImage || !sampleCountHistoryImage ||
            !normalHistoryImage || !roughnessHistoryImage || !depthImage || !depthHistoryImage ||
            !tileListBuffer || !indirectArgsBuffer ||
            !pipeline || pipeline->GetHandle() == VK_NULL_HANDLE || pipeline->GetLayout() == VK_NULL_HANDLE ||
            !storeHistoryPipeline || storeHistoryPipeline->GetHandle() == VK_NULL_HANDLE || storeHistoryPipeline->GetLayout() == VK_NULL_HANDLE ||
            !staticDescriptorSet) {
            g_indirectSpecularAMDHistoryValid = false;
            return;
        }

        const VkExtent2D extent = filteredImage->GetExtent2D();
        const VkExtent2D expectedAverageExtent = { (extent.width + 7) / 8, (extent.height + 7) / 8 };
        const auto hasExtent = [](AllocatedImage* image, VkExtent2D expected) {
            const VkExtent2D actual = image->GetExtent2D();
            return actual.width == expected.width && actual.height == expected.height;
        };

        const std::array<AllocatedImage*, 14> renderResolutionImages = {
            prefilteredVarianceImage,
            reprojectedImage,
            sampleCountImage,
            normalImage,
            roughnessImage,
            temporalImage,
            varianceImage,
            radianceHistoryImage,
            varianceHistoryImage,
            sampleCountHistoryImage,
            normalHistoryImage,
            roughnessHistoryImage,
            depthImage,
            depthHistoryImage
        };

        for (AllocatedImage* image : renderResolutionImages) {
            if (!hasExtent(image, extent)) {
                g_indirectSpecularAMDHistoryValid = false;
                return;
            }
        }

        if (!hasExtent(averageImage, expectedAverageExtent) ||
            !hasExtent(averageHistoryImage, expectedAverageExtent)) {
            g_indirectSpecularAMDHistoryValid = false;
            return;
        }

        filteredImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        prefilteredVarianceImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        reprojectedImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        averageHistoryImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        sampleCountHistoryImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        normalImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        roughnessImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        depthImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        temporalImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        varianceImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->GetHandle());
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->GetLayout(), 0, 1, staticDescriptorSet->GetHandlePtr(), 0, nullptr);
        vkCmdDispatchIndirect(commandBuffer, indirectArgsBuffer->GetBuffer(), sizeof(uint32_t) * 3);

        // Publish radiance and roughness history only after Resolve Temporal has
        // finished consuming this frame's previous-side bindings

        CopyAMDColorImage(commandBuffer, temporalImage, radianceHistoryImage);
        CopyAMDColorImage(commandBuffer, roughnessImage, roughnessHistoryImage);

        normalImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        depthImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        normalHistoryImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        depthHistoryImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        constexpr VkAccessFlags2 pingPongAccess = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
        varianceImage->Sync(commandBuffer, pingPongAccess, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        varianceHistoryImage->Sync(commandBuffer, pingPongAccess, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        prefilteredVarianceImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        sampleCountImage->Sync(commandBuffer, pingPongAccess, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        sampleCountHistoryImage->Sync(commandBuffer, pingPongAccess, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        averageImage->Sync(commandBuffer, pingPongAccess, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        averageHistoryImage->Sync(commandBuffer, pingPongAccess, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, storeHistoryPipeline->GetHandle());
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, storeHistoryPipeline->GetLayout(), 0, 1, staticDescriptorSet->GetHandlePtr(), 0, nullptr);
        vkCmdDispatch(commandBuffer, (extent.width + 7) / 8, (extent.height + 7) / 8, 1);
        g_indirectSpecularAMDHistoryValid = true;

        temporalImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);
        varianceHistoryImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);
        prefilteredVarianceImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);
        sampleCountHistoryImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);
    }

}
