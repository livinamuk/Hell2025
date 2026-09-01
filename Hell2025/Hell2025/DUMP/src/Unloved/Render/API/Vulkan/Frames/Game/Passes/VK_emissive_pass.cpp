#include "Unloved/Render/API/Vulkan/VK_renderer.h"
#include "Unloved/Render/API/Vulkan/VK_renderer_internal.h"

#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/Render/API/Vulkan/Types/vk_allocated_image.h"
#include "Hell/Render/API/Vulkan/Types/vk_descriptor_set.h"
#include "Hell/Render/API/Vulkan/Types/vk_mesh_buffer.h"
#include "Hell/Render/API/Vulkan/Types/vk_pipeline.h"
#include "Unloved/Render/API/Vulkan/VK_descriptor_indices.h"
#include "Unloved/Render/API/Vulkan/VK_draw.h"
#include "Unloved/Render/API/Vulkan/VK_push_constants.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Viewport/ViewportManager.h"

#include <array>

using namespace Unloved;

namespace {
    constexpr uint32_t BLOOM_MIP_COUNT = 3;

    glm::ivec2 GetHalfExtent(const glm::ivec2& extent) {
        return glm::max((extent + glm::ivec2(1)) / 2, glm::ivec2(1));
    }

    void DispatchEmissiveBloomFilter(VkCommandBuffer commandBuffer, VulkanPipeline& pipeline, VulkanDescriptorSet& descriptorSet, AllocatedImage& sourceImage, AllocatedImage& outputImage, uint32_t sourceTextureIndex, uint32_t outputImageIndex, int32_t sourceMip, const glm::ivec2& sourceOffset, const glm::ivec2& sourceExtent, const glm::ivec2& outputExtent, const glm::ivec2& direction, float filterScale) {
        sourceImage.Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        outputImage.Sync(commandBuffer, VK_ACCESS_2_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.GetHandle());
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.GetLayout(), 0, 1, descriptorSet.GetHandlePtr(), 0, nullptr);

        PushConstantsEmissiveBloomFilter pushConstants{};
        pushConstants.sourceOffset = sourceOffset;
        pushConstants.sourceExtent = sourceExtent;
        pushConstants.outputExtent = outputExtent;
        pushConstants.direction = direction;
        pushConstants.sourceMip = sourceMip;
        pushConstants.filterScale = filterScale;
        pushConstants.sourceTextureIndex = sourceTextureIndex;
        pushConstants.outputImageIndex = outputImageIndex;
        vkCmdPushConstants(commandBuffer, pipeline.GetLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pushConstants), &pushConstants);

        vkCmdDispatch(commandBuffer, static_cast<uint32_t>((outputExtent.x + 15) / 16), static_cast<uint32_t>((outputExtent.y + 7) / 8), 1);
    }

    void DispatchEmissiveBloomComposite(VkCommandBuffer commandBuffer, VulkanPipeline& pipeline, VulkanDescriptorSet& descriptorSet, AllocatedImage& emissiveImage, AllocatedImage& bloomImage, AllocatedImage& lightingImage, const glm::ivec2& viewportOffset, const glm::ivec2& viewportExtent, const std::array<glm::ivec2, BLOOM_MIP_COUNT>& bloomExtents) {
        emissiveImage.Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        bloomImage.Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        lightingImage.Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.GetHandle());
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.GetLayout(), 0, 1, descriptorSet.GetHandlePtr(), 0, nullptr);

        PushConstantsEmissiveBloomComposite pushConstants{};
        pushConstants.viewportOffset = viewportOffset;
        pushConstants.viewportExtent = viewportExtent;
        for (uint32_t mip = 0; mip < BLOOM_MIP_COUNT; mip++) {
            pushConstants.bloomExtents[mip] = bloomExtents[mip];
        }
        vkCmdPushConstants(commandBuffer, pipeline.GetLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pushConstants), &pushConstants);

        vkCmdDispatch(commandBuffer, static_cast<uint32_t>((viewportExtent.x + 15) / 16), static_cast<uint32_t>((viewportExtent.y + 3) / 4), 1);
    }
}

