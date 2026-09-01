#include "Unloved/Render/API/OpenGL/GL_renderer.h"

#include "Hell/Render/API/OpenGL/GL_back_end.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/Input.h"
#include "Hell/Time.h"
#include "Legacy/Timer.hpp"

#include "Unloved/Config/Config.h"
#include "Unloved/EditorSession/EditorSession.h"
#include "Unloved/Render/RendererConstants.h"
#include "Unloved/Render/RendererTypes.h"
#include "Unloved/Systems/Particles/ParticleManager.h"
#include "Unloved/Systems/DDGI/DDGIManager.h"
#include "Unloved/Systems/Ocean/Ocean.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Viewport/ViewportManager.h"
#include "Unloved/World/World.h"

#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Render/Renderer.h"

namespace Input = Hell::Input;

namespace OpenGL::Renderer {
    struct RESettings {
        glm::ivec2 gBufferResolution = glm::ivec2(1920, 1080);
        glm::ivec2 finalImageResolution = glm::ivec2(1920, 1080) / 2;
    } g_settings;

    void BlendedLighting();
    void ClearRenderTargetsRE();
    void CreateFramebuffersRE();

    void LightingPassRE();
    void SkyboxPassRE();
    void EmissiveForwardPass();
    void BubblesPass2();
    void BubblesPass3();

    void RenderFullscreenTriangle();

    void RenderGameREStyle() {
        ComputeOceanFFTPass();

        OpenGLRasterizerState state;
        state.depthMask = true;
        state.colorMask = true;
        OpenGL::RasterizerStateManager::ForceRasterizerState(state);

        ComputeSkinningPass();
        UpdateSSBOS();
        UpdateTerrainDisplacementBuffer();
        RenderShadowMaps();
        ClearRenderTargetsRE();

        DecalPaintingPass();

        VisibilityPass();
        VisibilityHeightMapPass();
        VisibilitySkinnedPass();
        VisibilityAlphaDiscardPass();
        VisibilitySkinnedHairPass();

        MaterialResolvePass();
        MaterialResolveHeightMapPass();
        MaterialResolveSkinnedPass();
        MaterialResolveProceduralPass();

        PhysicsShapesPass();
        OcclusionHiZPass();
        GrassPass();

        VatBloodPass();
        VATPass();

        EmissiveForwardPass();

        ComputeTileWorldBounds();
        ChristmasLightCullingPass();
        LightCullingPass();
        BloodDecalsPass();

        UpdateGlobalIllumintation();

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
        // TODO: OpenGL::BindSSBO(SSBO_IDX_LIGHTING_TILE_CHRISTMAS_LIGHTS, "TileChristmasLights");
        // TODO: OpenGL::BindSSBO(SSBO_IDX_LIGHTING_CHRISTMAS_LIGHTS, "ChristmasLightInstances");
        // TODO: OpenGL::BindSSBO(SSBO_IDX_LIGHTING_CHRISTMAS_INDEX_POOL, "ChristmasLightIndices");

        LightingPassRE();
        BlendedLighting();

        SkyboxPassRE();
        HairPassRE();
        PlasticPass();
        OceanGeometryPass();


        OceanUnderWaterFlags();
        OceanSurfaceCompositePass();

        GlassPass();
        RayMarchFog();
        OceanUnderwaterBlurPass();

        OceanUnderwaterCompositePass();

        WinstonPass();
        //StainedGlassPass();

        if (Unloved::World::HasOcean()) {
            BubblesPass2(); // wtf is this
            BubblesPass3(); // wtf is this
        }

        ParticlePass();

        // DDGI Debug
        const auto& rendererSettings = Unloved::Renderer::GetCurrentRendererSettings();
        if (rendererSettings.debugDrawPointCloud || rendererSettings.debugDrawPointCloudGrid || rendererSettings.debugDrawIrradianceProbes) {
            Hell::SlotMap<Unloved::DDGIVolume>& ddgiVolumes = Unloved::DDGIManager::GetVolumes();

            for (Unloved::DDGIVolume& ddgiVolume : ddgiVolumes) {
                if (rendererSettings.debugDrawPointCloud)       DrawPointCloud(ddgiVolume);
                if (rendererSettings.debugDrawPointCloudGrid)   DrawPointCloudGrid(ddgiVolume);
                if (rendererSettings.debugDrawIrradianceProbes) DrawProbes(ddgiVolume);
            }
        }

        SpriteSheetPass(); // Muzzle flash, etc
        FirePass();

        InventoryGaussianPass(); // TODO: optimize this. it is likely shit

        PostProcessingPassRE();
        DebugViewPass();
        HeightMapBrushPreviewPass();
        DebugPass();

        ExamineItemPass();

        EditorPass();
        OutlinePass();

        OpenGLFrameBuffer& finalImageFbo = OpenGL::ResourceManager::GetFrameBuffer("FinalImage");
        OpenGLFrameBuffer& gBuffer = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
        OpenGLFrameBuffer& presentFbo = OpenGL::ResourceManager::GetFrameBuffer("Present");

        // Downscale with linear filtering
        OpenGL::BlitFrameBuffer(&gBuffer, &finalImageFbo, "Lighting", "Color", GL_COLOR_BUFFER_BIT, GL_LINEAR);

        // Upscale with nearest filtering
        OpenGL::BlitFrameBuffer(&finalImageFbo, &presentFbo, "Color", "Color", GL_COLOR_BUFFER_BIT, GL_NEAREST);

        GameUIPass();

        PresentFinalImage(presentFbo);

        EditorUIPass();
    }


