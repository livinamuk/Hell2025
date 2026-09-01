#include "Unloved/Render/API/Vulkan/VK_renderer_internal.h"

#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/Render/API/Vulkan/Types/vk_allocated_image.h"
#include "Hell/Render/API/Vulkan/Types/vk_buffer.h"
#include "Hell/Render/API/Vulkan/Types/vk_descriptor_set.h"
#include "Hell/Render/API/Vulkan/Types/vk_pipeline.h"
#include "Unloved/Render/API/Vulkan/VK_descriptor_indices.h"

namespace VulkanRenderer {

    void HiZPass(VkCommandBuffer commandBuffer) {
        ProfilerVulkanZoneFunction();

        AllocatedImage* depthImage = VulkanResourceManager::GetAllocatedImage("Depth");
        AllocatedImage* hiZImage = VulkanResourceManager::GetAllocatedImage("HiZ");
        VulkanBuffer* atomicCounterBuffer = VulkanResourceManager::GetBuffer("HiZAtomicCounter");
        VulkanPipeline* pipeline = VulkanResourceManager::GetPipeline("HiZ");
        VulkanDescriptorSet* staticDescriptorSet = VulkanResourceManager::GetDescriptorSet("StaticDescriptorSet");

        if (!depthImage || !hiZImage || !atomicCounterBuffer || !pipeline || !staticDescriptorSet) return;
        if (pipeline->GetHandle() == VK_NULL_HANDLE || pipeline->GetLayout() == VK_NULL_HANDLE) return;
        if (hiZImage->GetMipLevelCount() == 0 || hiZImage->GetMipLevelCount() > VULKAN_STORAGE_IMAGE_R32F_HIZ_MIP_COUNT) return;

        const VkExtent2D depthExtent = depthImage->GetExtent2D();
        const VkExtent2D hierarchyExtent = hiZImage->GetExtent2D();
        if (depthExtent.width != hierarchyExtent.width || depthExtent.height != hierarchyExtent.height) return;

        depthImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        hiZImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);

        // SPD requires its global counter to be zero before the first run and
        // resets it after every successful run. Clearing here also recovers
        // safely if a prior dispatch was interrupted during renderer teardown.
        VkBufferMemoryBarrier2 counterToTransferBarrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2 };
        counterToTransferBarrier.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
        counterToTransferBarrier.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
        counterToTransferBarrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        counterToTransferBarrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        counterToTransferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        counterToTransferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        counterToTransferBarrier.buffer = atomicCounterBuffer->GetBuffer();
        counterToTransferBarrier.offset = 0;
        counterToTransferBarrier.size = sizeof(uint32_t);

        VkDependencyInfo counterToTransferDependency{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
        counterToTransferDependency.bufferMemoryBarrierCount = 1;
        counterToTransferDependency.pBufferMemoryBarriers = &counterToTransferBarrier;
        vkCmdPipelineBarrier2(commandBuffer, &counterToTransferDependency);

        vkCmdFillBuffer(commandBuffer, atomicCounterBuffer->GetBuffer(), 0, sizeof(uint32_t), 0u);

        VkBufferMemoryBarrier2 counterToComputeBarrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2 };
        counterToComputeBarrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        counterToComputeBarrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        counterToComputeBarrier.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
        counterToComputeBarrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
        counterToComputeBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        counterToComputeBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        counterToComputeBarrier.buffer = atomicCounterBuffer->GetBuffer();
        counterToComputeBarrier.offset = 0;
        counterToComputeBarrier.size = sizeof(uint32_t);

        VkDependencyInfo counterToComputeDependency{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
        counterToComputeDependency.bufferMemoryBarrierCount = 1;
        counterToComputeDependency.pBufferMemoryBarriers = &counterToComputeBarrier;
        vkCmdPipelineBarrier2(commandBuffer, &counterToComputeDependency);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->GetHandle());
        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_COMPUTE,
            pipeline->GetLayout(),
            0,
            1,
            staticDescriptorSet->GetHandlePtr(),
            0,
            nullptr);

        const uint32_t groupCountX = (depthExtent.width + 63u) / 64u;
        const uint32_t groupCountY = (depthExtent.height + 63u) / 64u;
        vkCmdDispatch(commandBuffer, groupCountX, groupCountY, 1);

        hiZImage->Sync(
            commandBuffer,
            VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);
    }

}
