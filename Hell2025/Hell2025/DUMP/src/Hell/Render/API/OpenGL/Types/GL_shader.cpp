#include "GL_shader.h"
#include <glad/gl.h>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_set>
#include <algorithm>
#include "Hell/Logging.h"

struct ShaderParseContext {
    std::unordered_set<std::string> includedPaths;
    bool rootVersionSeen = false;
};

void InsertDefines(std::string& source, const std::vector<std::string>& defines);
void ParseFile(const std::string& filepath, std::string& outputString, std::vector<std::string>& lineToFile, ShaderParseContext& context, const std::string& rootFilepath);
void StripUTF8BOMFromLine(std::string& line);
int GetErrorLineNumber(const std::string& error);
std::string GetErrorMessage(const std::string& line);
std::string GetLinkingErrors(uint32_t shader);
std::string GetShaderCompileErrors(uint32_t shader, const std::string& filename, const std::vector<std::string>& lineToFile);

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
    if (!StartsWith(trimmed, "#include")) {
        return false;
    }
    size_t firstQuote = trimmed.find('"');
    if (firstQuote == std::string::npos) return false;
    size_t secondQuote = trimmed.find('"', firstQuote + 1);
    if (secondQuote == std::string::npos) return false;
    outIncludeFile = trimmed.substr(firstQuote + 1, secondQuote - firstQuote - 1);
    return !outIncludeFile.empty();
}

OpenGLShader::OpenGLShader(std::vector<std::string> shaderPaths, const std::string subDirectory, const std::vector<std::string>& defines) {
    m_defines = defines;
    m_shaderPaths = shaderPaths;
    m_subDirectory = subDirectory;

    Load(m_shaderPaths);
}

bool OpenGLShader::Load(std::vector<std::string> shaderPaths) {
    // Compile shader modules
    std::vector<OpenGLShaderModule> modules;
    for (std::string& shaderPath : shaderPaths) {
        std::string fullPath = m_subDirectory.empty() ? shaderPath : m_subDirectory + "/" + shaderPath;
        modules.push_back(OpenGLShaderModule(fullPath, m_defines));
    }

    // Print compilation errors
    bool errorsFound = false;
    for (OpenGLShaderModule& module : modules) {
        if (module.CompilationFailed()) {
            errorsFound = true;
            break;
        }
    }
    if (errorsFound) {
        std::cout << "\n-------------------------------------------------------------------------\n\n";
        for (OpenGLShaderModule& module : modules) {
            if (module.CompilationFailed()) {
                std::cout << " COMPILATION ERROR: " << module.GetFilename() << "\n\n";
                std::cout << module.GetErrors() << "\n";
            }
            glDeleteShader(module.GetHandle());
        }
        std::cout << "-------------------------------------------------------------------------\n";
        return false;
    }
    // Attempt to link
    uint32_t tempHandle = glCreateProgram();
    for (OpenGLShaderModule& module : modules) {
        glAttachShader(tempHandle, module.GetHandle());
    }
    glLinkProgram(tempHandle);
    std::string linkingErrors = GetLinkingErrors(tempHandle);

    // Print any errors
    if (linkingErrors.length()) {
        std::cout << "\n-------------------------------------------------------------------------\n\n";
        std::cout << " LINKING ERROR: ";
        for (int i = 0; i < modules.size(); i++) {
            std::cout << modules[i].GetFilename();
            if (i != modules.size() - 1) {
                std::cout << "/";
            }
        }
        std::cout << linkingErrors << "\n";
        std::cout << "-------------------------------------------------------------------------\n";
        for (OpenGLShaderModule& module : modules) {
            glDeleteShader(module.GetHandle());
            //Logging::Debug() << module.GetFinalSource() << "\n\n";
        }
        return false;
    }
    // Otherwise store the handle to the compiled shader
    else {
        if (m_handle) {
            glDeleteProgram(m_handle);
        }
        m_handle = tempHandle;
        m_uniformLocations.clear();
    }
    for (OpenGLShaderModule& module : modules) {
        glDeleteShader(module.GetHandle());
    }
    return true;
}

bool OpenGLShader::Hotload() {
    return Load(m_shaderPaths);
}

int32_t OpenGLShader::GetUniformLocation(const std::string& name) {
    if (m_uniformLocations.find(name) == m_uniformLocations.end()) {
        m_uniformLocations[name] = glGetUniformLocation(m_handle, name.c_str());
    }
    return m_uniformLocations[name];
}

uint32_t OpenGLShader::GetHandle() {
    return m_handle;
}

