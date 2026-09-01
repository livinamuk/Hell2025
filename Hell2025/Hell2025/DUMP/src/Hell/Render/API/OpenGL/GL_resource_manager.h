#pragma once
#include "Hell/Render/API/OpenGL/Types/GL_cubemap_frame_buffer.h"
#include "Hell/Render/API/OpenGL/Types/GL_cubemapView.h"
#include "Hell/Render/API/OpenGL/Types/GL_frameBuffer.h"
#include "Hell/Render/API/OpenGL/Types/GL_generic_mesh.h"
#include "Hell/Render/API/OpenGL/Types/GL_mesh_buffer.h"
#include "Hell/Render/API/OpenGL/Types/GL_shader.h"
#include "Hell/Render/API/OpenGL/Types/GL_shadow_cube_map_array.h"
#include "Hell/Render/API/OpenGL/Types/GL_shadow_map.h"
#include "Hell/Render/API/OpenGL/Types/GL_shadow_map_array.h"
#include "Hell/Render/API/OpenGL/Types/GL_ssbo.h"
#include "Hell/Render/API/OpenGL/Types/GL_texture.h"
#include "Hell/Render/API/OpenGL/Types/GL_texture_3d.h"
#include "Hell/Render/API/OpenGL/Types/GL_texture_array.h"

#include <cstdint>
#include <string>
#include <vector>

namespace Hell::MemoryTracker {
    struct MemoryReport;
}

namespace OpenGL::ResourceManager {
    void CleanUp();
    void AppendMemoryReport(Hell::MemoryTracker::MemoryReport& report);

    OpenGLCubemapFrameBuffer& CreateCubemapFrameBuffer(const std::string& name);
    OpenGLCubemapFrameBuffer& GetCubemapFrameBuffer(const std::string& name);
    OpenGLCubemapFrameBuffer* GetCubemapFrameBufferPtr(const std::string& name);
    OpenGLCubemapFrameBuffer& GetCubemapFrameBufferById(uint64_t id);
    OpenGLCubemapFrameBuffer* GetCubemapFrameBufferPtrById(uint64_t id);
    void RemoveCubemapFrameBuffer(uint64_t id);

    uint64_t CreateCubemapView(const std::string& name);
    OpenGLCubemapView& GetCubemapView(const std::string& name);
    OpenGLCubemapView* GetCubemapViewPtr(const std::string& name);
    OpenGLCubemapView& GetCubemapViewById(uint64_t id);
    OpenGLCubemapView* GetCubemapViewPtrById(uint64_t id);
    void RemoveCubemapView(uint64_t id);

    OpenGLFrameBuffer& CreateFrameBuffer(const std::string& name);
    OpenGLFrameBuffer& GetFrameBuffer(const std::string& name);
    OpenGLFrameBuffer* GetFrameBufferPtr(const std::string& name);
    OpenGLFrameBuffer& GetFrameBufferById(uint64_t id);
    OpenGLFrameBuffer* GetFrameBufferPtrById(uint64_t id);
    void RemoveFrameBuffer(uint64_t id);

    uint64_t CreateGenericMesh();
    uint64_t CreateGenericMesh(const std::string& name);
    OpenGLGenericMesh& GetGenericMesh(uint64_t id);
    OpenGLGenericMesh& GetGenericMesh(const std::string& name);
    OpenGLGenericMesh* GetGenericMeshPtr(uint64_t id);
    OpenGLGenericMesh* GetGenericMeshPtr(const std::string& name);
    void RemoveGenericMesh(uint64_t id);

    uint64_t CreateMeshBuffer();
    uint64_t CreateMeshBuffer(const std::string& name);
    OpenGLMeshBuffer& GetMeshBuffer(uint64_t id);
    OpenGLMeshBuffer& GetMeshBuffer(const std::string& name);
    OpenGLMeshBuffer* GetMeshBufferPtr(uint64_t id);
    OpenGLMeshBuffer* GetMeshBufferPtr(const std::string& name);
    void RemoveMeshBuffer(uint64_t id);

    uint64_t CreateShader(const std::string& name);
    OpenGLShader& LoadShader(const std::string& name, const std::vector<std::string>& shaderPaths, const std::vector<std::string>& defines = std::vector<std::string>());
    OpenGLShader& LoadShader(const std::string& subDirectory, const std::string& name, const std::vector<std::string>& shaderPaths, const std::vector<std::string>& defines = std::vector<std::string>());
    OpenGLShader& GetShader(const std::string& name);
    OpenGLShader* GetShaderPtr(const std::string& name);
    OpenGLShader& GetShaderById(uint64_t id);
    OpenGLShader* GetShaderPtrById(uint64_t id);
    void HotloadShaders();
    void RemoveShader(uint64_t id);

    OpenGLShadowCubeMapArray& CreateShadowCubeMapArray(const std::string& name);
    OpenGLShadowCubeMapArray& GetShadowCubeMapArray(const std::string& name);
    OpenGLShadowCubeMapArray* GetShadowCubeMapArrayPtr(const std::string& name);
    OpenGLShadowCubeMapArray& GetShadowCubeMapArrayById(uint64_t id);
    OpenGLShadowCubeMapArray* GetShadowCubeMapArrayPtrById(uint64_t id);
    void RemoveShadowCubeMapArray(uint64_t id);

    uint64_t CreateShadowMap(const std::string& name);
    OpenGLShadowMap& GetShadowMap(const std::string& name);
    OpenGLShadowMap* GetShadowMapPtr(const std::string& name);
    OpenGLShadowMap& GetShadowMapById(uint64_t id);
    OpenGLShadowMap* GetShadowMapPtrById(uint64_t id);
    void RemoveShadowMap(uint64_t id);

    uint64_t CreateShadowMapArray(const std::string& name);
    OpenGLShadowMapArray& GetShadowMapArray(const std::string& name);
    OpenGLShadowMapArray* GetShadowMapArrayPtr(const std::string& name);
    OpenGLShadowMapArray& GetShadowMapArrayById(uint64_t id);
    OpenGLShadowMapArray* GetShadowMapArrayPtrById(uint64_t id);
    void RemoveShadowMapArray(uint64_t id);

    OpenGLSSBO& CreateSSBO(const std::string& name);
    OpenGLSSBO& GetSSBO(const std::string& name);
    OpenGLSSBO* GetSSBOPtr(const std::string& name);
    OpenGLSSBO& GetSSBOById(uint64_t id);
    OpenGLSSBO* GetSSBOPtrById(uint64_t id);
    void RemoveSSBO(uint64_t id);
    void RemoveSSBOByName(const std::string& name);

    uint64_t CreateTexture();
    OpenGLTexture& GetTexture(uint64_t id);
    OpenGLTexture* GetTexturePtr(uint64_t id);
    void RemoveTexture(uint64_t id);

    uint64_t CreateTexture3D(const std::string& name);
    OpenGLTexture3D& GetTexture3D(const std::string& name);
    OpenGLTexture3D* GetTexture3DPtr(const std::string& name);
    OpenGLTexture3D& GetTexture3DById(uint64_t id);
    OpenGLTexture3D* GetTexture3DPtrById(uint64_t id);
    void RemoveTexture3D(uint64_t id);

    uint64_t CreateTextureArray(const std::string& name);
    OpenGLTextureArray& GetTextureArray(const std::string& name);
    OpenGLTextureArray* GetTextureArrayPtr(const std::string& name);
    OpenGLTextureArray& GetTextureArrayById(uint64_t id);
    OpenGLTextureArray* GetTextureArrayPtrById(uint64_t id);
    void RemoveTextureArray(uint64_t id);
}
