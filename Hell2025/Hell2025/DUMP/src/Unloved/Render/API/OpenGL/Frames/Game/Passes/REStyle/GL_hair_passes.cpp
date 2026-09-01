#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Hell/Render/API/OpenGL/GL_back_end.h"
#include "Unloved/Render/RendererConstants.h"
#include "Unloved/Render/Renderer.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Hell/Audio.h"
namespace Audio = Hell::Audio;
#include "Unloved/Debug/Debug.h"

#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/Input.h"
namespace Input = Hell::Input;


namespace OpenGL::Renderer {

    //void HairLightingSkinnedResolvePass();
    void HairDepthPrep();
    void HairDepthPrePassRE();
    void HairForwardLightingPassRE();
    void HairCompositeRE();

    void HairPassRE() {
        HairDepthPrep();
        HairDepthPrePassRE();
        HairForwardLightingPassRE();
        HairCompositeRE();
    }

    void HairDepthPrep() {
        ProfilerOpenGLZoneFunction();

        static uint32_t dummyVao = 0;
        if (dummyVao == 0) {
            glGenVertexArrays(1, &dummyVao);
        }

        OpenGLFrameBuffer& gBuffer = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
        OpenGLFrameBuffer& hairfbo = OpenGL::ResourceManager::GetFrameBuffer("HairRE");

        hairfbo.Bind();
        hairfbo.SetViewport();
        hairfbo.DrawBuffer("Lighting");

        OpenGL::BindShader("HairDepthPrep");
        OpenGL::BindTextureUnit(0, gBuffer.GetDepthAttachmentHandle());

        OpenGLRasterizerState state;
        state.depthTestEnabled = true;
        state.blendEnable = false;
        state.cullfaceEnable = false;
        state.depthMask = true;
        state.colorMask = true;
        state.depthFunc = GL_ALWAYS;

        state.stencilTestEnabled = true;
        state.stencilFunc = GL_EQUAL;
        state.stencilRef = 0;
        state.stencilReadMask = STENCIL_BIT_HAIR;
        state.stencilWriteMask = 0x00;
        state.stencilFailOp = GL_KEEP;
        state.stencilDepthFailOp = GL_KEEP;
        state.stencilPassOp = GL_KEEP;

        OpenGL::RasterizerStateManager::SetRasterizerState(state);
        RenderFullscreenTriangle();
    }

    void HairDepthPrePassRE() {
        ProfilerOpenGLZoneFunction();

        const DrawCommandsSet& drawInfoSet = Unloved::RenderDataManager::GetDrawInfoSet();

        OpenGLFrameBuffer& fbo = OpenGL::ResourceManager::GetFrameBuffer("HairRE");
        fbo.Bind();
        fbo.DrawBuffer(GL_NONE);

        OpenGLRasterizerState state;
        state.depthTestEnabled = true;
        state.blendEnable = false;
        state.cullfaceEnable = false;
        state.depthMask = true;
        state.colorMask = false;
        state.depthFunc = GL_GREATER;

        // Masked
        OpenGL::BindShader("DepthPrePassAlphaDiscardRE");
        OpenGL::BindSSBO(SSBO_IDX_SAMPLERS, "Samplers");
        OpenGL::BindSSBO(SSBO_IDX_MATERIALS, "Materials");
        OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");
        OpenGL::BindSSBO(SSBO_IDX_SCENE_RENDER_ITEMS, "SceneRenderItems");
        OpenGL::BindSSBO(SSBO_IDX_DRAW_RENDER_ITEM_INDICES, "DrawRenderItemIndices");

        OpenGLMeshBuffer& meshBuffer = OpenGL::ResourceManager::GetMeshBuffer("AssetGeometry");
        glBindVertexArray(meshBuffer.GetVAO());
        MultiDrawPerViewportRE(fbo, drawInfoSet.hair, state);
        MultiDrawPerViewportRE(fbo, drawInfoSet.skinnedNonDeformingHair, state);

        glBindVertexArray(OpenGL::BackEnd::GetSkinnedVertexDataVAO());
        MultiDrawPerViewportRE(fbo, drawInfoSet.skinnedHair, state);

        glBindVertexArray(0);
    }

    void HairForwardLightingPassRE() {
        ProfilerOpenGLZoneFunction();
        const DrawCommandsSet& drawInfoSet = Unloved::RenderDataManager::GetDrawInfoSet();

        OpenGLFrameBuffer& fbo = OpenGL::ResourceManager::GetFrameBuffer("HairRE");
        OpenGLFrameBuffer& indirectDiffuseFbo = OpenGL::ResourceManager::GetFrameBuffer("IndirectDiffuse");

        fbo.Bind();
        fbo.DrawBuffers({ "Lighting" });

        BindShader("HairLightingForward");
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
        SetUniformFloat("u_renderResolutionScale", 1.0f);

        BindShadowMapsRE();
        BindTextureUnit(5, indirectDiffuseFbo.GetColorAttachmentHandleByName("Color"));
        BindTextureUnit(10, indirectDiffuseFbo.GetColorAttachmentHandleByName("Surface"));

        OpenGLRasterizerState maskedState;
        maskedState.blendEnable = false;
        maskedState.cullfaceEnable = false;
        maskedState.colorMask = true;
        maskedState.depthFunc = GL_EQUAL;
        maskedState.depthMask = false;
        maskedState.depthTestEnabled = true;

        glBindVertexArray(OpenGL::BackEnd::GetSkinnedVertexDataVAO());
        MultiDrawPerViewportRE(fbo, drawInfoSet.skinnedHair, maskedState);

        BindShader("LightingForward");
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
        SetUniformBool("u_solidAlpha", true);

        OpenGLMeshBuffer& meshBuffer = OpenGL::ResourceManager::GetMeshBuffer("AssetGeometry");
        glBindVertexArray(meshBuffer.GetVAO());
        MultiDrawPerViewportRE(fbo, drawInfoSet.hair, maskedState);
        MultiDrawPerViewportRE(fbo, drawInfoSet.skinnedNonDeformingHair, maskedState);

        SetUniformBool("u_solidAlpha", false);
    }

    void HairCompositeRE() {
        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer& gBuffer = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
        OpenGLFrameBuffer& hairFbo = OpenGL::ResourceManager::GetFrameBuffer("HairRE");

        BindShader("HairCompositeRE");

        BindImageTexture(0, gBuffer.GetColorAttachmentHandleByName("Lighting"), GL_READ_WRITE, GL_RGBA16F);
        BindImageTexture(1, gBuffer.GetColorAttachmentHandleByName("Emissive"), GL_WRITE_ONLY, GL_RGBA8);
        BindTextureUnit(2, hairFbo.GetColorAttachmentHandleByName("Lighting"));

        DispatchCompute(gBuffer.GetWidth() / TILE_SIZE, gBuffer.GetHeight() / TILE_SIZE, 1);
    }
}
