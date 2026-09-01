#include "GL_resource_manager.h"

#include "Hell/Logging.h"
#include "Hell/Common/Constants.h"
#include "Hell/Containers/SlotMap.h"
#include "Hell/MemoryTracker/MemoryTracker.h"
#include "Hell/ResourceManagement/ResourceID.h"

#include "Unloved/Debug/Debug.h"

#include <algorithm>
#include <iostream>
#include <unordered_map>

namespace OpenGL::ResourceManager {

    namespace {
        Hell::SlotMap<OpenGLCubemapFrameBuffer> g_cubemapFrameBuffers;
        Hell::SlotMap<OpenGLCubemapView> g_cubemapViews;
        Hell::SlotMap<OpenGLFrameBuffer> g_frameBuffers;
        Hell::SlotMap<OpenGLGenericMesh> g_genericMeshes;
        Hell::SlotMap<OpenGLMeshBuffer> g_meshBuffers;
        Hell::SlotMap<OpenGLShader> g_shaders;
        Hell::SlotMap<OpenGLShadowCubeMapArray> g_shadowCubeMapArrays;
        Hell::SlotMap<OpenGLShadowMap> g_shadowMaps;
        Hell::SlotMap<OpenGLShadowMapArray> g_shadowMapArrays;
        Hell::SlotMap<OpenGLSSBO> g_ssbos;
        Hell::SlotMap<OpenGLTexture> g_textures;
        Hell::SlotMap<OpenGLTexture3D> g_3dTextures;
        Hell::SlotMap<OpenGLTextureArray> g_textureArrays;

        std::unordered_map<std::string, uint64_t> g_cubemapFrameBufferIdByName;
        std::unordered_map<std::string, uint64_t> g_cubemapViewIdByName;
        std::unordered_map<std::string, uint64_t> g_frameBufferIdByName;
        std::unordered_map<std::string, uint64_t> g_genericMeshIdByName;
        std::unordered_map<std::string, uint64_t> g_meshBufferIdByName;
        std::unordered_map<std::string, uint64_t> g_shaderIdByName;
        std::unordered_map<std::string, uint64_t> g_shadowCubeMapArrayIdByName;
        std::unordered_map<std::string, uint64_t> g_shadowMapIdByName;
        std::unordered_map<std::string, uint64_t> g_shadowMapArrayIdByName;
        std::unordered_map<std::string, uint64_t> g_ssboIdByName;
        std::unordered_map<std::string, uint64_t> g_3dTextureIdByName;
        std::unordered_map<std::string, uint64_t> g_textureArrayIdByName;

        void SortMemoryReportCategory(Hell::MemoryTracker::MemoryReportCategory& category) {
            std::sort(category.entries.begin(), category.entries.end(), [](const auto& a, const auto& b) {
                return a.name < b.name;
                });
        }
    }

    void CleanUp() {
        for (OpenGLCubemapFrameBuffer& cubemapFrameBuffer : g_cubemapFrameBuffers) {
            cubemapFrameBuffer.CleanUp();
        }

        for (OpenGLCubemapView& cubemapView : g_cubemapViews) {
            cubemapView.CleanUp();
        }

        for (OpenGLFrameBuffer& frameBuffer : g_frameBuffers) {
            frameBuffer.CleanUp();
        }

        for (OpenGLGenericMesh& genericMesh : g_genericMeshes) {
            genericMesh.CleanUp();
        }

        for (OpenGLMeshBuffer& meshBuffer : g_meshBuffers) {
            meshBuffer.Reset();
        }

        for (OpenGLShadowCubeMapArray& shadowCubeMapArray : g_shadowCubeMapArrays) {
            shadowCubeMapArray.CleanUp();
        }

        for (OpenGLShadowMap& shadowMap : g_shadowMaps) {
            shadowMap.CleanUp();
        }

        for (OpenGLShadowMapArray& shadowMapArray : g_shadowMapArrays) {
            shadowMapArray.CleanUp();
        }

        for (OpenGLSSBO& ssbo : g_ssbos) {
            ssbo.CleanUp();
        }

        for (OpenGLTexture& texture : g_textures) {
            texture.MakeBindlessTextureNonResident();
            texture.Reset();
        }

        for (OpenGLTexture3D& texture3D : g_3dTextures) {
            texture3D.Reset();
        }

        for (OpenGLTextureArray& textureArray : g_textureArrays) {
            textureArray.CleanUp();
        }

        g_cubemapFrameBuffers.clear();
        g_cubemapFrameBufferIdByName.clear();
        g_cubemapViews.clear();
        g_cubemapViewIdByName.clear();
        g_frameBuffers.clear();
        g_frameBufferIdByName.clear();
        g_genericMeshes.clear();
        g_genericMeshIdByName.clear();
        g_meshBuffers.clear();
        g_meshBufferIdByName.clear();
        g_shaders.clear();
        g_shaderIdByName.clear();
        g_shadowCubeMapArrays.clear();
        g_shadowCubeMapArrayIdByName.clear();
        g_shadowMaps.clear();
        g_shadowMapIdByName.clear();
        g_shadowMapArrays.clear();
        g_shadowMapArrayIdByName.clear();
        g_ssbos.clear();
        g_ssboIdByName.clear();
        g_textures.clear();
        g_3dTextures.clear();
        g_3dTextureIdByName.clear();
        g_textureArrays.clear();
        g_textureArrayIdByName.clear();
    }