size_t OpenGLShader::GetCPUAllocatedByteCount() const {
    size_t byteCount = m_subDirectory.capacity();

    byteCount += m_defines.capacity() * sizeof(std::string);
    for (const std::string& define : m_defines) {
        byteCount += define.capacity();
    }

    byteCount += m_shaderPaths.capacity() * sizeof(std::string);
    for (const std::string& shaderPath : m_shaderPaths) {
        byteCount += shaderPath.capacity();
    }

    byteCount += m_uniformLocations.size() * (sizeof(std::string) + sizeof(int32_t));
    for (const auto& uniformLocation : m_uniformLocations) {
        byteCount += uniformLocation.first.capacity();
    }

    return byteCount;
}

OpenGLShaderModule::OpenGLShaderModule(const std::string& filename, const std::vector<std::string>& defines) {
    // Parse the source code
    ShaderParseContext context;
    std::vector<std::string> lineMap;
    std::string prasedShaderSource = "";

    ParseFile("res/shaders/OpenGL/" + filename, prasedShaderSource, lineMap, context, "res/shaders/OpenGL/" + filename);
    InsertDefines(prasedShaderSource, defines);

    // Get type based on extension
    std::string extension = std::filesystem::path(filename).extension().string();
    static const std::unordered_map<std::string, uint32_t> shaderTypeMap = {
        {".vert", GL_VERTEX_SHADER},
        {".frag", GL_FRAGMENT_SHADER},
        {".geom", GL_GEOMETRY_SHADER},
        {".tesc", GL_TESS_CONTROL_SHADER},
        {".tese", GL_TESS_EVALUATION_SHADER},
        {".comp", GL_COMPUTE_SHADER},
        {".task", GL_TASK_SHADER_EXT},
        {".mesh", GL_MESH_SHADER_EXT}
    };
    uint32_t shaderType = shaderTypeMap.contains(extension) ? shaderTypeMap.at(extension) : GL_NONE;

    // Check for errors
    const char* shaderCode = prasedShaderSource.c_str();
    m_handle = glCreateShader(shaderType);
    glShaderSource(m_handle, 1, &shaderCode, NULL);
    glCompileShader(m_handle);
    m_errors = GetShaderCompileErrors(m_handle, filename, lineMap);
    m_filename = filename;

    // Keep the line map (your GetLineMap() currently returned an uninitialized member)
    m_lineMap = lineMap;

    // Store for debug output
    m_finalSource = prasedShaderSource;
}

uint32_t OpenGLShaderModule::GetHandle() {
    return m_handle;
}

bool OpenGLShaderModule::CompilationFailed() {
    return m_errors.length();
}

std::string& OpenGLShaderModule::GetFilename() {
    return m_filename;
}

std::string& OpenGLShaderModule::GetErrors() {
    return m_errors;
}

std::vector<std::string>& OpenGLShaderModule::GetLineMap() {
    return m_lineMap;
}

static void ParseFile(const std::string& filepath, std::string& outputString, std::vector<std::string>& lineToFile, ShaderParseContext& context, const std::string& rootFilepath) {
    std::string baseDir = std::filesystem::path(filepath).parent_path().string();
    std::string filename = std::filesystem::path(filepath).filename().string();
    std::ifstream file(filepath);
    std::string line;
    bool firstLineOfThisFile = true;
    int fileLineNumber = 1;

    if (!file.is_open()) {
        std::cout << "\n-------------------------------------------------------------------------\n\n";
        std::cout << " SHADER PARSE ERROR: failed to open file: " << filepath << "\n";
        std::cout << "-------------------------------------------------------------------------\n";
        return;
    }

    while (std::getline(file, line)) {
        // Strip BOM chars
        if (firstLineOfThisFile) {
            StripUTF8BOMFromLine(line);
            firstLineOfThisFile = false;
        }

        // Handle includes
        std::string includeFile;
        if (TryParseInclude(line, includeFile)) {
            std::string includePath = std::filesystem::weakly_canonical(baseDir + "/" + includeFile).string();

            // Check if the included file is already in includedPaths
            if (context.includedPaths.insert(includePath).second) {
                ParseFile(includePath, outputString, lineToFile, context, rootFilepath);
            }

            fileLineNumber++;
            continue;
        }

        // Protect the output from accidental #version in includes.
        std::string trimmed = LTrimCopy(line);
        if (StartsWith(trimmed, "#version")) {
            if (filepath != rootFilepath) {
                std::cout << "\n-------------------------------------------------------------------------\n\n";
                std::cout << " SHADER PARSE WARNING: #version found in an included file, skipping it: " << filepath << " (line " << fileLineNumber << ")\n";
                std::cout << "-------------------------------------------------------------------------\n";
                fileLineNumber++;
                continue;
            }
            context.rootVersionSeen = true;
        }

        outputString += line + "\n";
        lineToFile.emplace_back(filename + " (line " + std::to_string(fileLineNumber) + ")");

        fileLineNumber++;
    }
}

