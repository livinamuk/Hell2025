#include "Unloved/Render/API/OpenGL/GL_renderer.h"

#include "Hell/Audio.h"
#include "Hell/Input.h"
#include "Legacy/Timer.hpp"
#include "Unloved/Render/Renderer.h"
#include "Unloved/Systems/DDGI/DDGIManager.h"
#include "Unloved/Systems/Ocean/Ocean.h"

#include "Hell/MemoryTracker/MemoryTracker.h"

namespace Audio = Hell::Audio;
namespace Input = Hell::Input;

namespace OpenGL::Renderer {

    void ClearRenderTargets();

    void RenderGame() {
        ProfilerOpenGLFrame();

		if (Unloved::Renderer::GetRendererMode() == RendererMode::RE_STYLE) {
			RenderGameREStyle();
			return;
		}



        ComputeOceanFFTPass();
        OceanHeightReadback();

        glDisable(GL_DITHER);

        if (Input::KeyPressed(HELL_KEY_N)) {
            Audio::PlayAudio(AUDIO_SELECT, 1.00f);
            FlipNormalMapY();
        }

        //BlitRoads();

        ComputeSkinningPass();
        ClearRenderTargets();

        UpdateSSBOS();
        RenderShadowMaps();
        SkyBoxPass();
        HeightMapPass();


        DecalPaintingPass();
        ProceduralGeometryPass();
        GeometryPass();
        PhysicsShapesPass();
        OcclusionHiZPass();
        GrassPass();
        MirrorGeometryPass();
        VatBloodPass();

        ComputeTileWorldBounds();
        ChristmasLightCullingPass();
        LightCullingPass();

        BloodDecalsPass();
        ComputeViewspaceDepth();

        // GI
        UpdateGlobalIllumintation();

        OpenGL::BindSSBO(SSBO_IDX_SAMPLERS, "Samplers");
        OpenGL::BindSSBO(SSBO_IDX_MATERIALS, "Materials");
        OpenGL::BindSSBO(SSBO_IDX_RENDERER_DATA, "RendererData");
        OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");
        OpenGL::BindSSBO(SSBO_IDX_LIGHTS, "Lights");
        OpenGL::BindSSBO(SSBO_IDX_LIGHTING_TILE_LIGHTS, "TileLights");
        OpenGL::BindSSBO(SSBO_IDX_TILE_WORLD_BOUNDS_OUTPUT, "TileWorldBounds");

        OpenGL::BindSSBO(SSBO_IDX_OLD_DEFERRED_PROBE_STATES, "ProbeStates");

        LightingPass();

        //FurPass();
        OceanGeometryPass();
        OceanUnderWaterFlags();
        OceanSurfaceCompositePass();

        GlassPass();
        ScreenspaceReflectionsPass();
        HairPass();
        //DepthPeeledTransparencyPass();
        PlasticPass();
        RayMarchFog();
        OceanUnderwaterBlurPass();
        OceanUnderwaterCompositePass();
        WinstonPass();
        SpriteSheetPass(); // Muzzle flash, etc
        InventoryGaussianPass();

        // Disabling lighting actually just clears it, that way you don't have fog and shit everywhere
        if (!Unloved::Renderer::GetCurrentRendererSettings().enableLighting) {
            OpenGLFrameBuffer& gBuffer = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
            gBuffer.Bind();
            gBuffer.ClearAttachment("Lighting", 0, 0, 0, 0);
        }

        const auto& rendererSettings = Unloved::Renderer::GetCurrentRendererSettings();
        if (rendererSettings.debugDrawPointCloud || rendererSettings.debugDrawPointCloudGrid || rendererSettings.debugDrawIrradianceProbes) {
            Hell::SlotMap<Unloved::DDGIVolume>& ddgiVolumes = Unloved::DDGIManager::GetVolumes();

            for (Unloved::DDGIVolume& ddgiVolume : ddgiVolumes) {
                if (rendererSettings.debugDrawPointCloud)       DrawPointCloud(ddgiVolume);
                if (rendererSettings.debugDrawPointCloudGrid)   DrawPointCloudGrid(ddgiVolume);
                if (rendererSettings.debugDrawIrradianceProbes) DrawProbes(ddgiVolume);
            }
        }

        PostProcessingPass();

        DebugViewPass();
        HeightMapBrushPreviewPass();
        DebugPass();

        ExamineItemPass();
        EditorPass();
        OutlinePass();

        //DownSampleFinalImage();

        //if (Input::KeyDown(HELL_KEY_U)) {
        //    OpenGLFrameBuffer* bloodFluidFbo = OpenGL::ResourceManager::GetFrameBufferPtr("BloodFluid");
        //    OpenGL::BlitFrameBuffer(bloodFluidFbo, &finalImageBuffer, "Depth", "Color", GL_COLOR_BUFFER_BIT, GL_LINEAR);
        //}
        //if (Input::KeyDown(HELL_KEY_Y)) {
        //    OpenGLFrameBuffer* bloodFluidFbo = OpenGL::ResourceManager::GetFrameBufferPtr("BloodFluid");
        //    OpenGL::BlitFrameBuffer(bloodFluidFbo, &finalImageBuffer, "Thickness", "Color", GL_COLOR_BUFFER_BIT, GL_LINEAR);
        //}
        //if (Input::KeyDown(HELL_KEY_T)) {
        //    OpenGLFrameBuffer* bloodFluidFbo = OpenGL::ResourceManager::GetFrameBufferPtr("BloodFluid");
        //    OpenGL::BlitFrameBuffer(bloodFluidFbo, &finalImageBuffer, "BlurIntermediate", "Color", GL_COLOR_BUFFER_BIT, GL_LINEAR);
        //}

        //BlitFog();

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
        BlitDebugTextures();

        // DEBUG RENDER FFT TEXTURES TO THE SCREEN
        if (Input::KeyPressed(HELL_KEY_5)) {
            Ocean::Settings settings = Ocean::GetSettings();
            settings.displayMode = Ocean::DisplayMode::BAND_0;
            Ocean::SetSettings(settings);
        }
        if (Input::KeyPressed(HELL_KEY_6)) {
            Ocean::Settings settings = Ocean::GetSettings();
            settings.displayMode = Ocean::DisplayMode::BAND_1;
            Ocean::SetSettings(settings);
        }
        if (Input::KeyPressed(HELL_KEY_7)) {
            Ocean::Settings settings = Ocean::GetSettings();
            settings.displayMode = Ocean::DisplayMode::COMBINED;
            Ocean::SetSettings(settings);
        }

    }