    // OpenGL Cubemap Frame Buffer

    OpenGLCubemapFrameBuffer& CreateCubemapFrameBuffer(const std::string& name) {
        auto it = g_cubemapFrameBufferIdByName.find(name);
        if (it != g_cubemapFrameBufferIdByName.end()) {
            return GetCubemapFrameBufferById(it->second);
        }

        uint64_t id = Hell::ResourceManagement::GetNextID(Hell::ResourceManagement::ResourceType::OPENGL_CUBEMAP_FRAME_BUFFER);
        g_cubemapFrameBuffers.emplace_with_id(id);
        g_cubemapFrameBufferIdByName[name] = id;
        OpenGLCubemapFrameBuffer& cubemapFrameBuffer = GetCubemapFrameBufferById(id);
        cubemapFrameBuffer.SetName(name);
        return cubemapFrameBuffer;
    }

    OpenGLCubemapFrameBuffer& GetCubemapFrameBuffer(const std::string& name) {
        OpenGLCubemapFrameBuffer* cubemapFrameBuffer = GetCubemapFrameBufferPtr(name);
        if (cubemapFrameBuffer) {
            return *cubemapFrameBuffer;
        }
        static OpenGLCubemapFrameBuffer invalid;
        return invalid;
    }

    OpenGLCubemapFrameBuffer* GetCubemapFrameBufferPtr(const std::string& name) {
        auto it = g_cubemapFrameBufferIdByName.find(name);
        if (it != g_cubemapFrameBufferIdByName.end()) {
            return GetCubemapFrameBufferPtrById(it->second);
        }
        return nullptr;
    }

    OpenGLCubemapFrameBuffer& GetCubemapFrameBufferById(uint64_t id) {
        OpenGLCubemapFrameBuffer* cubemapFrameBuffer = GetCubemapFrameBufferPtrById(id);
        if (cubemapFrameBuffer) {
            return *cubemapFrameBuffer;
        }
        static OpenGLCubemapFrameBuffer invalid;
        return invalid;
    }

    OpenGLCubemapFrameBuffer* GetCubemapFrameBufferPtrById(uint64_t id) {
        return g_cubemapFrameBuffers.get(id);
    }

    void RemoveCubemapFrameBuffer(uint64_t id) {
        for (auto it = g_cubemapFrameBufferIdByName.begin(); it != g_cubemapFrameBufferIdByName.end(); ++it) {
            if (it->second == id) {
                g_cubemapFrameBufferIdByName.erase(it);
                break;
            }
        }

        if (g_cubemapFrameBuffers.contains(id)) {
            g_cubemapFrameBuffers.get(id)->CleanUp();
            g_cubemapFrameBuffers.erase(id);
        }
    }

    // OpenGL Cubemap View

    uint64_t CreateCubemapView(const std::string& name) {
        auto it = g_cubemapViewIdByName.find(name);
        if (it != g_cubemapViewIdByName.end()) {
            return it->second;
        }

        uint64_t id = Hell::ResourceManagement::GetNextID(Hell::ResourceManagement::ResourceType::OPENGL_CUBEMAP_VIEW);
        g_cubemapViews.emplace_with_id(id);
        g_cubemapViewIdByName[name] = id;
        return id;
    }

    OpenGLCubemapView& GetCubemapView(const std::string& name) {
        OpenGLCubemapView* cubemapView = GetCubemapViewPtr(name);
        if (cubemapView) {
            return *cubemapView;
        }
        static OpenGLCubemapView invalid;
        return invalid;
    }

    OpenGLCubemapView* GetCubemapViewPtr(const std::string& name) {
        auto it = g_cubemapViewIdByName.find(name);
        if (it != g_cubemapViewIdByName.end()) {
            return GetCubemapViewPtrById(it->second);
        }
        return nullptr;
    }

    OpenGLCubemapView& GetCubemapViewById(uint64_t id) {
        OpenGLCubemapView* cubemapView = GetCubemapViewPtrById(id);
        if (cubemapView) {
            return *cubemapView;
        }
        static OpenGLCubemapView invalid;
        return invalid;
    }

    OpenGLCubemapView* GetCubemapViewPtrById(uint64_t id) {
        return g_cubemapViews.get(id);
    }

    void RemoveCubemapView(uint64_t id) {
        for (auto it = g_cubemapViewIdByName.begin(); it != g_cubemapViewIdByName.end(); ++it) {
            if (it->second == id) {
                g_cubemapViewIdByName.erase(it);
                break;
            }
        }

        if (g_cubemapViews.contains(id)) {
            g_cubemapViews.get(id)->CleanUp();
            g_cubemapViews.erase(id);
        }
    }

    // OpenGL Frame Buffer

