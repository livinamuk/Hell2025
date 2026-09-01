#pragma once
#include "Hell/Render/API/Vulkan/vk_common.h"
#include "Hell/Render/API/Vulkan/Types/vk_buffer.h"

#include <cstddef>
#include <string>
#include <vector>

struct VulkanShaderBindingTable{
    VulkanBuffer buffer;
    VkStridedDeviceAddressRegionKHR raygen{};
    VkStridedDeviceAddressRegionKHR miss{};
    VkStridedDeviceAddressRegionKHR hit{};
    VkStridedDeviceAddressRegionKHR callable{};
    size_t GetCPUAllocatedByteCount() const;
    size_t GetGPUAllocatedByteCount() const;
};

struct VulkanRaytracingPipeline {
    void AddDescriptorSetLayout(VkDescriptorSetLayout layout);
    void AddPushConstant(uint32_t size, VkShaderStageFlags stageFlags);
    void SetMaxRecursionDepth(uint32_t maxRecursionDepth);
    void AddRayGen(const std::string& shaderName);
    void AddMiss(const std::string& shaderName);
    void AddClosestHit(const std::string& shaderName);
    bool Build();
    void Cleanup();

    VkPipeline GetHandle() const                                  { return m_handle; }
    VkPipelineLayout GetLayout() const                            { return m_layout; }
    const VulkanShaderBindingTable& GetShaderBindingTable() const { return m_shaderBindingTable; }
    size_t GetCPUAllocatedByteCount() const;
    size_t GetGPUAllocatedByteCount() const;

private:
    VkPipeline m_handle = VK_NULL_HANDLE;
    VkPipelineLayout m_layout = VK_NULL_HANDLE;
    uint32_t m_maxRecursionDepth = 1;

    std::vector<VkDescriptorSetLayout> m_descriptorLayouts;
    std::vector<VkPushConstantRange> m_pushConstants;
    std::vector<VkPipelineShaderStageCreateInfo> m_stages;
    std::vector<VkRayTracingShaderGroupCreateInfoKHR> m_groups;

    VulkanShaderBindingTable m_shaderBindingTable;

};
