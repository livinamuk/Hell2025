#pragma once
#include "HellTypes.h"
#include "GL_backend.h"
#include "AssetManagement/BakeQueue.h"
#include "AssetManagement/AssetManager.h"
#include "BackEnd/Integration/GLFW.h"
#include <string>
#include <iostream>
#include <vector>
#include "GL_Util.h"
#include "Types/GL_pbo.hpp"

// remove me
#include "Renderer/Renderer.h"

namespace OpenGLBackEnd {
    float g_depthClearValue = 1.0f;

    // PBO texture loading
    const size_t MAX_TEXTURE_WIDTH = 4096;
    const size_t MAX_TEXTURE_HEIGHT = 4096;
    const size_t MAX_CHANNEL_COUNT = 4;
    const size_t MAX_DATA_SIZE = MAX_TEXTURE_WIDTH * MAX_TEXTURE_HEIGHT * MAX_CHANNEL_COUNT;
    std::vector<PBO> g_textureBakingPBOs;
    PBO g_heightMapVerticesReadBackPBO;
    PBO g_heightMapIndicesReadBackPBO;
    GLuint g_vertexDataVAO = 0;
    GLuint g_vertexDataVBO = 0;
    GLuint g_vertexDataEBO = 0;
    GLuint g_weightedVertexDataVAO = 0;
    GLuint g_weightedVertexDataVBO = 0;
    GLuint g_weightedVertexDataEBO = 0;
    GLuint g_skinnedVertexDataVAO = 0;
    GLuint g_skinnedVertexDataVBO = 0;
    GLuint g_allocatedSkinnedVertexBufferSize = 0;
    std::vector<GLuint64> g_bindlessTextureIDs;

    OpenGLHeightMapMesh g_heightMapMesh;

    void APIENTRY glDebugOutput(GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei /*length*/, const char* message, const void* /*userParam*/);
    void UpdateBindlessTextures();