    OpenGLFrameBuffer& CreateFrameBuffer(const std::string& name) {
        auto it = g_frameBufferIdByName.find(name);
        if (it != g_frameBufferIdByName.end()) {
            return GetFrameBufferById(it->second);
        }

        uint64_t id = Hell::ResourceManagement::GetNextID(Hell::ResourceManagement::ResourceType::OPENGL_FRAME_BUFFER);
        g_frameBuffers.emplace_with_id(id);
        g_frameBufferIdByName[name] = id;
        OpenGLFrameBuffer& frameBuffer = GetFrameBufferById(id);
        frameBuffer.SetName(name);
        return frameBuffer;
    }

    OpenGLFrameBuffer& GetFrameBuffer(const std::string& name) {
        OpenGLFrameBuffer* frameBuffer = GetFrameBufferPtr(name);
        if (frameBuffer) {
            return *frameBuffer;
        }
        static OpenGLFrameBuffer invalid;
        return invalid;
    }

    OpenGLFrameBuffer* GetFrameBufferPtr(const std::string& name) {
        auto it = g_frameBufferIdByName.find(name);
        if (it != g_frameBufferIdByName.end()) {
            return GetFrameBufferPtrById(it->second);
        }
        return nullptr;
    }

    OpenGLFrameBuffer& GetFrameBufferById(uint64_t id) {
        OpenGLFrameBuffer* frameBuffer = GetFrameBufferPtrById(id);
        if (frameBuffer) {
            return *frameBuffer;
        }
        static OpenGLFrameBuffer invalid;
        return invalid;
    }

    OpenGLFrameBuffer* GetFrameBufferPtrById(uint64_t id) {
        return g_frameBuffers.get(id);
    }

    void RemoveFrameBuffer(uint64_t id) {
        for (auto it = g_frameBufferIdByName.begin(); it != g_frameBufferIdByName.end(); ++it) {
            if (it->second == id) {
                g_frameBufferIdByName.erase(it);
                break;
            }
        }

        if (g_frameBuffers.contains(id)) {
            g_frameBuffers.get(id)->CleanUp();
            g_frameBuffers.erase(id);
        }
    }

    // OpenGL Generic Mesh

    uint64_t CreateGenericMesh() {
        uint64_t id = Hell::ResourceManagement::GetNextID(Hell::ResourceManagement::ResourceType::OPENGL_GENERIC_MESH);
        g_genericMeshes.emplace_with_id(id);
        return id;
    }

    uint64_t CreateGenericMesh(const std::string& name) {
        if (name.empty() || name == UNDEFINED_STRING) {
            return CreateGenericMesh();
        }

        auto it = g_genericMeshIdByName.find(name);
        if (it != g_genericMeshIdByName.end()) {
            return it->second;
        }

        uint64_t id = CreateGenericMesh();
        g_genericMeshIdByName[name] = id;
        return id;
    }

    OpenGLGenericMesh& GetGenericMesh(uint64_t id) {
        OpenGLGenericMesh* mesh3D = GetGenericMeshPtr(id);
        if (mesh3D) {
            return *mesh3D;
        }
        static OpenGLGenericMesh invalid;
        return invalid;
    }

    OpenGLGenericMesh& GetGenericMesh(const std::string& name) {
        OpenGLGenericMesh* genericMesh = GetGenericMeshPtr(name);
        if (genericMesh) {
            return *genericMesh;
        }
        static OpenGLGenericMesh invalid;
        return invalid;
    }

    OpenGLGenericMesh* GetGenericMeshPtr(uint64_t id) {
        return g_genericMeshes.get(id);
    }

    OpenGLGenericMesh* GetGenericMeshPtr(const std::string& name) {
        auto it = g_genericMeshIdByName.find(name);
        if (it != g_genericMeshIdByName.end()) {
            return GetGenericMeshPtr(it->second);
        }
        return nullptr;
    }

    void RemoveGenericMesh(uint64_t id) {
        for (auto it = g_genericMeshIdByName.begin(); it != g_genericMeshIdByName.end(); ++it) {
            if (it->second == id) {
                g_genericMeshIdByName.erase(it);
                break;
            }
        }

        if (g_genericMeshes.contains(id)) {
            g_genericMeshes.get(id)->CleanUp();
            g_genericMeshes.erase(id);
        }
    }

    // OpenGL Mesh Buffer

    uint64_t CreateMeshBuffer() {
        uint64_t id = Hell::ResourceManagement::GetNextID(Hell::ResourceManagement::ResourceType::OPENGL_MESH_BUFFER);
        g_meshBuffers.emplace_with_id(id);
        return id;
    }

    uint64_t CreateMeshBuffer(const std::string& name) {
        if (name.empty() || name == UNDEFINED_STRING) {
            return CreateMeshBuffer();
        }

        auto it = g_meshBufferIdByName.find(name);
        if (it != g_meshBufferIdByName.end()) {
            return it->second;
        }

        uint64_t id = CreateMeshBuffer();
        g_meshBufferIdByName[name] = id;
        return id;
    }