    void ClearRenderTargets() {
        glDepthMask(GL_TRUE);

        // Water
        OpenGLFrameBuffer& waterFrameBuffer = OpenGL::ResourceManager::GetFrameBuffer("Water");
        waterFrameBuffer.Bind();
        waterFrameBuffer.ClearAttachment("Lighting", 0, 0, 0, 0);
        waterFrameBuffer.ClearAttachmentUI("OceanFlags", 0, 0, 0, 0);
        waterFrameBuffer.ClearAttachmentUI("OceanMask", 0, 0, 0, 0);

        // GBuffer
        OpenGLFrameBuffer& gBuffer = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
        gBuffer.Bind();
        gBuffer.ClearAttachment("Lighting", 0, 0, 0, 1);
        gBuffer.ClearAttachment("BaseColorMetallic", 0, 0, 0, 1);
        gBuffer.ClearAttachment("NormalXYRoughnessMisc", 0, 0, 0, 1);
        gBuffer.ClearAttachment("VelocityXYOcclusionSubSurface", 0, 0, 0, 1);
        gBuffer.ClearAttachment("Emissive", 0.0f, 0.0f, 0.0f, 0.0f);
        gBuffer.ClearAttachmentUI("Visibility", 0, 0, 0, 0);
        gBuffer.ClearDepthAttachment(0.0f);
        gBuffer.ClearStencilBits(0);

        gBuffer.ClearAttachment("Glass", 0, 0, 0, 0); // TODO: remove me when/if u can

        // Decal mask
        OpenGLFrameBuffer& miscFullSizeFBO = OpenGL::ResourceManager::GetFrameBuffer("MiscFullSize");
        miscFullSizeFBO.Bind();
        miscFullSizeFBO.ClearTexImage("BloodScreenSpaceDecalMask", 0, 0, 0, 0);
    }
}
