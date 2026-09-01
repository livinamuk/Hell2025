#pragma once

#include "Hell/Render/API/Vulkan/vk_common.h"
#include "Hell/Render/API/Vulkan/Types/vk_buffer.h"
#include "Hell/Render/API/Vulkan/Types/vk_mesh_buffer.h"

#include "Unloved/Render/API/Vulkan/RayTracing/VK_raytracing_scene.h"
#include "Unloved/Render/API/Vulkan/VK_frame_data.h"
#include "Unloved/Render/RendererTypes.h"

#include <cstdint>
#include <vector>

namespace VulkanRenderer {
    namespace TransientBLASBuilder {
        void BeginFrame();
        void ReleaseFrameSlots(VulkanFrameData& frameData);
        void AddTransientRayQueryBLASInstances(VulkanFrameData& frameData, VulkanMeshBuffer& assetMeshBuffer, VulkanBuffer& skinnedVertexBuffer, const std::vector<std::vector<uint32_t>>& renderItemGroups, const std::vector<RenderItem>& sceneRenderItems, RayQueryScene& scene);

        VkDeviceSize GetScratchSize();
        bool RecordBuilds(VkCommandBuffer commandBuffer, VulkanFrameData& frameData, uint64_t scratchBaseAddress);
    }
}
