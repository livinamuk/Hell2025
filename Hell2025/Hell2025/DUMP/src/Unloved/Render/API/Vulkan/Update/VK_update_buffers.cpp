#include "Unloved/Render/API/Vulkan/VK_renderer_internal.h"

#include "Hell/Debug/DebugDraw.h"
#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/UI/UIBackEnd.h"

#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Render/RendererConstants.h"
#include "Unloved/Systems/DirtyTracker/DirtyTracker.h"

#include <algorithm>
#include <array>
#include <vector>

namespace VulkanRenderer {

    template <typename T>
    bool UpdateVectorBuffer(uint64_t bufferId, const std::vector<T>& data) {
        VulkanBuffer* buffer = VulkanResourceManager::GetBuffer(bufferId);
        VkDeviceSize size = sizeof(T) * data.size();
        if (!EnsureBufferSize(buffer, size)) return false;
        return UpdateBuffer(buffer, data.data(), size);
    }

    template <typename T>
    void UpdateGenericMesh(uint64_t meshId, const std::vector<T>& vertices) {
        VulkanGenericMesh* mesh = VulkanResourceManager::GetGenericMesh(meshId);
        if (!mesh) return;

        mesh->UpdateVertexData(vertices.empty() ? nullptr : vertices.data(), vertices.size(), T::GetLayout());
    }

    bool UpdateBuffers() {
        VulkanFrameData& frameData = GetCurrentFrameData();

        // DDGI

        const std::vector<GPUAABB>& dirtyDoorAABBs = Unloved::DirtyTracker::GetDirtyDoorAABBs();
        if (!UpdateVectorBuffer(frameData.ddgi.dirtyDoorAABBs, dirtyDoorAABBs)) return false;
        frameData.ddgi.dirtyDoorAABBCount = static_cast<uint32_t>(dirtyDoorAABBs.size());

        const std::vector<RenderItem>& sceneRenderItems = Unloved::RenderDataManager::GetSceneRenderItems();
        if (!UpdateVectorBuffer(frameData.buffers.sceneRenderItems, sceneRenderItems)) return false;

        const std::vector<uint32_t>& drawRenderItemIndices = Unloved::RenderDataManager::GetDrawRenderItemIndices();
        if (!UpdateVectorBuffer(frameData.buffers.drawRenderItemIndices, drawRenderItemIndices)) return false;

        // Lights

        const std::vector<GPULight>& lights = Unloved::RenderDataManager::GetGPULights();
        VulkanBuffer* lightsBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.lights);
        VkDeviceSize lightsBufferSize = sizeof(GPULight) * lights.size();
        if (!EnsureBufferSize(lightsBuffer, lightsBufferSize)) return false;
        if (!UpdateBuffer(lightsBuffer, lights.data(), lightsBufferSize)) return false;

        // Materials

        const std::vector<Material>& materials = Hell::ResourceManager::GetMaterials();
        VulkanBuffer* materialsBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.materials);
        VkDeviceSize materialsBufferSize = sizeof(Material) * materials.size();
        if (!EnsureBufferSize(materialsBuffer, materialsBufferSize)) return false;
        if (!UpdateBuffer(materialsBuffer, materials.data(), materialsBufferSize)) return false;

        // Sprite sheet instances

        const std::vector<SpriteSheetRenderItem>& spriteSheetInstanceData = Unloved::RenderDataManager::GetSpriteSheetInstanceData();
        VulkanBuffer* spriteSheetInstanceDataBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.spriteSheetInstanceData);
        VkDeviceSize spriteSheetInstanceDataBufferSize = sizeof(SpriteSheetRenderItem) * spriteSheetInstanceData.size();
        if (!EnsureBufferSize(spriteSheetInstanceDataBuffer, spriteSheetInstanceDataBufferSize)) return false;
        if (!UpdateBuffer(spriteSheetInstanceDataBuffer, spriteSheetInstanceData.data(), spriteSheetInstanceDataBufferSize)) return false;

        // Renderer data

        const RendererData& rendererData = Unloved::RenderDataManager::GetRendererData();
        VulkanBuffer* rendererDataBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.rendererData);
        VkDeviceSize rendererDataBufferSize = sizeof(RendererData);
        if (!EnsureBufferSize(rendererDataBuffer, rendererDataBufferSize)) return false;
        if (!UpdateBuffer(rendererDataBuffer, &rendererData, rendererDataBufferSize)) return false;

        // Skinning

        const std::vector<SkinningDispatchGroup>& skinningDispatchGroups= Unloved::RenderDataManager::GetSkinningDispatchGroups();
        const std::vector<SkinningJob>& skinningJobs = Unloved::RenderDataManager::GetSkinningJobs();
        const std::vector<glm::mat4>& skinningTransforms = Unloved::RenderDataManager::GetSkinningTransforms();
        const std::vector<glm::mat4>& previousSkinningTransforms = Unloved::RenderDataManager::GetPreviousSkinningTransforms();

        if (!UpdateVectorBuffer(frameData.buffers.skinningDispatchGroups, skinningDispatchGroups)) return false;
        if (!UpdateVectorBuffer(frameData.buffers.skinningJobs, skinningJobs)) return false;
        if (!UpdateVectorBuffer(frameData.buffers.skinningTransforms, skinningTransforms)) return false;
        if (!UpdateVectorBuffer(frameData.buffers.previousSkinningTransforms, previousSkinningTransforms)) return false;

        // Viewport data

        const std::vector<ViewportData>& viewportData = Unloved::RenderDataManager::GetViewportData();
        VulkanBuffer* viewportDataBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.viewportData);
        VkDeviceSize viewportDataBufferSize = sizeof(ViewportData) * viewportData.size();
        if (!EnsureBufferSize(viewportDataBuffer, viewportDataBufferSize)) return false;
        if (!UpdateBuffer(viewportDataBuffer, viewportData.data(), viewportDataBufferSize)) return false;

        // Debug draw

        UpdateGenericMesh(frameData.genericMeshes.debugLines2D, Hell::DebugDraw::GetLines2D());
        UpdateGenericMesh(frameData.genericMeshes.debugLines3D, Hell::DebugDraw::GetLines3D());
        UpdateGenericMesh(frameData.genericMeshes.debugPoints2D, Hell::DebugDraw::GetPoints2D());
        UpdateGenericMesh(frameData.genericMeshes.debugPoints3D, Hell::DebugDraw::GetPoints3D());

        return true;
    }

    bool UpdateBuffersUI() {
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
        if (!EnsureBufferSize(renderItemsBuffer, renderItemsBufferSize)) return false;
        return UpdateBuffer(renderItemsBuffer, renderItems.data(), renderItemsBufferSize);
    }
}
