#pragma once

#include "Hell/Render/API/OpenGL/Types/GL_frameBuffer.h"
#include "Hell/Render/API/OpenGL/Types/GL_shadow_cube_map_array.h"
#include "Hell/Render/RendererTypes.h"

#include <cstddef>
#include <cstdint>
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace OpenGL {
    // Dispatch
    void DispatchCompute(uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ);
    void DispatchComputeIndirect();

    // Shaders
    void BindShader(const std::string& name);
    void SetUniformBool(const std::string& name, bool value);
    void SetUniformInt(const std::string& name, int value);
    void SetUniformUInt(const std::string& name, uint32_t value);
    void SetUniformFloat(const std::string& name, float value);
    void SetUniformMat2(const std::string& name, const glm::mat2& value);
    void SetUniformMat3(const std::string& name, const glm::mat3& value);
    void SetUniformMat4(const std::string& name, const glm::mat4& value);
    void SetUniformIVec2(const std::string& name, const glm::ivec2& value);
    void SetUniformUVec2(const std::string& name, const glm::uvec2& value);
    void SetUniformVec2(const std::string& name, const glm::vec2& value);
    void SetUniformVec2(const std::string& name, float x, float y);
    void SetUniformVec3(const std::string& name, const glm::vec3& value);
    void SetUniformVec3(const std::string& name, float x, float y, float z);
    void SetUniformIVec3(const std::string& name, const glm::ivec3& value);
    void SetUniformUVec3(const std::string& name, const glm::uvec3& value);
    void SetUniformVec4(const std::string& name, const glm::vec4& value);
    void SetUniformVec4(const std::string& name, float x, float y, float z, float w);
    void SetUniformVec2Array(const std::string& name, const std::vector<glm::vec2>& data);
    void SetUniformIVec2Array(const std::string& name, const std::vector<glm::ivec2>& data);
    void UnbindShader();

    // SSBOS
    void BindDispatchBuffer(const std::string& name);
    void BindDrawIndirectBuffer(const std::string& name);
    void BindSSBO(uint32_t bindingIndex, const std::string& name);
    void BindSSBO(uint32_t bindingIndex, uint32_t vboHandle);
    void ClearSSBO(const std::string& name);
    void ClearSSBORange(const std::string& name, size_t offset, size_t size);
    void ReserveSSBO(const std::string& name, size_t size);
    void UpdateSSBO(const std::string& name, size_t size, const void* data);
    void UpdateSSBORange(const std::string& name, size_t offset, size_t size, const void* data);
    void UploadSSBOStatic(const std::string& name, size_t size, const void* data);

    // Textures
    void BindImageTexture(uint32_t bindingIndex, uint32_t textureHandle, uint32_t access, uint32_t format, bool layered = false);
    void BindImageTextureArray(uint32_t bindingIndex, uint32_t textureHandle, uint32_t access, uint32_t format);
    void BindTextureUnit(uint32_t bindingIndex, uint32_t textureHandle);

    // Blitting
    void BlitFrameBuffer(OpenGLFrameBuffer* srcFrameBuffer, OpenGLFrameBuffer* dstFrameBuffer, const char* srcName, const char* dstName, GLbitfield mask, GLenum filter);
    void BlitFrameBuffer(OpenGLFrameBuffer* srcFrameBuffer, OpenGLFrameBuffer* dstFrameBuffer, const char* srcName, const char* dstName, BlitRect srcRect, BlitRect dstRect, GLbitfield mask, GLenum filter);
    void BlitFrameBufferDepth(OpenGLFrameBuffer* srcFrameBuffer, OpenGLFrameBuffer* dstFrameBuffer);
    void BlitFrameBufferDepth(OpenGLFrameBuffer* srcFrameBuffer, OpenGLFrameBuffer* dstFrameBuffer, BlitRect srcRect, BlitRect dstRect);
    void BlitShadowCubeMapArray(OpenGLShadowCubeMapArray& src, OpenGLShadowCubeMapArray& dst, int srcLayerIndex, int dstLayerIndex);
    void BlitToDefaultFrameBuffer(OpenGLFrameBuffer* srcFrameBuffer, const char* srcName, GLbitfield mask, GLenum filter);
    void BlitToDefaultFrameBuffer(OpenGLFrameBuffer* srcFrameBuffer, const char* srcName, BlitRect srcRect, BlitRect dstRect, GLbitfield mask, GLenum filter);
}
