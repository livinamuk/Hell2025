#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Hell/Audio.h"
#include "Unloved/Render/Renderer.h"

#include "Unloved/Common/Constants.h"
#include "Unloved/Render/RendererConstants.h"

namespace OpenGL::Renderer {
    void ScreenspaceReflectionsPass() {
        if (!Unloved::Renderer::GetCurrentRendererSettings().screenspaceReflections)
            return;

        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer* gBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("GBuffer");
        OpenGLFrameBuffer* halfSizeFbo = OpenGL::ResourceManager::GetFrameBufferPtr("HalfSize");
        OpenGLFrameBuffer* fullSizeFBO = OpenGL::ResourceManager::GetFrameBufferPtr("MiscFullSize");
        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("ScreenspaceReflections");

        if (!gBuffer) return;
        if (!shader) return;
        if (!halfSizeFbo) return;
        if (!fullSizeFBO) return;

        // Down sample
        OpenGL::BlitFrameBuffer(gBuffer, halfSizeFbo, "Lighting", "DownsampledFinalLighting", GL_COLOR_BUFFER_BIT, GL_LINEAR);
        glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT);

        // Generate Mipmaps
        glGenerateTextureMipmap(halfSizeFbo->GetColorAttachmentHandleByName("DownsampledFinalLighting"));
        glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);

        OpenGL::BindShader("ScreenspaceReflections");
        OpenGL::BindSSBO(SSBO_IDX_RENDERER_DATA, "RendererData");
        OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");
        OpenGL::BindImageTexture(0, gBuffer->GetColorAttachmentHandleByName("Lighting"), GL_READ_WRITE, GL_RGBA16F);
        OpenGL::BindTextureUnit(1, gBuffer->GetColorAttachmentHandleByName("BaseColorMetallic"));
        OpenGL::BindTextureUnit(2, gBuffer->GetColorAttachmentHandleByName("NormalXYRoughnessMisc"));
        OpenGL::BindTextureUnit(3, gBuffer->GetColorAttachmentHandleByName("VelocityXYOcclusionSubSurface"));
        OpenGL::BindTextureUnit(4, gBuffer->GetDepthAttachmentHandle());
        OpenGL::BindTextureUnit(5, halfSizeFbo->GetColorAttachmentHandleByName("DownsampledFinalLighting"));
        OpenGL::BindTextureUnit(6, fullSizeFBO->GetColorAttachmentHandleByName("ViewspaceDepth"));

        OpenGL::DispatchCompute(gBuffer->GetWidth() / TILE_SIZE, gBuffer->GetHeight() / TILE_SIZE, 1);
    }
}
