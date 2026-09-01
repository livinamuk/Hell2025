#include "VK_acceleration_structure_utils.h"

#include "Hell/Render/API/Vulkan/Managers/vk_device_manager.h"
#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/Render/API/Vulkan/Types/vk_acceleration_structure.h"
#include "Hell/Render/VertexAttributes.h"

#include "Unloved/Render/RendererEnums.h"

#include <glm/matrix.hpp>

namespace VulkanRenderer {

    VkDeviceSize AlignUp(VkDeviceSize value, VkDeviceSize alignment) {
        if (alignment <= 1) return value;
        return ((value + alignment - 1) / alignment) * alignment;
    }

    VkDeviceSize AccelerationStructureScratchAlignment() {
        VkDeviceSize alignment = VulkanDeviceManager::GetAccelerationStructureProperties().minAccelerationStructureScratchOffsetAlignment;
        return alignment == 0 ? 1 : alignment;
    }

    VkTransformMatrixKHR TransformMatrixKHR(const glm::mat4& matrix) {
        // Vulkan wants a 3x4 row-major transform
        VkTransformMatrixKHR transform{};
        glm::mat4 transposed = glm::transpose(matrix);

        for (int x = 0; x < 3; x++) {
            for (int y = 0; y < 4; y++) {
                transform.matrix[x][y] = transposed[x][y];
            }
        }

        return transform;
    }

    RayQueryMeshInstance CreateRayQueryMeshInstance(const RenderItem& renderItem) {
        RayQueryMeshInstance meshInstance{};
        meshInstance.mesh.baseVertex = renderItem.baseVertex;
        meshInstance.mesh.baseIndex = renderItem.baseIndex;
        meshInstance.mesh.vertexCount = renderItem.vertexCount;
        meshInstance.mesh.indexCount = renderItem.indexCount;
        meshInstance.material.blendingMode = renderItem.blendingMode;
        meshInstance.material.materialIndex = renderItem.materialIndex;
        return meshInstance;
    }

    VkGeometryFlagsKHR GetRayQueryGeometryFlags(const RenderItem& renderItem) {
        RayQueryMaterial material{};
        material.blendingMode = renderItem.blendingMode;
        material.materialIndex = renderItem.materialIndex;
        return GetRayQueryGeometryFlags(material);
    }

    VkGeometryFlagsKHR GetRayQueryGeometryFlags(const RayQueryMaterial& material) {
        BlendingMode blendingMode = static_cast<BlendingMode>(material.blendingMode);
        bool requiresCandidateProcessing = material.materialIndex < 0 || blendingMode == BlendingMode::ALPHA_DISCARD || blendingMode == BlendingMode::HAIR_UNDER_LAYER || blendingMode == BlendingMode::HAIR || blendingMode == BlendingMode::MIRROR;
        return requiresCandidateProcessing ? 0 : VK_GEOMETRY_OPAQUE_BIT_KHR;
    }

