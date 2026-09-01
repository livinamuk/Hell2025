#include "vk_shader.h"
#include "shaderc/shaderc.hpp"
#include "Hell/Render/API/Vulkan/Managers/vk_device_manager.h"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <unordered_map>
#include <unordered_set>
#include <sstream>

struct VulkanShaderParseContext {
    std::unordered_set<std::string> includedPaths;
};

static std::string LTrimCopy(const std::string& s) {
    size_t i = 0;
    while (i < s.size() && (s[i] == ' ' || s[i] == '\t' || s[i] == '\r')) {
        i++;
    }
    return s.substr(i);
}

static bool StartsWith(const std::string& s, const char* prefix) {
    const size_t n = std::char_traits<char>::length(prefix);
    if (s.size() < n) return false;
    return s.compare(0, n, prefix) == 0;
}

static bool TryParseInclude(const std::string& line, std::string& outIncludeFile) {
    std::string trimmed = LTrimCopy(line);
    if (!StartsWith(trimmed, "#include")) return false;

    size_t firstQuote = trimmed.find('"');
    if (firstQuote == std::string::npos) return false;

    size_t secondQuote = trimmed.find('"', firstQuote + 1);
    if (secondQuote == std::string::npos) return false;

    outIncludeFile = trimmed.substr(firstQuote + 1, secondQuote - firstQuote - 1);
    return !outIncludeFile.empty();
}

static void StripUTF8BOMFromLine(std::string& line) {
    if (line.size() >= 3) {
        const unsigned char b0 = (unsigned char)line[0];
        const unsigned char b1 = (unsigned char)line[1];
        const unsigned char b2 = (unsigned char)line[2];
        if (b0 == 0xEF && b1 == 0xBB && b2 == 0xBF) {
            line.erase(0, 3);
        }
    }
}

static std::string NormalizePath(const std::filesystem::path& path) {
    std::error_code errorCode;
    std::filesystem::path normalized = std::filesystem::weakly_canonical(path, errorCode);
    if (errorCode) {
        normalized = path.lexically_normal();
    }
    return normalized.string();
}

static std::string ReadShaderFileWithIncludes(const std::string& filepath, VulkanShaderParseContext& context) {
    std::string line;
    std::ifstream stream(filepath);
    std::stringstream ss;

    if (!stream.is_open()) {
        std::cerr << "Could not open shader file: " << filepath << "\n";
        return "";
    }

    std::string baseDir = std::filesystem::path(filepath).parent_path().string();
    bool firstLineOfThisFile = true;

    while (getline(stream, line)) {
        if (firstLineOfThisFile) {
            StripUTF8BOMFromLine(line);
            firstLineOfThisFile = false;
        }

        std::string includeFile;
        if (TryParseInclude(line, includeFile)) {
            std::string includePath = NormalizePath(std::filesystem::path(baseDir) / includeFile);

            // Skip already included files
            if (context.includedPaths.insert(includePath).second) {
                std::string includeContent = ReadShaderFileWithIncludes(includePath, context);
                ss << includeContent << "\n";
            }
            continue;
        }

        ss << line << "\n";
    }
    return ss.str();
}

static shaderc_shader_kind GetShadercKind(VkShaderStageFlagBits stage) {
    switch (stage) {
    case VK_SHADER_STAGE_VERTEX_BIT:                  return shaderc_vertex_shader;
    case VK_SHADER_STAGE_FRAGMENT_BIT:                return shaderc_fragment_shader;
    case VK_SHADER_STAGE_COMPUTE_BIT:                 return shaderc_compute_shader;
    case VK_SHADER_STAGE_GEOMETRY_BIT:                return shaderc_geometry_shader;
    case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:    return shaderc_tess_control_shader;
    case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT: return shaderc_tess_evaluation_shader;
    case VK_SHADER_STAGE_RAYGEN_BIT_KHR:              return shaderc_raygen_shader;
    case VK_SHADER_STAGE_MISS_BIT_KHR:                return shaderc_miss_shader;
    case VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR:         return shaderc_closesthit_shader;
    default: return shaderc_glsl_infer_from_source;
    }
}

