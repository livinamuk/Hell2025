#include "Unloved/Render/API/Vulkan/VK_renderer_internal.h"

#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/UI/UIBackEnd.h"

#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Render/RendererConstants.h"

#include <algorithm>
#include <array>
#include <vector>

namespace VulkanRenderer {

    void UpdateBuffers() {
        const VulkanFrameData& frameData = GetCurrentFrameData();

        // Instance data

        const std::vector<RenderItem>& instanceData = Unloved::RenderDataManager::GetInstanceData();
        VulkanBuffer* instanceDataBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.instanceData);
        VkDeviceSize instanceBufferSize = sizeof(RenderItem) * instanceData.size();
        EnsureBufferSize(instanceDataBuffer, instanceBufferSize);
        UpdateBuffer(instanceDataBuffer, instanceData.data(), instanceBufferSize);

        // Lights

        const std::vector<GPULight>& lights = Unloved::RenderDataManager::GetGPULights();
        VulkanBuffer* lightsBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.lights);
        VkDeviceSize lightsBufferSize = sizeof(GPULight) * lights.size();
        EnsureBufferSize(lightsBuffer, lightsBufferSize);
        UpdateBuffer(lightsBuffer, lights.data(), lightsBufferSize);

        // Materials

        const std::vector<Material>& materials = Hell::ResourceManager::GetMaterials();
        VulkanBuffer* materialsBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.materials);
        VkDeviceSize materialsBufferSize = sizeof(Material) * materials.size();
        EnsureBufferSize(materialsBuffer, materialsBufferSize);
        UpdateBuffer(materialsBuffer, materials.data(), materialsBufferSize);

        // Renderer data

        const RendererData& rendererData = Unloved::RenderDataManager::GetRendererData();
        VulkanBuffer* rendererDataBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.rendererData);
        VkDeviceSize rendererDataBufferSize = sizeof(RendererData);
        EnsureBufferSize(rendererDataBuffer, rendererDataBufferSize);
        UpdateBuffer(rendererDataBuffer, &rendererData, rendererDataBufferSize);

        // Skinning transforms

        const std::vector<glm::mat4>& skinningTransforms = Unloved::RenderDataManager::GetSkinningTransforms();
        VulkanBuffer* skinningTransformsBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.skinningTransforms);
        VkDeviceSize skinningTransformsBufferSize = sizeof(glm::mat4) * skinningTransforms.size();
        EnsureBufferSize(skinningTransformsBuffer, skinningTransformsBufferSize);
        UpdateBuffer(skinningTransformsBuffer, skinningTransforms.data(), skinningTransformsBufferSize);

        // Viewport data

        const std::vector<ViewportData>& viewportData = Unloved::RenderDataManager::GetViewportData();
        VulkanBuffer* viewportDataBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.viewportData);
        VkDeviceSize viewportDataBufferSize = sizeof(ViewportData) * viewportData.size();
        EnsureBufferSize(viewportDataBuffer, viewportDataBufferSize);
        UpdateBuffer(viewportDataBuffer, viewportData.data(), viewportDataBufferSize);
    }

    void UpdateBuffersUI() {
        const VulkanFrameData& frameData = GetCurrentFrameData();

        // UI mesh
        if (VulkanGenericMesh* uiMesh = VulkanResourceManager::GetGenericMesh(frameData.genericMeshes.ui)) {
            const std::vector<Vertex2D>& vertices = UIBackEnd::GetVertices();
            const std::vector<uint32_t>& indices = UIBackEnd::GetIndices();
            uiMesh->UpdateVertexData(vertices.empty() ? nullptr : vertices.data(), vertices.size(), Vertex2D::GetLayout());
            uiMesh->UpdateIndexData(indices);
        }

        // UI render items
        const std::vector<RenderItemUI>& renderItems = UIBackEnd::GetRenderItems();
        VulkanBuffer* renderItemsBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.uiRenderItems);
        VkDeviceSize renderItemsBufferSize = sizeof(RenderItemUI) * renderItems.size();
        EnsureBufferSize(renderItemsBuffer, renderItemsBufferSize);
        UpdateBuffer(renderItemsBuffer, renderItems.data(), renderItemsBufferSize);
    }
}
