#include "GL_renderer.h"
#include "Hell/Render/API/OpenGL/Types/GL_texture.h"

#include "Hell/ResourceManagement/ResourceManager.h"

namespace OpenGL::Renderer {
    using namespace Unloved;


    void BlitDebugTextures() {
        // Render decal painting shit
        if (false) {
            DebugBlitFrameBufferTexture("DecalPainting", "UVMap", 0, 0, 480, 480);
            //DebugBlitFrameBufferTexture("DecalMasks", "DecalMask0", 0, 480, 480, 480);
        }

        // World height map
        if (false) {
            OpenGLFrameBuffer* worldFrameBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("World");
            OpenGLFrameBuffer* roadFrameBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("Road");
            DebugBlitFrameBufferTexture("World", "HeightMap", 0, 0, worldFrameBuffer->GetWidth(), worldFrameBuffer->GetHeight());
            DebugBlitFrameBufferTexture("Road", "RoadMask", worldFrameBuffer->GetWidth(), 0, roadFrameBuffer->GetWidth(), roadFrameBuffer->GetHeight());
        }

        // Ocean
        if (false) {
            DebugBlitFrameBufferTexture("FFT_band0", "Displacement", 0, 0, 300, 300);
            DebugBlitFrameBufferTexture("FFT_band0", "Slope", 300, 0, 300, 300);
            DebugBlitFrameBufferTexture("FFT_band1", "Displacement", 0, 300, 300, 300);
            DebugBlitFrameBufferTexture("FFT_band1", "Slope", 300, 300, 300, 300);
        }

        // Fog
        if (false) {
            DebugBlitFrameBufferTexture("Fog", "Color", 0, 0, 1920 * 2, 1080 * 2);
        }

        // Test texture
        if (false) {
            Texture* texture = Hell::ResourceManager::GetTextureByName("Glock_ALB");
            if (texture) {
                OpenGLTexture& glTexture = texture->GetGLTexture();
                DebugBlitOpenGLTexture(glTexture.GetHandle(), 0.5f);
            }
        }

    }

    void DebugBlitOpenGLTexture(GLuint textureHandle, float scale) {
        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("DebugTextureBlit");
        if (!shader) return;

        OpenGL::BindShader("DebugTextureBlit");
        OpenGL::SetUniformFloat("u_scale", scale);

        static GLuint vao = 0;
        if (vao == 0) {
            glCreateVertexArrays(1, &vao);
        }

        // Check if depth test was enabled, store that, and disable depth test
        GLboolean lastDepth = glIsEnabled(GL_DEPTH_TEST);
        if (lastDepth) glDisable(GL_DEPTH_TEST);

        glBindVertexArray(vao);
        glBindTextureUnit(0, textureHandle);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // Restore last depth state
        if (lastDepth) glEnable(GL_DEPTH_TEST);
    }

    void DebugBlitFrameBufferTexture(const std::string& frameBufferName, const std::string& attachmentName, GLint dstX, GLint dstY, GLint width, GLint height) {
        OpenGLFrameBuffer* frameBuffer = OpenGL::ResourceManager::GetFrameBufferPtr(frameBufferName);
        if (!frameBuffer) {
            std::cout << "DebugBlitFrameBufferTexture() failed because frameBufferName '" << frameBufferName << "' was not found\n";
            return;
        }

        GLenum attachment = frameBuffer->GetColorAttachmentSlotByName(attachmentName.c_str());
        if (attachment == GL_INVALID_VALUE) {
            std::cout << "DebugBlitFrameBufferTexture() failed because attachmentName '" << attachmentName << "' was not found in frameBuffer '" << frameBufferName << "'\n";
            return;
        }

        glBindFramebuffer(GL_READ_FRAMEBUFFER, frameBuffer->GetHandle());
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glReadBuffer(attachment);
        glDrawBuffer(GL_BACK);
        glBlitFramebuffer(0, 0, frameBuffer->GetWidth(), frameBuffer->GetHeight(), dstX, dstY, dstX + width, dstY + height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }
}
