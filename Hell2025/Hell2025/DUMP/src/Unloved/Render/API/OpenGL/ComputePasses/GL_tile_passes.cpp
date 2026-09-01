#include "Legacy/World/LegacyWorld.h"

#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Unloved/Render/Renderer.h"
#include "Unloved/Render/RendererTypes.h"
#include "Unloved/Systems/BloodOLD/BloodSystemOLD.h"
#include "Unloved/World/World.h"

namespace OpenGL::Renderer {

    namespace {
        std::vector<GPUChristmasLight> g_christmasLights;
    }

    void ComputeTileWorldBounds() {
        ProfilerOpenGLZoneFunction();

        uint32_t depthHandle = 0;

		switch (Unloved::Renderer::GetRendererMode()) {
		    case RendererMode::OLD_DEFERRED: depthHandle = OpenGL::ResourceManager::GetFrameBuffer("GBuffer").GetDepthAttachmentHandle();   break;
            case RendererMode::RE_STYLE:     depthHandle = OpenGL::ResourceManager::GetFrameBuffer("GBuffer").GetDepthAttachmentHandle(); break;
		}

        OpenGL::BindShader("TileWorldBounds");
        OpenGL::SetUniformInt("u_tileXCount", GetTileCountX());
		OpenGL::SetUniformInt("u_tileYCount", GetTileCountY());

        OpenGL::BindSSBO(SSBO_IDX_RENDERER_DATA, "RendererData");
        OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");
        OpenGL::BindSSBO(SSBO_IDX_TILE_WORLD_BOUNDS_OUTPUT, "TileWorldBounds");

        OpenGL::BindTextureUnit(0, depthHandle);

        OpenGL::DispatchCompute(GetTileCountX(), GetTileCountY(), 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    void LightCullingPass() {
        ProfilerOpenGLZoneFunction();

        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("LightCulling");

        if (!shader) return;

        OpenGL::BindShader("LightCulling");

        OpenGL::BindSSBO(SSBO_IDX_RENDERER_DATA, "RendererData");
        OpenGL::BindSSBO(SSBO_IDX_LIGHTS, "Lights");
        OpenGL::BindSSBO(SSBO_IDX_SPOT_LIGHTS, "SpotLights");
        OpenGL::BindSSBO(SSBO_IDX_LIGHT_CULLING_TILE_LIGHTS, "TileLights");
        OpenGL::BindSSBO(SSBO_IDX_LIGHT_CULLING_TILE_WORLD_BOUNDS, "TileWorldBounds");
        OpenGL::BindSSBO(SSBO_IDX_LIGHT_CULLING_TILE_SPOT_LIGHTS, "TileSpotLights");

        OpenGL::DispatchCompute(GetTileCountX(), GetTileCountY(), 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    void ChristmasLightCullingPass() {
        ProfilerOpenGLZoneFunction();

        // Clear the lights from last frame, coz they change
        g_christmasLights.clear();

        // Gather all the Christmas lights from ALL the ChristmasLightSets
        for (Unloved::ChristmasLightSet& christmasLightSet : Unloved::World::GetChristmasLightSets()) {
            const std::vector<GPUChristmasLight>& gpuLights = christmasLightSet.GetGPUChristmasLights();
            g_christmasLights.insert(g_christmasLights.end(), gpuLights.begin(), gpuLights.end());
        }

        // If no Christmas lights found, then initialize the SSBOs to zero
        if (g_christmasLights.empty()) {
            OpenGL::ClearSSBO("TileChristmasLights");
            OpenGL::ClearSSBO("ChristmasLightInstances");
            OpenGL::ClearSSBO("ChristmasLightIndices");
            OpenGL::ClearSSBO("ChristmasLightCounter");
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

            return;
        }


        OpenGL::UpdateSSBO("ChristmasLightInstances", g_christmasLights.size() * sizeof(GPUChristmasLight), g_christmasLights.data());
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        // Debug draw the lights as points
        //for (const GPUChristmasLight& light : g_gpuLights) {
        //    DebugDraw::DrawPoint(light.position, light.color);
        //}

        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("ChristmasLightCulling");
        if (!shader) return;

        OpenGL::BindShader("ChristmasLightCulling");
        OpenGL::BindSSBO(SSBO_IDX_RENDERER_DATA, "RendererData");
        OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");
        OpenGL::SetUniformInt("u_christmasLightCount", g_christmasLights.size());
        OpenGL::SetUniformInt("u_tileXCount", GetTileCountX());
        OpenGL::SetUniformInt("u_tileYCount", GetTileCountY());

        OpenGL::BindSSBO(SSBO_IDX_CHRISTMAS_CULLING_TILE_WORLD_BOUNDS, "TileWorldBounds");
        OpenGL::BindSSBO(SSBO_IDX_CHRISTMAS_CULLING_TILE_LIGHTS, "TileChristmasLights");
        OpenGL::BindSSBO(SSBO_IDX_CHRISTMAS_CULLING_LIGHTS, "ChristmasLightInstances");
        OpenGL::BindSSBO(SSBO_IDX_CHRISTMAS_CULLING_INDEX_POOL, "ChristmasLightIndices");
        OpenGL::BindSSBO(SSBO_IDX_CHRISTMAS_CULLING_COUNTER, "ChristmasLightCounter");

        OpenGL::DispatchCompute(GetTileCountX(), GetTileCountY(), 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    void BloodDecalTileCulling() {
        ProfilerOpenGLZoneFunction();

        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("BloodDecalsCulling");
        if (!shader) return;

        OpenGL::BindShader("BloodDecalsCulling");
        OpenGL::BindSSBO(SSBO_IDX_RENDERER_DATA, "RendererData");
        OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");
        OpenGL::SetUniformInt("u_decalCount", static_cast<int>(Unloved::BloodSystemOLD::GetBloodScreenSpaceDecals().size()));
        OpenGL::SetUniformInt("u_tileXCount", static_cast<int>(GetTileCountX()));
        OpenGL::SetUniformInt("u_tileYCount", static_cast<int>(GetTileCountY()));

        OpenGL::BindSSBO(SSBO_IDX_BLOOD_CULLING_TILE_WORLD_BOUNDS, "TileWorldBounds");
        OpenGL::BindSSBO(SSBO_IDX_BLOOD_CULLING_TILE_DECALS, "TileBloodDecals");
        OpenGL::BindSSBO(SSBO_IDX_BLOOD_CULLING_DECALS, "BloodDecalInstances");
        OpenGL::BindSSBO(SSBO_IDX_BLOOD_CULLING_INDEX_POOL, "BloodDecalIndices");
        OpenGL::BindSSBO(SSBO_IDX_BLOOD_CULLING_COUNTER, "BloodDecalCounter");

        OpenGL::DispatchCompute(GetTileCountX(), GetTileCountY(), 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        //if (Input::KeyPressed(HELL_KEY_SPACE)) {
        //    std::cout << "Blood count: " << Unloved::BloodSystem::GetBloodScreenSpaceDecals().size() << "\n";
        //}
    }
}
