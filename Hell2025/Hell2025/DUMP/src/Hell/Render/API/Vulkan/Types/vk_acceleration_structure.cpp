#pragma once
#include "vk_acceleration_structure.h"

#include "Hell/Render/API/Vulkan/Managers/vk_device_manager.h"
#include "Hell/Render/API/Vulkan/Managers/vk_command_manager.h"

#include "Hell/Render/VertexAttributes.h"

void VulkanAccelerationStructure::CreateBuffer(VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo) {
	VkBufferUsageFlags usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
	m_buffer = VulkanBuffer(buildSizeInfo.accelerationStructureSize, usage, VMA_MEMORY_USAGE_AUTO);
}

void VulkanAccelerationStructure::Cleanup() {
	if (m_handle != VK_NULL_HANDLE) {
		vkDestroyAccelerationStructureKHR(VulkanDeviceManager::GetDevice(), m_handle, nullptr);
		m_handle = VK_NULL_HANDLE;
	}
	m_buffer.Cleanup();
	m_deviceAddress = 0;
	m_built = false;
}

void VulkanAccelerationStructure::CreateBLAS(VulkanBuffer& vertexBuffer, VulkanBuffer& indexBuffer, VulkanBuffer transformBuffer, uint32_t vertexCount, uint32_t indexCount) {
	if (m_buffer.GetBuffer() != VK_NULL_HANDLE) {
		m_buffer.Cleanup();
	}

    VkDevice device = VulkanDeviceManager::GetDevice();

    VkDeviceOrHostAddressConstKHR vertexBufferDeviceAddress{};
    vertexBufferDeviceAddress.deviceAddress = vertexBuffer.GetDeviceAddress();

    VkDeviceOrHostAddressConstKHR indexBufferDeviceAddress{};
    indexBufferDeviceAddress.deviceAddress = indexBuffer.GetDeviceAddress();

    VkDeviceOrHostAddressConstKHR transformBufferDeviceAddress{};
    transformBufferDeviceAddress.deviceAddress = transformBuffer.GetDeviceAddress();

    // Standard geometry setup for triangles
    VkAccelerationStructureGeometryKHR geometry{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
    geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
    geometry.geometry.triangles.vertexData = vertexBufferDeviceAddress;
    geometry.geometry.triangles.maxVertex = vertexCount > 0 ? vertexCount - 1 : 0;
    geometry.geometry.triangles.vertexStride = sizeof(Vertex);
    geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
    geometry.geometry.triangles.indexData = indexBufferDeviceAddress;
    geometry.geometry.triangles.transformData = transformBufferDeviceAddress;

    VkAccelerationStructureBuildGeometryInfoKHR buildInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
    buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    buildInfo.geometryCount = 1;
    buildInfo.pGeometries = &geometry;

    const uint32_t numTriangles = indexCount / 3;
    VkAccelerationStructureBuildSizesInfoKHR sizeInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
    vkGetAccelerationStructureBuildSizesKHR(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo, &numTriangles, &sizeInfo);

    // Create the BLAS container and its internal VulkanBuffer
    CreateBuffer(sizeInfo);

    VkAccelerationStructureCreateInfoKHR createInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
    createInfo.buffer = GetBuffer();
    createInfo.size = sizeInfo.accelerationStructureSize;
    createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    vkCreateAccelerationStructureKHR(device, &createInfo, nullptr, &m_handle);

    // Managed scratch buffer (VulkanBuffer) still used for the build process
    CreateScratchBuffer(sizeInfo.buildScratchSize);

    buildInfo.dstAccelerationStructure = m_handle;
    buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    buildInfo.scratchData.deviceAddress = m_scratchBuffer.GetDeviceAddress();

    VkAccelerationStructureBuildRangeInfoKHR rangeInfo{ numTriangles, 0, 0, 0 };
    std::vector<VkAccelerationStructureBuildRangeInfoKHR*> pRangeInfos = { &rangeInfo };

    VulkanCommandManager::SubmitImmediate([&](VkCommandBuffer cmd) {
        vkCmdBuildAccelerationStructuresKHR(cmd, 1, &buildInfo, pRangeInfos.data());
        });

    // Get final address for the BLAS handle itself
    VkAccelerationStructureDeviceAddressInfoKHR addressInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR };
    addressInfo.accelerationStructure = m_handle;
    m_deviceAddress = vkGetAccelerationStructureDeviceAddressKHR(device, &addressInfo);

    // Cleanup
    m_scratchBuffer.Cleanup();
    m_built = true;
}

void VulkanAccelerationStructure::CreateScratchBuffer(VkDeviceSize size) {
    if (m_scratchBuffer.GetBuffer() != VK_NULL_HANDLE) m_scratchBuffer.Cleanup();

    VkBufferUsageFlags usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    m_scratchBuffer = VulkanBuffer(size, usage, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
}

size_t VulkanAccelerationStructure::GetCPUAllocatedByteCount() const {
    return sizeof(VulkanAccelerationStructure);
}

size_t VulkanAccelerationStructure::GetGPUAllocatedByteCount() const {
    return m_buffer.GetGPUAllocatedByteCount() + m_scratchBuffer.GetGPUAllocatedByteCount();
}