	void ClearRenderTargetsRE() {
		OpenGLFrameBuffer& gBuffer = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
        gBuffer.ClearAttachment("Lighting", 0, 0, 0, 1);
        gBuffer.ClearAttachment("BaseColorMetallic", 0, 0, 0, 1);
        gBuffer.ClearAttachment("NormalXYRoughnessMisc", 0, 0, 0, 1);
        gBuffer.ClearAttachment("VelocityXYOcclusionSubSurface", 0, 0, 0, 1);
        gBuffer.ClearAttachment("Glass", 0, 0, 0, 1);
        gBuffer.ClearAttachment("Emissive", 0.0f, 0.0f, 0.0f, 0.0f);
        gBuffer.ClearAttachmentUI("Visibility", 0, 0, 0, 0);
        gBuffer.ClearDepthAttachment(0.0f);
        gBuffer.ClearStencilBits(0);

		OpenGLFrameBuffer& hairFboRE = OpenGL::ResourceManager::GetFrameBuffer("HairRE");
		hairFboRE.ClearAttachment("Lighting", 0, 0, 0, 0);
		hairFboRE.ClearDepthAttachment(0.0f);

        OpenGLFrameBuffer& waterFrameBuffer = OpenGL::ResourceManager::GetFrameBuffer("Water");
        waterFrameBuffer.Bind();
        waterFrameBuffer.ClearAttachment("Lighting", 0, 0, 0, 0);
        waterFrameBuffer.ClearAttachmentUI("OceanFlags", 0, 0, 0, 0);
        waterFrameBuffer.ClearAttachmentUI("OceanMask", 0, 0, 0, 0);

        // Decal mask
        OpenGLFrameBuffer& miscFullSizeFBO = OpenGL::ResourceManager::GetFrameBuffer("MiscFullSize");
        miscFullSizeFBO.Bind();
        miscFullSizeFBO.ClearTexImage("BloodScreenSpaceDecalMask", 0, 0, 0, 0);
	}

	void BindShadowMapsRE() {
		OpenGLShadowMap& flashLightShadowMaps = OpenGL::ResourceManager::GetShadowMap("FlashlightShadowMaps");
		OpenGLShadowCubeMapArray& hiResShadowMaps = OpenGL::ResourceManager::GetShadowCubeMapArray("HiRes");
		OpenGLShadowCubeMapArray& lowResShadowMaps = OpenGL::ResourceManager::GetShadowCubeMapArray("LowRes");
		OpenGLShadowMapArray& moonShadowCascades = OpenGL::ResourceManager::GetShadowMapArray("MoonlightCSM");

		OpenGL::BindTextureUnit(TEX_IDX_SHADOW_MAP_FLASHLIGHT, flashLightShadowMaps.GetDepthTextureHandle());
		OpenGL::BindTextureUnit(TEX_IDX_SHADOW_MAP_HI_RES, hiResShadowMaps.GetDepthTexture());
		OpenGL::BindTextureUnit(TEX_IDX_SHADOW_MAP_LOW_RES, lowResShadowMaps.GetDepthTexture());
		OpenGL::BindTextureUnit(TEX_IDX_SHADOW_MAP_CSM, moonShadowCascades.GetDepthTexture());
	}