static std::vector<uint32_t> CompileGLSL(const std::string& path, VkShaderStageFlagBits stage) {
    VulkanShaderParseContext parseContext;
    std::string source = ReadShaderFileWithIncludes(path, parseContext);
    if (source.empty()) return {};

    static shaderc::Compiler compiler;
    shaderc::CompileOptions options;

    options.SetTargetSpirv(shaderc_spirv_version_1_6);
    options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
    options.SetOptimizationLevel(shaderc_optimization_level_performance);

    shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(source, GetShadercKind(stage), path.c_str(), options);

    if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
        std::cerr << "ERROR IN: " << path << "\n" << result.GetErrorMessage();
        return {};
    }

    return { result.cbegin(), result.cend() };
}

VulkanShaderModule::VulkanShaderModule(const std::string& filename, VkShaderStageFlagBits stage) {
    m_path = filename;
    m_stage = stage;
    Hotload();
}

VulkanShaderModule::VulkanShaderModule(VulkanShaderModule&& other) noexcept {
    m_module = other.m_module;
    m_stage = other.m_stage;
    m_path = std::move(other.m_path);
    other.m_module = VK_NULL_HANDLE;
}

VulkanShaderModule& VulkanShaderModule::operator=(VulkanShaderModule&& other) noexcept {
    if (this != &other) {
        m_module = other.m_module;
        m_stage = other.m_stage;
        m_path = std::move(other.m_path);
        other.m_module = VK_NULL_HANDLE;
    }
    return *this;
}

void VulkanShaderModule::Cleanup() {
    if (m_module != VK_NULL_HANDLE) {
        vkDestroyShaderModule(VulkanDeviceManager::GetDevice(), m_module, nullptr);
        m_module = VK_NULL_HANDLE;
    }
}

bool VulkanShaderModule::Hotload() {
    VkShaderModule newModule = VK_NULL_HANDLE;
    if (!CreateModule(newModule)) {
        return false;
    }

    Cleanup();
    m_module = newModule;
    return true;
}

bool VulkanShaderModule::CreateModule(VkShaderModule& module) const {
    module = VK_NULL_HANDLE;

    std::vector<uint32_t> spirv = CompileGLSL(m_path, m_stage);
    if (spirv.empty()) {
        return false;
    }

    VkDevice device = VulkanDeviceManager::GetDevice();

    VkShaderModuleCreateInfo createInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
    createInfo.codeSize = spirv.size() * sizeof(uint32_t);
    createInfo.pCode = spirv.data();

    if (vkCreateShaderModule(device, &createInfo, nullptr, &module) != VK_SUCCESS) {
        std::cerr << "Failed to create shader module: " << m_path << "\n";
        return false;
    }

    VkDebugUtilsObjectNameInfoEXT nameInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
    nameInfo.objectType = VK_OBJECT_TYPE_SHADER_MODULE;
    nameInfo.objectHandle = (uint64_t)module;
    nameInfo.pObjectName = m_path.c_str();
    auto setDebugName = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetDeviceProcAddr(device, "vkSetDebugUtilsObjectNameEXT"));
    if (setDebugName) setDebugName(device, &nameInfo);

    return true;
}

VulkanShader::VulkanShader(const std::vector<std::string>& filenames) {
    static const std::unordered_map<std::string, VkShaderStageFlagBits> shaderTypeMap = {
        {".vert", VK_SHADER_STAGE_VERTEX_BIT},
        {".frag", VK_SHADER_STAGE_FRAGMENT_BIT},
        {".comp", VK_SHADER_STAGE_COMPUTE_BIT},
        {".geom", VK_SHADER_STAGE_GEOMETRY_BIT},
        {".tesc", VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT},
        {".tese", VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT},
        {".rgen", VK_SHADER_STAGE_RAYGEN_BIT_KHR},
        {".rmiss", VK_SHADER_STAGE_MISS_BIT_KHR},
        {".rchit", VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR}
    };

    for (const std::string& filename : filenames) {
        std::string extension = std::filesystem::path(filename).extension().string();

        if (shaderTypeMap.contains(extension)) {
            std::string fullPath = "res/shaders/Vulkan/" + filename;

            if (std::filesystem::exists(fullPath)) {
                m_modules.emplace_back(VulkanShaderModule(fullPath, shaderTypeMap.at(extension)));
            }
            else {
                std::cerr << "SHADER FILE NOT FOUND: " << fullPath << "\n";
            }
        }
    }
}

VulkanShader::VulkanShader(VulkanShader&& other) noexcept {
    m_modules = std::move(other.m_modules);
}

VulkanShader& VulkanShader::operator=(VulkanShader&& other) noexcept {
    if (this != &other) {
        m_modules = std::move(other.m_modules);
    }
    return *this;
}

