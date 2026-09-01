#include "Unloved/Render/API/Vulkan/VK_renderer.h"
#include "Unloved/Render/API/Vulkan/VK_renderer_internal.h"

#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/Render/API/Vulkan/Managers/vk_device_manager.h"
#include "Hell/Render/API/Vulkan/Types/vk_acceleration_structure.h"
#include "Hell/Render/API/Vulkan/Types/vk_allocated_image.h"
#include "Hell/Render/API/Vulkan/Types/vk_buffer.h"
#include "Hell/Render/API/Vulkan/Types/vk_descriptor_set.h"
#include "Hell/Render/API/Vulkan/Types/vk_pipeline.h"
#include "Hell/Render/VertexAttributes.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/ResourceManagement/Types/MeshBuffer.h"

#include "Unloved/Render/API/Vulkan/VK_descriptor_indices.h"
#include "Unloved/Render/API/Vulkan/VK_push_constants.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Render/RendererConstants.h"
#include "Unloved/Viewport/ViewportManager.h"

#include <glm/matrix.hpp>
#include <algorithm>
#include <array>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace Unloved;

namespace VulkanRenderer {

    void RenderDeferredLighting(VkCommandBuffer commandBuffer) {

        AllocatedImage* lightingImage = VulkanResourceManager::GetAllocatedImage("Lighting");
        AllocatedImage* baseColorImage = VulkanResourceManager::GetAllocatedImage("BaseColorMetallic");
        AllocatedImage* normalImage = VulkanResourceManager::GetAllocatedImage("NormalXYRoughnessMisc");
        AllocatedImage* indirectDiffuseImage = VulkanResourceManager::GetAllocatedImage("IndirectDiffuse");
        AllocatedImage* indirectDiffuseSurfaceImage = VulkanResourceManager::GetAllocatedImage("IndirectDiffuseSurface");
        AllocatedImage* amdTemporalImage = VulkanResourceManager::GetAllocatedImage("IndirectSpecularAMDTemporal");
        AllocatedImage* amdMaterialRoughnessImage = VulkanResourceManager::GetAllocatedImage("IndirectSpecularAMDMaterialRoughness");
        VulkanPipeline* pipeline = VulkanResourceManager::GetPipeline("LightingDeferred");
        VulkanRenderState* renderState = VulkanResourceManager::GetRenderState("LightingDeferred");
        VulkanDescriptorSet* staticDescriptorSet = VulkanResourceManager::GetDescriptorSet("StaticDescriptorSet");
        VulkanDescriptorSetResource* rayQueryDescriptorSetResource = VulkanResourceManager::GetDescriptorSetResource("RayQueryDescriptorSet");
        const VulkanFrameData& frameData = GetCurrentFrameData();
        VulkanDescriptorSet* rayQueryDescriptorSet = rayQueryDescriptorSetResource ? &rayQueryDescriptorSetResource->GetSet(GetCurrentFrameIndex()) : nullptr;
        const uint64_t frameAddressTableDeviceAddress = GetFrameAddressTableDeviceAddress();
        VulkanBuffer* rayQueryBLASDataBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.rayQueryBLASData);
        VulkanBuffer* rayQuerySceneRenderItemIndexBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.rayQuerySceneRenderItemIndices);
        const int32_t brdfLutTextureIndex = Hell::ResourceManager::GetTextureBindlessIndexByName("BrdfLut", true);

        if (!pipeline) return;
        if (!renderState) return;
        if (!staticDescriptorSet) return;
        if (!rayQueryDescriptorSet) return;
        if (!lightingImage) return;
        if (!baseColorImage) return;
        if (!normalImage) return;
        if (!indirectDiffuseImage) return;
        if (!indirectDiffuseSurfaceImage) return;
        if (!amdTemporalImage || !amdMaterialRoughnessImage) return;
        if (frameAddressTableDeviceAddress == 0) return;
        if (!rayQueryBLASDataBuffer) return;
        if (!rayQuerySceneRenderItemIndexBuffer) return;
        VkExtent2D extent = lightingImage->GetExtent2D();
        baseColorImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);
        normalImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);
        indirectDiffuseImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);
        indirectDiffuseSurfaceImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);
        amdTemporalImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);
        amdMaterialRoughnessImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);
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

        PushConstantsDeferredLighting pushConstants{};
        pushConstants.frameAddressTableDeviceAddress = frameAddressTableDeviceAddress;
        pushConstants.rayQueryBLASDataDeviceAddress = rayQueryBLASDataBuffer->GetDeviceAddress();
        pushConstants.rayQuerySceneRenderItemIndicesDeviceAddress = rayQuerySceneRenderItemIndexBuffer->GetDeviceAddress();
        pushConstants.brdfLutTextureIndex = brdfLutTextureIndex;
        vkCmdPushConstants(commandBuffer, pipeline->GetLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstants), &pushConstants);

        vkCmdDraw(commandBuffer, 3, 1, 0, 0);

        SetStencilReference(commandBuffer, STENCIL_BIT_VIEW_WEAPON_LIGHTING);
        vkCmdDraw(commandBuffer, 3, 1, 0, 0);

        EndRenderState(commandBuffer);
    }

    void LightingPass(VkCommandBuffer commandBuffer) {
        ProfilerVulkanZoneFunction();
        RenderDeferredLighting(commandBuffer);
    }
}