std::string GetShaderCompileErrors(uint32_t shader, const std::string& /*filename*/, const std::vector<std::string>& lineToFile) {
    int success;
    char infoLog[1024];
    std::string result = "";
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
        // Parse error log to extract line numbers
        std::stringstream logStream(infoLog);
        std::string line;
        while (std::getline(logStream, line)) {
            if ((line.substr(0, 7) == "ERROR: ")) {
                int lineNumber = GetErrorLineNumber(line);
                if (lineNumber >= 0 && lineNumber < (int)lineToFile.size()) {
                    result += "  " + lineToFile[lineNumber] + ": " + GetErrorMessage(line) + "\n";
                }
            }
        }
    }
    return result;
}

std::string GetLinkingErrors(uint32_t programId) {
    GLint linkStatus;
    glGetProgramiv(programId, GL_LINK_STATUS, &linkStatus);

    if (linkStatus == GL_FALSE) {
        GLint logLength;
        glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &logLength);

        if (logLength > 0) {
            std::vector<char> infoLogBuffer(logLength + 1); // +1 for null terminator
            glGetProgramInfoLog(programId, logLength, NULL, &infoLogBuffer[0]);

            std::string fullLog(infoLogBuffer.data());
            std::stringstream logStream(fullLog);
            std::string line;
            std::string resultToShow = "\n";

            const std::string assemblyStartDelimiter = "-- internal assembly text --";
            bool assemblySectionEncountered = false;

            while (std::getline(logStream, line)) {
                if (assemblySectionEncountered) {
                    break;
                }

                resultToShow += "    " + line + "\n";

                // Now, check if THIS line was the delimiter
                if (line.find(assemblyStartDelimiter) != std::string::npos) {
                    resultToShow += "    (Following internal assembly text omitted for brevity)\n";
                    assemblySectionEncountered = true;
                    break;
                }
            }
            return resultToShow;
        }
        else {
            return "\n    An unknown linking error occurred (no info log available).\n";
        }
    }
    return "";
}

void InsertDefines(std::string& source, const std::vector<std::string>& defines) {
    if (defines.empty()) return;

    // Build defines string
    std::string definesBlock = "";
    for (const std::string& define : defines) {
        definesBlock += "#define " + define + "\n";
    }

    // Find the #version directive
    size_t versionPos = source.find("#version");

    if (versionPos != std::string::npos) {

        // Find the end of the version line
        size_t newlinePos = source.find('\n', versionPos);

        // Insert the defines
        if (newlinePos != std::string::npos) {
            source.insert(newlinePos + 1, definesBlock);
        }
    }
}

int GetErrorLineNumber(const std::string& error) {
    size_t firstColon = error.find(':');
    if (firstColon != std::string::npos) {
        size_t secondColon = error.find(':', firstColon + 1);
        if (secondColon != std::string::npos) {
            size_t thirdColon = error.find(':', secondColon + 1);
            if (thirdColon != std::string::npos) {
                std::string lineNumberStr = error.substr(secondColon + 1, thirdColon - secondColon - 1);
                return std::stoi(lineNumberStr);
            }
        }
    }
    return -1;
}

std::string GetErrorMessage(const std::string& line) {
    size_t firstColon = line.find(':');
    if (firstColon != std::string::npos) {
        size_t secondColon = line.find(':', firstColon + 1);
        if (secondColon != std::string::npos) {
            size_t thirdColon = line.find(':', secondColon + 1);
            if (thirdColon != std::string::npos) {
                size_t messageStart = thirdColon + 2; // Skip the colon and space
                if (messageStart < line.length()) {
                    return line.substr(messageStart);
                }
            }
        }
    }
    return ""; // Return empty string if parsing fails
}

void StripUTF8BOMFromLine(std::string& line) {
    if (line.size() >= 3) {
        const unsigned char b0 = (unsigned char)line[0];
        const unsigned char b1 = (unsigned char)line[1];
        const unsigned char b2 = (unsigned char)line[2];
        if (b0 == 0xEF && b1 == 0xBB && b2 == 0xBF) {
            line.erase(0, 3);
        }
    }
}
