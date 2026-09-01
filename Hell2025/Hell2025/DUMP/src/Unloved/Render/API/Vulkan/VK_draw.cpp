#include "Unloved/Render/API/Vulkan/VK_draw.h"

#include "Hell/Logging.h"
#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/Render/API/Vulkan/Types/vk_buffer.h"
#include "Unloved/Render/API/Vulkan/VK_renderer.h"
#include "Unloved/Render/API/Vulkan/VK_renderer_internal.h"
#include "Unloved/Render/RendererConstants.h"
#include "Unloved/Viewport/Viewport.h"

namespace VulkanRenderer {
    static VkDeviceSize g_drawCommandBufferOffset = 0;

    void ResetDrawCommandOffset() {
        // Reset shared draw command buffer offset
        g_drawCommandBufferOffset = 0;
    }

    VulkanDrawCommandBatch WriteDrawCommands(const std::vector<DrawIndexedIndirectCommand>& drawCommands) {
        if (drawCommands.size() > MAX_INDIRECT_DRAW_COMMAND_COUNT) {
            Logging::Error() << "VulkanRenderer::WriteDrawCommands() draw command count " << drawCommands.size() << " exceeded MAX_INDIRECT_DRAW_COMMAND_COUNT " << MAX_INDIRECT_DRAW_COMMAND_COUNT << "\n";
            __debugbreak();
            return {};
        }

        VulkanBuffer* drawCommandBuffer = VulkanResourceManager::GetBuffer(GetCurrentFrameData().buffers.drawCommands);
        if (!drawCommandBuffer) return {};
        return WriteDrawCommands(*drawCommandBuffer, drawCommands);
    }

    VulkanDrawCommandBatch WriteDrawCommands(VulkanBuffer& drawCommandBuffer, const std::vector<DrawIndexedIndirectCommand>& drawCommands) {
        VulkanDrawCommandBatch batch;
        if (drawCommands.empty()) return batch;

        VkDeviceSize writeSize = sizeof(DrawIndexedIndirectCommand) * drawCommands.size();
        VkDeviceSize offset = g_drawCommandBufferOffset;

        // Bail if buffer is full
        if (offset + writeSize > drawCommandBuffer.GetSize()) {
            Logging::Error() << "VulkanRenderer::WriteDrawCommands() draw command buffer capacity exceeded\n";
            __debugbreak();
            return batch;
        }

        batch.offset = offset;
        batch.count = static_cast<uint32_t>(drawCommands.size());

        // Write commands into shared indirect buffer
        drawCommandBuffer.UpdateData(drawCommands.data(), writeSize, offset);
        g_drawCommandBufferOffset += writeSize;
        return batch;
    }

    std::array<VulkanDrawCommandBatch, 4> WriteDrawCommandsByViewport(const std::vector<DrawIndexedIndirectCommand> drawCommands[4]) {
        std::array<VulkanDrawCommandBatch, 4> batches;
        for (uint32_t viewportIndex = 0; viewportIndex < 4; viewportIndex++) {
            batches[viewportIndex] = WriteDrawCommands(drawCommands[viewportIndex]);
        }
        return batches;
    }

    void BindVertexBuffer(VkCommandBuffer commandBuffer, VulkanBuffer* vertexBuffer) {
        if (!vertexBuffer || vertexBuffer->GetBuffer() == VK_NULL_HANDLE) {
            Logging::Error() << "VulkanRenderer::BindVertexBuffer() called without a valid vertex buffer\n";
            return;
        }

        const VkBuffer buffer = vertexBuffer->GetBuffer();
        const VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &buffer, &offset);
    }

    void BindIndexBuffer(VkCommandBuffer commandBuffer, VulkanBuffer* indexBuffer) {
        if (!indexBuffer || indexBuffer->GetBuffer() == VK_NULL_HANDLE) {
            Logging::Error() << "VulkanRenderer::BindIndexBuffer() called without a valid index buffer\n";
            return;
        }

        vkCmdBindIndexBuffer(commandBuffer, indexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
    }

    void SetStencilReference(VkCommandBuffer commandBuffer, uint32_t stencilReference) {
        vkCmdSetStencilReference(commandBuffer, VK_STENCIL_FACE_FRONT_AND_BACK, stencilReference);
    }

    void DrawIndexedCommands(VkCommandBuffer commandBuffer, const std::vector<DrawIndexedIndirectCommand>& drawCommands) {
        for (const DrawIndexedIndirectCommand& command : drawCommands) {
            vkCmdDrawIndexed(commandBuffer, command.indexCount, command.instanceCount, command.firstIndex, command.baseVertex, command.baseInstance);
        }
    }

    void MultiDrawIndexedCommands(VkCommandBuffer commandBuffer, const VulkanDrawCommandBatch& batch) {
        VulkanBuffer* drawCommandBuffer = VulkanResourceManager::GetBuffer(GetCurrentFrameData().buffers.drawCommands);
        if (!drawCommandBuffer) return;

        MultiDrawIndexedCommands(commandBuffer, *drawCommandBuffer, batch.offset, batch.count);
    }

    void MultiDrawIndexedCommands(VkCommandBuffer commandBuffer, VulkanBuffer& drawCommandBuffer, VkDeviceSize drawCommandOffset, uint32_t drawCount) {
        if (drawCount == 0) return;

        vkCmdDrawIndexedIndirect(commandBuffer, drawCommandBuffer.GetBuffer(), drawCommandOffset, drawCount, sizeof(DrawIndexedIndirectCommand));
    }

    void SetGameViewportAndScissor(VkCommandBuffer commandBuffer, const Unloved::Viewport& viewport, VkExtent2D extent) {
        glm::vec2 pos = viewport.GetPosition();
        glm::vec2 size = viewport.GetSize();

        int32_t x = static_cast<int32_t>(pos.x * extent.width);
        uint32_t width = static_cast<uint32_t>(size.x * extent.width);
        uint32_t height = static_cast<uint32_t>(size.y * extent.height);
        int32_t y = static_cast<int32_t>((1.0f - pos.y - size.y) * extent.height);

        VkViewport vkViewport{};
        vkViewport.x = static_cast<float>(x);
        vkViewport.y = static_cast<float>(y);
        vkViewport.width = static_cast<float>(width);
        vkViewport.height = static_cast<float>(height);
        vkViewport.minDepth = 0.0f;
        vkViewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &vkViewport);

        VkRect2D scissor{};
        scissor.offset.x = x;
        scissor.offset.y = y;
        scissor.extent.width = width;
        scissor.extent.height = height;
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    }
}
