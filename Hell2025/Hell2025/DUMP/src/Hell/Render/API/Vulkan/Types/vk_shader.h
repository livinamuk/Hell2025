#pragma once

#include "Hell/Render/API/Vulkan/vk_common.h"
#include <cstddef>
#include <string>
#include <vector>

struct VulkanShaderModule {
    VulkanShaderModule() = default;
    VulkanShaderModule(const std::string& filename, VkShaderStageFlagBits stage);
    VulkanShaderModule(const VulkanShaderModule&) = delete;
    VulkanShaderModule& operator=(const VulkanShaderModule&) = delete;
    VulkanShaderModule(VulkanShaderModule&& other) noexcept;
    VulkanShaderModule& operator=(VulkanShaderModule&& other) noexcept;

    void Cleanup();
    bool Hotload();

    VkShaderModule GetModule() const { return m_module; }
    VkShaderStageFlagBits GetStage() const { return m_stage; }
    const std::string& GetPath() const { return m_path; }
    size_t GetCPUAllocatedByteCount() const;
    size_t GetGPUAllocatedByteCount() const;

private:
    bool CreateModule(VkShaderModule& module) const;

    friend struct VulkanShader;

    VkShaderModule m_module = VK_NULL_HANDLE;
    VkShaderStageFlagBits m_stage = VK_SHADER_STAGE_ALL;
    std::string m_path;
};

struct VulkanShader {
    VulkanShader() = default;
    VulkanShader(const std::vector<std::string>& filenames);
    VulkanShader(const VulkanShader&) = delete;
    VulkanShader& operator=(const VulkanShader&) = delete;
    VulkanShader(VulkanShader&& other) noexcept;
    VulkanShader& operator=(VulkanShader&& other) noexcept;

    std::vector<VkPipelineShaderStageCreateInfo> GetStageCreateInfos() const;
    VkPipelineShaderStageCreateInfo GetStageCreateInfo(VkShaderStageFlagBits stage) const;
    void Cleanup();
    bool Hotload();
    std::vector<std::string> GetPaths() const;
    VkShaderModule GetVertexShader();
    VkShaderModule GetFragmentShader();
    VkShaderModule GetComputeShader();
    VkShaderModule GetTesselationControlShader();
    VkShaderModule GetTesselationEvaluationShader();
    size_t GetCPUAllocatedByteCount() const;
    size_t GetGPUAllocatedByteCount() const;

private:
    std::vector<VulkanShaderModule> m_modules;
    const char* m_entryPoint = "main";
};
