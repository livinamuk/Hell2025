#include "vk_raytracing_pipeline.h"

#include "Hell/Render/API/Vulkan/Managers/vk_device_manager.h"
#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"

#include <cstring>
#include <iostream>

static bool CheckResult(VkResult result, const std::string& message) {
    if (result != VK_SUCCESS) {
        std::cerr << "[Vulkan Raytracing Pipeline Error] " << message << " Result: " << result << "\n";
        return false;
    }
    return true;
}

static VkDeviceSize AlignedSize(VkDeviceSize value, VkDeviceSize alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}

void VulkanRaytracingPipeline::AddDescriptorSetLayout(VkDescriptorSetLayout layout) {
    m_descriptorLayouts.push_back(layout);
}

void VulkanRaytracingPipeline::AddPushConstant(uint32_t size, VkShaderStageFlags stageFlags) {
    constexpr uint32_t alignment = 4;

    if (size == 0 || stageFlags == 0) {
        std::cerr << "[Vulkan Raytracing Pipeline Error] Push constants require a non-zero size and shader stage flags\n";
        return;
    }

    const uint32_t alignedSize = (size + alignment - 1) & ~(alignment - 1);
    const uint32_t offset = m_pushConstants.empty() ? 0 : m_pushConstants.back().offset + m_pushConstants.back().size;

    VkPushConstantRange range{};
    range.offset = offset;
    range.size = alignedSize;
    range.stageFlags = stageFlags;
    m_pushConstants.push_back(range);
}

void VulkanRaytracingPipeline::SetMaxRecursionDepth(uint32_t maxRecursionDepth) {
    if (maxRecursionDepth == 0) {
        std::cerr << "[Vulkan Raytracing Pipeline Error] Maximum recursion depth must be greater than zero\n";
        return;
    }

    m_maxRecursionDepth = maxRecursionDepth;
}

void VulkanRaytracingPipeline::AddRayGen(const std::string& shaderName) {
    VulkanShader* shader = VulkanResourceManager::GetShader(shaderName);
    if (!shader) {
        std::cerr << "[Vulkan Raytracing Pipeline Error] Missing raygen shader: " << shaderName << "\n";
        return;
    }

    VkPipelineShaderStageCreateInfo stage = shader->GetStageCreateInfo(VK_SHADER_STAGE_RAYGEN_BIT_KHR);
    if (stage.module == VK_NULL_HANDLE) return;

    m_stages.push_back(stage);

    VkRayTracingShaderGroupCreateInfoKHR group{ VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR };
    group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    group.generalShader = (uint32_t)m_stages.size() - 1;
    group.closestHitShader = VK_SHADER_UNUSED_KHR;
    group.anyHitShader = VK_SHADER_UNUSED_KHR;
    group.intersectionShader = VK_SHADER_UNUSED_KHR;
    m_groups.push_back(group);
}

void VulkanRaytracingPipeline::AddMiss(const std::string& shaderName) {
    VulkanShader* shader = VulkanResourceManager::GetShader(shaderName);
    if (!shader) {
        std::cerr << "[Vulkan Raytracing Pipeline Error] Missing miss shader: " << shaderName << "\n";
        return;
    }

    VkPipelineShaderStageCreateInfo stage = shader->GetStageCreateInfo(VK_SHADER_STAGE_MISS_BIT_KHR);
    if (stage.module == VK_NULL_HANDLE) return;

    m_stages.push_back(stage);

    VkRayTracingShaderGroupCreateInfoKHR group{ VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR };
    group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    group.generalShader = (uint32_t)m_stages.size() - 1;
    group.closestHitShader = VK_SHADER_UNUSED_KHR;
    group.anyHitShader = VK_SHADER_UNUSED_KHR;
    group.intersectionShader = VK_SHADER_UNUSED_KHR;
    m_groups.push_back(group);
}

