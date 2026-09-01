#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Unloved/Render/Renderer.h"
#include "Unloved/Viewport/ViewportManager.h"
#include "World/LegacyWorld.h"

namespace OpenGL::Renderer {

    namespace {
        bool g_taaHistoryValid = false;
        int g_taaHistoryWidth = 0;
        int g_taaHistoryHeight = 0;
        uint32_t g_taaHistoryViewportMask = 0;

        void DispatchTAAViewport(const glm::ivec2& origin, const glm::ivec2& size, int32_t localSize) {
            OpenGL::SetUniformIVec2("u_viewportOrigin", origin);
            OpenGL::SetUniformIVec2("u_viewportSize", size);
            OpenGL::DispatchCompute((size.x + localSize - 1) / localSize, (size.y + localSize - 1) / localSize, 1);
        }

        void DispatchTAAViewports(const glm::ivec2& fullSize, int32_t localSize) {
            for (int32_t viewportIndex = 0; viewportIndex < 4; viewportIndex++) {
                const Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(viewportIndex);
                if (!viewport || !viewport->IsVisible()) continue;

                const glm::ivec2 origin = glm::ivec2(viewport->GetPosition() * glm::vec2(fullSize));
                const glm::ivec2 size = glm::ivec2(viewport->GetSize() * glm::vec2(fullSize));
                DispatchTAAViewport(origin, size, localSize);
            }
        }
    }

    // 1. TAA
    // 2. Emissive and bloom
    // 3. Tone mapping
    // 4. FXAA

    void TAAPass() {
        RendererSettings& rendererSettings = Unloved::Renderer::GetCurrentRendererSettings();
        if (!rendererSettings.enableTAA) {
            g_taaHistoryValid = false;
            return;
        }

        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer& gBuffer = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
        OpenGLFrameBuffer& taaFbo = OpenGL::ResourceManager::GetFrameBuffer("TAA");
        const glm::ivec2 renderSize(gBuffer.GetWidth(), gBuffer.GetHeight());
        const uint32_t activeViewportMask = Unloved::ViewportManager::GetActiveViewportMask();

        if (g_taaHistoryWidth != renderSize.x ||
            g_taaHistoryHeight != renderSize.y ||
            g_taaHistoryViewportMask != activeViewportMask) {
            g_taaHistoryValid = false;
        }

        glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);

        // FidelityFX TAA accumulation: current HDR, depth, history and velocity -> compressed output.
        OpenGL::BindShader("TAA");
        OpenGL::BindTextureUnit(0, gBuffer.GetColorAttachmentHandleByName("Lighting"));
        OpenGL::BindTextureUnit(1, gBuffer.GetDepthAttachmentHandle());
        OpenGL::BindTextureUnit(2, taaFbo.GetColorAttachmentHandleByName("History"));
        OpenGL::BindTextureUnit(3, gBuffer.GetColorAttachmentHandleByName("VelocityXYOcclusionSubSurface"));
        OpenGL::BindImageTexture(0, taaFbo.GetColorAttachmentHandleByName("Output"), GL_WRITE_ONLY, GL_RGBA16F);
        OpenGL::SetUniformBool("u_historyValid", g_taaHistoryValid);
        DispatchTAAViewports(renderSize, 16);

        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

        // FidelityFX post pass: restore HDR and replace history after all history reads are complete.
        OpenGL::BindShader("TAAPost");
        OpenGL::BindTextureUnit(0, taaFbo.GetColorAttachmentHandleByName("Output"));
        OpenGL::BindImageTexture(0, gBuffer.GetColorAttachmentHandleByName("Lighting"), GL_WRITE_ONLY, GL_RGBA16F);
        OpenGL::BindImageTexture(1, taaFbo.GetColorAttachmentHandleByName("History"), GL_WRITE_ONLY, GL_RGBA16F);
        DispatchTAAViewports(renderSize, 8);

        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

        g_taaHistoryValid = true;
        g_taaHistoryWidth = renderSize.x;
        g_taaHistoryHeight = renderSize.y;
        g_taaHistoryViewportMask = activeViewportMask;
    }

    void ToneMappingPassRE() {
        ProfilerOpenGLZoneFunction();
        OpenGLFrameBuffer& gBuffer = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
        OpenGLFrameBuffer& scratchFbo = OpenGL::ResourceManager::GetFrameBuffer("Scratch");

        OpenGL::BindShader("PostProcessing");
        OpenGL::BindSSBO(SSBO_IDX_RENDERER_DATA, "RendererData");
        OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");
        OpenGL::BindImageTexture(0, scratchFbo.GetColorAttachmentHandleByName("RGBA16F"), GL_WRITE_ONLY, GL_RGBA16F);
        OpenGL::BindImageTexture(1, gBuffer.GetColorAttachmentHandleByName("Lighting"), GL_READ_ONLY, GL_RGBA16F);

        OpenGL::DispatchCompute((scratchFbo.GetWidth() + 7) / 8, (scratchFbo.GetHeight() + 7) / 8, 1);
        glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_FRAMEBUFFER_BARRIER_BIT);
    }

    void FXAAPassRE() {
        RendererSettings& rendererSettings = Unloved::Renderer::GetCurrentRendererSettings();
        OpenGLFrameBuffer& gBuffer = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
        OpenGLFrameBuffer& scratchFbo = OpenGL::ResourceManager::GetFrameBuffer("Scratch");

        if (rendererSettings.enableFXAA) {
            ProfilerOpenGLZoneFunction();

            gBuffer.Bind();
            gBuffer.DrawBuffer("Lighting");

            OpenGL::BindShader("FXAA");
            OpenGL::BindSSBO(SSBO_IDX_RENDERER_DATA, "RendererData");
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
        else {
            OpenGL::BlitFrameBuffer(&scratchFbo, &gBuffer, "RGBA16F", "Lighting", GL_COLOR_BUFFER_BIT, GL_LINEAR);
        }
    }

    void PostProcessingPassRE() {
        RendererSettings& rendererSettings = Unloved::Renderer::GetCurrentRendererSettings();
        RendererOverrideState state = rendererSettings.rendererOverrideState;

        if (state == RendererOverrideState::NONE ||
            state == RendererOverrideState::CAMERA_NDOTL ||
            state == RendererOverrideState::INDIRECT_DIFFUSE) {

            TAAPass();
            EmissivePass();
            ToneMappingPassRE();
            FXAAPassRE();
        }
        else {
            g_taaHistoryValid = false;
        }
    }
}
