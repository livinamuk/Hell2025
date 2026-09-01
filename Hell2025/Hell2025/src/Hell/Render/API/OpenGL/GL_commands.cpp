#include "GL_commands.h"

#include "Hell/Backend/BackEnd.h"
#include "Hell/Logging.h"
#include "Hell/Render/API/OpenGL/GL_resource_manager.h"

#include <glad/gl.h>
#include <iostream>

namespace OpenGL {

    namespace {
        OpenGLShader* g_boundShader = nullptr;

        int GetBoundUniformLocation(const std::string& name) {
            if (!g_boundShader) {
                return -1;
            }
            return g_boundShader->GetUniformLocation(name);
        }

        OpenGLSSBO* GetRequiredSSBO(const std::string& name, const char* functionName) {
            OpenGLSSBO* ssbo = OpenGL::ResourceManager::GetSSBOPtr(name);
            if (!ssbo) {
                Logging::Error() << "OpenGL::" << functionName << "() failed to get '" << name << "'\n";
                return nullptr;
            }
            return ssbo;
        }
    }

    void BindShader(const std::string& name) {
        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr(name);

        if (!shader) {
            Logging::Error() << "OpenGL::BindShader() failed to get '" << name << "'\n";
            return;
        }

        if (g_boundShader && shader == g_boundShader) {
            return;
        }

        g_boundShader = shader;
        glUseProgram(g_boundShader->GetHandle());
    }

    void DispatchCompute(uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ) {
        glDispatchCompute(groupsX, groupsY, groupsZ);
    }

    void DispatchComputeIndirect() {
        glDispatchComputeIndirect(0);
    }

    void SetUniformBool(const std::string& name, bool value) {
        if (int location = GetBoundUniformLocation(name); location != -1) {
            glUniform1i(location, static_cast<int>(value));
        }
    }

    void SetUniformInt(const std::string& name, int value) {
        if (int location = GetBoundUniformLocation(name); location != -1) {
            glUniform1i(location, value);
        }
    }

    void SetUniformUInt(const std::string& name, uint32_t value) {
        if (int location = GetBoundUniformLocation(name); location != -1) {
            glUniform1ui(location, value);
        }
    }

    void SetUniformFloat(const std::string& name, float value) {
        if (int location = GetBoundUniformLocation(name); location != -1) {
            glUniform1f(location, value);
        }
    }

    void SetUniformMat2(const std::string& name, const glm::mat2& value) {
        if (int location = GetBoundUniformLocation(name); location != -1) {
            glUniformMatrix2fv(location, 1, GL_FALSE, &value[0][0]);
        }
    }

    void SetUniformMat3(const std::string& name, const glm::mat3& value) {
        if (int location = GetBoundUniformLocation(name); location != -1) {
            glUniformMatrix3fv(location, 1, GL_FALSE, &value[0][0]);
        }
    }

    void SetUniformMat4(const std::string& name, const glm::mat4& value) {
        if (int location = GetBoundUniformLocation(name); location != -1) {
            glUniformMatrix4fv(location, 1, GL_FALSE, &value[0][0]);
        }
    }

    void SetUniformIVec2(const std::string& name, const glm::ivec2& value) {
        if (int location = GetBoundUniformLocation(name); location != -1) {
            glUniform2i(location, value.x, value.y);
        }
    }

    void SetUniformUVec2(const std::string& name, const glm::uvec2& value) {
        if (int location = GetBoundUniformLocation(name); location != -1) {
            glUniform2uiv(location, 1, &value[0]);
        }
    }

    void SetUniformVec2(const std::string& name, const glm::vec2& value) {
        if (int location = GetBoundUniformLocation(name); location != -1) {
            glUniform2fv(location, 1, &value[0]);
        }
    }

    void SetUniformVec2(const std::string& name, float x, float y) {
        if (int location = GetBoundUniformLocation(name); location != -1) {
            glUniform2f(location, x, y);
        }
    }

    void SetUniformVec3(const std::string& name, const glm::vec3& value) {
        if (int location = GetBoundUniformLocation(name); location != -1) {
            glUniform3fv(location, 1, &value[0]);
        }
    }