    void Init() {

        BackEnd::GLFW::MakeContextCurrent();

        // Init glad
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            std::cout << "Failed to initialize GLAD\n";
            return;
        }
        // Print some shit
        GLint major, minor;
        glGetIntegerv(GL_MAJOR_VERSION, &major);
        glGetIntegerv(GL_MINOR_VERSION, &minor);
        const GLubyte* vendor = glGetString(GL_VENDOR);
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
        glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);

        // Texture baking PBOs
        for (int i = 0; i < 64; ++i) {
            PBO& pbo = g_textureBakingPBOs.emplace_back();
            pbo.Init(MAX_DATA_SIZE);
        }

        // Height map init shit (ABSTRACT ME BETTER!!!!!)
        int vertexBufferSize = HEIGHT_MAP_SIZE * HEIGHT_MAP_SIZE * sizeof(Vertex);
        int indexBufferSize = (HEIGHT_MAP_SIZE - 1) * (HEIGHT_MAP_SIZE - 1) * 6 * sizeof(uint32_t);
        g_heightMapVerticesReadBackPBO.Init(vertexBufferSize);
        g_heightMapIndicesReadBackPBO.Init(indexBufferSize);
    }

    void BeginFrame() {
        UpdateBindlessTextures();
    }

    void OpenGLBackEnd::UploadVertexData(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {
        if (vertices.empty() || indices.empty()) {
            return;
        }

        if (g_vertexDataVAO != 0) {
            glDeleteVertexArrays(1, &g_vertexDataVAO);
            glDeleteBuffers(1, &g_vertexDataVBO);
            glDeleteBuffers(1, &g_vertexDataEBO);
        }

        glGenVertexArrays(1, &g_vertexDataVAO);
        glGenBuffers(1, &g_vertexDataVBO);
        glGenBuffers(1, &g_vertexDataEBO);

        glBindVertexArray(g_vertexDataVAO);
        glBindBuffer(GL_ARRAY_BUFFER, g_vertexDataVBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_vertexDataEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tangent));

        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    void OpenGLBackEnd::UploadWeightedVertexData(std::vector<WeightedVertex>& vertices, std::vector<uint32_t>& indices) {

        if (vertices.empty() || indices.empty()) {
            return;
        }

        if (g_weightedVertexDataVAO != 0) {
            glDeleteVertexArrays(1, &g_weightedVertexDataVAO);
            glDeleteBuffers(1, &g_weightedVertexDataVBO);
            glDeleteBuffers(1, &g_weightedVertexDataEBO);
        }

        glGenVertexArrays(1, &g_weightedVertexDataVAO);
        glGenBuffers(1, &g_weightedVertexDataVBO);
        glGenBuffers(1, &g_weightedVertexDataEBO);

        glBindVertexArray(g_weightedVertexDataVAO);
        glBindBuffer(GL_ARRAY_BUFFER, g_weightedVertexDataVBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(WeightedVertex), vertices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_weightedVertexDataEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(WeightedVertex), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(WeightedVertex), (void*)offsetof(WeightedVertex, normal));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(WeightedVertex), (void*)offsetof(WeightedVertex, uv));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(WeightedVertex), (void*)offsetof(WeightedVertex, tangent));
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 4, GL_FLOAT, GL_INT, sizeof(WeightedVertex), (void*)offsetof(WeightedVertex, boneID));
        glEnableVertexAttribArray(5);
        glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(WeightedVertex), (void*)offsetof(WeightedVertex, weight));

        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    void OpenGLBackEnd::AllocateSkinnedVertexBufferSpace(uint32_t vertexCount) {
        if (g_skinnedVertexDataVAO == 0) {
            glGenVertexArrays(1, &g_skinnedVertexDataVAO);
        }

        if (g_allocatedSkinnedVertexBufferSize < (uint32_t)(vertexCount * sizeof(Vertex))) {
            if (g_skinnedVertexDataVBO != 0) {
                glDeleteBuffers(1, &g_skinnedVertexDataVBO);
            }

            glBindVertexArray(g_skinnedVertexDataVAO);
            glGenBuffers(1, &g_skinnedVertexDataVBO);
            glBindBuffer(GL_ARRAY_BUFFER, g_skinnedVertexDataVBO);

            glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(vertexCount * sizeof(Vertex)), nullptr, GL_STATIC_DRAW);

            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, (GLsizei)sizeof(Vertex), (void*)0);

            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, (GLsizei)sizeof(Vertex), (void*)offsetof(Vertex, normal));

            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, (GLsizei)sizeof(Vertex), (void*)offsetof(Vertex, uv));

            glEnableVertexAttribArray(3);
            glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, (GLsizei)sizeof(Vertex), (void*)offsetof(Vertex, tangent));

            glBindBuffer(GL_ARRAY_BUFFER, 0);

            g_allocatedSkinnedVertexBufferSize = (uint32_t)(vertexCount * sizeof(Vertex));
        }
    }

    void AllocateTextureMemory(Texture& texture) {
        OpenGLTexture& glTexture = texture.GetGLTexture();
        GLuint& handle = glTexture.GetHandle();

        if (handle != 0) return;

        float borderColor[] = {
            texture.GetBorderColor().r,
            texture.GetBorderColor().g,
            texture.GetBorderColor().b,
            texture.GetBorderColor().a };

        glCreateTextures(GL_TEXTURE_2D, 1, &handle);
        glTextureParameteri(handle, GL_TEXTURE_WRAP_S, OpenGLUtil::TextureWrapModeToGLEnum(texture.GetTextureWrapMode()));
        glTextureParameteri(handle, GL_TEXTURE_WRAP_T, OpenGLUtil::TextureWrapModeToGLEnum(texture.GetTextureWrapMode()));
        glTextureParameteri(handle, GL_TEXTURE_MIN_FILTER, OpenGLUtil::TextureFilterToGLEnum(texture.GetMinFilter()));
        glTextureParameteri(handle, GL_TEXTURE_MAG_FILTER, OpenGLUtil::TextureFilterToGLEnum(texture.GetMagFilter()));
        glTextureParameterfv(handle, GL_TEXTURE_BORDER_COLOR, borderColor);

        int width = texture.GetWidth();
        int height = texture.GetHeight();
        int levels = texture.MipmapsAreRequested() ? texture.GetMipmapLevelCount() : 1;

        // Allocate Immutable Storage
        GLenum internalFormat = texture.GetInternalFormat();
        if (texture.GetImageDataType() == ImageDataType::EXR) {
            internalFormat = GL_RGB16F; // Ensure float format for EXR
        }

        glTextureStorage2D(handle, levels, internalFormat, width, height);

        glTexture.MakeBindlessTextureResident();
    }

    void ImmediateBake(QueuedTextureBake& queuedTextureBake) {
        Texture* texture = static_cast<Texture*>(queuedTextureBake.texture);
        OpenGLTexture& glTexture = texture->GetGLTexture();
        int width = queuedTextureBake.width;
        int height = queuedTextureBake.height;
        int format = queuedTextureBake.format;
        int internalFormat = queuedTextureBake.internalFormat;
        int level = queuedTextureBake.mipmapLevel;
        int dataSize = queuedTextureBake.dataSize;
        const void* data = queuedTextureBake.data;

        GLuint textureHandle = glTexture.GetHandle();

        // Bake texture data
        if (texture->GetImageDataType() == ImageDataType::UNCOMPRESSED) {
            glTextureSubImage2D(textureHandle, level, 0, 0, width, height, format, GL_UNSIGNED_BYTE, data);
        }
        else if (texture->GetImageDataType() == ImageDataType::EXR) {
            //glTextureSubImage2D(textureHandle, 0, 0, 0, glTexture.GetWidth(), glTexture.GetHeight(), GL_RGBA, GL_FLOAT, glTexture.GetData());
        }
        else if (texture->GetImageDataType() == ImageDataType::COMPRESSED) {
            glCompressedTextureSubImage2D(textureHandle, level, 0, 0, width, height, internalFormat, dataSize, data);
        }

        texture->SetTextureDataLevelBakeState(level, BakeState::BAKE_COMPLETE);

        // Generate Mipmaps if none were supplied
        if (texture->MipmapsAreRequested()) {
            if (texture->GetTextureDataCount() == 1) {
                glGenerateTextureMipmap(textureHandle);
            }
        }
        // Cleanup bake queue
        BakeQueue::RemoveQueuedTextureBakeByJobID(queuedTextureBake.jobID);
    }


    void UpdateTextureBaking() {
        size_t bakeCommandsIssuedPerFrame = g_textureBakingPBOs.size();

        for (size_t i = 0; i < bakeCommandsIssuedPerFrame; i++) {
            // Update PBO states
            for (PBO& pbo : g_textureBakingPBOs) {
                pbo.UpdateState();
            }

            // If any have completed, remove the job ID from the queue
            for (PBO& pbo : g_textureBakingPBOs) {
                uint32_t jobID = pbo.GetCustomValue();
                if (pbo.IsSyncComplete() && jobID != -1) {
                    QueuedTextureBake* queuedTextureBake = BakeQueue::GetQueuedTextureBakeByJobID(jobID);
                    Texture* texture = static_cast<Texture*>(queuedTextureBake->texture);
                    texture->SetTextureDataLevelBakeState(queuedTextureBake->mipmapLevel, BakeState::BAKE_COMPLETE);

                    // Generate mipmaps if none were supplied
                    if (texture->MipmapsAreRequested()) {
                        if (texture->GetTextureDataCount() == 1) {
                            glGenerateTextureMipmap(texture->GetGLTexture().GetHandle());
                        }
                    }
                    BakeQueue::RemoveQueuedTextureBakeByJobID(jobID);
                    pbo.SetCustomValue(-1);
                }
            }

            // Bake the next queued texture bake (if one exists)
            if (BakeQueue::GetQueuedTextureBakeJobCount() > 0) {
                QueuedTextureBake* queuedTextureBake = BakeQueue::GetNextQueuedTextureBake();
                if (queuedTextureBake) {
                    AsyncBakeQueuedTextureBake(*queuedTextureBake);
                }
            }
        }
    }

    void AsyncBakeQueuedTextureBake(QueuedTextureBake& queuedTextureBake) {
        // Get next free PBO
        PBO* pbo = nullptr;
        for (PBO& queryPbo : g_textureBakingPBOs) {
            if (queryPbo.IsSyncComplete()) {
                pbo = &queryPbo;
                break;
            }
        }

        // Return early if no free PBOs
        if (!pbo) {
            std::cerr << "Warning: Attempting to use an active PBO!" << std::endl;
            return;
        }

        queuedTextureBake.inProgress = true;

        Texture* texture = static_cast<Texture*>(queuedTextureBake.texture);
        int jobID = queuedTextureBake.jobID;
        int width = queuedTextureBake.width;
        int height = queuedTextureBake.height;
        int format = queuedTextureBake.format;
        int internalFormat = queuedTextureBake.internalFormat;
        int level = queuedTextureBake.mipmapLevel;
        int dataSize = queuedTextureBake.dataSize;
        const void* data = queuedTextureBake.data;

        texture->SetTextureDataLevelBakeState(level, BakeState::BAKING_IN_PROGRESS);

        // Map PBO and copy data
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo->GetHandle());
        std::memcpy(pbo->GetPersistentBuffer(), data, dataSize);

        // Upload data to the texture using DSA
        GLuint textureHandle = texture->GetGLTexture().GetHandle();
        if (texture->GetImageDataType() == ImageDataType::UNCOMPRESSED) {
            glTextureSubImage2D(textureHandle, level, 0, 0, width, height, format, GL_UNSIGNED_BYTE, 0);
        }
        else if (texture->GetImageDataType() == ImageDataType::COMPRESSED) {
            glCompressedTextureSubImage2D(textureHandle, level, 0, 0, width, height, internalFormat, dataSize, 0);
        }
        else if (texture->GetImageDataType() == ImageDataType::EXR) {
            glTextureSubImage2D(textureHandle, level, 0, 0, width, height, GL_RGBA, GL_FLOAT, 0);
        }

        // Start PBO sync and assign job ID
        pbo->SyncStart();
        pbo->SetCustomValue(jobID);

        // Unbind PBO
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    }

    void CleanUpBakingPBOs() {
        for (PBO& pbo : g_textureBakingPBOs) {
            pbo.CleanUp();
        }
        g_textureBakingPBOs.clear();
    }

    void UpdateBindlessTextures() {
        g_bindlessTextureIDs.clear();
        g_bindlessTextureIDs.reserve(AssetManager::GetTextureCount());
        for (int i = 0; i < AssetManager::GetTextureCount(); i++) {
            g_bindlessTextureIDs.push_back(AssetManager::GetTextureByIndex(i)->GetGLTexture().GetBindlessID());
        }
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

    //void ReadBackHeightmapMeshData() {
    //    int indexCount = (     - 1) * (HEIGHT_MAP_SIZE - 1) * 6;
    //    int vertexCount = HEIGHT_MAP_SIZE * HEIGHT_MAP_SIZE;
    //    int vertexBufferSize = HEIGHT_MAP_SIZE * HEIGHT_MAP_SIZE * sizeof(Vertex);
    //    int indexBufferSize = (HEIGHT_MAP_SIZE - 1) * (HEIGHT_MAP_SIZE - 1) * 6 * sizeof(uint32_t);
    //
    //    g_heightMapVerticesReadBackPBO.UpdateState();
    //
    //    if (!g_heightMapVerticesReadBackPBO.IsSyncComplete()) {
    //        return; // Wait for sync
    //    }
    //
    //    // Bind PBO to store vertex buffer data
    //    glBindBuffer(GL_PIXEL_PACK_BUFFER, g_heightMapVerticesReadBackPBO.GetHandle());
    //    glBindBuffer(GL_SHADER_STORAGE_BUFFER, g_heightMapMesh.GetVBO());
    //
    //    // Copy the vertex buffer data into the PBO
    //    glCopyBufferSubData(GL_SHADER_STORAGE_BUFFER, GL_PIXEL_PACK_BUFFER, 0, 0, vertexBufferSize);
    //
    //    // Sync and map buffer to access data
    //    g_heightMapVerticesReadBackPBO.SyncStart();
    //    const Vertex* mappedBuffer = reinterpret_cast<const Vertex*>(g_heightMapVerticesReadBackPBO.GetPersistentBuffer());
    //
    //    if (mappedBuffer) {
    //        for (int i = 0; i < vertexCount; i++) {
    //            const Vertex& v = mappedBuffer[i];
    //            glm::vec3 pos = v.position;
    //            pos.y *= 10;
    //            //Renderer::DrawPoint(pos, GREEN);
    //        }
    //    }
    //
    //    // Unbind
    //    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    //    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    //}

    GLuint GetVertexDataVAO() { return g_vertexDataVAO; }
    GLuint GetVertexDataVBO() { return g_vertexDataVBO; }
    GLuint GetVertexDataEBO() { return g_vertexDataEBO; }
    GLuint GetWeightedVertexDataVAO() { return g_weightedVertexDataVAO; }
    GLuint GetWeightedVertexDataVBO() { return g_weightedVertexDataVBO; }
    GLuint GetWeightedVertexDataEBO() { return g_weightedVertexDataEBO; }
    GLuint GetSkinnedVertexDataVAO() { return g_skinnedVertexDataVAO; }
    GLuint GetSkinnedVertexDataVBO() { return g_skinnedVertexDataVBO; }
    OpenGLHeightMapMesh& GetHeightMapMesh() { return g_heightMapMesh; };
    const std::vector<GLuint64>& GetBindlessTextureIDs() { return g_bindlessTextureIDs; }

}
