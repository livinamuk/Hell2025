#include "Unloved/Render/API/Vulkan/VK_renderer_internal.h"

#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/Render/API/Vulkan/Types/vk_allocated_image.h"
#include "Hell/Render/API/Vulkan/Types/vk_buffer.h"
#include "Hell/Render/API/Vulkan/Types/vk_descriptor_set.h"
#include "Hell/Render/API/Vulkan/Types/vk_mesh_buffer.h"
#include "Hell/Render/API/Vulkan/Types/vk_pipeline.h"
#include "Unloved/Render/API/Vulkan/VK_draw.h"
#include "Unloved/Render/API/Vulkan/VK_push_constants.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Viewport/ViewportManager.h"

#include <array>

using namespace Unloved;

namespace VulkanRenderer {

    void SpriteSheetPass(VkCommandBuffer commandBuffer) {
        ProfilerVulkanZoneFunction();

        const DrawCommandsSet& drawInfoSet = RenderDataManager::GetDrawInfoSet();
        AllocatedImage* lightingImage = VulkanResourceManager::GetAllocatedImage("Lighting");
        VulkanPipeline* pipeline = VulkanResourceManager::GetPipeline("SpriteSheet");
        VulkanRenderState* renderState = VulkanResourceManager::GetRenderState("SpriteSheet");
        VulkanDescriptorSet* staticDescriptorSet = VulkanResourceManager::GetDescriptorSet("StaticDescriptorSet");
        VulkanMeshBuffer* meshBuffer = VulkanResourceManager::GetMeshBuffer("AssetGeometry");
        if (!lightingImage) return;
        if (!pipeline) return;
        if (!renderState) return;
        if (!staticDescriptorSet) return;
        if (!meshBuffer) return;
        if (!meshBuffer->GetVertexBuffer()) return;
        if (!meshBuffer->GetIndexBuffer()) return;

        PushConstantsSpriteSheet pushConstants{};
        pushConstants.frameAddressTableDeviceAddress = GetFrameAddressTableDeviceAddress();

        if (pushConstants.frameAddressTableDeviceAddress == 0) return;

        std::array<VulkanDrawCommandBatch, 4> spriteSheetCommands = WriteDrawCommandsByViewport(drawInfoSet.spriteSheets);

        VkExtent2D extent = lightingImage->GetExtent2D();
        if (!BeginRenderState(commandBuffer, *renderState, extent)) return;

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetHandle());
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetLayout(), 0, 1, staticDescriptorSet->GetHandlePtr(), 0, nullptr);
        vkCmdPushConstants(commandBuffer, pipeline->GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushConstants), &pushConstants);

        BindVertexBuffer(commandBuffer, meshBuffer->GetVertexBuffer());
        BindIndexBuffer(commandBuffer, meshBuffer->GetIndexBuffer());

        for (uint32_t i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (!viewport || !viewport->IsVisible()) continue;

            SetGameViewportAndScissor(commandBuffer, *viewport, extent);
            pushConstants.viewportIndex = i;
            vkCmdPushConstants(commandBuffer, pipeline->GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushConstants), &pushConstants);
            MultiDrawIndexedCommands(commandBuffer, spriteSheetCommands[i]);
        }

        EndRenderState(commandBuffer);
    }
}
