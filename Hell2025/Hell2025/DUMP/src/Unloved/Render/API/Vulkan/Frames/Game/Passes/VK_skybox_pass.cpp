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
#include "Unloved/Render/API/Vulkan/VK_draw.h"
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

    void SkyboxPass(VkCommandBuffer commandBuffer) {
        ProfilerVulkanZoneFunction();

        AllocatedImage* lightingImage = VulkanResourceManager::GetAllocatedImage("Lighting");
        VulkanPipeline* pipeline = VulkanResourceManager::GetPipeline("Skybox");
        VulkanRenderState* renderState = VulkanResourceManager::GetRenderState("Skybox");
        VulkanDescriptorSet* staticDescriptorSet = VulkanResourceManager::GetDescriptorSet("StaticDescriptorSet");
        VulkanCubemap* skyboxCubemap = VulkanResourceManager::CubemapExists("SkyboxNightSky") ? VulkanResourceManager::GetCubemap("SkyboxNightSky") : nullptr;
        const uint64_t frameAddressTableDeviceAddress = GetFrameAddressTableDeviceAddress();

        if (!pipeline) return;
        if (!renderState) return;
        if (!staticDescriptorSet) return;
        if (!skyboxCubemap) return;
        if (skyboxCubemap->GetImageView() == VK_NULL_HANDLE) return;
        if (skyboxCubemap->GetSampler() == VK_NULL_HANDLE) return;
        if (!lightingImage) return;
        if (frameAddressTableDeviceAddress == 0) return;

        VkExtent2D extent = lightingImage->GetExtent2D();
        lightingImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT); // Needed?
        if (!BeginRenderState(commandBuffer, *renderState, extent)) return;

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetHandle());
        VkDescriptorSet descriptorSets[] = { staticDescriptorSet->GetHandle() };
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetLayout(), 0, 1, descriptorSets, 0, nullptr);

        PushConstantsSkybox pushConstants{};
        pushConstants.frameAddressTableDeviceAddress = frameAddressTableDeviceAddress;
        vkCmdPushConstants(commandBuffer, pipeline->GetLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstants), &pushConstants);

        for (uint32_t viewportIndex = 0; viewportIndex < 4; viewportIndex++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(viewportIndex);
            if (!viewport || !viewport->IsVisible()) continue;

            SetGameViewportAndScissor(commandBuffer, *viewport, extent);
            vkCmdDraw(commandBuffer, 3, 1, 0, 0);
        }

        EndRenderState(commandBuffer);
    }
}