namespace VulkanRenderer {
    void EmissiveForwardPass(VkCommandBuffer commandBuffer) {
        ProfilerVulkanZoneFunction();

        const DrawCommandsSet& drawInfoSet = RenderDataManager::GetDrawInfoSet();
        AllocatedImage* emissiveImage = VulkanResourceManager::GetAllocatedImage("Emissive");
        AllocatedImage* depthImage = VulkanResourceManager::GetAllocatedImage("Depth");
        VulkanPipeline* pipeline = VulkanResourceManager::GetPipeline("EmissiveForward");
        VulkanRenderState* renderState = VulkanResourceManager::GetRenderState("EmissiveForward");
        VulkanDescriptorSet* staticDescriptorSet = VulkanResourceManager::GetDescriptorSet("StaticDescriptorSet");
        VulkanMeshBuffer* meshBuffer = VulkanResourceManager::GetMeshBuffer("AssetGeometry");

        if (!emissiveImage) return;
        if (!depthImage) return;
        if (!pipeline) return;
        if (!renderState) return;
        if (!staticDescriptorSet) return;
        if (!meshBuffer) return;
        if (!meshBuffer->GetVertexBuffer()) return;
        if (!meshBuffer->GetIndexBuffer()) return;

        std::array<VulkanDrawCommandBatch, 4> emissiveCommands = WriteDrawCommandsByViewport(drawInfoSet.emissive);
        VkExtent2D extent = emissiveImage->GetExtent2D();
        if (!BeginRenderState(commandBuffer, *renderState, extent)) return;

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetHandle());
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetLayout(), 0, 1, staticDescriptorSet->GetHandlePtr(), 0, nullptr);

        PushConstantsEmissive pushConstants{};
        pushConstants.frameAddressTableDeviceAddress = GetFrameAddressTableDeviceAddress();
        vkCmdPushConstants(commandBuffer, pipeline->GetLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstants), &pushConstants);

        BindVertexBuffer(commandBuffer, meshBuffer->GetVertexBuffer());
        BindIndexBuffer(commandBuffer, meshBuffer->GetIndexBuffer());

        for (uint32_t viewportIndex = 0; viewportIndex < 4; viewportIndex++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(viewportIndex);
            if (!viewport || !viewport->IsVisible()) continue;

            SetGameViewportAndScissor(commandBuffer, *viewport, extent);
            pushConstants.viewportIndex = viewportIndex;
            vkCmdPushConstants(commandBuffer, pipeline->GetLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstants), &pushConstants);
            MultiDrawIndexedCommands(commandBuffer, emissiveCommands[viewportIndex]);
        }

        EndRenderState(commandBuffer);
    }

    void EmissiveBloomPass(VkCommandBuffer commandBuffer) {
        ProfilerVulkanZoneFunction();

        AllocatedImage* emissiveImage = VulkanResourceManager::GetAllocatedImage("Emissive");
        AllocatedImage* bloomImageA = VulkanResourceManager::GetAllocatedImage("EmissiveBloomA");
        AllocatedImage* bloomImageB = VulkanResourceManager::GetAllocatedImage("EmissiveBloomB");
        AllocatedImage* lightingImage = VulkanResourceManager::GetAllocatedImage("Lighting");
        VulkanPipeline* filterPipeline = VulkanResourceManager::GetPipeline("EmissiveBloomFilter");
        VulkanPipeline* compositePipeline = VulkanResourceManager::GetPipeline("EmissiveBloomComposite");
        VulkanDescriptorSet* staticDescriptorSet = VulkanResourceManager::GetDescriptorSet("StaticDescriptorSet");

        if (!emissiveImage) return;
        if (!bloomImageA || bloomImageA->GetMipLevelCount() < BLOOM_MIP_COUNT) return;
        if (!bloomImageB || bloomImageB->GetMipLevelCount() < BLOOM_MIP_COUNT) return;
        if (!lightingImage) return;
        if (!filterPipeline) return;
        if (!compositePipeline) return;
        if (!staticDescriptorSet) return;

        VkExtent2D imageExtent = emissiveImage->GetExtent2D();
        for (uint32_t viewportIndex = 0; viewportIndex < 4; viewportIndex++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(viewportIndex);
            if (!viewport || !viewport->IsVisible()) continue;

            const glm::vec2 viewportPosition = viewport->GetPosition();
            const glm::vec2 viewportSize = viewport->GetSize();
            const glm::ivec2 viewportOffset(static_cast<int32_t>(viewportPosition.x * imageExtent.width), static_cast<int32_t>((1.0f - viewportPosition.y - viewportSize.y) * imageExtent.height));
            const glm::ivec2 viewportExtent(static_cast<int32_t>(viewportSize.x * imageExtent.width), static_cast<int32_t>(viewportSize.y * imageExtent.height));
            if (viewportExtent.x <= 0 || viewportExtent.y <= 0) continue;

            std::array<glm::ivec2, BLOOM_MIP_COUNT> bloomExtents{};
            bloomExtents[0] = GetHalfExtent(viewportExtent);
            for (uint32_t mip = 1; mip < BLOOM_MIP_COUNT; mip++) {
                bloomExtents[mip] = GetHalfExtent(bloomExtents[mip - 1]);
            }

            // The first source rectangle uses Vulkan's top-origin image coordinates.
            DispatchEmissiveBloomFilter(commandBuffer, *filterPipeline, *staticDescriptorSet, *emissiveImage, *bloomImageA, VULKAN_TEXTURE_IDX_EMISSIVE, VULKAN_STORAGE_IMAGE_R11G11B10F_IDX_BLOOM_A_MIP_0, 0, viewportOffset, viewportExtent, bloomExtents[0], glm::ivec2(1, 0), 1.0f);
            DispatchEmissiveBloomFilter(commandBuffer, *filterPipeline, *staticDescriptorSet, *bloomImageA, *bloomImageB, VULKAN_TEXTURE_IDX_EMISSIVE_BLOOM_A, VULKAN_STORAGE_IMAGE_R11G11B10F_IDX_BLOOM_B_MIP_0, 0, glm::ivec2(0), bloomExtents[0], bloomExtents[0], glm::ivec2(0, 1), 1.11803398875f);

            for (uint32_t mip = 1; mip < BLOOM_MIP_COUNT; mip++) {
                DispatchEmissiveBloomFilter(commandBuffer, *filterPipeline, *staticDescriptorSet, *bloomImageB, *bloomImageA, VULKAN_TEXTURE_IDX_EMISSIVE_BLOOM_B, VULKAN_STORAGE_IMAGE_R11G11B10F_IDX_BLOOM_A_MIP_0 + mip, static_cast<int32_t>(mip - 1), glm::ivec2(0), bloomExtents[mip - 1], bloomExtents[mip], glm::ivec2(1, 0), 1.0f);
                DispatchEmissiveBloomFilter(commandBuffer, *filterPipeline, *staticDescriptorSet, *bloomImageA, *bloomImageB, VULKAN_TEXTURE_IDX_EMISSIVE_BLOOM_A, VULKAN_STORAGE_IMAGE_R11G11B10F_IDX_BLOOM_B_MIP_0 + mip, static_cast<int32_t>(mip), glm::ivec2(0), bloomExtents[mip], bloomExtents[mip], glm::ivec2(0, 1), 1.0f);
            }

            // A and B are intentionally reused, so finish this viewport before starting another.
            DispatchEmissiveBloomComposite(commandBuffer, *compositePipeline, *staticDescriptorSet, *emissiveImage, *bloomImageB, *lightingImage, viewportOffset, viewportExtent, bloomExtents);
        }
    }
}