    void BubblesPass2() {
        //ProfilerOpenGLZoneFunction();

        const std::vector<ViewportData>& viewportData = Unloved::RenderDataManager::GetViewportData();

        OpenGLCubemapView& skyboxCubemapView = OpenGL::ResourceManager::GetCubemapView("SkyboxNightSky");
        OpenGLFrameBuffer& fbo = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");

        fbo.Bind();
        fbo.SetViewport();
        fbo.DrawBuffers({ "Lighting" });

        OpenGLRasterizerState state;
        state.depthTestEnabled = true;
        state.cullfaceEnable = false;
        state.depthMask = false;
        state.colorMask = true;
        state.depthFunc = GL_GREATER;

        state.blendEnable = true;
        state.blendFuncSrcfactor = GL_SRC_ALPHA;
        state.blendFuncDstfactor = GL_ONE_MINUS_SRC_ALPHA;

        OpenGL::RasterizerStateManager::SetRasterizerState(state);

        OpenGL::BindShader("Bubbles2");
        OpenGL::BindSSBO(SSBO_IDX_SAMPLERS, "Samplers");
        OpenGL::BindSSBO(SSBO_IDX_RENDERER_DATA, "RendererData");
        OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");
        OpenGL::BindSSBO(SSBO_IDX_SPOT_LIGHTS, "SpotLights");
        OpenGL::SetUniformFloat("u_time", Unloved::Session::GetSessionTime());
        OpenGL::BindTextureUnit(0, skyboxCubemapView.GetHandle());
        OpenGL::BindTextureUnit(1, GetTextureHandleByName("Bubbles_10x10"));

        BindEmptyVAO();

        ParticleManager::Update(Hell::Time::DeltaTime());

        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            OpenGL::Renderer::SetViewport(&fbo, viewport);
            OpenGL::SetUniformInt("u_viewportIndex", i);
            OpenGL::SetUniformMat4("u_projectionView", viewportData[i].jitteredProjectionViewReverseZ);
            OpenGL::SetUniformMat4("u_view", viewportData[i].view);
            OpenGL::SetUniformVec3("u_viewPos", viewportData[i].viewPos);

            for (Particle& particle : ParticleManager::GetParticles()) {
                OpenGL::SetUniformVec3("u_particlePosition", particle.position);
                OpenGL::SetUniformFloat("u_particleRotation", particle.rotation);
                OpenGL::SetUniformFloat("u_particleScale", particle.scale);
                OpenGL::SetUniformFloat("u_particleAlphaFade", particle.alphaFade);
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }
        }
    }

    void BubblesPass3() {
        //ProfilerOpenGLZoneFunction();

        const std::vector<ViewportData>& viewportData = Unloved::RenderDataManager::GetViewportData();

        OpenGLCubemapView& skyboxCubemapView = OpenGL::ResourceManager::GetCubemapView("SkyboxNightSky");
        OpenGLFrameBuffer& fbo = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");

        fbo.Bind();
        fbo.SetViewport();
        fbo.DrawBuffers({ "Lighting" });

        OpenGLRasterizerState state;
        state.depthTestEnabled = true;
        state.cullfaceEnable = false;
        state.depthMask = false;
        state.colorMask = true;
        state.depthFunc = GL_GREATER;

        state.blendEnable = true;
        state.blendFuncSrcfactor = GL_SRC_ALPHA;
        state.blendFuncDstfactor = GL_ONE_MINUS_SRC_ALPHA;

        OpenGL::RasterizerStateManager::SetRasterizerState(state);

        OpenGL::BindShader("Bubbles3");
        OpenGL::BindSSBO(SSBO_IDX_SAMPLERS, "Samplers");
        OpenGL::BindSSBO(SSBO_IDX_RENDERER_DATA, "RendererData");
        OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");
        OpenGL::BindSSBO(SSBO_IDX_SPOT_LIGHTS, "SpotLights");
        OpenGL::SetUniformFloat("u_time", Unloved::Session::GetSessionTime());
        OpenGL::BindTextureUnit(0, GetTextureHandleByName("UnderwaterBulletBubble"));

        BindEmptyVAO();

        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            OpenGL::Renderer::SetViewport(&fbo, viewport);
            OpenGL::SetUniformInt("u_viewportIndex", i);
            OpenGL::SetUniformMat4("u_projectionView", viewportData[i].jitteredProjectionViewReverseZ);
            OpenGL::SetUniformMat4("u_view", viewportData[i].view);
            OpenGL::SetUniformVec3("u_viewPos", viewportData[i].viewPos);

            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        //glm::vec3 pos = glm::vec3(36.0, 32.5, 37.0);
        //DrawPoint(pos, RED);
    }

    void BindDeferredLightingUniforms() {
        OpenGL::SetUniformFloat("u_oceanHeight", Unloved::World::HasOcean() ? Ocean::GetOceanOriginY() : -1000.0f);

        std::vector<float>& cascadeLevels = GetShadowCascadeLevels();
        OpenGL::SetUniformFloat("u_cascadeFarPlane", 256.0f); // ???
        OpenGL::SetUniformFloat("u_cascadePlaneDistances[0]", cascadeLevels[0]);
        OpenGL::SetUniformFloat("u_cascadePlaneDistances[1]", cascadeLevels[1]);
        OpenGL::SetUniformFloat("u_cascadePlaneDistances[2]", cascadeLevels[2]);
        OpenGL::SetUniformFloat("u_cascadePlaneDistances[3]", cascadeLevels[3]);
    }

    void LightingPassRE() {
        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer& gBuffer = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
        OpenGLFrameBuffer& indirectDiffuseFbo = OpenGL::ResourceManager::GetFrameBuffer("IndirectDiffuse");

        const bool useEditorRenderMode = Unloved::EditorSession::IsActive() && Unloved::EditorSession::GetRenderMode() != Unloved::EditorSession::EditorRenderMode::PBR;

        OpenGL::BindShader("LightingDeferred");

        if (useEditorRenderMode) {
            OpenGL::BindShader("LightingDeferredEditorRenderMode");
            OpenGL::SetUniformInt("u_editorRenderMode", static_cast<uint32_t>(Unloved::EditorSession::GetRenderMode()));
        }

        OpenGL::BindSSBO(SSBO_IDX_SAMPLERS, "Samplers");
        OpenGL::BindSSBO(SSBO_IDX_RENDERER_DATA, "RendererData");
        OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");
        OpenGL::BindSSBO(SSBO_IDX_SCENE_RENDER_ITEMS, "SceneRenderItems");
        OpenGL::BindSSBO(SSBO_IDX_LIGHTS, "Lights");
        OpenGL::BindSSBO(SSBO_IDX_SPOT_LIGHTS, "SpotLights");

        OpenGL::BindSSBO(SSBO_IDX_LIGHTING_TILE_LIGHTS, "TileLights");
        OpenGL::BindSSBO(SSBO_IDX_LIGHTING_TILE_SPOT_LIGHTS, "TileSpotLights");
        OpenGL::BindSSBO(SSBO_IDX_LIGHTING_TILE_CHRISTMAS_LIGHTS, "TileChristmasLights");
        OpenGL::BindSSBO(SSBO_IDX_LIGHTING_CHRISTMAS_LIGHTS, "ChristmasLightInstances");
        OpenGL::BindSSBO(SSBO_IDX_LIGHTING_CHRISTMAS_INDEX_POOL, "ChristmasLightIndices");

        OpenGL::BindTextureUnit(4, gBuffer.GetColorAttachmentHandleByName("BaseColorMetallic"));
        OpenGL::BindTextureUnit(5, gBuffer.GetColorAttachmentHandleByName("NormalXYRoughnessMisc"));
        OpenGL::BindTextureUnit(6, gBuffer.GetColorAttachmentHandleByName("VelocityXYOcclusionSubSurface"));
        OpenGL::BindTextureUnit(7, gBuffer.GetDepthAttachmentHandle());
        OpenGL::BindTextureUnit(8, indirectDiffuseFbo.GetColorAttachmentHandleByName("Color"));
        OpenGL::BindTextureUnit(10, indirectDiffuseFbo.GetColorAttachmentHandleByName("Surface"));

        BindShadowMapsRE();
        BindDeferredLightingUniforms();

        OpenGLFrameBuffer& fbo = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
        fbo.Bind();
        fbo.SetViewport();
        fbo.DrawBuffers({ "Lighting" });

        OpenGLRasterizerState state;
        state.depthTestEnabled = false;
        state.blendEnable = false;
        state.cullfaceEnable = false;
        state.depthMask = false;
        state.colorMask = true;

        state.stencilTestEnabled = true;
        state.stencilWriteMask = 0x00;
        state.stencilFailOp = GL_KEEP;
        state.stencilDepthFailOp = GL_KEEP;
        state.stencilPassOp = GL_KEEP;

        // Editor overrides do not use the dedicated foliage lighting path
        if (useEditorRenderMode) {
            state.stencilFunc = GL_NOTEQUAL;
            state.stencilRef = 0;
            state.stencilReadMask = STENCIL_BIT_WORLD_LIGHTING | STENCIL_BIT_VIEW_WEAPON_LIGHTING;
            OpenGL::RasterizerStateManager::SetRasterizerState(state);
            RenderFullscreenTriangle();
            return;
        }

        // World lighting, excluding grass
        OpenGL::SetUniformInt("u_grassLightingPass", 0);
        state.stencilFunc = GL_EQUAL;
        state.stencilReadMask = STENCIL_BIT_WORLD_LIGHTING | STENCIL_BIT_VIEW_WEAPON_LIGHTING | STENCIL_BIT_GRASS;
        state.stencilRef = STENCIL_BIT_WORLD_LIGHTING;
        OpenGL::RasterizerStateManager::SetRasterizerState(state);
        RenderFullscreenTriangle();

        // Grass
        const Config::Grass::Settings& grassSettings = Config::Grass::GetSettings();
        OpenGL::SetUniformInt("u_grassLightingPass", 1);
        OpenGL::SetUniformFloat("u_grassNormalUpBlend", grassSettings.normalUpBlend);
        OpenGL::SetUniformFloat("u_grassNormalBlendStartDistance", grassSettings.normalBlendStartDistance);
        OpenGL::SetUniformFloat("u_grassNormalBlendEndDistance", grassSettings.normalBlendEndDistance);
        OpenGL::SetUniformFloat("u_grassDiffuseWrap", grassSettings.diffuseWrap);
        OpenGL::SetUniformFloat("u_grassTransmissionPower", grassSettings.transmissionPower);
        OpenGL::SetUniformFloat("u_grassSpecularStrength", grassSettings.specularStrength);

        state.stencilRef = STENCIL_BIT_WORLD_LIGHTING | STENCIL_BIT_GRASS;
        OpenGL::RasterizerStateManager::SetRasterizerState(state);
        RenderFullscreenTriangle();

        // Bail now if in the editor
        if (useEditorRenderMode) {
            return;
        }

        // View-weapon lighting
        state.stencilRef = STENCIL_BIT_VIEW_WEAPON_LIGHTING;
        OpenGL::RasterizerStateManager::SetRasterizerState(state);

        OpenGL::BindShader("LightingDeferredViewWeapon");
        BindDeferredLightingUniforms();
        RenderFullscreenTriangle();
	}

	void BlendedLighting() {
		ProfilerOpenGLZoneFunction();

		const DrawCommandsSet& drawInfoSet = Unloved::RenderDataManager::GetDrawInfoSet();

		OpenGLFrameBuffer& fbo = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
        OpenGLFrameBuffer& indirectDiffuseFbo = OpenGL::ResourceManager::GetFrameBuffer("IndirectDiffuse");
		fbo.Bind();
		fbo.DrawBuffers({ "Lighting" });

		OpenGLRasterizerState state;
		state.depthTestEnabled = true;
		state.blendEnable = true;
		state.cullfaceEnable = false;
		state.depthMask = false;
		state.colorMask = true;
		state.depthFunc = GL_GREATER;

		// Opaque
		OpenGL::BindShader("LightingForward");
        OpenGL::BindSSBO(SSBO_IDX_SAMPLERS, "Samplers");
        OpenGL::BindSSBO(SSBO_IDX_MATERIALS, "Materials");
        OpenGL::BindSSBO(SSBO_IDX_RENDERER_DATA, "RendererData");
        OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");
        OpenGL::BindSSBO(SSBO_IDX_SCENE_RENDER_ITEMS, "SceneRenderItems");
        OpenGL::BindSSBO(SSBO_IDX_DRAW_RENDER_ITEM_INDICES, "DrawRenderItemIndices");
        OpenGL::BindSSBO(SSBO_IDX_LIGHTS, "Lights");
        BindShadowMapsRE();
        OpenGL::BindTextureUnit(5, indirectDiffuseFbo.GetColorAttachmentHandleByName("Color"));
        OpenGL::BindTextureUnit(10, indirectDiffuseFbo.GetColorAttachmentHandleByName("Surface"));

        OpenGLMeshBuffer& meshBuffer = OpenGL::ResourceManager::GetMeshBuffer("AssetGeometry");
		glBindVertexArray(meshBuffer.GetVAO());
		MultiDrawPerViewportRE(fbo, drawInfoSet.blended, state);

		//glBindVertexArray(OpenGL::BackEnd::GetWeightedVertexDataVAO());
		MultiDrawPerViewportRE(fbo, drawInfoSet.skinnedNonDeformingBlended, state);

		glBindVertexArray(OpenGL::BackEnd::GetSkinnedVertexDataVAO());
		MultiDrawPerViewportRE(fbo, drawInfoSet.skinnedBlended, state);
	}

    void SkyboxPassRE() {
        if (Unloved::EditorSession::IsActive()) return;

        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer& gBuffer = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
        OpenGLCubemapView* skyboxCubemapView = OpenGL::ResourceManager::GetCubemapViewPtr("SkyboxNightSky");

        gBuffer.Bind();
        gBuffer.DrawBuffers({ "Lighting" });

        OpenGL::BindShader("SkyboxRE");
        OpenGL::BindSSBO(SSBO_IDX_RENDERER_DATA, "RendererData");
        OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");

        OpenGLRasterizerState state;
        state.depthTestEnabled = false;
        state.blendEnable = false;
        state.cullfaceEnable = false;
        state.depthMask = false;
        state.depthFunc = GL_GREATER;

        state.stencilTestEnabled = true;
        state.stencilFunc = GL_EQUAL;
        state.stencilRef = 0; // This is any non-rendered pixel
        state.stencilReadMask = 0xFF;
        state.stencilWriteMask = 0x00;
        state.stencilFailOp = GL_KEEP;
        state.stencilDepthFailOp = GL_KEEP;
        state.stencilPassOp = GL_KEEP;

        OpenGL::RasterizerStateManager::SetRasterizerState(state);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxCubemapView->GetHandle());
        glBindVertexArray(OpenGL::ResourceManager::GetMeshBuffer("AssetGeometry").GetVAO());

        for (int32_t viewportIndex = 0; viewportIndex < 4; viewportIndex++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(viewportIndex);
            if (!viewport || !viewport->IsVisible()) continue;

            SetViewport(&gBuffer, viewport);
            RenderFullscreenTriangle();
        }
    }

    void EmissiveForwardPass() {
        ProfilerOpenGLZoneFunction();
        const DrawCommandsSet& drawInfoSet = Unloved::RenderDataManager::GetDrawInfoSet();

        OpenGLFrameBuffer& fbo = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
        fbo.Bind();
        fbo.DrawBuffers({ "Emissive" });

        OpenGL::BindShader("EmissiveForward");

        OpenGL::BindSSBO(SSBO_IDX_SAMPLERS, "Samplers");
        OpenGL::BindSSBO(SSBO_IDX_MATERIALS, "Materials");
        OpenGL::BindSSBO(SSBO_IDX_RENDERER_DATA, "RendererData");
        OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");
        OpenGL::BindSSBO(SSBO_IDX_SCENE_RENDER_ITEMS, "SceneRenderItems");
        OpenGL::BindSSBO(SSBO_IDX_DRAW_RENDER_ITEM_INDICES, "DrawRenderItemIndices");

        OpenGLRasterizerState state;
        state.depthTestEnabled = true;
        state.blendEnable = false;
        state.cullfaceEnable = false;
        state.depthMask = false;
        state.colorMask = true;
        state.depthFunc = GL_EQUAL;

        glBindVertexArray(OpenGL::ResourceManager::GetMeshBuffer("AssetGeometry").GetVAO());

        MultiDrawPerViewportRE(fbo, drawInfoSet.emissive, state);
    }

    void RenderFullscreenTriangle() {
        glBindVertexArray(OpenGL::ResourceManager::GetMeshBuffer("AssetGeometry").GetVAO());
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }
}
