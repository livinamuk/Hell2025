#include "VK_raytracing_scene.h"

#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/Render/API/Vulkan/Types/vk_acceleration_structure.h"
#include "Hell/Render/API/Vulkan/Types/vk_buffer.h"
#include "Hell/Render/API/Vulkan/Types/vk_descriptor_set.h"

#include "Unloved/Render/API/Vulkan/RayTracing/VK_acceleration_structure_utils.h"
#include "Unloved/Render/API/Vulkan/VK_renderer_internal.h"

#include <array>

namespace VulkanRenderer {

    void RayQueryScene::Clear() {
        m_instances.clear();
        m_blasData.clear();
        m_sceneRenderItemIndices.clear();
        m_instanceBuffer = nullptr;
        m_blasDataBuffer = nullptr;
        m_sceneRenderItemIndexBuffer = nullptr;
    }

    void RayQueryScene::Reserve(size_t blasInstanceCount, size_t sceneRenderItemIndexCount) {
        m_instances.reserve(blasInstanceCount);
        m_blasData.reserve(blasInstanceCount);
        m_sceneRenderItemIndices.reserve(sceneRenderItemIndexCount);
    }

    void RayQueryScene::AddBLASInstance(uint64_t blasDeviceAddress, VkTransformMatrixKHR transform, uint64_t vertexBufferAddress, uint64_t indexBufferAddress, uint32_t sceneRenderItemIndex, VkGeometryInstanceFlagsKHR opacityFlags) {
        uint32_t instanceCustomIndex = static_cast<uint32_t>(m_blasData.size());

        RayQueryBLASData& data = m_blasData.emplace_back();
        data.vertexBufferDeviceAddress = vertexBufferAddress;
        data.indexBufferDeviceAddress = indexBufferAddress;
        data.sceneRenderItemIndexOffset = static_cast<uint32_t>(m_sceneRenderItemIndices.size());
        data.sceneRenderItemIndexCount = 1;

        m_sceneRenderItemIndices.push_back(sceneRenderItemIndex);
        m_instances.push_back(CreateTLASInstance(blasDeviceAddress, transform, instanceCustomIndex, opacityFlags));
    }

    void RayQueryScene::AddBLASInstance(uint64_t blasDeviceAddress, VkTransformMatrixKHR transform, uint64_t vertexBufferAddress, uint64_t indexBufferAddress, const std::vector<uint32_t>& sceneRenderItemIndices) {
        if (sceneRenderItemIndices.empty()) return;

        uint32_t instanceCustomIndex = static_cast<uint32_t>(m_blasData.size());

        RayQueryBLASData& data = m_blasData.emplace_back();
        data.vertexBufferDeviceAddress = vertexBufferAddress;
        data.indexBufferDeviceAddress = indexBufferAddress;
        data.sceneRenderItemIndexOffset = static_cast<uint32_t>(m_sceneRenderItemIndices.size());
        data.sceneRenderItemIndexCount = static_cast<uint32_t>(sceneRenderItemIndices.size());

        m_sceneRenderItemIndices.insert(m_sceneRenderItemIndices.end(), sceneRenderItemIndices.begin(), sceneRenderItemIndices.end());

        m_instances.push_back(CreateTLASInstance(blasDeviceAddress, transform, instanceCustomIndex));
    }

    bool RayQueryScene::HasInstances() const {
        return !m_instances.empty();
    }

    uint32_t RayQueryScene::GetInstanceCount() const {
        return static_cast<uint32_t>(m_instances.size());
    }

    VkDeviceSize RayQueryScene::GetTLASScratchSize(const VulkanFrameData& frameData) const {
        return static_cast<VkDeviceSize>(frameData.accelerationStructures.rayQueryTLASScratchSize);
    }

