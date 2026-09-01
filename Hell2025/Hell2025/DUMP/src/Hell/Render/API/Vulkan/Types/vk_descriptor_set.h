#pragma once
#include "Hell/Render/API/Vulkan/vk_common.h"
#include <cstddef>
#include <vector>

enum class DescriptorSetLifetime {
    STATIC,
    PER_FRAME
};

struct VulkanDescriptorSet {
    VulkanDescriptorSet() = default;
    VulkanDescriptorSet(VkDescriptorSetLayout layout);

    VulkanDescriptorSet(const VulkanDescriptorSet&) = delete;
    VulkanDescriptorSet& operator=(const VulkanDescriptorSet&) = delete;
    VulkanDescriptorSet(VulkanDescriptorSet&&) noexcept = default;
    VulkanDescriptorSet& operator=(VulkanDescriptorSet&&) noexcept = default;

    void Cleanup();
    void WriteBuffer(uint32_t binding, VkBuffer buffer, VkDeviceSize range, VkDescriptorType type, uint32_t arrayElement = 0);
    void WriteImage(uint32_t binding, VkImageView imageView, VkSampler sampler, VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VkDescriptorType type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, uint32_t arrayElement = 0);
    void WriteAccelerationStructure(uint32_t binding, VkAccelerationStructureKHR accelerationStructure, uint32_t arrayElement = 0);
    void Update();

    VkDescriptorSet GetHandle() const           { return m_handle; }
    const VkDescriptorSet* GetHandlePtr() const { return &m_handle; }
    size_t GetCPUAllocatedByteCount() const;
    size_t GetGPUAllocatedByteCount() const;

private:
    VkDescriptorSet m_handle = VK_NULL_HANDLE;

    std::vector<VkWriteDescriptorSet> m_writes;
    std::vector<VkDescriptorBufferInfo> m_bufferInfos;
    std::vector<VkDescriptorImageInfo> m_imageInfos;
    std::vector<VkAccelerationStructureKHR> m_accelerationStructures;
    std::vector<VkWriteDescriptorSetAccelerationStructureKHR> m_asInfos;
};

struct VulkanDescriptorSetResource {
    VulkanDescriptorSetResource() = default;
    VulkanDescriptorSetResource(VkDescriptorSetLayoutCreateInfo layoutInfo, DescriptorSetLifetime lifetime);

    VulkanDescriptorSetResource(const VulkanDescriptorSetResource&) = delete;
    VulkanDescriptorSetResource& operator=(const VulkanDescriptorSetResource&) = delete;
    VulkanDescriptorSetResource(VulkanDescriptorSetResource&&) noexcept = default;
    VulkanDescriptorSetResource& operator=(VulkanDescriptorSetResource&&) noexcept = default;

    void Cleanup();

    VulkanDescriptorSet& GetSet(uint32_t frameIndex = 0);
    const VulkanDescriptorSet& GetSet(uint32_t frameIndex = 0) const;

    VkDescriptorSetLayout GetLayout() const   { return m_layout; }
    DescriptorSetLifetime GetLifetime() const { return m_lifetime; }
    size_t GetCPUAllocatedByteCount() const;
    size_t GetGPUAllocatedByteCount() const;

private:
    VkDescriptorSetLayout m_layout = VK_NULL_HANDLE;
    DescriptorSetLifetime m_lifetime = DescriptorSetLifetime::STATIC;
    std::vector<VulkanDescriptorSet> m_sets;
};
