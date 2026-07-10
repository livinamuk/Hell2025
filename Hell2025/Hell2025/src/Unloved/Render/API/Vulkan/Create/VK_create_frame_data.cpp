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
        VkBufferUsageFlags usageSkinnedVertices = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        VkBufferUsageFlags usageRayQueryInstances = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
        VkBufferUsageFlags usageRayQueryScratch = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        VmaAllocationCreateFlags vmaFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VkDeviceSize dummySize = 64;
        VkDeviceSize tileCount = Unloved::Renderer::GetTileCount();

        for (VulkanFrameData& frameData : g_frameData) {
            frameData.accelerationStructures.rayQueryTLAS = VulkanResourceManager::CreateAccelerationStructure();
            frameData.buffers.instanceData = VulkanResourceManager::CreateBuffer(dummySize, usageStorage, VMA_MEMORY_USAGE_AUTO, vmaFlags);
            frameData.buffers.viewportData = VulkanResourceManager::CreateBuffer(sizeof(ViewportData) * MAX_VIEWPORT_COUNT, usageStorage, VMA_MEMORY_USAGE_AUTO, vmaFlags);
            frameData.buffers.rendererData = VulkanResourceManager::CreateBuffer(sizeof(RendererData), usageStorage, VMA_MEMORY_USAGE_AUTO, vmaFlags);
            frameData.buffers.lights = VulkanResourceManager::CreateBuffer(dummySize, usageStorage, VMA_MEMORY_USAGE_AUTO, vmaFlags);
            frameData.buffers.materials = VulkanResourceManager::CreateBuffer(dummySize, usageStorage, VMA_MEMORY_USAGE_AUTO, vmaFlags);
            frameData.buffers.drawCommands = VulkanResourceManager::CreateBuffer(sizeof(DrawIndexedIndirectCommand) * MAX_INDIRECT_DRAW_COMMAND_COUNT, usageIndirect, VMA_MEMORY_USAGE_AUTO, vmaFlags);
            frameData.buffers.skinningTransforms = VulkanResourceManager::CreateBuffer(dummySize, usageStorage, VMA_MEMORY_USAGE_AUTO, vmaFlags);
            frameData.buffers.skinnedVertices = VulkanResourceManager::CreateBuffer(dummySize, usageSkinnedVertices, VMA_MEMORY_USAGE_GPU_ONLY);
            frameData.buffers.rayQueryInstances = VulkanResourceManager::CreateBuffer(1, usageRayQueryInstances, VMA_MEMORY_USAGE_AUTO, vmaFlags);
            frameData.buffers.rayQueryInstanceData = VulkanResourceManager::CreateBuffer(1, usageStorage, VMA_MEMORY_USAGE_AUTO, vmaFlags);
            frameData.buffers.rayQueryGeometryData = VulkanResourceManager::CreateBuffer(1, usageStorage, VMA_MEMORY_USAGE_AUTO, vmaFlags);
            frameData.buffers.rayQueryScratch = VulkanResourceManager::CreateBuffer(1, usageRayQueryScratch, VMA_MEMORY_USAGE_AUTO);
            frameData.buffers.uiRenderItems = VulkanResourceManager::CreateBuffer(dummySize * VULKAN_MAX_UI_RENDER_ITEMS, usageStorage, VMA_MEMORY_USAGE_AUTO, vmaFlags);
            frameData.buffers.tileLights = VulkanResourceManager::CreateBuffer(tileCount * sizeof(TileLights), usageStorage, VMA_MEMORY_USAGE_AUTO, vmaFlags);
            frameData.buffers.tileWorldBounds = VulkanResourceManager::CreateBuffer(tileCount * sizeof(TileWorldBounds), usageStorage, VMA_MEMORY_USAGE_AUTO, vmaFlags);
            frameData.genericMeshes.ui = VulkanResourceManager::CreateGenericMesh();
        }
    }
}