    OpenGLMeshBuffer& GetMeshBuffer(uint64_t id) {
        OpenGLMeshBuffer* meshBuffer = GetMeshBufferPtr(id);
        if (meshBuffer) {
            return *meshBuffer;
        }
        static OpenGLMeshBuffer invalid;
        return invalid;
    }

    OpenGLMeshBuffer& GetMeshBuffer(const std::string& name) {
        OpenGLMeshBuffer* meshBuffer = GetMeshBufferPtr(name);
        if (meshBuffer) {
            return *meshBuffer;
        }
        static OpenGLMeshBuffer invalid;
        return invalid;
    }

    OpenGLMeshBuffer* GetMeshBufferPtr(uint64_t id) {
        return g_meshBuffers.get(id);
    }

    OpenGLMeshBuffer* GetMeshBufferPtr(const std::string& name) {
        auto it = g_meshBufferIdByName.find(name);
        if (it != g_meshBufferIdByName.end()) {
            return GetMeshBufferPtr(it->second);
        }
        return nullptr;
    }

    void RemoveMeshBuffer(uint64_t id) {
        for (auto it = g_meshBufferIdByName.begin(); it != g_meshBufferIdByName.end(); ++it) {
            if (it->second == id) {
                g_meshBufferIdByName.erase(it);
                break;
            }
        }

        if (g_meshBuffers.contains(id)) {
            g_meshBuffers.get(id)->Reset();
            g_meshBuffers.erase(id);
        }
    }

    // OpenGL Shader

    uint64_t CreateShader(const std::string& name) {
        auto it = g_shaderIdByName.find(name);
        if (it != g_shaderIdByName.end()) {
            return it->second;
        }

        uint64_t id = Hell::ResourceManagement::GetNextID(Hell::ResourceManagement::ResourceType::OPENGL_SHADER);
        g_shaders.emplace_with_id(id);
        g_shaderIdByName[name] = id;
        return id;
    }

    OpenGLShader& GetShader(const std::string& name) {
        OpenGLShader* shader = GetShaderPtr(name);
        if (shader) {
            return *shader;
        }
        static OpenGLShader invalid;
        return invalid;
    }

    OpenGLShader* GetShaderPtr(const std::string& name) {
        auto it = g_shaderIdByName.find(name);
        if (it != g_shaderIdByName.end()) {
            return GetShaderPtrById(it->second);
        }
        return nullptr;
    }

    OpenGLShader& GetShaderById(uint64_t id) {
        OpenGLShader* shader = GetShaderPtrById(id);
        if (shader) {
            return *shader;
        }
        static OpenGLShader invalid;
        return invalid;
    }

    OpenGLShader* GetShaderPtrById(uint64_t id) {
        return g_shaders.get(id);
    }

    OpenGLShader& LoadShader(const std::string& name, const std::vector<std::string>& shaderPaths, const std::vector<std::string>& defines) {
        if (OpenGLShader* shader = GetShaderPtr(name)) {
            Logging::Error() << "OpenGL::ResourceManager::LoadShader() failed: '" << name << "' already exists\n";
            return *shader;
        }

        uint64_t id = CreateShader(name);
        OpenGLShader& shader = GetShaderById(id);
        shader = OpenGLShader(shaderPaths, "", defines);
        return shader;
    }

    OpenGLShader& LoadShader(const std::string& subDirectory, const std::string& name, const std::vector<std::string>& shaderPaths, const std::vector<std::string>& defines) {
        if (OpenGLShader* shader = GetShaderPtr(name)) {
            Logging::Error() << "OpenGL::ResourceManager::LoadShader() failed: '" << name << "' already exists\n";
            return *shader;
        }

        uint64_t id = CreateShader(name);
        OpenGLShader& shader = GetShaderById(id);
        shader = OpenGLShader(shaderPaths, subDirectory, defines);
        return shader;
    }

    void HotloadShaders() {
        std::string failedShaders = "FAILED TO HOTLOAD";

        bool allSucceeded = true;
        for (OpenGLShader& shader : g_shaders) {
            if (!shader.Hotload()) {
                allSucceeded = false;
                failedShaders += "\n- ";
                for (const std::string& path : shader.GetPaths()) {
                    failedShaders += path + " ";
                }
            }
        }

        if (allSucceeded) {
            std::cout << "Hotloaded shaders\n";
            Debug::BlitQuickDebugMessage("HOTLOADED SHADERS");
        }
        else {
            Debug::BlitQuickDebugMessage(failedShaders);
        }
    }

    void RemoveShader(uint64_t id) {
        for (auto it = g_shaderIdByName.begin(); it != g_shaderIdByName.end(); ++it) {
            if (it->second == id) {
                g_shaderIdByName.erase(it);
                break;
            }
        }

        if (g_shaders.contains(id)) {
            g_shaders.erase(id);
        }
    }

    // OpenGL Shadow Cube Map Array

    OpenGLShadowCubeMapArray& CreateShadowCubeMapArray(const std::string& name) {
        if (g_shadowCubeMapArrayIdByName.contains(name)) {
            return GetShadowCubeMapArray(name);
        }

        uint64_t id = Hell::ResourceManagement::GetNextID(Hell::ResourceManagement::ResourceType::OPENGL_SHADOW_CUBE_MAP_ARRAY);
        g_shadowCubeMapArrays.emplace_with_id(id);
        g_shadowCubeMapArrayIdByName[name] = id;

        return GetShadowCubeMapArray(name);
    }

