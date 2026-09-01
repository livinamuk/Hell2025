#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Unloved/Render/Renderer.h"
#include "Unloved/Viewport/ViewportManager.h"
#include "World/LegacyWorld.h"

namespace OpenGL::Renderer {

    // 1. Emissive and bloom
    // 2. Tone mapping
    // 3. FXAA

    void ToneMapping() {
        //ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer& gBuffer = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
        OpenGLFrameBuffer& scratchFbo = OpenGL::ResourceManager::GetFrameBuffer("Scratch");

        OpenGL::BindShader("PostProcessing");
        OpenGL::BindSSBO(SSBO_IDX_RENDERER_DATA, "RendererData");
        OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");

        OpenGL::BindImageTexture(0, scratchFbo.GetColorAttachmentHandleByName("RGBA16F"), GL_WRITE_ONLY, GL_RGBA16F);
        OpenGL::BindImageTexture(1, gBuffer.GetColorAttachmentHandleByName("Lighting"), GL_READ_ONLY, GL_RGBA16F);

        OpenGL::DispatchCompute((gBuffer.GetWidth() + 7) / 8, (gBuffer.GetHeight() + 7) / 8, 1);
        glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
    }

    void FXAA() {
        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer& gBuffer = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
        OpenGLFrameBuffer& scratchFbo = OpenGL::ResourceManager::GetFrameBuffer("Scratch");

        gBuffer.Bind();
        gBuffer.DrawBuffer("Lighting");

        OpenGL::BindShader("FXAA");
        OpenGL::BindTextureUnit(0, scratchFbo.GetColorAttachmentHandleByName("RGBA16F"));

        OpenGLRasterizerState state;
        state.depthTestEnabled = false;
        state.depthMask = false;
        state.cullfaceEnable = false;
        state.blendEnable = false;
        state.colorMask = true;
        OpenGL::RasterizerStateManager::SetRasterizerState(state);

        for (int32_t viewportIndex = 0; viewportIndex < 4; viewportIndex++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(viewportIndex);
            if (!viewport || !viewport->IsVisible()) continue;

            SetViewport(&gBuffer, viewport);
            RenderFullscreenTriangle();
        }
    }

    void PostProcessingPass() {
        RendererSettings& rendererSettings = Unloved::Renderer::GetCurrentRendererSettings();
        RendererOverrideState state = rendererSettings.rendererOverrideState;

        if (state == RendererOverrideState::NONE ||
            state == RendererOverrideState::CAMERA_NDOTL ||
            state == RendererOverrideState::INDIRECT_DIFFUSE) {
            if (rendererSettings.enableLighting) {
                EmissivePass();
            }
            ToneMapping();
            FXAA();
        }
    }
}