    void SetUniformVec3(const std::string& name, float x, float y, float z) {
        if (int location = GetBoundUniformLocation(name); location != -1) {
            glUniform3f(location, x, y, z);
        }
    }

    void SetUniformIVec3(const std::string& name, const glm::ivec3& value) {
        if (int location = GetBoundUniformLocation(name); location != -1) {
            glUniform3iv(location, 1, &value[0]);
        }
    }

    void SetUniformUVec3(const std::string& name, const glm::uvec3& value) {
        if (int location = GetBoundUniformLocation(name); location != -1) {
            glUniform3uiv(location, 1, &value[0]);
        }
    }

    void SetUniformVec4(const std::string& name, const glm::vec4& value) {
        if (int location = GetBoundUniformLocation(name); location != -1) {
            glUniform4fv(location, 1, &value[0]);
        }
    }

    void SetUniformVec4(const std::string& name, float x, float y, float z, float w) {
        if (int location = GetBoundUniformLocation(name); location != -1) {
            glUniform4f(location, x, y, z, w);
        }
    }

    void SetUniformVec2Array(const std::string& name, const std::vector<glm::vec2>& data) {
        if (int location = GetBoundUniformLocation(name); location != -1) {
            glUniform2fv(location, static_cast<GLsizei>(data.size()), reinterpret_cast<const float*>(data.data()));
        }
    }

    void SetUniformIVec2Array(const std::string& name, const std::vector<glm::ivec2>& data) {
        if (int location = GetBoundUniformLocation(name); location != -1) {
            glUniform2iv(location, static_cast<GLsizei>(data.size()), reinterpret_cast<const int*>(data.data()));
        }
    }

    void UnbindShader() {
        g_boundShader = nullptr;
        glUseProgram(0);
    }