void VulkanRaytracingPipeline::AddClosestHit(const std::string& shaderName) {
    VulkanShader* shader = VulkanResourceManager::GetShader(shaderName);
    if (!shader) {
        std::cerr << "[Vulkan Raytracing Pipeline Error] Missing closest-hit shader: " << shaderName << "\n";
        return;
    }

    VkPipelineShaderStageCreateInfo stage = shader->GetStageCreateInfo(VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
    if (stage.module == VK_NULL_HANDLE) return;

    m_stages.push_back(stage);

    VkRayTracingShaderGroupCreateInfoKHR group{ VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR };
    group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
    group.generalShader = VK_SHADER_UNUSED_KHR;
    group.closestHitShader = (uint32_t)m_stages.size() - 1;
    group.anyHitShader = VK_SHADER_UNUSED_KHR;
    group.intersectionShader = VK_SHADER_UNUSED_KHR;
    m_groups.push_back(group);
}

bool VulkanRaytracingPipeline::Build() {
    VkDevice device = VulkanDeviceManager::GetDevice();
    const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& rtProperties = VulkanDeviceManager::GetRayTracingPipelineProperties();

    Cleanup();

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = (uint32_t)m_descriptorLayouts.size();
    layoutInfo.pSetLayouts = m_descriptorLayouts.data();
    layoutInfo.pushConstantRangeCount = (uint32_t)m_pushConstants.size();
    layoutInfo.pPushConstantRanges = m_pushConstants.data();

    if (!CheckResult(vkCreatePipelineLayout(device, &layoutInfo, nullptr, &m_layout), "Failed to create pipeline layout")) {
        return false;
    }

    VkRayTracingPipelineCreateInfoKHR pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
    pipelineInfo.stageCount = (uint32_t)m_stages.size();
    pipelineInfo.pStages = m_stages.data();
    pipelineInfo.groupCount = (uint32_t)m_groups.size();
    pipelineInfo.pGroups = m_groups.data();
    pipelineInfo.maxPipelineRayRecursionDepth = m_maxRecursionDepth;
    pipelineInfo.layout = m_layout;

    if (!CheckResult(vkCreateRayTracingPipelinesKHR(device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_handle), "Failed to create raytracing pipeline")) {
        return false;
    }

    const uint32_t handleSize = rtProperties.shaderGroupHandleSize;
    const uint32_t handleSizeAligned = AlignedSize(handleSize, rtProperties.shaderGroupHandleAlignment);
    const uint32_t groupCount = (uint32_t)m_groups.size();
    const uint32_t shaderHandleStorageSize = groupCount * handleSizeAligned;

    std::vector<uint8_t> shaderHandleStorage(shaderHandleStorageSize);
    if (!CheckResult(vkGetRayTracingShaderGroupHandlesKHR(device, m_handle, 0, groupCount, shaderHandleStorageSize, shaderHandleStorage.data()), "Failed to get shader group handles")) {
        return false;
    }

    std::vector<uint32_t> raygenGroups;
    std::vector<uint32_t> missGroups;
    std::vector<uint32_t> hitGroups;

    for (uint32_t i = 0; i < groupCount; i++) {
        if (m_groups[i].type == VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR) {
            if (raygenGroups.empty()) raygenGroups.push_back(i);
            else missGroups.push_back(i);
        }
        else if (m_groups[i].type == VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR) {
            hitGroups.push_back(i);
        }
    }

    VkDeviceSize raygenOffset = 0;
    VkDeviceSize raygenSize = handleSizeAligned * raygenGroups.size();

    VkDeviceSize missOffset = AlignedSize(raygenOffset + raygenSize, rtProperties.shaderGroupBaseAlignment);
    VkDeviceSize missSize = handleSizeAligned * missGroups.size();

    VkDeviceSize hitOffset = AlignedSize(missOffset + missSize, rtProperties.shaderGroupBaseAlignment);
    VkDeviceSize hitSize = handleSizeAligned * hitGroups.size();

    VkDeviceSize sbtSize = hitOffset + hitSize;
    std::vector<uint8_t> sbtData((size_t)sbtSize);

    auto copyGroupHandles = [&](const std::vector<uint32_t>& groupIndices, VkDeviceSize dstOffset) {
        for (uint32_t i = 0; i < groupIndices.size(); i++) {
            uint8_t* dst = sbtData.data() + dstOffset + i * handleSizeAligned;
            const uint8_t* src = shaderHandleStorage.data() + groupIndices[i] * handleSizeAligned;
            memcpy(dst, src, handleSize);
        }
        };

    copyGroupHandles(raygenGroups, raygenOffset);
    copyGroupHandles(missGroups, missOffset);
    copyGroupHandles(hitGroups, hitOffset);

    VkBufferUsageFlags usage =
        VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR |
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

    VmaAllocationCreateFlags flags =
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
        VMA_ALLOCATION_CREATE_MAPPED_BIT;

    m_shaderBindingTable.buffer = VulkanBuffer(sbtSize, usage, VMA_MEMORY_USAGE_AUTO, flags);
    m_shaderBindingTable.buffer.UpdateData(sbtData.data(), sbtSize);

    uint64_t sbtAddress = m_shaderBindingTable.buffer.GetDeviceAddress();

    m_shaderBindingTable.raygen.deviceAddress = sbtAddress + raygenOffset;
    m_shaderBindingTable.raygen.stride = handleSizeAligned;
    m_shaderBindingTable.raygen.size = raygenSize;

    m_shaderBindingTable.miss.deviceAddress = sbtAddress + missOffset;
    m_shaderBindingTable.miss.stride = handleSizeAligned;
    m_shaderBindingTable.miss.size = missSize;

    m_shaderBindingTable.hit.deviceAddress = sbtAddress + hitOffset;
    m_shaderBindingTable.hit.stride = handleSizeAligned;
    m_shaderBindingTable.hit.size = hitSize;

    m_shaderBindingTable.callable = {};

    return true;
}

void VulkanRaytracingPipeline::Cleanup() {
    VkDevice device = VulkanDeviceManager::GetDevice();

    if (m_layout != VK_NULL_HANDLE) vkDestroyPipelineLayout(device, m_layout, nullptr);
    if (m_handle != VK_NULL_HANDLE) vkDestroyPipeline(device, m_handle, nullptr);

    m_shaderBindingTable.buffer.Cleanup();
    m_shaderBindingTable.raygen = {};
    m_shaderBindingTable.miss = {};
    m_shaderBindingTable.hit = {};
    m_shaderBindingTable.callable = {};

    m_layout = VK_NULL_HANDLE;
    m_handle = VK_NULL_HANDLE;
}

size_t VulkanShaderBindingTable::GetCPUAllocatedByteCount() const {
    return sizeof(VulkanShaderBindingTable);
}

size_t VulkanShaderBindingTable::GetGPUAllocatedByteCount() const {
    return buffer.GetGPUAllocatedByteCount();
}

size_t VulkanRaytracingPipeline::GetCPUAllocatedByteCount() const {
    return sizeof(VulkanRaytracingPipeline) +
        (m_descriptorLayouts.capacity() * sizeof(VkDescriptorSetLayout)) +
        (m_pushConstants.capacity() * sizeof(VkPushConstantRange)) +
        (m_stages.capacity() * sizeof(VkPipelineShaderStageCreateInfo)) +
        (m_groups.capacity() * sizeof(VkRayTracingShaderGroupCreateInfoKHR));
}

size_t VulkanRaytracingPipeline::GetGPUAllocatedByteCount() const {
    return m_shaderBindingTable.GetGPUAllocatedByteCount();
}
