#pragma once
#include "Hell/Render/API/Vulkan/vk_common.h"
#include "Hell/Render/API/Vulkan/Types/vk_buffer.h"

struct VulkanAccelerationStructure {
	VulkanAccelerationStructure() = default;
	VulkanAccelerationStructure(const VulkanAccelerationStructure&) = delete;
	VulkanAccelerationStructure& operator=(const VulkanAccelerationStructure&) = delete;
	VulkanAccelerationStructure(VulkanAccelerationStructure&&) noexcept = default;
	VulkanAccelerationStructure& operator=(VulkanAccelerationStructure&&) noexcept = default;
	~VulkanAccelerationStructure() = default;

	void CreateBuffer(VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo);
	void CreateBLAS(VulkanBuffer& vertexBuffer, VulkanBuffer& indexBuffer, VulkanBuffer transformBuffer, uint32_t vertexCount, uint32_t indexCount);
	void Cleanup();

	VkAccelerationStructureKHR GetHandle() const	{ return m_handle; }
	VkBuffer GetBuffer() const						{ return m_buffer.GetBuffer(); }
	uint64_t GetDeviceAddress() const				{ return m_deviceAddress; }
	size_t GetCPUAllocatedByteCount() const;
	size_t GetGPUAllocatedByteCount() const;

//private:
	VkAccelerationStructureKHR m_handle = VK_NULL_HANDLE;
	uint64_t m_deviceAddress = 0;
	bool m_built = false;
	VulkanBuffer m_buffer;

private:
	VulkanBuffer m_scratchBuffer;

	void CreateScratchBuffer(VkDeviceSize size);
};