    OpenGLShadowCubeMapArray& GetShadowCubeMapArray(const std::string& name) {
        OpenGLShadowCubeMapArray* shadowCubeMapArray = GetShadowCubeMapArrayPtr(name);
        if (shadowCubeMapArray) {
            return *shadowCubeMapArray;
        }
        static OpenGLShadowCubeMapArray invalid;
        return invalid;
    }

    OpenGLShadowCubeMapArray* GetShadowCubeMapArrayPtr(const std::string& name) {
        auto it = g_shadowCubeMapArrayIdByName.find(name);
        if (it != g_shadowCubeMapArrayIdByName.end()) {
            return GetShadowCubeMapArrayPtrById(it->second);
        }
        return nullptr;
    }

    OpenGLShadowCubeMapArray& GetShadowCubeMapArrayById(uint64_t id) {
        OpenGLShadowCubeMapArray* shadowCubeMapArray = GetShadowCubeMapArrayPtrById(id);
        if (shadowCubeMapArray) {
            return *shadowCubeMapArray;
        }
        static OpenGLShadowCubeMapArray invalid;
        return invalid;
    }

    OpenGLShadowCubeMapArray* GetShadowCubeMapArrayPtrById(uint64_t id) {
        return g_shadowCubeMapArrays.get(id);
    }

    void RemoveShadowCubeMapArray(uint64_t id) {
        for (auto it = g_shadowCubeMapArrayIdByName.begin(); it != g_shadowCubeMapArrayIdByName.end(); ++it) {
            if (it->second == id) {
                g_shadowCubeMapArrayIdByName.erase(it);
                break;
            }
        }

        if (g_shadowCubeMapArrays.contains(id)) {
            g_shadowCubeMapArrays.get(id)->CleanUp();
            g_shadowCubeMapArrays.erase(id);
        }
    }

    // OpenGL Shadow Map

    uint64_t CreateShadowMap(const std::string& name) {
        auto it = g_shadowMapIdByName.find(name);
        if (it != g_shadowMapIdByName.end()) {
            return it->second;
        }

        uint64_t id = Hell::ResourceManagement::GetNextID(Hell::ResourceManagement::ResourceType::OPENGL_SHADOW_MAP);
        g_shadowMaps.emplace_with_id(id);
        g_shadowMapIdByName[name] = id;
        return id;
    }

    OpenGLShadowMap& GetShadowMap(const std::string& name) {
        OpenGLShadowMap* shadowMap = GetShadowMapPtr(name);
        if (shadowMap) {
            return *shadowMap;
        }
        static OpenGLShadowMap invalid;
        return invalid;
    }

    OpenGLShadowMap* GetShadowMapPtr(const std::string& name) {
        auto it = g_shadowMapIdByName.find(name);
        if (it != g_shadowMapIdByName.end()) {
            return GetShadowMapPtrById(it->second);
        }
        return nullptr;
    }

    OpenGLShadowMap& GetShadowMapById(uint64_t id) {
        OpenGLShadowMap* shadowMap = GetShadowMapPtrById(id);
        if (shadowMap) {
            return *shadowMap;
        }
        static OpenGLShadowMap invalid;
        return invalid;
    }

    OpenGLShadowMap* GetShadowMapPtrById(uint64_t id) {
        return g_shadowMaps.get(id);
    }

    void RemoveShadowMap(uint64_t id) {
        for (auto it = g_shadowMapIdByName.begin(); it != g_shadowMapIdByName.end(); ++it) {
            if (it->second == id) {
                g_shadowMapIdByName.erase(it);
                break;
            }
        }

        if (g_shadowMaps.contains(id)) {
            g_shadowMaps.get(id)->CleanUp();
            g_shadowMaps.erase(id);
        }
    }

    // OpenGL Shadow Map Array

    uint64_t CreateShadowMapArray(const std::string& name) {
        auto it = g_shadowMapArrayIdByName.find(name);
        if (it != g_shadowMapArrayIdByName.end()) {
            return it->second;
        }

        uint64_t id = Hell::ResourceManagement::GetNextID(Hell::ResourceManagement::ResourceType::OPENGL_SHADOW_MAP_ARRAY);
        g_shadowMapArrays.emplace_with_id(id);
        g_shadowMapArrayIdByName[name] = id;
        return id;
    }

    OpenGLShadowMapArray& GetShadowMapArray(const std::string& name) {
        OpenGLShadowMapArray* shadowMapArray = GetShadowMapArrayPtr(name);
        if (shadowMapArray) {
            return *shadowMapArray;
        }
        static OpenGLShadowMapArray invalid;
        return invalid;
    }

    OpenGLShadowMapArray* GetShadowMapArrayPtr(const std::string& name) {
        auto it = g_shadowMapArrayIdByName.find(name);
        if (it != g_shadowMapArrayIdByName.end()) {
            return GetShadowMapArrayPtrById(it->second);
        }
        return nullptr;
    }

