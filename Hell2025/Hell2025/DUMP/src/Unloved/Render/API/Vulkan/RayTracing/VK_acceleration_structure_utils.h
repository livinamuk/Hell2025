#pragma once

#include "Hell/Render/API/Vulkan/vk_common.h"

#include "Unloved/Render/RendererTypes.h"

#include <glm/matrix.hpp>

#include <cstdint>
#include <vector>

namespace VulkanRenderer {
    VkDeviceSize AlignUp(VkDeviceSize value, VkDeviceSize alignment);
    VkDeviceSize AccelerationStructureScratchAlignment();
    VkTransformMatrixKHR TransformMatrixKHR(const glm::mat4& matrix);

    RayQueryMeshInstance CreateRayQueryMeshInstance(const RenderItem& renderItem);
    VkGeometryFlagsKHR GetRayQueryGeometryFlags(const RenderItem& renderItem);
    VkGeometryFlagsKHR GetRayQueryGeometryFlags(const RayQueryMaterial& material);
    VkAccelerationStructureGeometryKHR CreateTriangleGeometry(uint64_t vertexBufferAddress, uint64_t indexBufferAddress, const RayQueryMesh& mesh, VkGeometryFlagsKHR geometryFlags, uint64_t transformAddress = 0);
    VkAccelerationStructureBuildSizesInfoKHR QueryBottomLevelBuildSize(uint64_t vertexBufferAddress, uint64_t indexBufferAddress, const std::vector<RayQueryMeshInstance>& meshInstances, VkBuildAccelerationStructureFlagsKHR flags);
    VkAccelerationStructureBuildSizesInfoKHR QueryTopLevelBuildSize(uint64_t instanceBufferAddress, uint32_t instanceCount);

    bool PrepareAccelerationStructure(uint64_t id, VkAccelerationStructureTypeKHR type, VkAccelerationStructureBuildSizesInfoKHR sizeInfo);
    void RecordAccelerationStructureBuildBarrier(VkCommandBuffer commandBuffer, VkPipelineStageFlags dstStageMask, VkAccessFlags dstAccessMask);
    void RecordTopLevelBuild(VkCommandBuffer commandBuffer, uint64_t tlasId, uint64_t instanceBufferAddress, uint32_t instanceCount, uint64_t scratchBufferAddress);
}
