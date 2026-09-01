#pragma once

#include "Hell/Render/API/Vulkan/vk_common.h"
#include "Hell/Render/DrawCommandTypes.h"
#include "Unloved/Render/RendererTypes.h"

#include <array>
#include <vector>

struct VulkanBuffer;

namespace Unloved {
    struct Viewport;
}

namespace VulkanRenderer {
    struct VulkanDrawCommandBatch {
        VkDeviceSize offset = 0;
        uint32_t count = 0;
    };

    void ResetDrawCommandOffset();
    VulkanDrawCommandBatch WriteDrawCommands(const std::vector<DrawIndexedIndirectCommand>& drawCommands);
    VulkanDrawCommandBatch WriteDrawCommands(VulkanBuffer& drawCommandBuffer, const std::vector<DrawIndexedIndirectCommand>& drawCommands);
    std::array<VulkanDrawCommandBatch, 4> WriteDrawCommandsByViewport(const std::vector<DrawIndexedIndirectCommand> drawCommands[4]);
    void DrawIndexedCommands(VkCommandBuffer commandBuffer, const std::vector<DrawIndexedIndirectCommand>& drawCommands);
    void MultiDrawIndexedCommands(VkCommandBuffer commandBuffer, const VulkanDrawCommandBatch& batch);
    void MultiDrawIndexedCommands(VkCommandBuffer commandBuffer, VulkanBuffer& drawCommandBuffer, VkDeviceSize drawCommandOffset, uint32_t drawCount);
    void SetGameViewportAndScissor(VkCommandBuffer commandBuffer, const Unloved::Viewport& viewport, VkExtent2D extent);
}
