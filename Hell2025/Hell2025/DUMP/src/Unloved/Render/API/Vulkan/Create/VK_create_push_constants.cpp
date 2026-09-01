#include "Unloved/Render/API/Vulkan/VK_renderer_internal.h"

#include "Hell/Logging.h"
#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/Render/API/Vulkan/Types/vk_buffer.h"

namespace {
    uint64_t GetDeviceAddressOrZero(uint64_t bufferId) {
        const VulkanBuffer* buffer = VulkanResourceManager::GetBuffer(bufferId);
        return buffer ? buffer->GetDeviceAddress() : 0;
    }

    bool PopulateTableEntry(uint64_t& tableEntry, uint64_t bufferId, const char* tableEntryName) {
        tableEntry = GetDeviceAddressOrZero(bufferId);
        if (tableEntry != 0) return true;

        Logging::Error() << "FrameAddressTable entry '" << tableEntryName << "' has no valid device address\n";
        return false;
    }
}

namespace VulkanRenderer {
    bool UpdateFrameAddressTable() {
        const VulkanFrameData& frameData = GetCurrentFrameData();

        FrameAddressTable table{};
        bool valid = true;
        valid &= PopulateTableEntry(table.sceneRenderItemBuffer, frameData.buffers.sceneRenderItems, "sceneRenderItemBuffer");
        valid &= PopulateTableEntry(table.drawRenderItemIndexBuffer, frameData.buffers.drawRenderItemIndices, "drawRenderItemIndexBuffer");
        valid &= PopulateTableEntry(table.viewportDataBuffer, frameData.buffers.viewportData, "viewportDataBuffer");
        valid &= PopulateTableEntry(table.rendererDataBuffer, frameData.buffers.rendererData, "rendererDataBuffer");
        valid &= PopulateTableEntry(table.materialBuffer, frameData.buffers.materials, "materialBuffer");
        valid &= PopulateTableEntry(table.lightBuffer, frameData.buffers.lights, "lightBuffer");
        valid &= PopulateTableEntry(table.spriteSheetRenderItemBuffer, frameData.buffers.spriteSheetInstanceData, "spriteSheetRenderItemBuffer");
        valid &= PopulateTableEntry(table.uiRenderItemBuffer, frameData.buffers.uiRenderItems, "uiRenderItemBuffer");
        valid &= PopulateTableEntry(table.tileLightBuffer, frameData.buffers.tileLights, "tileLightBuffer");
        valid &= PopulateTableEntry(table.tileWorldBoundsBuffer, frameData.buffers.tileWorldBounds, "tileWorldBoundsBuffer");

        VulkanBuffer* tableBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.frameAddressTable);
        if (!tableBuffer || tableBuffer->GetDeviceAddress() == 0) {
            Logging::Error() << "FrameAddressTable buffer has no valid device address\n";
            valid = false;
        }
        if (!valid) return false;

        if (!UpdateBuffer(tableBuffer, &table, sizeof(table))) {
            Logging::Error() << "Failed to update the current FrameAddressTable buffer\n";
            return false;
        }
        return true;
    }

    uint64_t GetFrameAddressTableDeviceAddress() {
        const VulkanFrameData& frameData = GetCurrentFrameData();
        return GetDeviceAddressOrZero(frameData.buffers.frameAddressTable);
    }
}
