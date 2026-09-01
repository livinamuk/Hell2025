#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Hell/Render/API/OpenGL/GL_back_end.h"
#include "Hell/Backend/BackEnd.h"
#include "Hell/Common/String.h"
#include "Unloved/Config/Config.h"
#include "Unloved/Viewport/ViewportManager.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Hell/Audio.h"
namespace Audio = Hell::Audio;
#include "Unloved/Render/Renderer.h"
#include "Unloved/World/World.h"
#include "Unloved/Debug/Debug.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/Input.h"
namespace Input = Hell::Input;



namespace OpenGL::Renderer {

    bool g_cullFace = false;

    void RenderHairLayer(int peelCount);

    void UpdateHairDebugInput() {
        RendererSettings& renderSettings = Unloved::Renderer::GetCurrentRendererSettings();
        int peelCount = renderSettings.depthPeelCount;
        if (Input::KeyPressed(HELL_KEY_RIGHT) && peelCount < 7) {
            Audio::PlayAudio("UI_Select.wav", 1.0f);
            renderSettings.depthPeelCount++;
            Debug::BlitQuickDebugMessage("Hair depth peel count: " + std::to_string(renderSettings.depthPeelCount));
        }
        if (Input::KeyPressed(HELL_KEY_LEFT) && peelCount > 0) {
            Audio::PlayAudio("UI_Select.wav", 1.0f);
            renderSettings.depthPeelCount--;
            Debug::BlitQuickDebugMessage("Hair depth peel count: " + std::to_string(renderSettings.depthPeelCount));
        }

        if (Input::KeyPressed(HELL_KEY_Z)) {
            Audio::PlayAudio("UI_Select.wav", 1.0f);
            g_cullFace = !g_cullFace;
            Debug::BlitQuickDebugMessage("Hair cull face enabled: " + Hell::String::FormatBool(g_cullFace));
        }
    }

    void HairPass() {
        ProfilerOpenGLZoneFunction();

        const DrawCommandsSet& drawInfoSet = Unloved::RenderDataManager::GetDrawInfoSet();
        const RendererSettings& renderSettings = Unloved::Renderer::GetCurrentRendererSettings();
        const Resolutions& resolutions = Config::GetResolutions();

        OpenGLFrameBuffer* gBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("GBuffer");
        OpenGLFrameBuffer* hairFrameBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("Hair");
        OpenGLShader* depthPeelShader = OpenGL::ResourceManager::GetShaderPtr("HairDepthPeel");
        OpenGLShader* hairLightingShader = OpenGL::ResourceManager::GetShaderPtr("HairLighting");
        OpenGLShader* finalCompositeShader = OpenGL::ResourceManager::GetShaderPtr("HairFinalComposite");
        OpenGLShadowCubeMapArray* hiResShadowMaps = OpenGL::ResourceManager::GetShadowCubeMapArrayPtr("HiRes");
        OpenGLShadowCubeMapArray* lowResShadowMaps = OpenGL::ResourceManager::GetShadowCubeMapArrayPtr("LowRes");
        OpenGLShadowMap* flashlightShadowMaps = OpenGL::ResourceManager::GetShadowMapPtr("FlashlightShadowMaps");

        if (!finalCompositeShader) return;
        if (!gBuffer) return;
        if (!hairFrameBuffer) return;
        if (!depthPeelShader) return;
        if (!hairLightingShader) return;
        if (!hiResShadowMaps) return;
        if (!lowResShadowMaps) return;
        if (!flashlightShadowMaps) return;

        UpdateHairDebugInput();

        OpenGLMeshBuffer& meshBuffer = OpenGL::ResourceManager::GetMeshBuffer("AssetGeometry");

        // Clear textures do initial values
        hairFrameBuffer->Bind();
        hairFrameBuffer->ClearAttachment("Composite", 0, 0, 0, 0);
        hairFrameBuffer->ClearAttachmentR("ViewspaceDepthPrevious", 0.0f);

        // Bind skinned VBO to weighted VAO
        glBindVertexArray(OpenGL::BackEnd::GetSkinnedVertexDataVAO());
        glBindBuffer(GL_ARRAY_BUFFER, OpenGL::BackEnd::GetSkinnedVertexDataVBO());
        //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, OpenGL::BackEnd::GetWeightedVertexDataEBO());
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshBuffer.GetEBO());

