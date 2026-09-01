#include "vk_deletion_queue.h"

#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/Render/API/Vulkan/vk_common.h"
#include "Hell/ResourceManagement/ResourceID.h"

#include <array>
#include <vector>

namespace VulkanDeletionQueue {
    std::array<std::vector<uint64_t>, FRAME_OVERLAP> g_entries;
    uint32_t g_frameIndex = 0;

    void Remove(uint64_t id) {
        switch (Hell::ResourceManagement::GetType(id)) {
        case Hell::ResourceManagement::ResourceType::VULKAN_ACCELERATION_STRUCTURE: VulkanResourceManager::RemoveAccelerationStructure(id); break;
        case Hell::ResourceManagement::ResourceType::VULKAN_BUFFER:                 VulkanResourceManager::RemoveBuffer(id);                break;
        case Hell::ResourceManagement::ResourceType::VULKAN_GENERIC_MESH:           VulkanResourceManager::RemoveGenericMesh(id);           break;
        case Hell::ResourceManagement::ResourceType::VULKAN_MESH_BUFFER:            VulkanResourceManager::RemoveMeshBuffer(id);            break;
        case Hell::ResourceManagement::ResourceType::VULKAN_TEXTURE:                VulkanResourceManager::RemoveTexture(id);               break;
        default: break;
        }
    }

    void Retire(uint64_t id) {
        g_entries[g_frameIndex % FRAME_OVERLAP].push_back(id);
    }

    void SetFrameIndex(uint32_t frameIndex) {
        g_frameIndex = frameIndex % FRAME_OVERLAP;
    }

    void Flush(uint32_t frameIndex) {
        std::vector<uint64_t>& entries = g_entries[frameIndex % FRAME_OVERLAP];

        for (uint64_t id : entries) {
            Remove(id);
        }

        entries.clear();
    }

    void FlushAll() {
        for (uint32_t i = 0; i < FRAME_OVERLAP; i++) {
            Flush(i);
        }
    }
}