    void BindDispatchBuffer(const std::string& name) {
        if (OpenGLSSBO* ssbo = GetRequiredSSBO(name, "BindDispatchBuffer")) {
            glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, ssbo->GetHandle());
        }
    }

    void BindDrawIndirectBuffer(const std::string& name) {
        if (OpenGLSSBO* ssbo = GetRequiredSSBO(name, "BindDrawIndirectBuffer")) {
            glBindBuffer(GL_DRAW_INDIRECT_BUFFER, ssbo->GetHandle());
        }
    }

    void BindSSBO(uint32_t bindingIndex, const std::string& name) {
        if (OpenGLSSBO* ssbo = GetRequiredSSBO(name, "BindSSBO")) {
            ssbo->Bind(bindingIndex);
        }
    }

    void BindSSBO(uint32_t bindingIndex, uint32_t vboHandle) {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingIndex, vboHandle);
    }

    void ClearSSBO(const std::string& name) {
        if (OpenGLSSBO* ssbo = GetRequiredSSBO(name, "ClearSSBO")) {
            ssbo->Clear();
        }
    }

    void ClearSSBORange(const std::string& name, size_t offset, size_t size) {
        if (OpenGLSSBO* ssbo = GetRequiredSSBO(name, "ClearSSBORange")) {
            ssbo->ClearRange(offset, size);
        }
    }

    void ReserveSSBO(const std::string& name, size_t size) {
        if (OpenGLSSBO* ssbo = GetRequiredSSBO(name, "ReserveSSBO")) {
            ssbo->Reserve(size);
        }
    }

    void UpdateSSBO(const std::string& name, size_t size, const void* data) {
        OpenGLSSBO* ssbo = GetRequiredSSBO(name, "UpdateSSBO");
        if (ssbo && size > 0) {
            ssbo->Update(size, data);
        }
    }

    void UpdateSSBORange(const std::string& name, size_t offset, size_t size, const void* data) {
        OpenGLSSBO* ssbo = GetRequiredSSBO(name, "UpdateSSBORange");
        if (ssbo && size > 0) {
            ssbo->UpdateRange(offset, size, data);
        }
    }

    void UploadSSBOStatic(const std::string& name, size_t size, const void* data) {
        OpenGLSSBO* ssbo = GetRequiredSSBO(name, "UploadSSBOStatic");
        if (ssbo && size > 0) {
            ssbo->UploadStatic(size, data);
        }
    }

    void BindImageTexture(uint32_t bindingIndex, uint32_t textureHandle, uint32_t access, uint32_t format, bool layered) {
        glBindImageTexture(static_cast<GLuint>(bindingIndex), static_cast<GLuint>(textureHandle), 0, layered, 0, static_cast<GLenum>(access), static_cast<GLenum>(format));
    }

    void BindImageTextureArray(uint32_t bindingIndex, uint32_t textureHandle, uint32_t access, uint32_t format) {
        glBindImageTexture(static_cast<GLuint>(bindingIndex), static_cast<GLuint>(textureHandle), 0, GL_TRUE, 0, static_cast<GLenum>(access), static_cast<GLenum>(format));
    }

    void BindTextureUnit(uint32_t bindingIndex, uint32_t textureHandle) {
        glBindTextureUnit(static_cast<GLuint>(bindingIndex), static_cast<GLuint>(textureHandle));
    }

    void BlitFrameBuffer(OpenGLFrameBuffer* srcFrameBuffer, OpenGLFrameBuffer* dstFrameBuffer, const char* srcName, const char* dstName, GLbitfield mask, GLenum filter) {
        GLint srcAttachmentSlot = srcFrameBuffer->GetColorAttachmentSlotByName(srcName);
        GLint dstAttachmentSlot = dstFrameBuffer->GetColorAttachmentSlotByName(dstName);
        if (srcAttachmentSlot != GL_INVALID_VALUE && dstAttachmentSlot != GL_INVALID_VALUE) {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, srcFrameBuffer->GetHandle());
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dstFrameBuffer->GetHandle());
            glReadBuffer(srcAttachmentSlot);
            glDrawBuffer(dstAttachmentSlot);
            BlitRect srcRect;
            srcRect.x0 = 0;
            srcRect.y0 = 0;
            srcRect.x1 = srcFrameBuffer->GetWidth();
            srcRect.y1 = srcFrameBuffer->GetHeight();
            BlitRect dstRect;
            dstRect.y0 = 0;
            dstRect.x0 = 0;
            dstRect.x1 = dstFrameBuffer->GetWidth();
            dstRect.y1 = dstFrameBuffer->GetHeight();
            glBlitFramebuffer(srcRect.x0, srcRect.y0, srcRect.x1, srcRect.y1, dstRect.x0, dstRect.y0, dstRect.x1, dstRect.y1, mask, filter);
        }
    }

    void BlitFrameBuffer(OpenGLFrameBuffer* srcFrameBuffer, OpenGLFrameBuffer* dstFrameBuffer, const char* srcName, const char* dstName, BlitRect srcRect, BlitRect dstRect, GLbitfield mask, GLenum filter) {
        GLint srcAttachmentSlot = srcFrameBuffer->GetColorAttachmentSlotByName(srcName);
        GLint dstAttachmentSlot = dstFrameBuffer->GetColorAttachmentSlotByName(dstName);
        if (srcAttachmentSlot != GL_INVALID_VALUE && dstAttachmentSlot != GL_INVALID_VALUE) {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, srcFrameBuffer->GetHandle());
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dstFrameBuffer->GetHandle());
            glReadBuffer(srcAttachmentSlot);
            glDrawBuffer(dstAttachmentSlot);
            glBlitFramebuffer(srcRect.x0, srcRect.y0, srcRect.x1, srcRect.y1, dstRect.x0, dstRect.y0, dstRect.x1, dstRect.y1, mask, filter);
        }
    }

    void BlitFrameBufferDepth(OpenGLFrameBuffer* srcFrameBuffer, OpenGLFrameBuffer* dstFrameBuffer) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, srcFrameBuffer->GetHandle());
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dstFrameBuffer->GetHandle());
        BlitRect srcRect{ 0, 0, srcFrameBuffer->GetWidth(), srcFrameBuffer->GetHeight() };
        BlitRect dstRect{ 0, 0, dstFrameBuffer->GetWidth(), dstFrameBuffer->GetHeight() };
        glBlitFramebuffer(srcRect.x0, srcRect.y0, srcRect.x1, srcRect.y1, dstRect.x0, dstRect.y0, dstRect.x1, dstRect.y1, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    }

    void BlitFrameBufferDepth(OpenGLFrameBuffer* srcFrameBuffer, OpenGLFrameBuffer* dstFrameBuffer, BlitRect srcRect, BlitRect dstRect) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, srcFrameBuffer->GetHandle());
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dstFrameBuffer->GetHandle());
        glBlitFramebuffer(srcRect.x0, srcRect.y0, srcRect.x1, srcRect.y1, dstRect.x0, dstRect.y0, dstRect.x1, dstRect.y1, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    }

    void BlitShadowCubeMapArray(OpenGLShadowCubeMapArray& src, OpenGLShadowCubeMapArray& dst, int srcLayerIndex, int dstLayerIndex, uint8_t faceMask) {
        if (srcLayerIndex < 0 || dstLayerIndex < 0) return;
        if (srcLayerIndex == dstLayerIndex && src.GetDepthTexture() == dst.GetDepthTexture()) return;

        if (src.GetSize() != dst.GetSize()) {
            std::cout << "BlitShadowCubeMapArray(): source and destination sizes differ\n";
            return;
        }

        if (srcLayerIndex >= src.GetLayerCount() || dstLayerIndex >= dst.GetLayerCount()) {
            std::cout << "BlitShadowCubeMapArray(): cubemap index out of range\n";
            return;
        }

        const GLsizei size = src.GetSize();

        faceMask &= uint8_t(0x3f);
        uint32_t faceIndex = 0;
        while (faceIndex < 6) {
            while (faceIndex < 6 && (faceMask & uint8_t(1u << faceIndex)) == 0) faceIndex++;
            if (faceIndex == 6) break;

            const uint32_t firstFace = faceIndex;
            while (faceIndex < 6 && (faceMask & uint8_t(1u << faceIndex)) != 0) faceIndex++;
            const GLsizei faceCount = static_cast<GLsizei>(faceIndex - firstFace);
            glCopyImageSubData(
                src.GetDepthTexture(),
                GL_TEXTURE_CUBE_MAP_ARRAY,
                0,
                0,
                0,
                srcLayerIndex * 6 + static_cast<GLint>(firstFace),
                dst.GetDepthTexture(),
                GL_TEXTURE_CUBE_MAP_ARRAY,
                0,
                0,
                0,
                dstLayerIndex * 6 + static_cast<GLint>(firstFace),
                size,
                size,
                faceCount);
        }
    }

    void BlitToDefaultFrameBuffer(OpenGLFrameBuffer* srcFrameBuffer, const char* srcName, GLbitfield mask, GLenum filter) {
        GLint srcAttachmentSlot = srcFrameBuffer->GetColorAttachmentSlotByName(srcName);
        if (srcAttachmentSlot != GL_INVALID_VALUE) {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, srcFrameBuffer->GetHandle());
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
            glReadBuffer(srcAttachmentSlot);
            glDrawBuffer(GL_BACK);
            BlitRect srcRect;
            srcRect.x0 = 0;
            srcRect.y0 = 0;
            srcRect.x1 = srcFrameBuffer->GetWidth();
            srcRect.y1 = srcFrameBuffer->GetHeight();
            BlitRect dstRect;
            dstRect.y0 = 0;
            dstRect.x0 = 0;
            dstRect.x1 = Hell::BackEnd::GetCurrentWindowWidth();
            dstRect.y1 = Hell::BackEnd::GetCurrentWindowHeight();
            glBlitFramebuffer(srcRect.x0, srcRect.y0, srcRect.x1, srcRect.y1, dstRect.x0, dstRect.y0, dstRect.x1, dstRect.y1, mask, filter);
        }
    }

    void BlitToDefaultFrameBuffer(OpenGLFrameBuffer* srcFrameBuffer, const char* srcName, BlitRect srcRect, BlitRect dstRect, GLbitfield mask, GLenum filter) {
        GLint srcAttachmentSlot = srcFrameBuffer->GetColorAttachmentSlotByName(srcName);
        if (srcAttachmentSlot != GL_INVALID_VALUE) {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, srcFrameBuffer->GetHandle());
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
            glReadBuffer(srcAttachmentSlot);
            glDrawBuffer(GL_BACK);
            glBlitFramebuffer(srcRect.x0, srcRect.y0, srcRect.x1, srcRect.y1, dstRect.x0, dstRect.y0, dstRect.x1, dstRect.y1, mask, filter);
        }
    }
}