    OpenGLShadowMapArray& GetShadowMapArrayById(uint64_t id) {
        OpenGLShadowMapArray* shadowMapArray = GetShadowMapArrayPtrById(id);
        if (shadowMapArray) {
            return *shadowMapArray;
        }
        static OpenGLShadowMapArray invalid;
        return invalid;
    }

    OpenGLShadowMapArray* GetShadowMapArrayPtrById(uint64_t id) {
        return g_shadowMapArrays.get(id);
    }

    void RemoveShadowMapArray(uint64_t id) {
        for (auto it = g_shadowMapArrayIdByName.begin(); it != g_shadowMapArrayIdByName.end(); ++it) {
            if (it->second == id) {
                g_shadowMapArrayIdByName.erase(it);
                break;
            }
        }

        if (g_shadowMapArrays.contains(id)) {
            g_shadowMapArrays.get(id)->CleanUp();
            g_shadowMapArrays.erase(id);
        }
    }

    // OpenGL SSBO

    OpenGLSSBO& CreateSSBO(const std::string& name) {
        auto it = g_ssboIdByName.find(name);
        if (it != g_ssboIdByName.end()) {
            return GetSSBOById(it->second);
        }

        uint64_t id = Hell::ResourceManagement::GetNextID(Hell::ResourceManagement::ResourceType::OPENGL_SSBO);
        g_ssbos.emplace_with_id(id);
        g_ssboIdByName[name] = id;
        return GetSSBOById(id);
    }

    OpenGLSSBO& GetSSBO(const std::string& name) {
        OpenGLSSBO* ssbo = GetSSBOPtr(name);
        if (ssbo) {
            return *ssbo;
        }
        static OpenGLSSBO invalid;
        return invalid;
    }

    OpenGLSSBO* GetSSBOPtr(const std::string& name) {
        auto it = g_ssboIdByName.find(name);
        if (it != g_ssboIdByName.end()) {
            return GetSSBOPtrById(it->second);
        }
        return nullptr;
    }

    OpenGLSSBO& GetSSBOById(uint64_t id) {
        OpenGLSSBO* ssbo = GetSSBOPtrById(id);
        if (ssbo) {
            return *ssbo;
        }
        static OpenGLSSBO invalid;
        return invalid;
    }

    OpenGLSSBO* GetSSBOPtrById(uint64_t id) {
        return g_ssbos.get(id);
    }

    void RemoveSSBO(uint64_t id) {
        for (auto it = g_ssboIdByName.begin(); it != g_ssboIdByName.end(); ++it) {
            if (it->second == id) {
                g_ssboIdByName.erase(it);
                break;
            }
        }

        if (g_ssbos.contains(id)) {
            g_ssbos.get(id)->CleanUp();
            g_ssbos.erase(id);
        }
    }

    void RemoveSSBOByName(const std::string& name) {
        auto it = g_ssboIdByName.find(name);
        if (it == g_ssboIdByName.end()) return;

        RemoveSSBO(it->second);
    }

    // OpenGL Texture

    uint64_t CreateTexture() {
        uint64_t id = Hell::ResourceManagement::GetNextID(Hell::ResourceManagement::ResourceType::OPENGL_TEXTURE);
        g_textures.emplace_with_id(id);
        return id;
    }

    OpenGLTexture& GetTexture(uint64_t id) {
        OpenGLTexture* texture = GetTexturePtr(id);
        if (texture) {
            return *texture;
        }
        static OpenGLTexture invalid;
        return invalid;
    }

    OpenGLTexture* GetTexturePtr(uint64_t id) {
        return g_textures.get(id);
    }

    void RemoveTexture(uint64_t id) {
        if (g_textures.contains(id)) {
            OpenGLTexture* texture = g_textures.get(id);
            texture->MakeBindlessTextureNonResident();
            texture->Reset();
            g_textures.erase(id);
        }
    }

    // OpenGL Texture 3D

    uint64_t CreateTexture3D(const std::string& name) {
        auto it = g_3dTextureIdByName.find(name);
        if (it != g_3dTextureIdByName.end()) {
            return it->second;
        }

        uint64_t id = Hell::ResourceManagement::GetNextID(Hell::ResourceManagement::ResourceType::OPENGL_TEXTURE_3D);
        g_3dTextures.emplace_with_id(id);
        g_3dTextureIdByName[name] = id;
        return id;
    }

    OpenGLTexture3D& GetTexture3D(const std::string& name) {
        OpenGLTexture3D* texture3D = GetTexture3DPtr(name);
        if (texture3D) {
            return *texture3D;
        }
        static OpenGLTexture3D invalid;
        return invalid;
    }

    OpenGLTexture3D* GetTexture3DPtr(const std::string& name) {
        auto it = g_3dTextureIdByName.find(name);
        if (it != g_3dTextureIdByName.end()) {
            return GetTexture3DPtrById(it->second);
        }
        return nullptr;
    }

