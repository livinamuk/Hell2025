#pragma once
#include <glm/glm.hpp>
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <string>
#include <vector>

struct OpenGLShaderModule {
    OpenGLShaderModule(const std::string& filename, const std::vector<std::string>& defines);
    uint32_t GetHandle();
    bool CompilationFailed();
    std::string& GetFilename();
    std::string& GetErrors();
    std::vector<std::string>& GetLineMap();
    const std::string& GetFinalSource() const { return m_finalSource; }

private:
    uint32_t m_handle = 0;
    std::string m_filename = "";
    std::string m_errors = "";
    std::vector<std::string> m_lineMap;
    std::string m_finalSource = "";
};

struct OpenGLShader {
    OpenGLShader() = default;
	OpenGLShader(std::vector<std::string> shaderPaths, const std::string subDirectory, const std::vector<std::string>& defines);
    bool Load(std::vector<std::string> shaderPaths);
	bool Hotload();

    uint32_t GetHandle();
    int32_t GetUniformLocation(const std::string& name);
    const std::vector<std::string>& GetPaths() const { return m_shaderPaths; }
    size_t GetCPUAllocatedByteCount() const;

private:
    std::vector<std::string> m_defines;
    std::vector<std::string> m_shaderPaths;
    std::unordered_map<std::string, int32_t> m_uniformLocations;
    uint32_t m_handle = 0;
    std::string m_subDirectory = "";
};
