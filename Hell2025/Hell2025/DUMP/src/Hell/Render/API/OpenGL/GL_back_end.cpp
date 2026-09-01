#pragma once
#include "Unloved/Common/Types.h"
#include "GL_back_end.h"
#include "GL_resource_manager.h"
#include "GL_support.h"
#include "Hell/Backend/Integration/GLFW.h"
#include "Hell/Logging.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include <algorithm>
#include <string>
#include <iostream>
#include <utility>
#include <vector>
#include "GL_util.h"

// remove me
#include "Unloved/Render/Renderer.h"

namespace OpenGL::BackEnd {
    float g_depthClearValue = 1.0f;

    GLuint g_skinnedVertexDataVAO = 0;
    GLuint g_skinnedVertexDataVBO = 0;
    GLuint g_previousSkinnedPositionBuffer = 0;
    GLuint g_allocatedSkinnedVertexBufferSize = 0;
    std::vector<GLuint64> g_bindlessTextureIDs;

    void APIENTRY glDebugOutput(GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei /*length*/, const char* message, const void* /*userParam*/);
    void UpdateBindlessTextures();

    void Init() {

        Hell::BackEnd::GLFW::MakeContextCurrent();

        // Init glad
        if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress)) {
            std::cout << "Failed to initialize GLAD\n";
            return;
        }
        // Print some shit
        GLint major, minor;
        glGetIntegerv(GL_MAJOR_VERSION, &major);
        glGetIntegerv(GL_MINOR_VERSION, &minor);

        const GLubyte* renderer = glGetString(GL_RENDERER);

        std::cout << "\nGPU: " << renderer << "\n";
        std::cout << "GL version: " << major << "." << minor << "\n\n";

        int flags;

        glGetIntegerv(GL_CONTEXT_FLAGS, &flags);

        if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
            //std::cout << "Debug GL context enabled\n";
            glEnable(GL_DEBUG_OUTPUT);
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); // makes sure errors are displayed synchronously
            glDebugMessageCallback(glDebugOutput, nullptr);
        }
        else {
            std::cout << "Debug GL context not available\n";
        }

        // Clear screen to black
        glClear(GL_COLOR_BUFFER_BIT);

        // Match Vulkan matrix shit
        glClipControl(GL_UPPER_LEFT, GL_ZERO_TO_ONE);
        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

        OpenGL::Support::CheckSupport();
    }

    void BeginFrame() {
        UpdateBindlessTextures();
    }

    void AllocateSkinnedVertexBufferSpace(uint32_t vertexCount) {
        const uint32_t requiredSize = vertexCount * sizeof(Vertex);

        if (g_skinnedVertexDataVAO == 0) {
            glGenVertexArrays(1, &g_skinnedVertexDataVAO);
            glBindVertexArray(g_skinnedVertexDataVAO);

            glGenBuffers(1, &g_skinnedVertexDataVBO);
            glBindBuffer(GL_ARRAY_BUFFER, g_skinnedVertexDataVBO);

            glGenBuffers(1, &g_previousSkinnedPositionBuffer);

            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, (GLsizei)sizeof(Vertex), (void*)0);

            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, (GLsizei)sizeof(Vertex), (void*)offsetof(Vertex, normal));

            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, (GLsizei)sizeof(Vertex), (void*)offsetof(Vertex, uv));

            glEnableVertexAttribArray(3);
            glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, (GLsizei)sizeof(Vertex), (void*)offsetof(Vertex, tangent));

            // The element array buffer binding is part of the VAO state.
            OpenGLMeshBuffer& assetMeshBuffer = OpenGL::ResourceManager::GetMeshBuffer("AssetGeometry");
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, assetMeshBuffer.GetEBO());

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
        }

        if (g_allocatedSkinnedVertexBufferSize < requiredSize) {
            uint32_t newSize = requiredSize;

            glBindBuffer(GL_ARRAY_BUFFER, g_skinnedVertexDataVBO);
            glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)newSize, nullptr, GL_DYNAMIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, 0);

            const uint32_t previousPositionBufferSize = vertexCount * sizeof(float) * 3;
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, g_previousSkinnedPositionBuffer);
            glBufferData(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr)previousPositionBufferSize, nullptr, GL_DYNAMIC_DRAW);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

            g_allocatedSkinnedVertexBufferSize = newSize;
            std::cout << "Recreated skinned vertex buffer. Size: " << g_allocatedSkinnedVertexBufferSize << "\n";
        }
    }

    void AllocateTextureMemory(Texture& texture) {
        OpenGLTexture& glTexture = texture.GetGLTexture();
        if (glTexture.GetHandle() != 0) return;

        int width = texture.GetWidth();
        int height = texture.GetHeight();
        int levels = texture.MipmapsAreRequested() ? texture.GetMipmapLevelCount() : 1;
        GLenum internalFormat = OpenGL::Util::ImageFormatToGLInternalFormat(texture.GetImageFormat());

        glTexture.Create(width, height, internalFormat, levels);
        glTexture.SetWrapModeS(texture.GetTextureWrapModeS());
        glTexture.SetWrapModeT(texture.GetTextureWrapModeT());
        glTexture.SetMinFilter(texture.GetMinFilter());
        glTexture.SetMagFilter(texture.GetMagFilter());
        glTexture.SetBorderColor(
            texture.GetBorderColor().r,
            texture.GetBorderColor().g,
            texture.GetBorderColor().b,
            texture.GetBorderColor().a
        );
        glTexture.MakeBindlessTextureResident();
    }

    void UpdateBindlessTextures() {
        int32_t highestBindlessIndex = -1;

        for (auto& [name, texture] : Hell::ResourceManager::GetTextures()) {
            if (texture.GetBindlessIndex() < 0) {
                Logging::Error() << "OpenGL::BackEnd::UpdateBindlessTextures() failed: texture '" << texture.GetFileName() << "' has invalid bindless index " << texture.GetBindlessIndex() << "\n";
                return;
            }

            highestBindlessIndex = std::max(highestBindlessIndex, texture.GetBindlessIndex());
        }

        std::vector<GLuint64> bindlessTextureIDs(static_cast<size_t>(highestBindlessIndex + 1), 0);
        std::vector<bool> assignedSlots(bindlessTextureIDs.size(), false);

        for (auto& [name, texture] : Hell::ResourceManager::GetTextures()) {
            const int32_t bindlessIndex = texture.GetBindlessIndex();

            if (static_cast<size_t>(bindlessIndex) >= bindlessTextureIDs.size()) {
                Logging::Error() << "OpenGL::BackEnd::UpdateBindlessTextures() failed: texture '" << texture.GetFileName() << "' has out-of-range bindless index " << bindlessIndex << "\n";
                return;
            }

            if (assignedSlots[bindlessIndex]) {
                Logging::Error() << "OpenGL::BackEnd::UpdateBindlessTextures() failed: duplicate bindless index " << bindlessIndex << " for texture '" << texture.GetFileName() << "'\n";
                return;
            }

            if (texture.GetUploadState() != UploadState::UPLOADED) {
                assignedSlots[bindlessIndex] = true;
                continue;
            }

            OpenGLTexture* glTexture = OpenGL::ResourceManager::GetTexturePtr(texture.GetOpenGLId());
            if (!glTexture || glTexture->GetHandle() == 0) {
                Logging::Error() << "OpenGL::BackEnd::UpdateBindlessTextures() failed: uploaded texture '" << texture.GetFileName() << "' has no OpenGL texture\n";
                return;
            }

            bindlessTextureIDs[bindlessIndex] = glTexture->GetBindlessID();
            assignedSlots[bindlessIndex] = true;
        }

        for (size_t i = 0; i < assignedSlots.size(); i++) {
            if (!assignedSlots[i]) {
                Logging::Error() << "OpenGL::BackEnd::UpdateBindlessTextures() failed: bindless index " << i << " is unassigned\n";
                return;
            }
        }

        g_bindlessTextureIDs = std::move(bindlessTextureIDs);
    }

    void SetDepthClearValue(float value) {
        if (g_depthClearValue != value) {
            g_depthClearValue = value;
            glClearDepth(g_depthClearValue);
        }
    }

    void APIENTRY glDebugOutput(GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei /*length*/, const char* message, const void* /*userParam*/) {
        // Ignore non-significant error codes
        if (id == 131169 || // Framebuffer detailed info: The driver allocated storage for renderbuffer [X].
            id == 131185 || // Buffer detailed info: The driver is using video memory for buffer [X].
            id == 131218 || // Program/shader state performance warning: Fragment shader in program [X] is being recompiled based on state.
            id == 131204 || // Texture state usage warning: Texture [X] is base level inconsistent. Level [0] has inconsistent dimensions or formats.
            id == 131154    // Pixel-path performance warning: Pixel transfer is synchronized with 3D rendering.
            ) {
            return;
        }
        std::cout << "---------------\n";
        std::cout << "Debug message (" << id << "): " << message << "\n";
        switch (source) {
        case GL_DEBUG_SOURCE_API:             std::cout << "Source: API"; break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::cout << "Source: Window System"; break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cout << "Source: Shader Compiler"; break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:     std::cout << "Source: Third Party"; break;
        case GL_DEBUG_SOURCE_APPLICATION:     std::cout << "Source: Application"; break;
        case GL_DEBUG_SOURCE_OTHER:           std::cout << "Source: Other"; break;
        }
        std::cout << "\n";
        switch (type) {
        case GL_DEBUG_TYPE_ERROR:               std::cout << "Type: Error"; break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cout << "Type: Deprecated Behaviour"; break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::cout << "Type: Undefined Behaviour"; break;
        case GL_DEBUG_TYPE_PORTABILITY:         std::cout << "Type: Portability"; break;
        case GL_DEBUG_TYPE_PERFORMANCE:         std::cout << "Type: Performance"; break;
        case GL_DEBUG_TYPE_MARKER:              std::cout << "Type: Marker"; break;
        case GL_DEBUG_TYPE_PUSH_GROUP:          std::cout << "Type: Push Group"; break;
        case GL_DEBUG_TYPE_POP_GROUP:           std::cout << "Type: Pop Group"; break;
        case GL_DEBUG_TYPE_OTHER:               std::cout << "Type: Other"; break;
        }
        std::cout << "\n";
        switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:         std::cout << "Severity: high"; break;
        case GL_DEBUG_SEVERITY_MEDIUM:       std::cout << "Severity: medium"; break;
        case GL_DEBUG_SEVERITY_LOW:          std::cout << "Severity: low"; break;
        case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << "Severity: notification"; break;
        }    std::cout << "\n\n\n";
    }

    GLuint GetSkinnedVertexDataVAO()                     { return g_skinnedVertexDataVAO; }
    GLuint GetSkinnedVertexDataVBO()                     { return g_skinnedVertexDataVBO; }
    GLuint GetPreviousSkinnedPositionBuffer()            { return g_previousSkinnedPositionBuffer; }
    const std::vector<GLuint64>& GetBindlessTextureIDs() { return g_bindlessTextureIDs; }

}
