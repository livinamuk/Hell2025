#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Unloved/Debug/Scratch.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Viewport/ViewportManager.h"

#include "Hell/ResourceManagement/ResourceManager.h"

namespace OpenGL::Renderer {

    void GlassPass() {
        ProfilerOpenGLZoneFunction();

        const DrawCommandsSet& drawInfoSet = Unloved::RenderDataManager::GetDrawInfoSet();

        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("Glass");
        OpenGLFrameBuffer* gBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("GBuffer");
        OpenGLShadowMap* flashLightShadowMapsFBO = OpenGL::ResourceManager::GetShadowMapPtr("FlashlightShadowMaps");
        OpenGLShadowCubeMapArray* hiResShadowMaps = OpenGL::ResourceManager::GetShadowCubeMapArrayPtr("StaticHiRes");
        OpenGLShadowCubeMapArray* lowResShadowMaps = OpenGL::ResourceManager::GetShadowCubeMapArrayPtr("StaticLowRes");

        if (!shader) return;
        if (!gBuffer) return;
        if (!flashLightShadowMapsFBO) return;
        if (!hiResShadowMaps) return;
        if (!lowResShadowMaps) return;

        OpenGL::BindShader("Glass");
        OpenGL::BindSSBO(SSBO_IDX_SAMPLERS, "Samplers");
        OpenGL::BindSSBO(SSBO_IDX_MATERIALS, "Materials");
        OpenGL::BindSSBO(SSBO_IDX_RENDERER_DATA, "RendererData");
        OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");
        OpenGL::BindSSBO(SSBO_IDX_LIGHTS, "Lights");
        OpenGL::BindSSBO(SSBO_IDX_SPOT_LIGHTS, "SpotLights");
        OpenGL::BindSSBO(SSBO_IDX_SCENE_RENDER_ITEMS, "SceneRenderItems");
        OpenGL::BindSSBO(SSBO_IDX_DRAW_RENDER_ITEM_INDICES, "DrawRenderItemIndices");
        OpenGL::BindSSBO(SSBO_IDX_GLASS_LIGHT_RANGES, "GlassLightRanges");
        OpenGL::BindSSBO(SSBO_IDX_GLASS_LIGHT_INDICES, "GlassLightIndices");
        OpenGL::BindSSBO(SSBO_IDX_GLASS_SPOT_LIGHT_RANGES, "GlassSpotLightRanges");
        OpenGL::BindSSBO(SSBO_IDX_GLASS_SPOT_LIGHT_INDICES, "GlassSpotLightIndices");
        OpenGL::SetUniformBool("u_flipNormalMapY", ShouldFlipNormalMapY());
        OpenGL::SetUniformBool("u_pointShadowsEnabled", Debug::Scratch::GetBool("Glass Shadows", true));

        gBuffer->Bind();
        gBuffer->DrawBuffer("Lighting");

        OpenGLRasterizerState state;
        state.depthTestEnabled = true;
        state.blendEnable = true;
        state.cullfaceEnable = true;
        state.cullfaceMode = GL_BACK;
        state.depthMask = false;
        state.colorMask = true;
        state.depthFunc = GL_GREATER;
        state.blendFuncSrcfactor = GL_ONE;
        state.blendFuncDstfactor = GL_SRC1_COLOR;
        OpenGL::RasterizerStateManager::SetRasterizerState(state);

        glBindVertexArray(OpenGL::ResourceManager::GetMeshBuffer("AssetGeometry").GetVAO());
        glBindTextureUnit(TEX_IDX_SHADOW_MAP_FLASHLIGHT, flashLightShadowMapsFBO->GetDepthTextureHandle());
        glBindTextureUnit(TEX_IDX_SHADOW_MAP_HI_RES, hiResShadowMaps->GetDepthTexture());
        glBindTextureUnit(TEX_IDX_SHADOW_MAP_LOW_RES, lowResShadowMaps->GetDepthTexture());

        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            OpenGL::Renderer::SetViewport(gBuffer, viewport);
            OpenGL::SetUniformInt("u_viewportIndex", i);

            MultiDrawIndirect(drawInfoSet.glassDrawCommands[i]);
        }
    }
}
