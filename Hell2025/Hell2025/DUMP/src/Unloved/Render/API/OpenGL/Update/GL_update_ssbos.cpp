#include "Unloved/Render/API/OpenGL/GL_renderer.h"

#include "Hell/Render/API/OpenGL/GL_back_end.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/UI/UIBackEnd.h"

#include "Unloved/Render/RenderDataManager.h"

namespace OpenGL::Renderer {

    namespace {
        void ResetSSBOCounter(const std::string& ssboName) {
            const uint32_t zero = 0;
            OpenGL::UpdateSSBO(ssboName, sizeof(zero), &zero);
        }

        template<typename T>
        void UpdateSSBOValue(const std::string& ssboName, const T& value) {
            OpenGL::UpdateSSBO(ssboName, sizeof(T), &value);
        }

        template<typename T>
        void UpdateSSBOVector(const std::string& ssboName, const std::vector<T>& vector) {
            if (vector.empty()) return;
            OpenGL::UpdateSSBO(ssboName, vector.size() * sizeof(T), vector.data());
        }
    }

    void UpdateSSBOS() {
        ResetSSBOCounter("BloodDecalCounter");
        ResetSSBOCounter("ChristmasLightCounter");

        UpdateSSBOValue("RendererData", Unloved::RenderDataManager::GetRendererData());

        UpdateSSBOVector("BloodDecalInstances", Unloved::RenderDataManager::GetBloodScreenSpaceDecalInstanceData());
        UpdateSSBOVector("GlassLightRanges", Unloved::RenderDataManager::GetGlassLightRanges());
        UpdateSSBOVector("GlassLightIndices", Unloved::RenderDataManager::GetGlassLightIndices());
        UpdateSSBOVector("GlassSpotLightRanges", Unloved::RenderDataManager::GetGlassSpotLightRanges());
        UpdateSSBOVector("GlassSpotLightIndices", Unloved::RenderDataManager::GetGlassSpotLightIndices());
        UpdateSSBOVector("SceneRenderItems", Unloved::RenderDataManager::GetSceneRenderItems());
        UpdateSSBOVector("DrawRenderItemIndices", Unloved::RenderDataManager::GetDrawRenderItemIndices());
        UpdateSSBOVector("Lights", Unloved::RenderDataManager::GetGPULights());
        UpdateSSBOVector("SpotLights", Unloved::RenderDataManager::GetGPUSpotLights());
        UpdateSSBOVector("Materials", Hell::ResourceManager::GetMaterials());
        UpdateSSBOVector("RenderItemsUI", UIBackEnd::GetRenderItems());
        UpdateSSBOVector("Samplers", OpenGL::BackEnd::GetBindlessTextureIDs());
        UpdateSSBOVector("SpriteSheetInstanceData", Unloved::RenderDataManager::GetSpriteSheetInstanceData());
        UpdateSSBOVector("ViewportData", Unloved::RenderDataManager::GetViewportData());


        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        OpenGL::BindSSBO(SSBO_IDX_SAMPLERS, "Samplers");
        OpenGL::BindSSBO(SSBO_IDX_MATERIALS, "Materials");
        OpenGL::BindSSBO(SSBO_IDX_RENDERER_DATA, "RendererData");
        OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");
        OpenGL::BindSSBO(SSBO_IDX_SCENE_RENDER_ITEMS, "SceneRenderItems");
        OpenGL::BindSSBO(SSBO_IDX_DRAW_RENDER_ITEM_INDICES, "DrawRenderItemIndices");
        OpenGL::BindSSBO(SSBO_IDX_LIGHTS, "Lights");
        OpenGL::BindSSBO(SSBO_IDX_SPOT_LIGHTS, "SpotLights");
    }
}