        for (int j = 0; j < renderSettings.depthPeelCount; j++) {

            OpenGL::RasterizerStateManager::ForceRasterizerState("GeometryPass_Default");
            glDrawBuffer(GL_NONE);
            glDepthFunc(GL_LESS);
            glDepthMask(GL_TRUE);
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

            hairFrameBuffer->ClearDepthAttachment();

            OpenGL::BindShader("HairDepthPeel");
            OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");
            OpenGL::BindSSBO(SSBO_IDX_SCENE_RENDER_ITEMS, "SceneRenderItems");
            OpenGL::BindSSBO(SSBO_IDX_DRAW_RENDER_ITEM_INDICES, "DrawRenderItemIndices");
            OpenGL::BindImageTexture(0, hairFrameBuffer->GetColorAttachmentHandleByName("ViewspaceDepthPrevious"), GL_READ_ONLY, GL_R32F);
            OpenGL::BindTextureUnit(1, gBuffer->GetDepthAttachmentHandle());

            if (!g_cullFace) glDisable(GL_CULL_FACE);

            // Standard hair depth
            glBindVertexArray(meshBuffer.GetVAO());
            for (int i = 0; i < 4; i++) {
                if (drawInfoSet.hair[i].empty()) continue;

                Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
                if (!viewport->IsVisible()) continue;

                OpenGL::Renderer::SetViewport(hairFrameBuffer, viewport);
                OpenGL::SetUniformInt("u_viewportIndex", i);
                MultiDrawIndirect(drawInfoSet.hair[i]);
            }

            // Skinned hair
            glBindVertexArray(OpenGL::BackEnd::GetSkinnedVertexDataVAO());
            for (int i = 0; i < 4; i++) {
                if (drawInfoSet.skinnedHair[i].empty()) continue;

                Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
                if (!viewport->IsVisible()) continue;

                OpenGL::Renderer::SetViewport(hairFrameBuffer, viewport);
                OpenGL::SetUniformInt("u_viewportIndex", i);
                MultiDrawIndirect(drawInfoSet.skinnedHair[i]);
            }

            // Color pass
            OpenGL::RasterizerStateManager::ForceRasterizerState("HairLighting");

            if (!g_cullFace) glDisable(GL_CULL_FACE);

            hairFrameBuffer->DrawBuffers({ "Composite", "ViewspaceDepthPrevious" });

            int compositeBufferIndex = 0;
            int viewspaceDepthBufferIndex = 1;

            // Enable blending for draw buffer 0 (composite texture)
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            glDepthMask(GL_FALSE);
            glEnable(GL_BLEND);
            glDepthFunc(GL_EQUAL);
            glBlendEquationSeparatei(compositeBufferIndex, GL_FUNC_ADD, GL_FUNC_ADD);
            glBlendFuncSeparatei(compositeBufferIndex, /* RGB */ GL_ONE_MINUS_DST_ALPHA, GL_ONE, /* A*/ GL_ONE_MINUS_DST_ALPHA, GL_ONE);

            // Disable blending on draw buffer 1 (previous depth texture)
            glDisablei(GL_BLEND, viewspaceDepthBufferIndex);

            OpenGL::BindShader("HairLighting");
            OpenGL::BindSSBO(SSBO_IDX_SAMPLERS, "Samplers");
            OpenGL::BindSSBO(SSBO_IDX_MATERIALS, "Materials");
            OpenGL::BindSSBO(SSBO_IDX_RENDERER_DATA, "RendererData");
            OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");
            OpenGL::BindSSBO(SSBO_IDX_SCENE_RENDER_ITEMS, "SceneRenderItems");
            OpenGL::BindSSBO(SSBO_IDX_DRAW_RENDER_ITEM_INDICES, "DrawRenderItemIndices");
            OpenGL::BindSSBO(SSBO_IDX_LIGHTS, "Lights");
            OpenGL::BindSSBO(SSBO_IDX_SPOT_LIGHTS, "SpotLights");
            OpenGL::BindSSBO(SSBO_IDX_LIGHTING_TILE_LIGHTS, "TileLights");
            OpenGL::BindSSBO(SSBO_IDX_LIGHTING_TILE_SPOT_LIGHTS, "TileSpotLights");
            OpenGL::SetUniformVec3("u_moonlightDir", Unloved::World::GetMoonlightDirection());
            glBindTextureUnit(TEX_IDX_SHADOW_MAP_FLASHLIGHT, flashlightShadowMaps->GetDepthTextureHandle());
            glBindTextureUnit(TEX_IDX_SHADOW_MAP_HI_RES, hiResShadowMaps->GetDepthTexture());
            glBindTextureUnit(TEX_IDX_SHADOW_MAP_LOW_RES, lowResShadowMaps->GetDepthTexture());

            // Standard hair color
            glBindVertexArray(meshBuffer.GetVAO());
            for (int i = 0; i < 4; i++) {
                if (drawInfoSet.hair[i].empty()) continue;

                Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
                if (!viewport->IsVisible()) continue;

                OpenGL::Renderer::SetViewport(hairFrameBuffer, viewport);
                OpenGL::SetUniformInt("u_viewportIndex", i);
                MultiDrawIndirect(drawInfoSet.hair[i]);
            }

            // Skinned hair color
            glBindVertexArray(OpenGL::BackEnd::GetSkinnedVertexDataVAO());
            for (int i = 0; i < 4; i++) {
                if (drawInfoSet.skinnedHair[i].empty()) continue;

                Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
                if (!viewport->IsVisible()) continue;

                OpenGL::Renderer::SetViewport(hairFrameBuffer, viewport);
                OpenGL::SetUniformInt("u_viewportIndex", i);
                MultiDrawIndirect(drawInfoSet.skinnedHair[i]);
            }
        }

        // Composite peeled final color back into gbuffer
        OpenGL::BindShader("HairFinalComposite");

        OpenGL::BindImageTexture(0, gBuffer->GetColorAttachmentHandleByName("Lighting"), GL_READ_WRITE, GL_RGBA16F);
        OpenGL::BindTextureUnit(1, hairFrameBuffer->GetColorAttachmentHandleByName("Composite"));

        glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
        OpenGL::DispatchCompute((gBuffer->GetWidth() + 7) / 8, (gBuffer->GetHeight() + 7) / 8, 1);

        // Clean up
        glBindVertexArray(0);
    }
}
