#include "Unloved/Render/API/Vulkan/VK_renderer_internal.h"
#include "Unloved/Render/RendererConstants.h"

#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/Render/API/Vulkan/Types/vk_timer.h"
#include "Hell/Render/API/Vulkan/Types/vk_allocated_image.h"
#include "Hell/Render/API/Vulkan/Types/vk_descriptor_set.h"
#include "Hell/Render/API/Vulkan/Types/vk_pipeline.h"

namespace VulkanRenderer {

    void ComputeTileWorldBounds(VkCommandBuffer commandBuffer) {
        ProfilerVulkanZoneFunction();

        AllocatedImage* depthImage = VulkanResourceManager::GetAllocatedImage("Depth");
        AllocatedImage* lightingImage = VulkanResourceManager::GetAllocatedImage("Lighting");
        VulkanPipeline* pipeline = VulkanResourceManager::GetPipeline("TileWorldBounds");
        VulkanDescriptorSet* staticDescriptorSet = VulkanResourceManager::GetDescriptorSet("StaticDescriptorSet");
        VulkanBuffer* tileWorldBoundsBuffer = VulkanResourceManager::GetBuffer(GetCurrentFrameData().buffers.tileWorldBounds);

        if (!depthImage) return;
        if (!lightingImage) return;
        if (!pipeline) return;
        if (!staticDescriptorSet) return;
        if (!tileWorldBoundsBuffer) return;

        VkExtent2D extent = depthImage->GetExtent2D();
        depthImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        lightingImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->GetHandle());
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->GetLayout(), 0, 1, staticDescriptorSet->GetHandlePtr(), 0, nullptr);

        uint32_t groupCountX = (extent.width + TILE_SIZE - 1) / TILE_SIZE;
        uint32_t groupCountY = (extent.height + TILE_SIZE - 1) / TILE_SIZE;

        PushConstantsTileWorldBounds pushConstants{};
        pushConstants.frameAddressTableDeviceAddress = GetFrameAddressTableDeviceAddress();
        pushConstants.tileXCount = static_cast<int32_t>(groupCountX);
        pushConstants.tileYCount = static_cast<int32_t>(groupCountY);
        vkCmdPushConstants(commandBuffer, pipeline->GetLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pushConstants), &pushConstants);

        vkCmdDispatch(commandBuffer, groupCountX, groupCountY, 1);

        VkBufferMemoryBarrier barrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
        barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.buffer = tileWorldBoundsBuffer->GetBuffer();
        barrier.offset = 0;
        barrier.size = tileWorldBoundsBuffer->GetSize();

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);
    }

    void LightCullingPass(VkCommandBuffer commandBuffer) {
        ProfilerVulkanZoneFunction();

        AllocatedImage* depthImage = VulkanResourceManager::GetAllocatedImage("Depth");
        AllocatedImage* lightingImage = VulkanResourceManager::GetAllocatedImage("Lighting");
        VulkanPipeline* pipeline = VulkanResourceManager::GetPipeline("TileLightCulling");
        VulkanDescriptorSet* staticDescriptorSet = VulkanResourceManager::GetDescriptorSet("StaticDescriptorSet");
        VulkanBuffer* tileLightsBuffer = VulkanResourceManager::GetBuffer(GetCurrentFrameData().buffers.tileLights);

        if (!depthImage) return;
        if (!lightingImage) return;
        if (!pipeline) return;
        if (!staticDescriptorSet) return;
        if (!tileLightsBuffer) return;

        VkExtent2D extent = depthImage->GetExtent2D();
        depthImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        lightingImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->GetHandle());
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->GetLayout(), 0, 1, staticDescriptorSet->GetHandlePtr(), 0, nullptr);

        uint32_t groupCountX = (extent.width + TILE_SIZE - 1) / TILE_SIZE;
        uint32_t groupCountY = (extent.height + TILE_SIZE - 1) / TILE_SIZE;

        PushConstantsTileLightCulling pushConstants{};
        pushConstants.frameAddressTableDeviceAddress = GetFrameAddressTableDeviceAddress();

        vkCmdPushConstants(commandBuffer, pipeline->GetLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pushConstants), &pushConstants);

        vkCmdDispatch(commandBuffer, groupCountX, groupCountY, 1);

        VkBufferMemoryBarrier barrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
        barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.buffer = tileLightsBuffer->GetBuffer();
        barrier.offset = 0;
        barrier.size = tileLightsBuffer->GetSize();

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);
    }
}