    VkAccelerationStructureGeometryKHR CreateTriangleGeometry(uint64_t vertexBufferAddress, uint64_t indexBufferAddress, const RayQueryMesh& mesh, VkGeometryFlagsKHR geometryFlags, uint64_t transformAddress) {
        // baseVertex and baseIndex are baked into the device addresses
        VkDeviceOrHostAddressConstKHR vertexBufferDeviceAddress{};
        vertexBufferDeviceAddress.deviceAddress = vertexBufferAddress + static_cast<uint64_t>(mesh.baseVertex) * sizeof(Vertex);

        VkDeviceOrHostAddressConstKHR indexBufferDeviceAddress{};
        indexBufferDeviceAddress.deviceAddress = indexBufferAddress + static_cast<uint64_t>(mesh.baseIndex) * sizeof(uint32_t);

        VkDeviceOrHostAddressConstKHR transformBufferDeviceAddress{};
        transformBufferDeviceAddress.deviceAddress = transformAddress;

        VkAccelerationStructureGeometryKHR geometry{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
        geometry.flags = geometryFlags;
        geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
        geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
        geometry.geometry.triangles.vertexData = vertexBufferDeviceAddress;
        geometry.geometry.triangles.maxVertex = mesh.vertexCount - 1;
        geometry.geometry.triangles.vertexStride = sizeof(Vertex);
        geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
        geometry.geometry.triangles.indexData = indexBufferDeviceAddress;
        geometry.geometry.triangles.transformData = transformBufferDeviceAddress;
        return geometry;
    }

    VkAccelerationStructureBuildSizesInfoKHR QueryBottomLevelBuildSize(uint64_t vertexBufferAddress, uint64_t indexBufferAddress, const std::vector<RayQueryMeshInstance>& meshInstances, VkBuildAccelerationStructureFlagsKHR flags) {
        VkAccelerationStructureBuildSizesInfoKHR sizeInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
        if (meshInstances.empty()) return sizeInfo;

        std::vector<VkAccelerationStructureGeometryKHR> geometries;
        std::vector<uint32_t> primitiveCounts;
        geometries.reserve(meshInstances.size());
        primitiveCounts.reserve(meshInstances.size());

        for (const RayQueryMeshInstance& meshInstance : meshInstances) {
            geometries.push_back(CreateTriangleGeometry(vertexBufferAddress, indexBufferAddress, meshInstance.mesh, GetRayQueryGeometryFlags(meshInstance.material)));
            primitiveCounts.push_back(meshInstance.mesh.indexCount / 3);
        }

        VkAccelerationStructureBuildGeometryInfoKHR buildInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
        buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        buildInfo.flags = flags;
        buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        buildInfo.geometryCount = static_cast<uint32_t>(geometries.size());
        buildInfo.pGeometries = geometries.data();

        vkGetAccelerationStructureBuildSizesKHR(VulkanDeviceManager::GetDevice(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo, primitiveCounts.data(), &sizeInfo);
        return sizeInfo;
    }

    VkAccelerationStructureBuildSizesInfoKHR QueryTopLevelBuildSize(uint64_t instanceBufferAddress, uint32_t instanceCount) {
        VkAccelerationStructureGeometryInstancesDataKHR instancesData{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR };
        instancesData.arrayOfPointers = VK_FALSE;
        instancesData.data.deviceAddress = instanceBufferAddress;

        VkAccelerationStructureGeometryKHR geometry{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
        geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
        geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
        geometry.geometry.instances = instancesData;

        VkAccelerationStructureBuildGeometryInfoKHR buildInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
        buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        buildInfo.geometryCount = 1;
        buildInfo.pGeometries = &geometry;

        VkAccelerationStructureBuildSizesInfoKHR sizeInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
        vkGetAccelerationStructureBuildSizesKHR(VulkanDeviceManager::GetDevice(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo, &instanceCount, &sizeInfo);
        return sizeInfo;
    }

    bool PrepareAccelerationStructure(uint64_t id, VkAccelerationStructureTypeKHR type, VkAccelerationStructureBuildSizesInfoKHR sizeInfo) {
        VulkanAccelerationStructure* accelerationStructure = VulkanResourceManager::GetAccelerationStructure(id);
        if (!accelerationStructure) return false;

        VkDevice device = VulkanDeviceManager::GetDevice();
        accelerationStructure->Cleanup();
        accelerationStructure->CreateBuffer(sizeInfo);

        VkAccelerationStructureCreateInfoKHR createInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
        createInfo.buffer = accelerationStructure->GetBuffer();
        createInfo.size = sizeInfo.accelerationStructureSize;
        createInfo.type = type;
        if (vkCreateAccelerationStructureKHR(device, &createInfo, nullptr, &accelerationStructure->m_handle) != VK_SUCCESS) return false;

        VkAccelerationStructureDeviceAddressInfoKHR addressInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR };
        addressInfo.accelerationStructure = accelerationStructure->m_handle;
        accelerationStructure->m_deviceAddress = vkGetAccelerationStructureDeviceAddressKHR(device, &addressInfo);
        return accelerationStructure->m_deviceAddress != 0;
    }

    void RecordAccelerationStructureBuildBarrier(VkCommandBuffer commandBuffer, VkPipelineStageFlags dstStageMask, VkAccessFlags dstAccessMask) {
        VkMemoryBarrier barrier{ VK_STRUCTURE_TYPE_MEMORY_BARRIER };
        barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
        barrier.dstAccessMask = dstAccessMask;
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, dstStageMask, 0, 1, &barrier, 0, nullptr, 0, nullptr);
    }

    void RecordTopLevelBuild(VkCommandBuffer commandBuffer, uint64_t tlasId, uint64_t instanceBufferAddress, uint32_t instanceCount, uint64_t scratchBufferAddress) {
        VulkanAccelerationStructure* accelerationStructure = VulkanResourceManager::GetAccelerationStructure(tlasId);
        if (!accelerationStructure || accelerationStructure->GetHandle() == VK_NULL_HANDLE) return;

        VkAccelerationStructureGeometryInstancesDataKHR instancesData{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR };
        instancesData.arrayOfPointers = VK_FALSE;
        instancesData.data.deviceAddress = instanceBufferAddress;

        VkAccelerationStructureGeometryKHR geometry{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
        geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
        geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
        geometry.geometry.instances = instancesData;

        VkAccelerationStructureBuildGeometryInfoKHR buildInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
        buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        buildInfo.dstAccelerationStructure = accelerationStructure->GetHandle();
        buildInfo.geometryCount = 1;
        buildInfo.pGeometries = &geometry;
        buildInfo.scratchData.deviceAddress = scratchBufferAddress;

        VkAccelerationStructureBuildRangeInfoKHR rangeInfo{ instanceCount, 0, 0, 0 };
        const VkAccelerationStructureBuildRangeInfoKHR* rangeInfoPtr = &rangeInfo;
        vkCmdBuildAccelerationStructuresKHR(commandBuffer, 1, &buildInfo, &rangeInfoPtr);
        accelerationStructure->m_built = true;
    }
}
