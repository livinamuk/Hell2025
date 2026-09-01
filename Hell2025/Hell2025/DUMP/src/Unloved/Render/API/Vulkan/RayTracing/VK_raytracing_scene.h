#pragma once

#include "Hell/Render/API/Vulkan/vk_common.h"
#include "Hell/Render/API/Vulkan/Types/vk_buffer.h"
#include "Hell/Render/API/Vulkan/Types/vk_descriptor_set.h"

#include "Unloved/Render/API/Vulkan/VK_frame_data.h"
#include "Unloved/Render/RendererTypes.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace VulkanRenderer {
    struct RayQueryBLASData {
        uint64_t vertexBufferDeviceAddress = 0;
        uint64_t indexBufferDeviceAddress = 0;
        uint32_t sceneRenderItemIndexOffset = 0;
        uint32_t sceneRenderItemIndexCount = 0;
        uint32_t padding0 = 0;
        uint32_t padding1 = 0;
    };
    static_assert(sizeof(RayQueryBLASData) == 32);

    struct RayQueryScene {
        void Clear();
        void Reserve(size_t blasInstanceCount, size_t sceneRenderItemIndexCount);

        void AddBLASInstance(uint64_t blasDeviceAddress, VkTransformMatrixKHR transform, uint64_t vertexBufferAddress, uint64_t indexBufferAddress, uint32_t sceneRenderItemIndex, VkGeometryInstanceFlagsKHR opacityFlags);
        void AddBLASInstance(uint64_t blasDeviceAddress, VkTransformMatrixKHR transform, uint64_t vertexBufferAddress, uint64_t indexBufferAddress, const std::vector<uint32_t>& sceneRenderItemIndices);

        bool HasInstances() const;
        uint32_t GetInstanceCount() const;
        VkDeviceSize GetTLASScratchSize(const VulkanFrameData& frameData) const;

        bool Upload(VkCommandBuffer commandBuffer, VulkanFrameData& frameData);
        bool ResizeTLAS(VulkanFrameData& frameData, uint32_t instanceCapacity);
        void RecordTLASBuild(VkCommandBuffer commandBuffer, VulkanFrameData& frameData, uint64_t scratchBaseAddress);
        bool BindDescriptor(VulkanFrameData& frameData, VulkanDescriptorSet* descriptorSet, uint32_t binding);

    private:
        VkAccelerationStructureInstanceKHR CreateTLASInstance(uint64_t accelerationStructureAddress, VkTransformMatrixKHR transform, uint32_t instanceCustomIndex, VkGeometryInstanceFlagsKHR opacityFlags = 0) const;

        std::vector<VkAccelerationStructureInstanceKHR> m_instances;
        std::vector<RayQueryBLASData> m_blasData;
        std::vector<uint32_t> m_sceneRenderItemIndices;

        VulkanBuffer* m_instanceBuffer = nullptr;
        VulkanBuffer* m_blasDataBuffer = nullptr;
        VulkanBuffer* m_sceneRenderItemIndexBuffer = nullptr;
    };
}