    OpenGLTexture3D& GetTexture3DById(uint64_t id) {
        OpenGLTexture3D* texture3D = GetTexture3DPtrById(id);
        if (texture3D) {
            return *texture3D;
        }
        static OpenGLTexture3D invalid;
        return invalid;
    }

    OpenGLTexture3D* GetTexture3DPtrById(uint64_t id) {
        return g_3dTextures.get(id);
    }

    void RemoveTexture3D(uint64_t id) {
        for (auto it = g_3dTextureIdByName.begin(); it != g_3dTextureIdByName.end(); ++it) {
            if (it->second == id) {
                g_3dTextureIdByName.erase(it);
                break;
            }
        }

        if (g_3dTextures.contains(id)) {
            g_3dTextures.get(id)->Reset();
            g_3dTextures.erase(id);
        }
    }

    // OpenGL Texture Array

    uint64_t CreateTextureArray(const std::string& name) {
        auto it = g_textureArrayIdByName.find(name);
        if (it != g_textureArrayIdByName.end()) {
            return it->second;
        }

        uint64_t id = Hell::ResourceManagement::GetNextID(Hell::ResourceManagement::ResourceType::OPENGL_TEXTURE_ARRAY);
        g_textureArrays.emplace_with_id(id);
        g_textureArrayIdByName[name] = id;
        return id;
    }

    OpenGLTextureArray& GetTextureArray(const std::string& name) {
        OpenGLTextureArray* textureArray = GetTextureArrayPtr(name);
        if (textureArray) {
            return *textureArray;
        }
        static OpenGLTextureArray invalid;
        return invalid;
    }

    OpenGLTextureArray* GetTextureArrayPtr(const std::string& name) {
        auto it = g_textureArrayIdByName.find(name);
        if (it != g_textureArrayIdByName.end()) {
            return GetTextureArrayPtrById(it->second);
        }
        return nullptr;
    }

    OpenGLTextureArray& GetTextureArrayById(uint64_t id) {
        OpenGLTextureArray* textureArray = GetTextureArrayPtrById(id);
        if (textureArray) {
            return *textureArray;
        }
        static OpenGLTextureArray invalid;
        return invalid;
    }

    OpenGLTextureArray* GetTextureArrayPtrById(uint64_t id) {
        return g_textureArrays.get(id);
    }

    void RemoveTextureArray(uint64_t id) {
        for (auto it = g_textureArrayIdByName.begin(); it != g_textureArrayIdByName.end(); ++it) {
            if (it->second == id) {
                g_textureArrayIdByName.erase(it);
                break;
            }
        }

        if (g_textureArrays.contains(id)) {
            g_textureArrays.get(id)->CleanUp();
            g_textureArrays.erase(id);
        }
    }

    // Memory Report

