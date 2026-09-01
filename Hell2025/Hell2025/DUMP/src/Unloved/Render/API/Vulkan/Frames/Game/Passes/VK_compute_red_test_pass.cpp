#include "Unloved/Render/API/Vulkan/VK_renderer.h"

#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/Render/API/Vulkan/Types/vk_timer.h"
#include "Hell/Render/API/Vulkan/Types/vk_allocated_image.h"
#include "Hell/Render/API/Vulkan/Types/vk_descriptor_set.h"
#include "Hell/Render/API/Vulkan/Types/vk_pipeline.h"

namespace VulkanRenderer {

    void ComputeRedTestPass(VkCommandBuffer commandBuffer) {
        ProfilerVulkanZoneFunction();

        AllocatedImage* lightingImage = VulkanResourceManager::GetAllocatedImage("Lighting");
        VulkanPipeline* pipeline = VulkanResourceManager::GetPipeline("ComputeRedTest");
        VulkanDescriptorSet* staticDescriptorSet = VulkanResourceManager::GetDescriptorSet("StaticDescriptorSet");

        if (!lightingImage) return;
        if (!pipeline) return;
        if (!staticDescriptorSet) return;

        VkExtent2D extent = lightingImage->GetExtent2D();
        lightingImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->GetHandle());
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->GetLayout(), 0, 1, staticDescriptorSet->GetHandlePtr(), 0, nullptr);

        uint32_t groupCountX = (extent.width + 15) / 16;
        uint32_t groupCountY = (extent.height + 15) / 16;
        vkCmdDispatch(commandBuffer, groupCountX, groupCountY, 1);
    }
}
