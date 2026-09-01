#include "Unloved/Render/API/Vulkan/VK_renderer_internal.h"

#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/Render/DrawCommandTypes.h"
#include "Hell/Render/VertexAttributes.h"
#include "Hell/ResourceManagement/Types/Material.h"
#include "Hell/UI/UITypes.h"

#include "Unloved/Common/Constants.h"
#include "Unloved/Render/RendererConstants.h"
#include "Unloved/Render/RendererTypes.h"
#include "Unloved/Render/Renderer.h"

#include <glm/mat4x4.hpp>

namespace VulkanRenderer {
    void CreateFrameData() {
        VkBufferUsageFlags usageStorage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        VkBufferUsageFlags usageIndirect = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        VkBufferUsageFlags usageDDGIStorage = usageStorage | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        VkBufferUsageFlags usageDDGIIndirect = usageDDGIStorage | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
        VkBufferUsageFlags usageSkinnedVertices = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        VkBufferUsageFlags usageRayQueryInstances = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
        VkBufferUsageFlags usageRayQueryScratch = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        VmaAllocationCreateFlags vmaFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VkDeviceSize dummySize = 64;
        VkDeviceSize tileCount = Unloved::Renderer::GetTileCount();

        for (VulkanFrameData& frameData : g_frameData) {
            frameData.buffers.frameAddressTable = VulkanResourceManager::CreateBuffer(sizeof(FrameAddressTable), usageStorage, VMA_MEMORY_USAGE_AUTO, vmaFlags);
            frameData.buffers.sceneRenderItems = VulkanResourceManager::CreateBuffer(dummySize, usageStorage, VMA_MEMORY_USAGE_AUTO, vmaFlags);
            frameData.buffers.drawRenderItemIndices = VulkanResourceManager::CreateBuffer(dummySize, usageStorage, VMA_MEMORY_USAGE_AUTO, vmaFlags);
            frameData.buffers.viewportData = VulkanResourceManager::CreateBuffer(sizeof(ViewportData) * MAX_VIEWPORT_COUNT, usageStorage, VMA_MEMORY_USAGE_AUTO, vmaFlags);
            frameData.buffers.rendererData = VulkanResourceManager::CreateBuffer(sizeof(RendererData), usageStorage, VMA_MEMORY_USAGE_AUTO, vmaFlags);
            frameData.buffers.lights = VulkanResourceManager::CreateBuffer(dummySize, usageStorage, VMA_MEMORY_USAGE_AUTO, vmaFlags);
            frameData.buffers.materials = VulkanResourceManager::CreateBuffer(dummySize, usageStorage, VMA_MEMORY_USAGE_AUTO, vmaFlags);
            frameData.buffers.spriteSheetInstanceData = VulkanResourceManager::CreateBuffer(dummySize, usageStorage, VMA_MEMORY_USAGE_AUTO, vmaFlags);
            frameData.buffers.drawCommands = VulkanResourceManager::CreateBuffer(sizeof(DrawIndexedIndirectCommand) * MAX_INDIRECT_DRAW_COMMAND_COUNT, usageIndirect, VMA_MEMORY_USAGE_AUTO, vmaFlags);
            frameData.buffers.pointShadowFaceData = VulkanResourceManager::CreateBuffer(dummySize, usageStorage, VMA_MEMORY_USAGE_AUTO, vmaFlags);
            frameData.buffers.skinningDispatchGroups= VulkanResourceManager::CreateBuffer(dummySize, usageStorage, VMA_MEMORY_USAGE_AUTO, vmaFlags);
            frameData.buffers.skinningJobs = VulkanResourceManager::CreateBuffer(dummySize, usageStorage, VMA_MEMORY_USAGE_AUTO, vmaFlags);
            frameData.buffers.skinningTransforms = VulkanResourceManager::CreateBuffer(dummySize, usageStorage, VMA_MEMORY_USAGE_AUTO, vmaFlags);
            frameData.buffers.previousSkinningTransforms = VulkanResourceManager::CreateBuffer(dummySize, usageStorage, VMA_MEMORY_USAGE_AUTO, vmaFlags);
            frameData.buffers.skinnedVertices = VulkanResourceManager::CreateBuffer(dummySize, usageSkinnedVertices, VMA_MEMORY_USAGE_GPU_ONLY);
            frameData.buffers.previousSkinnedPositions = VulkanResourceManager::CreateBuffer(dummySize, usageStorage, VMA_MEMORY_USAGE_GPU_ONLY);
            frameData.buffers.rayQueryInstances = VulkanResourceManager::CreateBuffer(1, usageRayQueryInstances, VMA_MEMORY_USAGE_AUTO, vmaFlags);
            frameData.buffers.rayQueryBLASData = VulkanResourceManager::CreateBuffer(1, usageStorage, VMA_MEMORY_USAGE_AUTO, vmaFlags);
            frameData.buffers.rayQuerySceneRenderItemIndices = VulkanResourceManager::CreateBuffer(1, usageStorage, VMA_MEMORY_USAGE_AUTO, vmaFlags);
            frameData.buffers.rayQueryScratch = VulkanResourceManager::CreateBuffer(1, usageRayQueryScratch, VMA_MEMORY_USAGE_AUTO);
            frameData.buffers.ddgiRayQueryScratch = VulkanResourceManager::CreateBuffer(1, usageRayQueryScratch, VMA_MEMORY_USAGE_AUTO);
            frameData.buffers.uiRenderItems = VulkanResourceManager::CreateBuffer(dummySize * VULKAN_MAX_UI_RENDER_ITEMS, usageStorage, VMA_MEMORY_USAGE_AUTO, vmaFlags);
            frameData.ddgi.dirtyDoorAABBs = VulkanResourceManager::CreateBuffer(dummySize, usageDDGIStorage, VMA_MEMORY_USAGE_AUTO, vmaFlags);
            frameData.ddgi.probeIndexCounter = VulkanResourceManager::CreateBuffer(sizeof(uint32_t), usageDDGIStorage, VMA_MEMORY_USAGE_AUTO);
            frameData.ddgi.probeDistanceCounter = VulkanResourceManager::CreateBuffer(sizeof(uint32_t), usageDDGIStorage, VMA_MEMORY_USAGE_AUTO);
            frameData.ddgi.probeDistanceIndices = VulkanResourceManager::CreateBuffer(dummySize, usageDDGIStorage, VMA_MEMORY_USAGE_AUTO);
            frameData.ddgi.probeDistanceDispatchArgs = VulkanResourceManager::CreateBuffer(sizeof(DispatchIndirectCommand), usageDDGIIndirect, VMA_MEMORY_USAGE_AUTO);
            frameData.ddgi.probeIrradianceCounter = VulkanResourceManager::CreateBuffer(sizeof(uint32_t), usageDDGIStorage, VMA_MEMORY_USAGE_AUTO);
            frameData.ddgi.probeIrradianceIndices = VulkanResourceManager::CreateBuffer(dummySize, usageDDGIStorage, VMA_MEMORY_USAGE_AUTO);
            frameData.ddgi.probeIrradianceDispatchArgs = VulkanResourceManager::CreateBuffer(sizeof(DispatchIndirectCommand), usageDDGIIndirect, VMA_MEMORY_USAGE_AUTO);
            frameData.genericMeshes.ui = VulkanResourceManager::CreateGenericMesh();
            frameData.genericMeshes.debugLines2D = VulkanResourceManager::CreateGenericMesh();
            frameData.genericMeshes.debugLines3D = VulkanResourceManager::CreateGenericMesh();
            frameData.genericMeshes.debugPoints2D = VulkanResourceManager::CreateGenericMesh();
            frameData.genericMeshes.debugPoints3D = VulkanResourceManager::CreateGenericMesh();
            frameData.accelerationStructures.rayQueryTLAS = VulkanResourceManager::CreateAccelerationStructure();

            frameData.buffers.tileLights = VulkanResourceManager::CreateBuffer(tileCount * sizeof(TileLights), usageStorage, VMA_MEMORY_USAGE_AUTO, vmaFlags);
            frameData.buffers.tileWorldBounds = VulkanResourceManager::CreateBuffer(tileCount * sizeof(TileWorldBounds), usageStorage, VMA_MEMORY_USAGE_AUTO, vmaFlags);
        }
    }
}