    bool RayQueryScene::Upload(VkCommandBuffer commandBuffer, VulkanFrameData& frameData) {
        if (m_instances.empty()) return false;

        VkDeviceSize instanceBufferSize = sizeof(VkAccelerationStructureInstanceKHR) * m_instances.size();
        VkDeviceSize blasDataSize = sizeof(RayQueryBLASData) * m_blasData.size();
        VkDeviceSize sceneRenderItemIndexDataSize = sizeof(uint32_t) * m_sceneRenderItemIndices.size();

        if (!EnsureBufferSize(frameData.buffers.rayQueryInstances, instanceBufferSize)) return false;
        if (!EnsureBufferSize(frameData.buffers.rayQueryBLASData, blasDataSize)) return false;
        if (!EnsureBufferSize(frameData.buffers.rayQuerySceneRenderItemIndices, sceneRenderItemIndexDataSize)) return false;

        m_instanceBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.rayQueryInstances);
        m_blasDataBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.rayQueryBLASData);
        m_sceneRenderItemIndexBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.rayQuerySceneRenderItemIndices);
        if (!m_instanceBuffer) return false;
        if (!m_blasDataBuffer) return false;
        if (!m_sceneRenderItemIndexBuffer) return false;

        m_instanceBuffer->UpdateData(m_instances.data(), instanceBufferSize);
        m_blasDataBuffer->UpdateData(m_blasData.data(), blasDataSize);
        m_sceneRenderItemIndexBuffer->UpdateData(m_sceneRenderItemIndices.data(), sceneRenderItemIndexDataSize);

        VkBufferMemoryBarrier instanceUploadBarrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
        instanceUploadBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        instanceUploadBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
        instanceUploadBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        instanceUploadBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        instanceUploadBarrier.buffer = m_instanceBuffer->GetBuffer();
        instanceUploadBarrier.offset = 0;
        instanceUploadBarrier.size = instanceBufferSize;
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 0, nullptr, 1, &instanceUploadBarrier, 0, nullptr);

        std::array<VkBufferMemoryBarrier, 2> metadataUploadBarriers{};
        for (VkBufferMemoryBarrier& barrier : metadataUploadBarriers) {
            barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.offset = 0;
        }

        metadataUploadBarriers[0].buffer = m_blasDataBuffer->GetBuffer();
        metadataUploadBarriers[0].size = blasDataSize;
        metadataUploadBarriers[1].buffer = m_sceneRenderItemIndexBuffer->GetBuffer();
        metadataUploadBarriers[1].size = sceneRenderItemIndexDataSize;
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, static_cast<uint32_t>(metadataUploadBarriers.size()), metadataUploadBarriers.data(), 0, nullptr);

        return true;
    }

    bool RayQueryScene::ResizeTLAS(VulkanFrameData& frameData, uint32_t instanceCapacity) {
        if (!m_instanceBuffer) return false;
        if (frameData.accelerationStructures.rayQueryTLAS == 0) return false;

        VkAccelerationStructureBuildSizesInfoKHR sizeInfo = QueryTopLevelBuildSize(m_instanceBuffer->GetDeviceAddress(), instanceCapacity);
        if (!PrepareAccelerationStructure(frameData.accelerationStructures.rayQueryTLAS, VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR, sizeInfo)) return false;

        frameData.accelerationStructures.rayQueryTLASInstanceCapacity = instanceCapacity;
        frameData.accelerationStructures.rayQueryTLASScratchSize = sizeInfo.buildScratchSize;
        return true;
    }

    void RayQueryScene::RecordTLASBuild(VkCommandBuffer commandBuffer, VulkanFrameData& frameData, uint64_t scratchBaseAddress) {
        if (!m_instanceBuffer) return;
        RecordTopLevelBuild(commandBuffer, frameData.accelerationStructures.rayQueryTLAS, m_instanceBuffer->GetDeviceAddress(), GetInstanceCount(), scratchBaseAddress);
    }

    bool RayQueryScene::BindDescriptor(VulkanFrameData& frameData, VulkanDescriptorSet* descriptorSet, uint32_t binding) {
        if (!descriptorSet) return false;

        VulkanAccelerationStructure* tlas = VulkanResourceManager::GetAccelerationStructure(frameData.accelerationStructures.rayQueryTLAS);
        if (!tlas || tlas->GetHandle() == VK_NULL_HANDLE) return false;

        descriptorSet->WriteAccelerationStructure(binding, tlas->GetHandle());
        descriptorSet->Update();
        return true;
    }

    VkAccelerationStructureInstanceKHR RayQueryScene::CreateTLASInstance(uint64_t accelerationStructureAddress, VkTransformMatrixKHR transform, uint32_t instanceCustomIndex, VkGeometryInstanceFlagsKHR opacityFlags) const {
        // instanceCustomIndex is the shader lookup into RayQueryBLASData
        VkAccelerationStructureInstanceKHR instance{};
        instance.transform = transform;
        instance.instanceCustomIndex = instanceCustomIndex;
        instance.mask = 0xff;
        instance.instanceShaderBindingTableRecordOffset = 0;
        instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR | opacityFlags;
        instance.accelerationStructureReference = accelerationStructureAddress;
        return instance;
    }
}