    void AppendMemoryReport(Hell::MemoryTracker::MemoryReport& report) {
        if (!g_cubemapFrameBufferIdByName.empty()) {
            Hell::MemoryTracker::MemoryReportCategory& category = report.categories.emplace_back();
            category.name = "OpenGL Cubemap Frame Buffers";
            category.entries.reserve(g_cubemapFrameBufferIdByName.size());

            for (const auto& [name, id] : g_cubemapFrameBufferIdByName) {
                OpenGLCubemapFrameBuffer* cubemapFrameBuffer = GetCubemapFrameBufferPtrById(id);
                if (!cubemapFrameBuffer) {
                    continue;
                }

                Hell::MemoryTracker::MemoryReportEntry& entry = category.entries.emplace_back();
                entry.name = name;
                entry.gpuBytes = cubemapFrameBuffer->GetGPUAllocatedByteCount();
            }

            SortMemoryReportCategory(category);
        }

        if (!g_cubemapViewIdByName.empty()) {
            Hell::MemoryTracker::MemoryReportCategory& category = report.categories.emplace_back();
            category.name = "OpenGL Cubemap Views";
            category.entries.reserve(g_cubemapViewIdByName.size());

            for (const auto& [name, id] : g_cubemapViewIdByName) {
                OpenGLCubemapView* cubemapView = GetCubemapViewPtrById(id);
                if (!cubemapView) {
                    continue;
                }

                Hell::MemoryTracker::MemoryReportEntry& entry = category.entries.emplace_back();
                entry.name = name;
                entry.gpuBytes = cubemapView->GetGPUAllocatedByteCount();
            }

            SortMemoryReportCategory(category);
        }

        if (!g_frameBufferIdByName.empty()) {
            Hell::MemoryTracker::MemoryReportCategory& category = report.categories.emplace_back();
            category.name = "OpenGL Frame Buffers";
            category.entries.reserve(g_frameBufferIdByName.size());

            for (const auto& [name, id] : g_frameBufferIdByName) {
                OpenGLFrameBuffer* frameBuffer = GetFrameBufferPtrById(id);
                if (!frameBuffer) {
                    continue;
                }

                Hell::MemoryTracker::MemoryReportEntry& entry = category.entries.emplace_back();
                entry.name = name;
                entry.gpuBytes = frameBuffer->GetGPUAllocatedByteCount();
            }

            SortMemoryReportCategory(category);
        }

        if (!g_shaderIdByName.empty()) {
            Hell::MemoryTracker::MemoryReportCategory& category = report.categories.emplace_back();
            category.name = "OpenGL Shaders";
            category.entries.reserve(g_shaderIdByName.size());

            for (const auto& [name, id] : g_shaderIdByName) {
                OpenGLShader* shader = GetShaderPtrById(id);
                if (!shader) {
                    continue;
                }

                Hell::MemoryTracker::MemoryReportEntry& entry = category.entries.emplace_back();
                entry.name = name;
                entry.cpuBytes = shader->GetCPUAllocatedByteCount();
            }

            SortMemoryReportCategory(category);
        }

        if (!g_shadowCubeMapArrayIdByName.empty()) {
            Hell::MemoryTracker::MemoryReportCategory& category = report.categories.emplace_back();
            category.name = "OpenGL Shadow Cube Map Arrays";
            category.entries.reserve(g_shadowCubeMapArrayIdByName.size());

            for (const auto& [name, id] : g_shadowCubeMapArrayIdByName) {
                OpenGLShadowCubeMapArray* shadowCubeMapArray = GetShadowCubeMapArrayPtrById(id);
                if (!shadowCubeMapArray) {
                    continue;
                }

                Hell::MemoryTracker::MemoryReportEntry& entry = category.entries.emplace_back();
                entry.name = name;
                entry.gpuBytes = shadowCubeMapArray->GetGPUAllocatedByteCount();
            }

            SortMemoryReportCategory(category);
        }

        if (!g_shadowMapIdByName.empty()) {
            Hell::MemoryTracker::MemoryReportCategory& category = report.categories.emplace_back();
            category.name = "OpenGL Shadow Maps";
            category.entries.reserve(g_shadowMapIdByName.size());

            for (const auto& [name, id] : g_shadowMapIdByName) {
                OpenGLShadowMap* shadowMap = GetShadowMapPtrById(id);
                if (!shadowMap) {
                    continue;
                }

                Hell::MemoryTracker::MemoryReportEntry& entry = category.entries.emplace_back();
                entry.name = name;
                entry.gpuBytes = shadowMap->GetGPUAllocatedByteCount();
            }

            SortMemoryReportCategory(category);
        }

        if (!g_shadowMapArrayIdByName.empty()) {
            Hell::MemoryTracker::MemoryReportCategory& category = report.categories.emplace_back();
            category.name = "OpenGL Shadow Map Arrays";
            category.entries.reserve(g_shadowMapArrayIdByName.size());

            for (const auto& [name, id] : g_shadowMapArrayIdByName) {
                OpenGLShadowMapArray* shadowMapArray = GetShadowMapArrayPtrById(id);
                if (!shadowMapArray) {
                    continue;
                }

                Hell::MemoryTracker::MemoryReportEntry& entry = category.entries.emplace_back();
                entry.name = name;
                entry.gpuBytes = shadowMapArray->GetGPUAllocatedByteCount();
            }

            SortMemoryReportCategory(category);
        }

        if (!g_ssboIdByName.empty()) {
            Hell::MemoryTracker::MemoryReportCategory& category = report.categories.emplace_back();
            category.name = "OpenGL SSBOs";
            category.entries.reserve(g_ssboIdByName.size());

            for (const auto& [name, id] : g_ssboIdByName) {
                OpenGLSSBO* ssbo = GetSSBOPtrById(id);
                if (!ssbo) {
                    continue;
                }

                Hell::MemoryTracker::MemoryReportEntry& entry = category.entries.emplace_back();
                entry.name = name;
                entry.gpuBytes = ssbo->GetGPUAllocatedByteCount();
            }

            SortMemoryReportCategory(category);
        }

        if (!g_3dTextureIdByName.empty()) {
            Hell::MemoryTracker::MemoryReportCategory& category = report.categories.emplace_back();
            category.name = "OpenGL 3D Textures";
            category.entries.reserve(g_3dTextureIdByName.size());

            for (const auto& [name, id] : g_3dTextureIdByName) {
                OpenGLTexture3D* texture3D = GetTexture3DPtrById(id);
                if (!texture3D) {
                    continue;
                }

                Hell::MemoryTracker::MemoryReportEntry& entry = category.entries.emplace_back();
                entry.name = name;
                entry.gpuBytes = texture3D->GetGPUAllocatedByteCount();
            }

            SortMemoryReportCategory(category);
        }

        if (!g_textureArrayIdByName.empty()) {
            Hell::MemoryTracker::MemoryReportCategory& category = report.categories.emplace_back();
            category.name = "OpenGL Texture Arrays";
            category.entries.reserve(g_textureArrayIdByName.size());

            for (const auto& [name, id] : g_textureArrayIdByName) {
                OpenGLTextureArray* textureArray = GetTextureArrayPtrById(id);
                if (!textureArray) {
                    continue;
                }

                Hell::MemoryTracker::MemoryReportEntry& entry = category.entries.emplace_back();
                entry.name = name;
                entry.gpuBytes = textureArray->GetGPUAllocatedByteCount();
            }

            SortMemoryReportCategory(category);
        }
    }
}