std::vector<VkPipelineShaderStageCreateInfo> VulkanShader::GetStageCreateInfos() const {
    std::vector<VkPipelineShaderStageCreateInfo> infos;
    infos.reserve(m_modules.size());

    for (const VulkanShaderModule& module : m_modules) {
        if (module.GetModule() == VK_NULL_HANDLE) {
            continue;
        }

        VkPipelineShaderStageCreateInfo info{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
        info.stage = module.GetStage();
        info.module = module.GetModule();
        info.pName = m_entryPoint;
        infos.push_back(info);
    }

    return infos;
}

VkPipelineShaderStageCreateInfo VulkanShader::GetStageCreateInfo(VkShaderStageFlagBits stage) const {
    for (const auto& module : m_modules) {
        if (module.GetStage() == stage) {
            VkPipelineShaderStageCreateInfo info = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
            info.stage = module.GetStage();
            info.module = module.GetModule();
            info.pName = m_entryPoint;
            return info;
        }
    }
    return {}; // Returns empty/null if stage not found
}

void VulkanShader::Cleanup() {
    for (auto& module : m_modules) {
        module.Cleanup();
    }
    m_modules.clear();
}

bool VulkanShader::Hotload() {
    std::vector<VkShaderModule> newModules;
    newModules.reserve(m_modules.size());

    for (const VulkanShaderModule& module : m_modules) {
        VkShaderModule newModule = VK_NULL_HANDLE;

        if (!module.CreateModule(newModule)) {
            VkDevice device = VulkanDeviceManager::GetDevice();
            for (VkShaderModule createdModule : newModules) {
                vkDestroyShaderModule(device, createdModule, nullptr);
            }
            return false;
        }

        newModules.push_back(newModule);
    }

    for (size_t i = 0; i < m_modules.size(); i++) {
        m_modules[i].Cleanup();
        m_modules[i].m_module = newModules[i];
    }

    return true;
}

std::vector<std::string> VulkanShader::GetPaths() const {
    std::vector<std::string> paths;
    paths.reserve(m_modules.size());

    for (const VulkanShaderModule& module : m_modules) {
        paths.push_back(module.GetPath());
    }

    return paths;
}

VkShaderModule VulkanShader::GetVertexShader() {
    for (auto& module : m_modules) {
        if (module.GetStage() == VK_SHADER_STAGE_VERTEX_BIT) return module.GetModule();
    }
    return VK_NULL_HANDLE;
}

VkShaderModule VulkanShader::GetFragmentShader() {
    for (auto& module : m_modules) {
        if (module.GetStage() == VK_SHADER_STAGE_FRAGMENT_BIT) return module.GetModule();
    }
    return VK_NULL_HANDLE;
}

VkShaderModule VulkanShader::GetComputeShader() {
    for (auto& module : m_modules) {
        if (module.GetStage() == VK_SHADER_STAGE_COMPUTE_BIT) return module.GetModule();
    }
    return VK_NULL_HANDLE;
}

VkShaderModule VulkanShader::GetTesselationControlShader() {
    for (auto& module : m_modules) {
        if (module.GetStage() == VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT) return module.GetModule();
    }
    return VK_NULL_HANDLE;
}

VkShaderModule VulkanShader::GetTesselationEvaluationShader() {
    for (auto& module : m_modules) {
        if (module.GetStage() == VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT) return module.GetModule();
    }
    return VK_NULL_HANDLE;
}

size_t VulkanShaderModule::GetCPUAllocatedByteCount() const {
    return sizeof(VulkanShaderModule) + m_path.capacity();
}

size_t VulkanShaderModule::GetGPUAllocatedByteCount() const {
    return 0;
}

size_t VulkanShader::GetCPUAllocatedByteCount() const {
    size_t byteCount = sizeof(VulkanShader);

    if (m_modules.capacity() > m_modules.size()) {
        byteCount += (m_modules.capacity() - m_modules.size()) * sizeof(VulkanShaderModule);
    }

    for (const VulkanShaderModule& module : m_modules) {
        byteCount += module.GetCPUAllocatedByteCount();
    }

    return byteCount;
}

size_t VulkanShader::GetGPUAllocatedByteCount() const {
    size_t byteCount = 0;

    for (const VulkanShaderModule& module : m_modules) {
        byteCount += module.GetGPUAllocatedByteCount();
    }

    return byteCount;
}
