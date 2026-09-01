#include "RenderDataManager.h"

#include "Hell/Backend/BackEnd.h"
#include "Hell/Common/Bit.h"
#include "Hell/Logging.h"
#include "Hell/Math/Math.h"
#include "Hell/Math/Transform.h"
#include "Hell/Profiling/CPUProfiler.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/UI/UIBackEnd.h"

#include "Unloved/Camera/Frustum.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Config/Config.h"
#include "Unloved/Config/FlashlightConfig.h"
#include "Unloved/EditorSession/Interaction/EditorSelection.h"
#include "Unloved/EditorSession/EditorSession.h"
#include "Unloved/EditorSession/Interaction/EditorVisibility.h"
#include "Unloved/EditorSession/Core/EditorViewports.h"
#include "Unloved/Objects/Lighting/SpotLight.h"
#include "Unloved/Objects/Renderables/SkinnedGameObject.h"
#include "Unloved/Systems/Ocean/Ocean.h"
#include "Unloved/Systems/Mirrors/MirrorManager.h"
#include "Unloved/Systems/BloodOLD/BloodSystemOLD.h"
#include "Unloved/Systems/ShadowMaps/ShadowMapManager.h"
#include "Unloved/Viewport/ViewportManager.h"
#include "Unloved/World/World.h"


// Get me out of here
#include "World/LegacyWorld.h"
#include <vector>
#include "Hell/Input.h"
#include "Unloved/Render/RendererConstants.h"
#include "Timer.hpp"
#include "Unloved/Common/Constants.h"
#include "Unloved/Render/Renderer.h"
#include "Unloved/Render/RendererUtil.h"
#include <array>
//
#include "../../../res/shaders/common/flags.glsl"

using namespace Hell;

namespace Unloved::RenderDataManager {
    using namespace Unloved;

    DrawCommandsSet g_drawCommandsSet;
    FlashLightShadowMapDrawInfo g_flashLightShadowMapDrawInfo;
    RendererData g_rendererData;
    std::array<std::vector<DrawIndexedIndirectCommand>, static_cast<size_t>(UICanvas::COUNT)> g_drawCommandsUI;
    std::vector<GPULight> g_gpuLights;
    std::vector<GPUSpotLight> g_gpuSpotLights;

	std::vector<RenderItem> g_sceneRenderItems;
	std::vector<uint32_t> g_drawRenderItemIndices;
	std::vector<uint32_t> g_heightMapRenderItemIndices;

	std::vector<uint32_t> g_renderItemIndicesProcedural;
    std::vector<uint32_t> g_renderItemIndicesPhysicsShapes;
    std::vector<uint32_t> g_renderItemIndices;
    std::vector<uint32_t> g_renderItemIndicesBlended;
    std::vector<uint32_t> g_renderItemIndicesAlphaDiscarded;
    std::vector<uint32_t> g_viewWeaponRenderItemIndices;
    std::vector<uint32_t> g_viewWeaponRenderItemIndicesAlphaDiscarded;
    std::vector<uint32_t> g_renderItemIndicesHair;
    std::vector<uint32_t> g_renderItemIndicesGlass;
	std::vector<uint32_t> g_renderItemIndicesMirror;
    std::vector<uint32_t> g_renderItemIndicesPlastic;
    std::vector<uint32_t> g_renderItemIndicesToiletWater;

    std::vector<uint32_t> g_renderItemIndicesPointLightShadows;
    std::vector<uint32_t> g_renderItemIndicesStaticPointLightShadows;
    std::vector<uint32_t> g_renderItemIndicesDynamicPointLightShadows;
    std::vector<uint32_t> g_renderItemIndicesMoonLightShadows;

    std::vector<uint32_t> g_renderItemIndicesEmissive;
    std::vector<uint32_t> g_renderItemIndicesOutline;
    std::vector<uint32_t> g_renderItemIndicesOutlinePhysicsShapes;
    std::vector<uint32_t> g_renderItemIndicesOutlineProcedural;
    std::vector<uint32_t> g_renderItemIndicesOutlineSkinned;

    std::vector<GlassLightRange> g_glassLightRanges;
    std::vector<uint32_t> g_glassLightIndices;
    std::vector<GlassLightRange> g_glassSpotLightRanges;
    std::vector<uint32_t> g_glassSpotLightIndices;
    std::vector<ViewportData> g_viewportData;

    std::vector<DecalPaintingInfo> g_decalPaintingInfo;

    std::vector<BloodDecalInstanceData> g_bloodScreenSpaceDecalInstances;

    std::vector<glm::mat4> g_skinningTransforms;
    std::vector<glm::mat4> g_previousSkinningTransforms;
    std::vector<uint32_t> g_combinedSkinnedRenderItemIndices;
    std::vector<uint32_t> g_skinnedRenderItemIndicesDefault;
    std::vector<uint32_t> g_skinnedRenderItemIndicesAlphaDiscard;
    std::vector<uint32_t> g_skinnedRenderItemIndicesBlended;
    std::vector<uint32_t> g_skinnedRenderItemIndicesHair;
    std::vector<uint32_t> g_skinnedViewWeaponRenderItemIndicesDefault;
    std::vector<uint32_t> g_skinnedViewWeaponRenderItemIndicesAlphaDiscard;

    std::vector<uint32_t> g_skinnedNonDeformingRenderItemIndices;
    std::vector<uint32_t> g_skinnedNonDeformingRenderItemIndicesAlphaDiscard;
    std::vector<uint32_t> g_skinnedNonDeformingRenderItemIndicesBlended;
    std::vector<uint32_t> g_skinnedNonDeformingRenderItemIndicesHair;
    std::vector<uint32_t> g_skinnedNonDeformingViewWeaponRenderItemIndices;
    std::vector<uint32_t> g_skinnedNonDeformingViewWeaponRenderItemIndicesAlphaDiscard;

    std::vector<SpriteSheetRenderItem> g_spriteSheetRenderItems;
    std::vector<SpriteSheetRenderItem> g_spriteSheetInstanceData;

    std::vector<RenderItem> g_nonDeformingSkinnedMeshRenderItemsDepthPeeledTransparent;
    uint32_t g_baseSkinnedVertex = 0;
    int32_t g_blackTextureIndex = -1;

    std::vector<float> g_shadowCascadeLevels{ 5.0f, 10.0f, 20.0f, 40.0f }; // WARNING! YOU have a duplicate of this in GL_renderer.h

    std::vector<SkinningJob> g_skinningJobs;
    std::vector<SkinningMorphJob> g_skinningMorphJobs;
    std::vector<SkinningMorphTarget> g_skinningMorphTargets;
    std::vector<SkinningDispatchGroup> g_skinningDispatchGroups;

    std::vector<std::vector<uint32_t>> g_transientRayQueryRenderItemGroups;
    std::vector<uint32_t> g_proceduralRayQueryRenderItemIndices;
    std::vector<uint32_t> g_persistentRayQueryRenderItemIndices;

    uint64_t g_frameIndex = 0;
    glm::vec2 g_jitterPx = glm::vec2(0.0f);


    const std::vector<SkinningJob>& GetSkinningJobs()                                      { return g_skinningJobs; }
    const std::vector<SkinningMorphJob>& GetSkinningMorphJobs()                            { return g_skinningMorphJobs; }
    const std::vector<SkinningMorphTarget>& GetSkinningMorphTargets()                      { return g_skinningMorphTargets; }
    const std::vector<SkinningDispatchGroup>& GetSkinningDispatchGroups()                  { return g_skinningDispatchGroups; }
    const std::vector<std::vector<uint32_t>>& GetTransientRayQueryRenderItemGroups()       { return g_transientRayQueryRenderItemGroups; }
    const std::vector<uint32_t>& GetProceduralRayQueryRenderItemIndices()                  { return g_proceduralRayQueryRenderItemIndices; }
    const std::vector<uint32_t>& GetPersistentRayQueryRenderItemIndices()                  { return g_persistentRayQueryRenderItemIndices; }

    void CreateGPULights();
    void CreateGPUSpotLights();
    void UpdateViewportData();
    void UpdateRendererData();
    void UpdateDrawCommandsSet();
    void UpdatePointLightShadowMapDrawCommands();
    void ClearPointLightShadowMapDrawCommands(PointLightShadowMapDrawCommands& drawCommands);
    void CreatePointLightShadowMapDrawCommands(PointLightShadowMapDrawCommands& drawCommands, const std::vector<ShadowMapInfo>& shadowMaps, const std::vector<uint32_t>& renderItemIndices, bool includeSkinned, bool includeProcedural);

    // Compute skinning
    void CreateSkinningData(); // name me better
    void CreateSkinningDistpachGroups();

    void CreateDrawCommandsFromIndices(std::vector<DrawIndexedIndirectCommand>& drawCommands, const std::vector<uint32_t>& renderItemIndices, Unloved::Frustum* frustum, int viewportIndex, bool ignoreNonShadowCasters = false, uint64_t excludedObjectId = 0, bool useShadowMesh = false);
    void CreateHeightMapRenderItems();
    void CreateHeightMapDrawCommands(std::vector<DrawIndexedIndirectCommand>& drawCommands, Unloved::Frustum& frustum);
    void CreateDrawCommandsSkinnedFromIndices(std::vector<DrawIndexedIndirectCommand>& commands, const std::vector<uint32_t>& renderItemIndices, int viewportIndex, Unloved::Frustum* frustum = nullptr);
    void CreateDrawCommandsNonDeformingSkinnedFromIndices(std::vector<DrawIndexedIndirectCommand>& commands, const std::vector<uint32_t>& renderItemIndices, int viewportIndex, Unloved::Frustum* frustum = nullptr);

	void CreateDrawCommandProceduralFromIndices(std::vector<DrawIndexedIndirectCommand>& drawCommands, const std::vector<uint32_t>& renderItemIndices, Unloved::Frustum* frustum, int viewportIndex);
    void CreateSpriteSheetDrawCommands();

    void CreateShadowCubeMapMultiDrawIndirectCommands(std::vector<DrawIndexedIndirectCommand>& commands, const std::vector<uint32_t>& renderItemIndices, uint32_t faceIndex, Light* light, BlendingMode blendingModeFilter);
    void CreateMoonLightShadowMapDrawCommands();
    void ClearDrawCommandsSet();
    void SortDrawCommandRenderItems();
    void CreateViewportDrawCommands();
    void CreateFlashLightShadowMapDrawCommands();
    void UpdateBloodScreenSpaceDecalInstances();
    uint32_t AddSceneRenderItem(const RenderItem& renderItem);
    bool IsValidMesh(const Mesh* mesh);
    void AddRenderItemToTransientRayQueryBLAS(uint32_t renderItemIndex, uint32_t& groupIndex);
    void AddPersistentRayQueryRenderItem(uint32_t renderItemIndex);
    void AddProceduralRayQueryRenderItem(uint32_t renderItemIndex);
    void CreatePhysicsShapeDrawCommands();

    void BeginFrame() {
        g_sceneRenderItems.clear();
        g_drawRenderItemIndices.clear();
        g_heightMapRenderItemIndices.clear();

        // Compute skinning
        g_skinningJobs.clear();
        g_skinningMorphJobs.clear();
        g_skinningMorphTargets.clear();
        g_skinningDispatchGroups.clear();
        g_skinningTransforms.clear();
        g_previousSkinningTransforms.clear();
        g_baseSkinnedVertex = 0;

        // Vulkan raytracing
        g_transientRayQueryRenderItemGroups.clear();
        g_proceduralRayQueryRenderItemIndices.clear();
        g_persistentRayQueryRenderItemIndices.clear();

        // Skinned (deforming)
        g_combinedSkinnedRenderItemIndices.clear();
        g_skinnedRenderItemIndicesDefault.clear();
        g_skinnedRenderItemIndicesAlphaDiscard.clear();
        g_skinnedRenderItemIndicesBlended.clear();
        g_skinnedRenderItemIndicesHair.clear();
        g_skinnedViewWeaponRenderItemIndicesDefault.clear();
        g_skinnedViewWeaponRenderItemIndicesAlphaDiscard.clear();

        // Skinned (non deforming)
        g_skinnedNonDeformingRenderItemIndices.clear();
        g_skinnedNonDeformingRenderItemIndicesAlphaDiscard.clear();
        g_skinnedNonDeformingRenderItemIndicesBlended.clear();
        g_skinnedNonDeformingRenderItemIndicesHair.clear();
        g_skinnedNonDeformingViewWeaponRenderItemIndices.clear();
        g_skinnedNonDeformingViewWeaponRenderItemIndicesAlphaDiscard.clear();

		g_nonDeformingSkinnedMeshRenderItemsDepthPeeledTransparent.clear();

		g_renderItemIndicesProcedural.clear();
        g_renderItemIndicesPhysicsShapes.clear();

        g_spriteSheetRenderItems.clear();
        g_spriteSheetInstanceData.clear();

        g_renderItemIndices.clear();
		g_renderItemIndicesMirror.clear();
		g_renderItemIndicesPlastic.clear();
        g_renderItemIndicesBlended.clear();
        g_renderItemIndicesAlphaDiscarded.clear();
        g_viewWeaponRenderItemIndices.clear();
        g_viewWeaponRenderItemIndicesAlphaDiscarded.clear();
        g_renderItemIndicesHair.clear();
        g_renderItemIndicesEmissive.clear();
        g_renderItemIndicesToiletWater.clear();

        g_renderItemIndicesPointLightShadows.clear();
        g_renderItemIndicesStaticPointLightShadows.clear();
        g_renderItemIndicesDynamicPointLightShadows.clear();
        g_renderItemIndicesMoonLightShadows.clear();

        // Think about better names for these containers below
        g_renderItemIndicesOutline.clear();
        g_renderItemIndicesOutlinePhysicsShapes.clear();
        g_renderItemIndicesOutlineProcedural.clear();
        g_renderItemIndicesOutlineSkinned.clear();
        g_decalPaintingInfo.clear();
		g_renderItemIndicesGlass.clear();

        for (std::vector<DrawIndexedIndirectCommand>& drawCommands : g_drawCommandsUI) drawCommands.clear();

        g_blackTextureIndex = ResourceManager::GetTextureBindlessIndexByName("Black");

        // TAA Jitter
        g_frameIndex++;
        g_jitterPx = glm::vec2(Hell::Random::Halton(g_frameIndex, 2u), Hell::Random::Halton(g_frameIndex, 3u)) - 0.5f;
    }

    void Update() {
        ProfilerCPUZoneFunction();

        CreateGPULights();
        CreateGPUSpotLights();
        CreateSkinningDistpachGroups();

        UpdateViewportData();
        UpdateRendererData();
        UpdateDrawCommandsSet();
        UpdateDrawCommandsUI();
        UpdatePointLightShadowMapDrawCommands();
    }

    void UpdateDrawCommandsUI() {
        for (size_t canvasIndex = 0; canvasIndex < static_cast<size_t>(UICanvas::COUNT); canvasIndex++) {
            const UICanvas canvas = static_cast<UICanvas>(canvasIndex);
            const std::vector<RenderItemUI>& renderItems = UIBackEnd::GetRenderItems(canvas);
            std::vector<DrawIndexedIndirectCommand>& drawCommands = g_drawCommandsUI[canvasIndex];
            drawCommands.resize(renderItems.size());

            const uint32_t baseInstance = UIBackEnd::GetRenderItemBaseInstance(canvas);
            for (uint32_t i = 0; i < renderItems.size(); i++) {
                const RenderItemUI& renderItem = renderItems[i];
                DrawIndexedIndirectCommand& command = drawCommands[i];
                command.indexCount = renderItem.indexCount;
                command.instanceCount = 1;
                command.firstIndex = renderItem.baseIndex;
                command.baseVertex = renderItem.baseVertex;
                command.baseInstance = baseInstance + i;
            }
        }
    }

    const std::vector<DrawIndexedIndirectCommand>& GetDrawCommandsUI() { return GetDrawCommandsUI(UICanvas::INTERNAL); }

    const std::vector<DrawIndexedIndirectCommand>& GetDrawCommandsUI(UICanvas canvas) {
        const size_t canvasIndex = static_cast<size_t>(canvas);
        return g_drawCommandsUI[canvasIndex < g_drawCommandsUI.size() ? canvasIndex : 0];
    }

    void UpdateViewportData() {
        const RendererSettings& rendererSettings = Renderer::GetCurrentRendererSettings();
        const Resolutions& resolutions = Config::GetResolutions();

        g_viewportData.resize(4);

        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            g_viewportData[i].colorTint = WHITE;
            g_viewportData[i].colorContrast = 1.0f;
            g_viewportData[i].isInShop = false;

            glm::mat4 viewMatrix = glm::mat4(1);
            if (EditorSession::IsActive()) {
                viewMatrix = EditorSession::Viewports::GetViewMatrix(i);
                g_viewportData[i].orthoSize = viewport->GetOrthoSize();
				g_viewportData[i].isOrtho = viewport->IsOrthographic();
				g_viewportData[i].fov = viewport->GetPerspectiveFOV();

				g_viewportData[i].vignetteIntensityScalar = 0.0f;
				g_viewportData[i].vignetteColor = glm::vec4(0.0f);
            }
            else {
                g_viewportData[i].orthoSize = 0.0f;
                g_viewportData[i].isOrtho = false;
                g_viewportData[i].fov = Unloved::Session::GetLocalPlayerFovByViewportIndex(i);

                Unloved::Player* player = Unloved::Session::GetLocalPlayerByViewportIndex(i);
                if (player) {
                    g_viewportData[i].colorTint = glm::vec4(player->GetViewportColorTint(), 1.0f);
                    g_viewportData[i].colorContrast = player->GetViewportContrast();

                    if (player->IsDead()) {
                        viewMatrix = player->m_deathCamViewMatrix;
                    }
                    else {
                        viewMatrix = Unloved::Session::GetLocalPlayerCameraByViewportIndex(i)->GetViewMatrix();
                    }

                    g_viewportData[i].isInShop = player->IsInShop();

                    g_viewportData[i].vignetteIntensityScalar = player->GetVignettIntensityScalar();
					g_viewportData[i].vignetteColor = glm::vec4(player->GetVignetteColor(), 0.0f);
                }
            }
            glm::mat4 inverseView = glm::inverse(viewMatrix);
            glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
            glm::vec3 cameraRight = glm::vec3(inverseView[0]);
            glm::vec3 cameraUp = glm::vec3(inverseView[1]);
            glm::vec3 cameraForward = -glm::vec3(inverseView[2]);

            // Is there any previous data?
            bool previousDataExists = !Hell::Math::NearlyEqual(g_viewportData[i].previousProjectionView, glm::mat4(1.0f));

            // Previous
            if (previousDataExists) {
                g_viewportData[i].previousProjectionView = g_viewportData[i].projectionView;
            }

            g_viewportData[i].prevProjectionView = g_viewportData[i].projectionView;
            g_viewportData[i].prevProjectionViewReverseZ = g_viewportData[i].projectionViewReverseZ;

            g_viewportData[i].cameraForward = glm::vec4(cameraForward, 0.0f);
            g_viewportData[i].cameraRight = glm::vec4(cameraRight, 0.0f);
			g_viewportData[i].cameraUp = glm::vec4(cameraUp, 0.0f);
			g_viewportData[i].projection = viewport->GetProjectionMatrix();
            g_viewportData[i].inverseProjection = glm::inverse(g_viewportData[i].projection);
            g_viewportData[i].view = viewMatrix;
            g_viewportData[i].inverseView = inverseView;
            g_viewportData[i].projectionView = g_viewportData[i].projection * g_viewportData[i].view;
            g_viewportData[i].inverseProjectionView = glm::inverse(g_viewportData[i].projectionView);
            g_viewportData[i].skyboxProjectionView = viewport->GetPerpsectiveMatrix() * g_viewportData[i].view;
            g_viewportData[i].width = (int)(resolutions.gBuffer.x * viewport->GetSize().x);
            g_viewportData[i].height = (int)(resolutions.gBuffer.y * viewport->GetSize().y);
            g_viewportData[i].xOffset = (int)(resolutions.gBuffer.x * viewport->GetPosition().x);
            g_viewportData[i].yOffset = (int)(resolutions.gBuffer.y * viewport->GetPosition().y);
            g_viewportData[i].posX = viewport->GetPosition().x;
            g_viewportData[i].posY = viewport->GetPosition().y;
            g_viewportData[i].sizeX = viewport->GetSize().x;
            g_viewportData[i].sizeY = viewport->GetSize().y;
            g_viewportData[i].viewPos = g_viewportData[i].inverseView[3];

			g_viewportData[i].projectionReverseZ = viewport->GetProjectionMatrixReverseZ();
			g_viewportData[i].inverseProjectionReverseZ = glm::inverse(g_viewportData[i].projectionReverseZ);
			g_viewportData[i].projectionViewReverseZ = g_viewportData[i].projectionReverseZ * g_viewportData[i].view;
			g_viewportData[i].inverseProjectionViewReverseZ = glm::inverse(g_viewportData[i].projectionViewReverseZ);

            glm::vec2 projectionShiftNdc(0.0f);
            if (Renderer::GetCurrentRendererSettings().enableTAA) {
                const glm::vec2 viewportSize(
                    static_cast<float>(g_viewportData[i].width),
                    static_cast<float>(g_viewportData[i].height)
                );

                projectionShiftNdc = static_cast<float>(-rendererSettings.taaJitterScale) * g_jitterPx / viewportSize;
            }

            // GLM uses column-major indexing: matrix[column][row]. Pre-multiplying
            // applies a homogeneous clip-space translation after projection.
            glm::mat4 jitterMatrix(1.0f);
            jitterMatrix[3][0] = projectionShiftNdc.x;
            jitterMatrix[3][1] = projectionShiftNdc.y;

            g_viewportData[i].jitteredProjectionViewReverseZ = jitterMatrix * g_viewportData[i].projectionViewReverseZ;
            g_viewportData[i].inverseJitteredProjectionViewReverseZ = glm::inverse(g_viewportData[i].jitteredProjectionViewReverseZ);

            // If no previous then use current frame values
            if (previousDataExists) {
                g_viewportData[i].previousProjectionView = g_viewportData[i].projectionView;
            }

            viewport->GetFrustum().Update(g_viewportData[i].projectionView);

            g_viewportData[i].frustumPlane0 = viewport->GetFrustum().GetPlane(0);
            g_viewportData[i].frustumPlane1 = viewport->GetFrustum().GetPlane(1);
            g_viewportData[i].frustumPlane2 = viewport->GetFrustum().GetPlane(2);
            g_viewportData[i].frustumPlane3 = viewport->GetFrustum().GetPlane(3);
            g_viewportData[i].frustumPlane4 = viewport->GetFrustum().GetPlane(4);
            g_viewportData[i].frustumPlane5 = viewport->GetFrustum().GetPlane(5);

            // Flashlight
            if (EditorSession::IsActive()) {
                g_viewportData[i].flashlightModifer = 0;
                g_viewportData[i].flashlightProjectionView = glm::mat4(1);
                g_viewportData[i].flashlightDir = glm::vec4(0.0f);
                g_viewportData[i].flashlightPosition = glm::vec4(0.0f);
            }
            else {
                Unloved::Player* player = Unloved::Session::GetLocalPlayerByViewportIndex(i);
                if (player) {
                    g_viewportData[i].flashlightProjectionView = player->GetFlashlightProjectionView();
                    g_viewportData[i].flashlightDir = glm::vec4(player->GetFlashlightDirection(), 0.0f);
                    g_viewportData[i].flashlightPosition = glm::vec4(player->GetFlashlightPosition(), 0.0f);
                    g_viewportData[i].flashlightModifer = player->GetFlashLightModifer();
                }
            }

            // CSM matrices
            glm::vec3 lightDir = Unloved::World::GetMoonlightDirection();
            float viewportWidth = g_viewportData[i].width;
            float viewportHeight = g_viewportData[i].height;
            float fov = g_viewportData[i].fov;
            const std::vector<glm::mat4> lightProjectionViews = RendererUtil::GetLightProjectionViews(viewMatrix, lightDir, g_shadowCascadeLevels, viewportWidth, viewportHeight, fov);

            if (lightProjectionViews.size() != SHADOW_CASCADE_COUNT) Logging::Error() << "INCORRECT SIZE: " << lightProjectionViews.size();
            for (int j = 0; j < SHADOW_CASCADE_COUNT && j < lightProjectionViews.size(); j++) {
                g_viewportData[i].csmLightProjectionView[j] = lightProjectionViews[j];
            }
        }
    }

    void UpdateRendererData() {
        const RendererSettings& rendererSettings = Renderer::GetCurrentRendererSettings();
        const Resolutions& resolutions = Config::GetResolutions();
        const Config::Christmas::Settings& christmasSettings = Config::Christmas::GetSettings();
        const Config::Moonlight::Settings& moonlightSettings = Config::Moonlight::GetSettings();
        const Config::Flashlight::Settings& flashlightSettings = Config::Flashlight::GetSettings();
        const Ocean::Settings oceanSettings = Ocean::GetSettings();
        const Ocean::SurfaceSettings& oceanSurface = oceanSettings.surface;
        const Ocean::CompositeSettings& oceanComposite = oceanSettings.composite;
        const Ocean::SurfaceCompositeSettings& oceanSurfaceComposite = oceanComposite.surface;
        const Ocean::UnderwaterCompositeSettings& oceanUnderwater = oceanComposite.underwater;
        IESProfile* flashlightIESProfile = Hell::ResourceManager::GetIESProfilePtr(flashlightSettings.iesProfile);

        g_rendererData.nearPlane = Config::GetNearPlane();
        g_rendererData.farPlane = Config::GetFarPlane();
        g_rendererData.gBufferWidth = (float)resolutions.gBuffer.x;
        g_rendererData.gBufferHeight = (float)resolutions.gBuffer.y;
        g_rendererData.hairBufferWidth = (float)resolutions.hair.x;
        g_rendererData.hairBufferHeight = (float)resolutions.hair.y;
        g_rendererData.viewportSplitX = 0.0f;
        g_rendererData.viewportSplitY = 0.0f;
        g_rendererData.activeViewportMask = Unloved::ViewportManager::GetActiveViewportMask();

        Unloved::Viewport* viewport1 = Unloved::ViewportManager::GetViewportByIndex(1);
        Unloved::Viewport* viewport2 = Unloved::ViewportManager::GetViewportByIndex(2);
        if (!viewport1->IsVisible()) {
            g_rendererData.viewportLayout = (int)ViewportLayout::SINGLE;
        }
        else if (!viewport2->IsVisible()) {
            if (g_viewportData[0].yOffset == g_viewportData[1].yOffset) {
                g_rendererData.viewportLayout = (int)ViewportLayout::COLUMNS;
                g_rendererData.viewportSplitX = g_viewportData[1].posX;
            }
            else {
                g_rendererData.viewportLayout = (int)ViewportLayout::ROWS;
                g_rendererData.viewportSplitY = g_viewportData[1].posY;
            }
        }
        else {
            g_rendererData.viewportLayout = (int)ViewportLayout::GRID;
            g_rendererData.viewportSplitX = g_viewportData[1].posX;
            g_rendererData.viewportSplitY = g_viewportData[2].posY;
        }
        g_rendererData.time = Session::GetSessionTime();
        g_rendererData.rendererOverrideState = (int)rendererSettings.rendererOverrideState;
        g_rendererData.normalizedMouseX = Hell::Math::MapRange(Hell::Input::GetMouseX(), 0, Hell::BackEnd::GetCurrentWindowWidth(), 0.0f, 1.0f);
        g_rendererData.normalizedMouseY = Hell::Math::MapRange(Hell::Input::GetMouseY(), 0, Hell::BackEnd::GetCurrentWindowHeight(), 0.0f, 1.0f);
        g_rendererData.tileCountX = Renderer::GetTileCountX();
        g_rendererData.tileCountY = Renderer::GetTileCountY();
        g_rendererData.lightCount = static_cast<uint32_t>(g_gpuLights.size());
        g_rendererData.spotLightCount = static_cast<uint32_t>(g_gpuSpotLights.size());
        g_rendererData.moonLightDir = glm::vec4(World::GetMoonlightDirection(), 0.0f);
        g_rendererData.moonLightColorStrength = glm::vec4(moonlightSettings.color, moonlightSettings.strength);
        g_rendererData.enableDDGI = rendererSettings.enableDDGI;
        g_rendererData.enableDDGIReflections = rendererSettings.enableDDGIReflections;
        g_rendererData.enableIndirectSpecular = rendererSettings.enableIndirectSpecular;
        g_rendererData.taaJitterPx = g_jitterPx;
        g_rendererData.enableTAA = rendererSettings.enableTAA;
        g_rendererData.indirectSpecularFactor = rendererSettings.indirectSpecularFactor;
        g_rendererData.indirectSpecularRoughnessDampening = rendererSettings.indirectSpecularRoughnessDampening;
        g_rendererData.directPointShadowMode = static_cast<uint32_t>(rendererSettings.directPointShadowMode);
        g_rendererData.emissiveStrength = rendererSettings.emissiveStrength;
        g_rendererData.christmasLightRadius = christmasSettings.lightRadius;
        g_rendererData.christmasLightStrength = christmasSettings.lightStrength;
        g_rendererData.irradianceDampening = rendererSettings.irradianceDampening;

        g_rendererData.flashlightColor = glm::vec4(flashlightSettings.color, 1.0f);
        g_rendererData.flashlightRange = flashlightSettings.range;
        g_rendererData.flashlightFalloffExponent = flashlightSettings.falloffExponent;
        g_rendererData.flashlightBrightness = flashlightSettings.brightness;
        g_rendererData.flashlightIESConeScale = flashlightSettings.iesConeScale;
        g_rendererData.flashlightIESInnerAngle = flashlightSettings.iesInnerAngle;
        g_rendererData.flashlightIESOuterAngle = flashlightSettings.iesOuterAngle;
        g_rendererData.flashlightIESContrast = flashlightSettings.iesContrast;
        g_rendererData.flashlightIESVerticalScale = flashlightIESProfile ? flashlightIESProfile->GetVScale() : 0.0f;
        g_rendererData.flashlightIESVerticalBias = flashlightIESProfile ? flashlightIESProfile->GetVBias() : 0.0f;
        g_rendererData.flashlightIESHorizontalBias = flashlightIESProfile ? flashlightIESProfile->GetHBias() : 0.0f;
        g_rendererData.flashlightIESTextureIndex = flashlightIESProfile ? static_cast<int32_t>(flashlightIESProfile->GetTextureIndex()) : -1;
        g_rendererData.flashlightIESEnabled = flashlightSettings.iesEnabled ? 1u : 0u;
        g_rendererData.flashlightCenterSpotRange = flashlightSettings.centerSpotRange;
        g_rendererData.flashlightCenterSpotFalloffExponent = flashlightSettings.centerSpotFalloffExponent;
        g_rendererData.flashlightCenterSpotBrightness = flashlightSettings.centerSpotBrightness;
        g_rendererData.flashlightCenterSpotInnerAngle = flashlightSettings.centerSpotInnerAngle;
        g_rendererData.flashlightCenterSpotOuterAngle = flashlightSettings.centerSpotOuterAngle;
        g_rendererData.flashlightCenterSpotEnabled = flashlightSettings.centerSpotEnabled ? 1u : 0u;

        g_rendererData.oceanSurfaceAlbedo = glm::vec4(oceanSurface.albedo, 0.0f);
        g_rendererData.oceanSurfaceFogColor = glm::vec4(oceanSurface.fogColor, 0.0f);
        g_rendererData.oceanSurfaceRippleVelocity = glm::vec4(oceanSurface.rippleVelocity0, oceanSurface.rippleVelocity1);
        g_rendererData.oceanUnderwaterTint = glm::vec4(oceanComposite.underwaterTint, 0.0f);
        g_rendererData.oceanUnderwaterRayFogColor = glm::vec4(oceanUnderwater.rayFogColor, 0.0f);
        g_rendererData.oceanOriginY = Ocean::GetOceanOriginY();
        g_rendererData.oceanDisplayMode = static_cast<int32_t>(oceanSettings.displayMode);
        g_rendererData.oceanSurfaceSpecularAntiAliasing = oceanSurface.specularAntiAliasing ? 1u : 0u;
        g_rendererData.oceanSurfaceNormalScale = oceanSurface.normalScale;
        g_rendererData.oceanSurfaceNormalConvergeStartDistance = oceanSurface.normalConvergeStartDistance;
        g_rendererData.oceanSurfaceNormalConvergeEndDistance = oceanSurface.normalConvergeEndDistance;
        g_rendererData.oceanSurfaceNormalConvergeMaxFactor = oceanSurface.normalConvergeMaxFactor;
        g_rendererData.oceanSurfaceNormalConvergeExponent = oceanSurface.normalConvergeExponent;
        g_rendererData.oceanSurfaceNormalSoftening = oceanSurface.normalSoftening;
        g_rendererData.oceanSurfaceRippleTiling = oceanSurface.rippleTiling;
        g_rendererData.oceanSurfaceRippleStrength = oceanSurface.rippleStrength;
        g_rendererData.oceanSurfaceRippleSecondLayerScale = oceanSurface.rippleSecondLayerScale;
        g_rendererData.oceanSurfaceRoughness = oceanSurface.roughness;
        g_rendererData.oceanSurfaceReflectance = oceanSurface.reflectance;
        g_rendererData.oceanSurfaceReflectionGamma = oceanSurface.reflectionGamma;
        g_rendererData.oceanSurfaceDiffuseStrength = oceanSurface.diffuseStrength;
        g_rendererData.oceanSurfaceSssHeightRange = oceanSurface.sssHeightRange;
        g_rendererData.oceanSurfaceSssStrength = oceanSurface.sssStrength;
        g_rendererData.oceanSurfaceUnderwaterSssStrength = oceanSurface.underwaterSssStrength;
        g_rendererData.oceanSurfaceSssRadiusMinimum = oceanSurface.sssRadiusMinimum;
        g_rendererData.oceanSurfaceSssRadiusMaximum = oceanSurface.sssRadiusMaximum;
        g_rendererData.oceanSurfaceSssIntensity = oceanSurface.sssIntensity;
        g_rendererData.oceanSurfaceSssFalloff = oceanSurface.sssFalloff;
        g_rendererData.oceanSurfaceSssSaturation = oceanSurface.sssSaturation;
        g_rendererData.oceanSurfaceFogStartDistance = oceanSurface.fogStartDistance;
        g_rendererData.oceanSurfaceFogEndDistance = oceanSurface.fogEndDistance;
        g_rendererData.oceanSurfaceFogExponent = oceanSurface.fogExponent;
        g_rendererData.oceanSurfaceFogStrength = oceanSurface.fogStrength;
        g_rendererData.oceanSurfaceCompositePlaneHeightOffset = oceanSurfaceComposite.planeHeightOffset;
        g_rendererData.oceanSurfaceCompositeDistortionSpeed = oceanSurfaceComposite.distortionSpeed;
        g_rendererData.oceanSurfaceCompositeDistortionStrength = oceanSurfaceComposite.distortionStrength;
        g_rendererData.oceanSurfaceCompositeDistortionTiling = oceanSurfaceComposite.distortionTiling;
        g_rendererData.oceanSurfaceCompositeRefractionTintStrength = oceanSurfaceComposite.refractionTintStrength;
        g_rendererData.oceanUnderwaterRayFogStrength = oceanUnderwater.rayFogStrength;
        g_rendererData.oceanUnderwaterDarknessCurve = oceanUnderwater.darknessCurve;
        g_rendererData.oceanUnderwaterDistortionSpeed = oceanUnderwater.distortionSpeed;
        g_rendererData.oceanUnderwaterDistortionStrength = oceanUnderwater.distortionStrength;
        g_rendererData.oceanUnderwaterDepthTintStrength = oceanUnderwater.depthTintStrength;
        g_rendererData.oceanUnderwaterDepthTintOriginalWeight = oceanUnderwater.depthTintOriginalWeight;
        g_rendererData.oceanUnderwaterGeometryWaterColorSquaredStrength = oceanUnderwater.geometryWaterColorSquaredStrength;
        g_rendererData.oceanUnderwaterGeometryWaterColorStrength = oceanUnderwater.geometryWaterColorStrength;
        g_rendererData.oceanUnderwaterGeometryTintStrength = oceanUnderwater.geometryTintStrength;
        g_rendererData.oceanUnderwaterOpenWaterTintStrength = oceanUnderwater.openWaterTintStrength;
        g_rendererData.oceanUnderwaterOpenWaterBrightness = oceanUnderwater.openWaterBrightness;
    }

    void SortRenderItemIndices(std::vector<uint32_t>& renderItemIndices) {
        std::sort(renderItemIndices.begin(), renderItemIndices.end(), [](uint32_t a, uint32_t b) {
            return g_sceneRenderItems[a].meshId < g_sceneRenderItems[b].meshId;
        });
    }

    bool ValidateRenderItemMeshRange(const RenderItem& renderItem, const char* submitFunction) {
        if (renderItem.vertexCount != 0 && renderItem.indexCount != 0) {
            return true;
        }

        Logging::Error()
            << submitFunction << " rejected a RenderItem with an empty mesh range: meshId=" << renderItem.meshId
            << ", vertexCount=" << renderItem.vertexCount
            << ", indexCount=" << renderItem.indexCount
            << ", baseVertex=" << renderItem.baseVertex
            << ", baseIndex=" << renderItem.baseIndex << "\n";

        __debugbreak();

        return false;
    }

    void SetDrawCommandMeshRange(DrawIndexedIndirectCommand& command, const RenderItem& renderItem) {
        command.indexCount = renderItem.indexCount;
        command.firstIndex = renderItem.baseIndex;
        command.baseVertex = renderItem.baseVertex;
    }

    uint32_t GetShadowMeshId(const RenderItem& renderItem) {
        return renderItem.shadowMeshId ? renderItem.shadowMeshId : renderItem.meshId;
    }

    void SetShadowDrawCommandMeshRange(DrawIndexedIndirectCommand& command, const RenderItem& renderItem) {
        if (renderItem.shadowMeshId) {
            if (Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(renderItem.shadowMeshId)) {
                command.indexCount = mesh->indexCount;
                command.firstIndex = mesh->baseIndex;
                command.baseVertex = mesh->baseVertex;
                return;
            }
        }

        SetDrawCommandMeshRange(command, renderItem);
    }

    void CreateMoonLightShadowMapDrawCommands() {
        ProfilerCPUZone("Moon shadow commands");

        auto& set = g_drawCommandsSet;
        int viewportCount = 4;
        int cascadeCount = SHADOW_CASCADE_COUNT;

        // Clear last frames draw commands
        for (int x = 0; x < viewportCount; x++) {
            for (int y = 0; y < cascadeCount; y++) {
                set.moonLightCascades[x][y].clear();
            }
        }

        Unloved::Frustum frustum;

        for (int i = 0; i < viewportCount; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (!viewport || !viewport->IsVisible()) continue;

            for (int j = 0; j < cascadeCount; j++) {
                frustum.Update(g_viewportData[i].csmLightProjectionView[j]);

                CreateDrawCommandsFromIndices(set.moonLightCascades[i][j], g_renderItemIndicesMoonLightShadows, &frustum, i, false, 0, true);
            }
        }
    }

	const std::vector<RenderItem>& GetNonDeformingSkinnedMeshRenderItemsDepthPeeledTransparent() {
		return g_nonDeformingSkinnedMeshRenderItemsDepthPeeledTransparent;
	}

    void CreateSpriteSheetDrawCommands() {
        ProfilerCPUZone("Sprite sheet commands");

        g_spriteSheetInstanceData.clear();

        Mesh* quadMesh = Hell::ResourceManager::GetQuadMesh();
        if (!IsValidMesh(quadMesh)) return;

        auto& set = g_drawCommandsSet;

        for (int viewportIndex = 0; viewportIndex < 4; viewportIndex++) {
            std::vector<DrawIndexedIndirectCommand>& commands = set.spriteSheets[viewportIndex];
            commands.clear();

            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(viewportIndex);
            if (!viewport || !viewport->IsVisible()) continue;

            const int instanceStart = static_cast<int>(g_spriteSheetInstanceData.size());
            const glm::vec3 cameraPosition = glm::vec3(g_viewportData[viewportIndex].viewPos);

            for (const SpriteSheetRenderItem& renderItem : g_spriteSheetRenderItems) {
                if (renderItem.exclusiveViewportIndex != -1 && renderItem.exclusiveViewportIndex != viewportIndex) continue;

                glm::vec3 position = glm::vec3(renderItem.modelMatrix[3]);
                glm::vec3 toCamera = position - cameraPosition;
                float distanceSquared = glm::dot(toCamera, toCamera);

                size_t insertIndex = g_spriteSheetInstanceData.size();
                g_spriteSheetInstanceData.push_back(renderItem);

                while (insertIndex > static_cast<size_t>(instanceStart)) {
                    const SpriteSheetRenderItem& previousRenderItem = g_spriteSheetInstanceData[insertIndex - 1];
                    glm::vec3 previousPosition = glm::vec3(previousRenderItem.modelMatrix[3]);
                    glm::vec3 previousToCamera = previousPosition - cameraPosition;
                    float previousDistanceSquared = glm::dot(previousToCamera, previousToCamera);

                    if (previousDistanceSquared >= distanceSquared) {
                        break;
                    }

                    g_spriteSheetInstanceData[insertIndex] = previousRenderItem;
                    insertIndex--;
                }

                g_spriteSheetInstanceData[insertIndex] = renderItem;
            }

            const uint32_t instanceCount = static_cast<uint32_t>(g_spriteSheetInstanceData.size() - instanceStart);
            if (instanceCount == 0) continue;

            DrawIndexedIndirectCommand& command = commands.emplace_back();
            command.indexCount = quadMesh->indexCount;
            command.instanceCount = instanceCount;
            command.firstIndex = quadMesh->baseIndex;
            command.baseVertex = quadMesh->baseVertex;
            command.baseInstance = instanceStart;
        }
    }


    void CreateGlassDrawCommands() {
        ProfilerCPUZone("Glass commands");

        // Reuse these every frame
        static std::vector<uint32_t> visibleGlassRenderItemIndices[4];

        auto& set = g_drawCommandsSet;
        // Pack per-draw glass lighting metadata into buffers keyed by draw index.
        g_glassLightRanges.clear();
        g_glassLightIndices.clear();
        g_glassSpotLightRanges.clear();
        g_glassSpotLightIndices.clear();

        for (int viewportIndex = 0; viewportIndex < 4; viewportIndex++) {
            std::vector<uint32_t>& renderItemIndices = visibleGlassRenderItemIndices[viewportIndex];
            std::vector<DrawIndexedIndirectCommand>& drawCommands = set.glassDrawCommands[viewportIndex];
            renderItemIndices.clear();
            drawCommands.clear();

            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(viewportIndex);
            if (!viewport->IsVisible()) continue;

            Unloved::Player* player = Unloved::Session::GetLocalPlayerByViewportIndex(viewportIndex);
            if (!player) continue;

            Unloved::Frustum& frustum = viewport->GetFrustum();

            // Cull glass for this viewport
            for (uint32_t renderItemIndex : g_renderItemIndicesGlass) {
                const RenderItem& renderItem = g_sceneRenderItems[renderItemIndex];
                if (frustum.IntersectsAABB(AABB(renderItem.aabbMin, renderItem.aabbMax))) {
                    renderItemIndices.push_back(renderItemIndex);
                }
            }

            // Glass needs to render back to front
            std::sort(renderItemIndices.begin(), renderItemIndices.end(), [player](uint32_t aIndex, uint32_t bIndex) {
                const RenderItem& a = g_sceneRenderItems[aIndex];
                const RenderItem& b = g_sceneRenderItems[bIndex];
                float distA = glm::distance(player->GetCameraPosition(), glm::vec3(a.modelMatrix[3]));
                float distB = glm::distance(player->GetCameraPosition(), glm::vec3(b.modelMatrix[3]));
                return distA > distB;
            });

            // Build the commands and matching draw-indexed lighting metadata together
            for (uint32_t renderItemIndex : renderItemIndices) {
                const RenderItem& renderItem = g_sceneRenderItems[renderItemIndex];
                DrawIndexedIndirectCommand command;
                SetDrawCommandMeshRange(command, renderItem);

                command.instanceCount = 1;

                uint32_t drawIndex = static_cast<uint32_t>(g_drawRenderItemIndices.size());
                g_drawRenderItemIndices.push_back(renderItemIndex);
                command.baseInstance = drawIndex;
                drawCommands.push_back(command);

                GlassLightRange lightRange{};
                lightRange.offset = static_cast<uint32_t>(g_glassLightIndices.size());

                const AABB renderItemBounds(glm::vec3(renderItem.aabbMin), glm::vec3(renderItem.aabbMax));

                // Cull the full light list once per glass instance
                for (uint32_t lightIndex = 0; lightIndex < g_gpuLights.size(); lightIndex++) {
                    const GPULight& light = g_gpuLights[lightIndex];
                    const glm::vec3 lightBoundsMin = glm::vec3(light.worldBoundsMin);
                    const glm::vec3 lightBoundsMax = glm::vec3(light.worldBoundsMax);

                    if (!renderItemBounds.IntersectsAABB(lightBoundsMin, lightBoundsMax)) continue;

                    const glm::vec3 lightPosition(light.posX, light.posY, light.posZ);
                    if (!renderItemBounds.IntersectsSphere(lightPosition, light.radius)) continue;

                    g_glassLightIndices.push_back(lightIndex);
                }

                lightRange.count = static_cast<uint32_t>(g_glassLightIndices.size()) - lightRange.offset;

                GlassLightRange spotLightRange{};
                spotLightRange.offset = static_cast<uint32_t>(g_glassSpotLightIndices.size());

                // Spot-light cone bounds are maintained by SpotLight. The shader
                // performs the exact cone/IES rejection after this conservative test.
                for (uint32_t lightIndex = 0; lightIndex < g_gpuSpotLights.size(); lightIndex++) {
                    const GPUSpotLight& light = g_gpuSpotLights[lightIndex];
                    if (!renderItemBounds.IntersectsAABB(glm::vec3(light.worldBoundsMin), glm::vec3(light.worldBoundsMax))) continue;
                    g_glassSpotLightIndices.push_back(lightIndex);
                }

                spotLightRange.count = static_cast<uint32_t>(g_glassSpotLightIndices.size()) - spotLightRange.offset;

                g_glassLightRanges.resize(drawIndex + 1);
                g_glassSpotLightRanges.resize(drawIndex + 1);
                g_glassLightRanges[drawIndex] = lightRange;
                g_glassSpotLightRanges[drawIndex] = spotLightRange;
            }
        }
    }

    void ClearDrawCommandsSet() {
        ProfilerCPUZone("Clear commands");

        auto& set = g_drawCommandsSet;

        for (int i = 0; i < 4; i++) {
            set.standard[i].clear();
            set.blended[i].clear();
            set.alphaDiscard[i].clear();
            set.viewWeaponAlphaDiscard[i].clear();
            set.viewWeaponStandard[i].clear();
            set.hair[i].clear();
            set.mirrorRenderItems[i].clear();
            set.plastic[i].clear();
            set.procedural[i].clear();
            set.heightMap[i].clear();
            set.emissive[i].clear();
            set.spriteSheets[i].clear();
            set.physicsShapes[i].clear();
        }

        for (int i = 0; i < MAX_SHADOWED_SPOT_LIGHTS; i++) {
            g_flashLightShadowMapDrawInfo.flashlightShadowMapGeometry[i].clear();
            g_flashLightShadowMapDrawInfo.heightMapChunkIndices[i].clear();
            g_flashLightShadowMapDrawInfo.projectionView[i] = glm::mat4(1.0f);
            g_flashLightShadowMapDrawInfo.ownerViewportIndex[i] = -1;
            g_flashLightShadowMapDrawInfo.active[i] = false;
        }
    }

    void SortDrawCommandRenderItems() {
        ProfilerCPUZone("Sort render items");

        SortRenderItemIndices(g_renderItemIndices);
        SortRenderItemIndices(g_renderItemIndicesMirror);
        SortRenderItemIndices(g_renderItemIndicesBlended);
        SortRenderItemIndices(g_renderItemIndicesAlphaDiscarded);
        SortRenderItemIndices(g_viewWeaponRenderItemIndices);
        SortRenderItemIndices(g_viewWeaponRenderItemIndicesAlphaDiscarded);
        SortRenderItemIndices(g_renderItemIndicesHair);
        SortRenderItemIndices(g_renderItemIndicesPlastic);
        SortRenderItemIndices(g_renderItemIndicesProcedural);
        SortRenderItemIndices(g_renderItemIndicesPhysicsShapes);

        SortRenderItemIndices(g_renderItemIndicesPointLightShadows);
        SortRenderItemIndices(g_renderItemIndicesStaticPointLightShadows);
        SortRenderItemIndices(g_renderItemIndicesDynamicPointLightShadows);
        SortRenderItemIndices(g_renderItemIndicesMoonLightShadows);

        SortRenderItemIndices(g_renderItemIndicesEmissive);
    }

    void CreateViewportDrawCommands() {
        ProfilerCPUZone("Viewport commands");

        auto& set = g_drawCommandsSet;
        CreateHeightMapRenderItems();

        // Lil hack to include bullet decals in mirrors
        int count = g_renderItemIndices.size() + g_renderItemIndicesAlphaDiscarded.size();
        std::vector<uint32_t> potentialMirrorItems;
        potentialMirrorItems.reserve(count);
        potentialMirrorItems.insert(potentialMirrorItems.end(), g_renderItemIndices.begin(), g_renderItemIndices.end());
        potentialMirrorItems.insert(potentialMirrorItems.end(), g_renderItemIndicesAlphaDiscarded.begin(), g_renderItemIndicesAlphaDiscarded.end());

        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            Unloved::Frustum& frustum = viewport->GetFrustum();
            CreateDrawCommandsFromIndices(set.standard[i], g_renderItemIndices, &frustum, i);
            CreateDrawCommandsFromIndices(set.standard[i], g_renderItemIndicesMirror, &frustum, i);
            CreateDrawCommandsFromIndices(set.blended[i], g_renderItemIndicesBlended, &frustum, i);
            CreateDrawCommandsFromIndices(set.alphaDiscard[i], g_renderItemIndicesAlphaDiscarded, &frustum, i);
            CreateDrawCommandsFromIndices(set.viewWeaponAlphaDiscard[i], g_viewWeaponRenderItemIndicesAlphaDiscarded, &frustum, i);
            CreateDrawCommandsFromIndices(set.viewWeaponStandard[i], g_viewWeaponRenderItemIndices, &frustum, i);
            CreateDrawCommandsFromIndices(set.hair[i], g_renderItemIndicesHair, &frustum, i);
            CreateDrawCommandsFromIndices(set.plastic[i], g_renderItemIndicesPlastic, &frustum, i);
            CreateDrawCommandsFromIndices(set.emissive[i], g_renderItemIndicesEmissive, &frustum, i);
            CreateDrawCommandProceduralFromIndices(set.procedural[i], g_renderItemIndicesProcedural, &frustum, i);
            CreateHeightMapDrawCommands(set.heightMap[i], frustum);

            if (Mirror* mirror = Unloved::MirrorManager::GetMirrorByObjectId(viewport->GetMirrorId())) {
                CreateDrawCommandsFromIndices(set.mirrorRenderItems[i], potentialMirrorItems, mirror->GetFrustum(i), i);
            }
        }
    }

    void CreateFlashLightShadowMapDrawCommands() {
        ProfilerCPUZone("Flashlight commands");

        for (SpotLight& light : World::GetSpotLights()) {
            const int32_t shadowLayer = light.GetShadowLayer();
            if (!light.IsActive() || !light.CastsShadows()) continue;
            if (shadowLayer < 0 || shadowLayer >= MAX_SHADOWED_SPOT_LIGHTS) continue;

            const int32_t ownerViewportIndex = light.GetOwnerViewportIndex();
            Unloved::Frustum& spotLightFrustum = light.GetFrustum();
            g_flashLightShadowMapDrawInfo.projectionView[shadowLayer] = light.GetData().projectionView;
            g_flashLightShadowMapDrawInfo.ownerViewportIndex[shadowLayer] = ownerViewportIndex;
            g_flashLightShadowMapDrawInfo.active[shadowLayer] = true;

            // Build multi draw commands for regular geometry
            CreateDrawCommandsFromIndices(g_flashLightShadowMapDrawInfo.flashlightShadowMapGeometry[shadowLayer], g_renderItemIndices, &spotLightFrustum, ownerViewportIndex, true, light.GetOwnerObjectId(), true);

            // Frustum cull the heightmap chunks
            std::vector<HeightMapChunk>& chunks = LegacyWorld::GetHeightMapChunks();
            for (int i = 0; i < chunks.size(); i++) {
                HeightMapChunk& chunk = chunks[i];
                if (spotLightFrustum.IntersectsAABBFast(AABB(chunk.aabbMin, chunk.aabbMax))) {
                    g_flashLightShadowMapDrawInfo.heightMapChunkIndices[shadowLayer].push_back(i);
                }
            }
        }
    }

    void CreatePhysicsShapeDrawCommands() {
        ProfilerCPUZone("Physics shape commands");

        auto& set = g_drawCommandsSet;
        for (int viewportIndex = 0; viewportIndex < 4; viewportIndex++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(viewportIndex);
            if (!viewport || !viewport->IsVisible()) continue;
            CreateDrawCommandsFromIndices(set.physicsShapes[viewportIndex], g_renderItemIndicesPhysicsShapes, nullptr, viewportIndex);
        }
    }

    void UpdateBloodScreenSpaceDecalInstances() {
        ProfilerCPUZone("Blood decals");

        std::vector<BloodScreenSpaceDecal>& bloodScreenSpaceDecals = Unloved::BloodSystemOLD::GetBloodScreenSpaceDecals();
        std::sort(bloodScreenSpaceDecals.begin(), bloodScreenSpaceDecals.end(), [](const BloodScreenSpaceDecal& a, const BloodScreenSpaceDecal& b) {
            return a.m_type < b.m_type;
        });

        int instanceCount = static_cast<int>(bloodScreenSpaceDecals.size());
        g_bloodScreenSpaceDecalInstances.resize(instanceCount);

        int bloodDecalTextureIndices[4] = { -1, -1, -1, -1 };
        glm::vec2 bloodDecalAspectScales[4] = {
            glm::vec2(1.0f),
            glm::vec2(1.0f),
            glm::vec2(1.0f),
            glm::vec2(1.0f)
        };

        if (instanceCount > 0) {
            const char* bloodDecalTextureNames[4] = {
                "BloodDecal4",
                "BloodDecal6",
                "BloodDecal7",
                "BloodDecal9"
            };

            for (int type = 0; type < 4; type++) {
                Texture* texture = Hell::ResourceManager::GetTextureByName(bloodDecalTextureNames[type]);
                if (!texture) continue;

                float textureWidth = glm::max(static_cast<float>(texture->GetWidth()), 1.0f);
                float textureHeight = glm::max(static_cast<float>(texture->GetHeight()), 1.0f);
                float shortestTextureSide = glm::min(textureWidth, textureHeight);

                bloodDecalTextureIndices[type] = texture->GetBindlessIndex();
                bloodDecalAspectScales[type] = glm::vec2(textureWidth, textureHeight) / shortestTextureSide;
            }
        }

        for (int i = 0; i < instanceCount; i++) {
            BloodScreenSpaceDecal& decal = bloodScreenSpaceDecals[i];
            BloodDecalInstanceData& instance = g_bloodScreenSpaceDecalInstances[i];
            instance.modelMatrix = decal.GetModelMatrix();
            instance.inverseModelMatrix = decal.GetInverseModelMatrix();
            instance.type = decal.GetType();
            instance.textureIndex = -1;
            instance.aspectScaleX = 1.0f;
            instance.aspectScaleY = 1.0f;

            if (instance.type < 0 || instance.type >= 4) continue;

            instance.textureIndex = bloodDecalTextureIndices[instance.type];
            instance.aspectScaleX = bloodDecalAspectScales[instance.type].x;
            instance.aspectScaleY = bloodDecalAspectScales[instance.type].y;
        }
    }

    void UpdateDrawCommandsSet() {
        ProfilerCPUZone("Draw command sets");

        ClearDrawCommandsSet();
        CreateGlassDrawCommands();

        SortDrawCommandRenderItems();
        CreateViewportDrawCommands();

        CreateSpriteSheetDrawCommands();

        CreateSkinningData();
        CreatePhysicsShapeDrawCommands();

        // CSM render items (moon light shadow maps)
        CreateMoonLightShadowMapDrawCommands();

        CreateFlashLightShadowMapDrawCommands();

        UpdateBloodScreenSpaceDecalInstances();
    }

    void ClearPointLightShadowMapDrawCommands(PointLightShadowMapDrawCommands& drawCommands) {
        for (int shadowMapIndex = 0; shadowMapIndex < MAX_SHADOW_MAP_ARRAY_LEVELS; shadowMapIndex++) {
            for (int faceIndex = 0; faceIndex < 6; faceIndex++) {
                drawCommands.assetGeometry[shadowMapIndex][faceIndex].clear();
                drawCommands.assetGeometryAlphaDiscard[shadowMapIndex][faceIndex].clear();
                drawCommands.assetGeometryHair[shadowMapIndex][faceIndex].clear();
                drawCommands.assetGeometrySkinned[shadowMapIndex][faceIndex].clear();
                drawCommands.assetGeometrySkinnedAlphaDiscard[shadowMapIndex][faceIndex].clear();
                drawCommands.assetGeometrySkinnedHair[shadowMapIndex][faceIndex].clear();
                drawCommands.assetGeometrySkinnedNonDeforming[shadowMapIndex][faceIndex].clear();
                drawCommands.assetGeometrySkinnedNonDeformingAlphaDiscard[shadowMapIndex][faceIndex].clear();
                drawCommands.assetGeometrySkinnedNonDeformingHair[shadowMapIndex][faceIndex].clear();
                drawCommands.procedural[shadowMapIndex][faceIndex].clear();
            }
        }
    }

    void CreatePointLightShadowMapDrawCommands(PointLightShadowMapDrawCommands& drawCommands, const std::vector<ShadowMapInfo>& shadowMaps, const std::vector<uint32_t>& renderItemIndices, bool includeSkinned, bool includeProcedural) {
        for (const ShadowMapInfo& shadowMapInfo : shadowMaps) {
            Light* light = Unloved::World::GetLightByObjectId(shadowMapInfo.lightId);
            if (!light) continue;

            const int shadowMapIndex = shadowMapInfo.shadowMapIndex;
            if (shadowMapIndex == -1) continue;

            for (uint32_t faceIndex = 0; faceIndex < 6; faceIndex++) {
                Unloved::Frustum* frustum = light->GetFrustumByFaceIndex(faceIndex);
                if (!frustum) continue;

                CreateShadowCubeMapMultiDrawIndirectCommands(drawCommands.assetGeometry[shadowMapIndex][faceIndex], renderItemIndices, faceIndex, light, BlendingMode::DEFAULT);
                CreateShadowCubeMapMultiDrawIndirectCommands(drawCommands.assetGeometryAlphaDiscard[shadowMapIndex][faceIndex], renderItemIndices, faceIndex, light, BlendingMode::ALPHA_DISCARD);
                CreateShadowCubeMapMultiDrawIndirectCommands(drawCommands.assetGeometryHair[shadowMapIndex][faceIndex], renderItemIndices, faceIndex, light, BlendingMode::HAIR);

                if (includeSkinned) {
                    CreateDrawCommandsSkinnedFromIndices(drawCommands.assetGeometrySkinned[shadowMapIndex][faceIndex], g_skinnedRenderItemIndicesDefault, -1, frustum);
                    CreateDrawCommandsSkinnedFromIndices(drawCommands.assetGeometrySkinnedAlphaDiscard[shadowMapIndex][faceIndex], g_skinnedRenderItemIndicesAlphaDiscard, -1, frustum);
                    CreateDrawCommandsSkinnedFromIndices(drawCommands.assetGeometrySkinnedHair[shadowMapIndex][faceIndex], g_skinnedRenderItemIndicesHair, -1, frustum);
                    CreateDrawCommandsNonDeformingSkinnedFromIndices(drawCommands.assetGeometrySkinnedNonDeforming[shadowMapIndex][faceIndex], g_skinnedNonDeformingRenderItemIndices, -1, frustum);
                    CreateDrawCommandsNonDeformingSkinnedFromIndices(drawCommands.assetGeometrySkinnedNonDeformingAlphaDiscard[shadowMapIndex][faceIndex], g_skinnedNonDeformingRenderItemIndicesAlphaDiscard, -1, frustum);
                    CreateDrawCommandsNonDeformingSkinnedFromIndices(drawCommands.assetGeometrySkinnedNonDeformingHair[shadowMapIndex][faceIndex], g_skinnedNonDeformingRenderItemIndicesHair, -1, frustum);
                }

                if (includeProcedural) {
                    CreateDrawCommandProceduralFromIndices(drawCommands.procedural[shadowMapIndex][faceIndex], g_renderItemIndicesProcedural, frustum, -1);
                }
            }
        }
    }

    void UpdatePointLightShadowMapDrawCommands() {
        DrawCommandsSet& set = g_drawCommandsSet;

        ClearPointLightShadowMapDrawCommands(set.staticHiResShadowMapDrawCommands);
        ClearPointLightShadowMapDrawCommands(set.staticLowResShadowMapDrawCommands);
        ClearPointLightShadowMapDrawCommands(set.compositeHiResShadowMapDrawCommands);
        ClearPointLightShadowMapDrawCommands(set.compositeLowResShadowMapDrawCommands);

        CreatePointLightShadowMapDrawCommands(set.staticHiResShadowMapDrawCommands, ShadowMapManager::GetStaticDirtyHiResShadowMaps(), g_renderItemIndicesStaticPointLightShadows, false, true);
        CreatePointLightShadowMapDrawCommands(set.staticLowResShadowMapDrawCommands, ShadowMapManager::GetStaticDirtyLowResShadowMaps(), g_renderItemIndicesStaticPointLightShadows, false, true);

        // Both modes feed the same composite pass. Only its caster set changes.
        const bool staticCacheEnabled = ShadowMapManager::StaticCacheEnabled();
        const std::vector<uint32_t>& compositeRenderItemIndices = staticCacheEnabled ? g_renderItemIndicesDynamicPointLightShadows : g_renderItemIndicesPointLightShadows;
        const bool includeProcedural = !staticCacheEnabled;
        CreatePointLightShadowMapDrawCommands(set.compositeHiResShadowMapDrawCommands, ShadowMapManager::GetCompositeDirtyHiResShadowMaps(), compositeRenderItemIndices, true, includeProcedural);
        CreatePointLightShadowMapDrawCommands(set.compositeLowResShadowMapDrawCommands, ShadowMapManager::GetCompositeDirtyLowResShadowMaps(), compositeRenderItemIndices, true, includeProcedural);
    }

    void CreateDrawCommandProceduralFromIndices(std::vector<DrawIndexedIndirectCommand>& drawCommands, const std::vector<uint32_t>& renderItemIndices, Unloved::Frustum* frustum, int viewportIndex) {
        uint32_t drawIndexOffset = static_cast<uint32_t>(g_drawRenderItemIndices.size());
        g_drawRenderItemIndices.reserve(g_drawRenderItemIndices.size() + renderItemIndices.size());

        for (uint32_t renderItemIndex : renderItemIndices) {
            const RenderItem& renderItem = g_sceneRenderItems[renderItemIndex];

            if (!frustum || frustum->IntersectsAABBFast(renderItem)) {
                g_drawRenderItemIndices.push_back(renderItemIndex);
            }
        }

        drawCommands.reserve(drawCommands.size() + g_drawRenderItemIndices.size() - drawIndexOffset);

        DrawIndexedIndirectCommand* currentCommand = nullptr;
        uint32_t currentMeshId = 0;

        for (uint32_t i = drawIndexOffset; i < g_drawRenderItemIndices.size(); i++) {
            const RenderItem& renderItem = g_sceneRenderItems[g_drawRenderItemIndices[i]];

            if (!currentCommand || renderItem.meshId != currentMeshId) {
                currentMeshId = renderItem.meshId;
                currentCommand = &drawCommands.emplace_back();
                SetDrawCommandMeshRange(*currentCommand, renderItem);
                currentCommand->baseInstance = i;
                currentCommand->instanceCount = 1;
            }
            else {
                currentCommand->instanceCount++;
            }
        }
    }

    void CreateDrawCommandsFromIndices(std::vector<DrawIndexedIndirectCommand>& drawCommands, const std::vector<uint32_t>& renderItemIndices, Unloved::Frustum* frustum, int viewportIndex, bool ignoreNonShadowCasters, uint64_t excludedObjectId, bool useShadowMesh) {
        uint32_t drawIndexOffset = static_cast<uint32_t>(g_drawRenderItemIndices.size());
        g_drawRenderItemIndices.reserve(g_drawRenderItemIndices.size() + renderItemIndices.size());

        for (uint32_t renderItemIndex : renderItemIndices) {
            const RenderItem& renderItem = g_sceneRenderItems[renderItemIndex];
            bool shadowCasting = ((renderItem.shadowFlags & SHADOW_FLAG_POINT_LIGHT) != 0u);

            if (excludedObjectId != 0) {
                uint64_t renderItemObjectId = 0;
                Hell::Bit::UnpackUint64(renderItem.objectIdLowerBit, renderItem.objectIdUpperBit, renderItemObjectId);
                if (renderItemObjectId == excludedObjectId) continue;
            }

            if (ignoreNonShadowCasters && !shadowCasting) continue;
            if (renderItem.ignoredViewportIndex != -1 && renderItem.ignoredViewportIndex == viewportIndex) continue;
            if (renderItem.exclusiveViewportIndex != -1 && renderItem.exclusiveViewportIndex != viewportIndex) continue;

            if (!frustum || frustum->IntersectsAABBFast(renderItem)) {
                g_drawRenderItemIndices.push_back(renderItemIndex);
            }
        }

        drawCommands.reserve(drawCommands.size() + g_drawRenderItemIndices.size() - drawIndexOffset);

        DrawIndexedIndirectCommand* currentCommand = nullptr;
        uint32_t currentMeshId = 0;

        for (uint32_t i = drawIndexOffset; i < g_drawRenderItemIndices.size(); i++) {
            const RenderItem& renderItem = g_sceneRenderItems[g_drawRenderItemIndices[i]];
            const uint32_t meshId = useShadowMesh ? GetShadowMeshId(renderItem) : renderItem.meshId;

            if (!currentCommand || meshId != currentMeshId) {
                currentMeshId = meshId;
                currentCommand = &drawCommands.emplace_back();
                if (useShadowMesh) SetShadowDrawCommandMeshRange(*currentCommand, renderItem);
                else SetDrawCommandMeshRange(*currentCommand, renderItem);
                currentCommand->baseInstance = i;
                currentCommand->instanceCount = 1;
            }
            else {
                currentCommand->instanceCount++;
            }
        }
    }

    void CreateHeightMapRenderItems() {
        std::vector<HeightMapChunk>& chunks = LegacyWorld::GetHeightMapChunks();
        Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("HeightMapGeometry");

        Transform transform;
        transform.scale = glm::vec3(HEIGHTMAP_SCALE_XZ, HEIGHTMAP_SCALE_Y, HEIGHTMAP_SCALE_XZ);
        glm::mat4 modelMatrix = transform.to_mat4();
        glm::mat4 inverseModelMatrix = glm::inverse(modelMatrix);

        int32_t materialIndex = Hell::ResourceManager::GetMaterialIndexByName("Ground_MudVeg");
        if (materialIndex == -1) {
            materialIndex = Hell::ResourceManager::GetMaterialIndexByName("CheckerBoard");
        }
        if (EditorSession::IsHeightMapEditorActive()) {
            materialIndex = Hell::ResourceManager::GetMaterialIndexByName("CheckerBoard");
        }

        g_heightMapRenderItemIndices.reserve(chunks.size());

        for (HeightMapChunk& chunk : chunks) {
            Mesh* mesh = meshBuffer.GetMeshById(chunk.meshId);
            if (!IsValidMesh(mesh)) continue;

            RenderItem renderItem;
            renderItem.modelMatrix = modelMatrix;
            renderItem.prevModelMatrix = modelMatrix;
            renderItem.inverseModelMatrix = inverseModelMatrix;
            renderItem.aabbMin = glm::vec4(chunk.aabbMin, 0.0f);
            renderItem.aabbMax = glm::vec4(chunk.aabbMax, 0.0f);
            renderItem.vertexCount = mesh->vertexCount;
            renderItem.indexCount = mesh->indexCount;
            renderItem.baseVertex = mesh->baseVertex;
            renderItem.baseIndex = mesh->baseIndex;
            renderItem.materialIndex = materialIndex;
            renderItem.meshId = chunk.meshId;

            g_heightMapRenderItemIndices.push_back(AddSceneRenderItem(renderItem));
        }
    }

    void CreateHeightMapDrawCommands(std::vector<DrawIndexedIndirectCommand>& drawCommands, Unloved::Frustum& frustum) {
        drawCommands.reserve(g_heightMapRenderItemIndices.size());
        g_drawRenderItemIndices.reserve(g_drawRenderItemIndices.size() + g_heightMapRenderItemIndices.size());

        for (uint32_t renderItemIndex : g_heightMapRenderItemIndices) {
            const RenderItem& renderItem = g_sceneRenderItems[renderItemIndex];
            if (EditorSession::IsInactive() && !frustum.IntersectsAABBFast(renderItem)) continue;

            uint32_t drawIndex = static_cast<uint32_t>(g_drawRenderItemIndices.size());
            g_drawRenderItemIndices.push_back(renderItemIndex);

            DrawIndexedIndirectCommand& command = drawCommands.emplace_back();
            command.indexCount = renderItem.indexCount;
            command.instanceCount = 1;
            command.firstIndex = renderItem.baseIndex;
            command.baseVertex = renderItem.baseVertex;
            command.baseInstance = drawIndex;
        }
    }

    void CreateDrawCommandsSkinnedFromIndices(std::vector<DrawIndexedIndirectCommand>& commands, const std::vector<uint32_t>& renderItemIndices, int viewportIndex, Unloved::Frustum* frustum) {
        commands.clear();
        commands.reserve(renderItemIndices.size());
        g_drawRenderItemIndices.reserve(g_drawRenderItemIndices.size() + renderItemIndices.size());

        for (uint32_t renderItemIndex : renderItemIndices) {
            const RenderItem& renderItem = g_sceneRenderItems[renderItemIndex];
            if (renderItem.ignoredViewportIndex != -1 && renderItem.ignoredViewportIndex == viewportIndex) continue;
            if (renderItem.exclusiveViewportIndex != -1 && renderItem.exclusiveViewportIndex != viewportIndex) continue;

            if (frustum) {
                uint64_t objectId = 0;
                Hell::Bit::UnpackUint64(renderItem.objectIdLowerBit, renderItem.objectIdUpperBit, objectId);

                SkinnedGameObject* skinnedGameObject = World::GetSkinnedGameObjectByObjectId(objectId);
                if (!skinnedGameObject) continue;
                if (!frustum->IntersectsAABBFast(skinnedGameObject->GetSkinnedAABB())) continue;
            }

            DrawIndexedIndirectCommand& command = commands.emplace_back();
            SetDrawCommandMeshRange(command, renderItem);
            command.baseInstance = static_cast<uint32_t>(g_drawRenderItemIndices.size());
            command.instanceCount = 1;
            g_drawRenderItemIndices.push_back(renderItemIndex);
        }
    }

    void CreateDrawCommandsNonDeformingSkinnedFromIndices(std::vector<DrawIndexedIndirectCommand>& commands, const std::vector<uint32_t>& renderItemIndices, int viewportIndex, Unloved::Frustum* frustum) {
        CreateDrawCommandsSkinnedFromIndices(commands, renderItemIndices, viewportIndex, frustum);
    }

    void CreateShadowCubeMapMultiDrawIndirectCommands(std::vector<DrawIndexedIndirectCommand>& drawCommands, const std::vector<uint32_t>& renderItemIndices, uint32_t faceIndex, Light* light, BlendingMode blendingModeFilter) {
        drawCommands.clear();

        if (!light) return;

        // Get face frustum, bail if invalid
        Unloved::Frustum* frustum = light->GetFrustumByFaceIndex(faceIndex);
        if (!frustum) return;

        uint32_t drawIndexOffset = static_cast<uint32_t>(g_drawRenderItemIndices.size());
        g_drawRenderItemIndices.reserve(g_drawRenderItemIndices.size() + renderItemIndices.size());

        // Append canonical scene indices if they are within the light frustum.
        // renderItemIndices is already sorted by this point.
        // but if anything breaks, check here! (maybe you re-ordered things)
        for (uint32_t renderItemIndex : renderItemIndices) {
            const RenderItem& renderItem = g_sceneRenderItems[renderItemIndex];
            if ((renderItem.shadowFlags & SHADOW_FLAG_POINT_LIGHT) == 0u) continue;

            if ((BlendingMode)renderItem.blendingMode != blendingModeFilter) {
                continue;
            }

            if (frustum->IntersectsAABBFast(renderItem)) {
                g_drawRenderItemIndices.push_back(renderItemIndex);
            }
        }

        drawCommands.reserve(g_drawRenderItemIndices.size() - drawIndexOffset);

        DrawIndexedIndirectCommand* currentCommand = nullptr;
        uint32_t currentMeshId = 0;

        for (uint32_t i = drawIndexOffset; i < g_drawRenderItemIndices.size(); i++) {
            const RenderItem& renderItem = g_sceneRenderItems[g_drawRenderItemIndices[i]];
            const uint32_t meshId = GetShadowMeshId(renderItem);

            if (!currentCommand || meshId != currentMeshId) {
                currentMeshId = meshId;
                currentCommand = &drawCommands.emplace_back();
                SetShadowDrawCommandMeshRange(*currentCommand, renderItem);
                currentCommand->baseInstance = i;
                currentCommand->instanceCount = 1;
            }
            else {
                currentCommand->instanceCount++;
            }
        }
    }

    void CreateSkinningDistpachGroups() {
        constexpr uint32_t SKINNING_WORKGROUP_SIZE = 128;

        g_skinningDispatchGroups.clear();

        // One dispatch group skins one chunk of one skinning job
        for (uint32_t jobIndex = 0; jobIndex < g_skinningJobs.size(); jobIndex++) {
            const SkinningJob& skinningJob = g_skinningJobs[jobIndex];

            // Split the job into fixed size vertex chunks
            for (uint32_t vertexOffset = 0; vertexOffset < skinningJob.vertexCount; vertexOffset += SKINNING_WORKGROUP_SIZE) {
                SkinningDispatchGroup& group = g_skinningDispatchGroups.emplace_back();
                group.jobIndex = jobIndex;
                group.vertexOffset = vertexOffset;
                group.padding0 = 0;
                group.padding1 = 0;
            }
        }
    }

    void CreateSkinningData() {
        ProfilerCPUZone("Skinning commands");

        auto& set = g_drawCommandsSet;

        // Sort render item indices by mesh index
        SortRenderItemIndices(g_skinnedRenderItemIndicesDefault);
        SortRenderItemIndices(g_skinnedRenderItemIndicesAlphaDiscard);
        SortRenderItemIndices(g_skinnedRenderItemIndicesBlended);
        SortRenderItemIndices(g_skinnedRenderItemIndicesHair);
        SortRenderItemIndices(g_skinnedViewWeaponRenderItemIndicesDefault);
        SortRenderItemIndices(g_skinnedViewWeaponRenderItemIndicesAlphaDiscard);

        SortRenderItemIndices(g_skinnedNonDeformingRenderItemIndices);
        SortRenderItemIndices(g_skinnedNonDeformingRenderItemIndicesAlphaDiscard);
        SortRenderItemIndices(g_skinnedNonDeformingRenderItemIndicesBlended);
        SortRenderItemIndices(g_skinnedNonDeformingRenderItemIndicesHair);
        SortRenderItemIndices(g_skinnedNonDeformingViewWeaponRenderItemIndices);
        SortRenderItemIndices(g_skinnedNonDeformingViewWeaponRenderItemIndicesAlphaDiscard);

        // Create the per viewport draw commands
        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            CreateDrawCommandsSkinnedFromIndices(set.skinnedStandard[i], g_skinnedRenderItemIndicesDefault, i);
            CreateDrawCommandsSkinnedFromIndices(set.skinnedAlphaDiscard[i], g_skinnedRenderItemIndicesAlphaDiscard, i);
            CreateDrawCommandsSkinnedFromIndices(set.skinnedBlended[i], g_skinnedRenderItemIndicesBlended, i);
            CreateDrawCommandsSkinnedFromIndices(set.skinnedHair[i], g_skinnedRenderItemIndicesHair, i);
            CreateDrawCommandsSkinnedFromIndices(set.skinnedViewWeaponAlphaDiscard[i], g_skinnedViewWeaponRenderItemIndicesAlphaDiscard, i);
            CreateDrawCommandsSkinnedFromIndices(set.skinnedViewWeaponStandard[i], g_skinnedViewWeaponRenderItemIndicesDefault, i);
        }

        // Combine all deforming skinned indices for passes that need every skinned item.
        g_combinedSkinnedRenderItemIndices.clear();
        g_combinedSkinnedRenderItemIndices.insert(g_combinedSkinnedRenderItemIndices.end(), g_skinnedRenderItemIndicesDefault.begin(), g_skinnedRenderItemIndicesDefault.end());
        g_combinedSkinnedRenderItemIndices.insert(g_combinedSkinnedRenderItemIndices.end(), g_skinnedRenderItemIndicesAlphaDiscard.begin(), g_skinnedRenderItemIndicesAlphaDiscard.end());
        g_combinedSkinnedRenderItemIndices.insert(g_combinedSkinnedRenderItemIndices.end(), g_skinnedRenderItemIndicesBlended.begin(), g_skinnedRenderItemIndicesBlended.end());
        g_combinedSkinnedRenderItemIndices.insert(g_combinedSkinnedRenderItemIndices.end(), g_skinnedRenderItemIndicesHair.begin(), g_skinnedRenderItemIndicesHair.end());
        g_combinedSkinnedRenderItemIndices.insert(g_combinedSkinnedRenderItemIndices.end(), g_skinnedViewWeaponRenderItemIndicesDefault.begin(), g_skinnedViewWeaponRenderItemIndicesDefault.end());
        g_combinedSkinnedRenderItemIndices.insert(g_combinedSkinnedRenderItemIndices.end(), g_skinnedViewWeaponRenderItemIndicesAlphaDiscard.begin(), g_skinnedViewWeaponRenderItemIndicesAlphaDiscard.end());

        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            CreateDrawCommandsNonDeformingSkinnedFromIndices(set.skinnedNonDeformingAlphaDiscard[i], g_skinnedNonDeformingRenderItemIndicesAlphaDiscard, i);
            CreateDrawCommandsNonDeformingSkinnedFromIndices(set.skinnedNonDeformingBlended[i], g_skinnedNonDeformingRenderItemIndicesBlended, i);
            CreateDrawCommandsNonDeformingSkinnedFromIndices(set.skinnedNonDeformingStandard[i], g_skinnedNonDeformingRenderItemIndices, i);
            CreateDrawCommandsNonDeformingSkinnedFromIndices(set.skinnedNonDeformingHair[i], g_skinnedNonDeformingRenderItemIndicesHair, i);
            CreateDrawCommandsNonDeformingSkinnedFromIndices(set.skinnedNonDeformingViewWeaponAlphaDiscard[i], g_skinnedNonDeformingViewWeaponRenderItemIndicesAlphaDiscard, i);
            CreateDrawCommandsNonDeformingSkinnedFromIndices(set.skinnedNonDeformingViewWeaponStandard[i], g_skinnedNonDeformingViewWeaponRenderItemIndices, i);
        }
    }

    const RendererData& GetRendererData() {
        return g_rendererData;
    }

    const std::vector<ViewportData>& GetViewportData() {
        return g_viewportData;
    }

    const std::vector<uint32_t>& GetRenderItemIndicesOutline() {
        return g_renderItemIndicesOutline;
    }

    const std::vector<uint32_t>& GetRenderItemIndicesOutlinePhysicsShapes() {
        return g_renderItemIndicesOutlinePhysicsShapes;
    }

    const std::vector<uint32_t>& GetRenderItemIndicesOutlineProcedural() {
        return g_renderItemIndicesOutlineProcedural;
    }

    const std::vector<uint32_t>& GetRenderItemIndicesOutlineSkinned() {
        return g_renderItemIndicesOutlineSkinned;
    }

    const std::vector<uint32_t>& GetRenderItemIndicesPlastic() {
        return g_renderItemIndicesPlastic;
    }

    const std::vector<uint32_t>& GetRenderItemIndicesProcedural() {
        return g_renderItemIndicesProcedural;
    }

    const std::vector<uint32_t>& GetCombinedSkinnedRenderItemIndices() {
        return g_combinedSkinnedRenderItemIndices;
    }

    std::size_t GetRenderItemCount(BlendingMode blendingMode) {
        switch (blendingMode) {
        case BlendingMode::DEFAULT:       return g_renderItemIndices.size();
        case BlendingMode::ALPHA_DISCARD: return g_renderItemIndicesAlphaDiscarded.size();
        case BlendingMode::BLENDED:       return g_renderItemIndicesBlended.size();
        case BlendingMode::GLASS:         return g_renderItemIndicesGlass.size();
        case BlendingMode::HAIR:          return g_renderItemIndicesHair.size();
        case BlendingMode::MIRROR:        return g_renderItemIndicesMirror.size();
        case BlendingMode::TOILET_WATER:  return g_renderItemIndicesToiletWater.size();
        case BlendingMode::PLASTIC:       return g_renderItemIndicesPlastic.size();
        default:                          return 0;
        }
    }

    std::size_t GetSkinnedRenderItemCount(BlendingMode blendingMode) {
        switch (blendingMode) {
        case BlendingMode::DEFAULT:       return g_skinnedRenderItemIndicesDefault.size() + g_skinnedViewWeaponRenderItemIndicesDefault.size();
        case BlendingMode::ALPHA_DISCARD: return g_skinnedRenderItemIndicesAlphaDiscard.size() + g_skinnedViewWeaponRenderItemIndicesAlphaDiscard.size();
        case BlendingMode::BLENDED:       return g_skinnedRenderItemIndicesBlended.size();
        case BlendingMode::HAIR:          return g_skinnedRenderItemIndicesHair.size();
        default:                          return 0;
        }
    }

    // Submissions
    void CreateGPULights() {
        g_gpuLights.clear();

        for (Light& light : World::GetLights()) {
            if (EditorSession::Visibility::ShouldHide(light.GetObjectId())) continue;
            GPULight& gpuLight = g_gpuLights.emplace_back();
            gpuLight.colorR = light.GetColor().r;
            gpuLight.colorG = light.GetColor().g;
            gpuLight.colorB = light.GetColor().b;
            gpuLight.posX = light.GetPosition().x;
            gpuLight.posY = light.GetPosition().y;
            gpuLight.posZ = light.GetPosition().z;
            gpuLight.radius = light.GetRadius();
            gpuLight.strength = light.GetStrength();
            gpuLight.isDirtyForRaytracing = light.IsDirtyForRaytracing();
            gpuLight.hiResShadowMapIndex = ShadowMapManager::GetHiResShadowMapIndex(light.GetObjectId());
            gpuLight.lowResShadowMapIndex = ShadowMapManager::GetLowResShadowMapIndex(light.GetObjectId());
            gpuLight.worldBoundsMin = glm::vec4(light.GetWorldBoundsMin(), 0.0f);
            gpuLight.worldBoundsMax = glm::vec4(light.GetWorldBoundsMax(), 0.0f);

            IESProfile* iesProfile = nullptr;
            switch (light.GetIESProfileType()) {
                case IESProfileType::LAMP_0: iesProfile = Hell::ResourceManager::GetIESProfilePtr("Lamp0"); break;
                case IESProfileType::LAMP_1: iesProfile = Hell::ResourceManager::GetIESProfilePtr("Lamp1"); break;
                case IESProfileType::LAMP_2: iesProfile = Hell::ResourceManager::GetIESProfilePtr("Lamp2"); break;
                case IESProfileType::LAMP_3: iesProfile = Hell::ResourceManager::GetIESProfilePtr("Lamp3"); break;
                case IESProfileType::LAMP_4: iesProfile = Hell::ResourceManager::GetIESProfilePtr("Lamp4"); break;
                case IESProfileType::LAMP_5: iesProfile = Hell::ResourceManager::GetIESProfilePtr("Lamp5"); break;
                case IESProfileType::LAMP_6: iesProfile = Hell::ResourceManager::GetIESProfilePtr("Lamp6"); break;
                case IESProfileType::LAMP_7: iesProfile = Hell::ResourceManager::GetIESProfilePtr("Lamp7"); break;
                case IESProfileType::LAMP_8: iesProfile = Hell::ResourceManager::GetIESProfilePtr("Lamp8"); break;
                case IESProfileType::LAMP_9: iesProfile = Hell::ResourceManager::GetIESProfilePtr("Lamp9"); break;
                case IESProfileType::LAMP_10: iesProfile = Hell::ResourceManager::GetIESProfilePtr("Lamp10"); break;
                case IESProfileType::LAMP_11: iesProfile = Hell::ResourceManager::GetIESProfilePtr("Lamp11"); break;
                default: break;
            }

            if (iesProfile) {
                gpuLight.iesExposure = light.GetIESExposure();
                gpuLight.iesTextureIndex = iesProfile->GetTextureIndex();
                gpuLight.iesVScale = iesProfile->GetVScale();
                gpuLight.iesVBias = iesProfile->GetVBias();
                gpuLight.iesHScale = iesProfile->GetHScale();
                gpuLight.iesHBias = iesProfile->GetHBias();
                gpuLight.iesMaxIntensity = iesProfile->GetMaxIntensity();

                // TODO: move to light init and this WHOLE FUNCTION to Light::CreateGPULight()
                glm::vec3 forward = light.GetForward();
                if (glm::dot(forward, forward) < 0.0001f) {
                    forward = glm::vec3(0.0f, -1.0f, 0.0f); // Default to straight down
                }
                else {
                    forward = glm::normalize(forward);
                }

                float twist = glm::radians(light.GetTwist());
                glm::vec3 tempUp = (std::abs(forward.y) > 0.999f) ? glm::vec3(0, 0, 1) : glm::vec3(0, 1, 0);
                glm::vec3 right = glm::normalize(glm::cross(tempUp, forward));
                glm::vec3 up = glm::cross(forward, right);
                glm::vec3 finalRight = right * std::cos(twist) + up * std::sin(twist);
                glm::vec3 finalUp = glm::cross(forward, finalRight);

                gpuLight.forward = forward;
                gpuLight.right = finalRight;
                gpuLight.up = finalUp;
            }
        }
    }

    void CreateGPUSpotLights() {
        g_gpuSpotLights.clear();

        // Shadow layers are transient render resources, never stable light IDs.
        // Keep each local player's layer stable, then give any remaining layers
        // to remote-player/enemy flashlights in world order.
        bool usedShadowLayers[MAX_SHADOWED_SPOT_LIGHTS]{};
        for (SpotLight& light : World::GetSpotLights()) {
            light.SetShadowLayer(-1);
        }

        for (SpotLight& light : World::GetSpotLights()) {
            const int32_t ownerViewportIndex = light.GetOwnerViewportIndex();
            if (!light.IsActive() || !light.CastsShadows()) continue;
            if (ownerViewportIndex < 0 || ownerViewportIndex >= MAX_SHADOWED_SPOT_LIGHTS) continue;
            if (usedShadowLayers[ownerViewportIndex]) continue;

            light.SetShadowLayer(ownerViewportIndex);
            usedShadowLayers[ownerViewportIndex] = true;
        }

        for (SpotLight& light : World::GetSpotLights()) {
            if (!light.IsActive() || !light.CastsShadows() || light.GetShadowLayer() >= 0) continue;

            for (int32_t layer = 0; layer < MAX_SHADOWED_SPOT_LIGHTS; layer++) {
                if (usedShadowLayers[layer]) continue;
                light.SetShadowLayer(layer);
                usedShadowLayers[layer] = true;
                break;
            }
        }

        for (SpotLight& light : World::GetSpotLights()) {
            if (!light.IsActive()) continue;

            const SpotLightData& data = light.GetData();
            uint32_t flags = 0;
            if (data.castsShadows) flags |= SPOT_LIGHT_FLAG_CAST_SHADOWS;
            if (data.skipOwnerShadow) flags |= SPOT_LIGHT_FLAG_SKIP_OWNER_SHADOW;
            if (data.useFlashlightViewDistanceScale) flags |= SPOT_LIGHT_FLAG_VIEW_DISTANCE_SCALE;

            GPUSpotLight& gpuLight = g_gpuSpotLights.emplace_back();
            gpuLight.projectionView = data.projectionView;
            gpuLight.positionModifier = glm::vec4(data.position, data.modifier);
            gpuLight.direction = glm::vec4(data.direction, 0.0f);
            gpuLight.worldBoundsMin = glm::vec4(light.GetWorldBoundsMin(), 0.0f);
            gpuLight.worldBoundsMax = glm::vec4(light.GetWorldBoundsMax(), 0.0f);
            gpuLight.metadata = glm::ivec4(light.GetShadowLayer(), light.GetOwnerViewportIndex(), static_cast<int32_t>(flags), 0);
        }
    }

    // RENDER ITEM SUBMISSIONS

    uint32_t AddSceneRenderItem(const RenderItem& renderItem) {
        uint32_t renderItemIndex = static_cast<uint32_t>(g_sceneRenderItems.size());
        g_sceneRenderItems.push_back(renderItem);
        return renderItemIndex;
    }

    void AddRenderItemToCategory(std::vector<uint32_t>& renderItemIndices, uint32_t renderItemIndex) {
        renderItemIndices.push_back(renderItemIndex);
    }

	void SubmitRenderItemProcedural(const RenderItem& renderItem) {
		uint64_t objectId = 0;
		Hell::Bit::UnpackUint64(renderItem.objectIdLowerBit, renderItem.objectIdUpperBit, objectId);
		if (EditorSession::Visibility::ShouldHide(objectId)) return;
		if (!ValidateRenderItemMeshRange(renderItem, "RenderDataManager::SubmitRenderItemProcedural()")) return;

		uint32_t renderItemIndex = AddSceneRenderItem(renderItem);
        AddRenderItemToCategory(g_renderItemIndicesProcedural, renderItemIndex);

        if (EditorSession::Selection::ShouldOutlineObject(objectId)) AddRenderItemToCategory(g_renderItemIndicesOutlineProcedural, renderItemIndex);

        if (Hell::BackEnd::GetAPI() == API::VULKAN) {
            AddProceduralRayQueryRenderItem(renderItemIndex);
        }
    }

    void SubmitRenderItemPhysicsShape(const RenderItem& renderItem, bool outline) {
        if (!ValidateRenderItemMeshRange(renderItem, "RenderDataManager::SubmitRenderItemPhysicsShape()")) return;

        const uint32_t renderItemIndex = AddSceneRenderItem(renderItem);
        AddRenderItemToCategory(g_renderItemIndicesPhysicsShapes, renderItemIndex);
        if (outline) AddRenderItemToCategory(g_renderItemIndicesOutlinePhysicsShapes, renderItemIndex);
    }

    void SubmitRenderItemsMirror(const std::vector<RenderItem>& renderItems) {
        std::vector<uint32_t> renderItemIndices;
        renderItemIndices.reserve(renderItems.size());

        for (const RenderItem& renderItem : renderItems) {
            renderItemIndices.push_back(AddSceneRenderItem(renderItem));
        }

        g_renderItemIndicesMirror.insert(g_renderItemIndicesMirror.begin(), renderItemIndices.begin(), renderItemIndices.end());
    }

    void SubmitDecalPaintingInfo(DecalPaintingInfo decalPaintingInfo) {
        g_decalPaintingInfo.push_back(decalPaintingInfo);
    }

    void SubmitSkinnedRenderItems(const std::vector<RenderItem>& renderItems) {
        std::vector<uint32_t> renderItemIndices;
        renderItemIndices.reserve(renderItems.size());

        for (const RenderItem& renderItem : renderItems) {
            renderItemIndices.push_back(AddSceneRenderItem(renderItem));
        }

        g_skinnedRenderItemIndicesDefault.insert(g_skinnedRenderItemIndicesDefault.begin(), renderItemIndices.begin(), renderItemIndices.end());
    }

    void SubmitAnimatedMeshNodes(const AnimatedMeshNodes& animatedMeshNodes) {
        // Get parent
        SkinnedGameObject* skinnedGameObject = Unloved::World::GetSkinnedGameObjectByObjectId(animatedMeshNodes.m_parentId);
        if (!skinnedGameObject) return;
        const bool viewWeapon = skinnedGameObject->IsViewWeapon();
        std::vector<uint32_t>& skinnedDefaultIndices = viewWeapon ? g_skinnedViewWeaponRenderItemIndicesDefault : g_skinnedRenderItemIndicesDefault;
        std::vector<uint32_t>& skinnedAlphaDiscardIndices = viewWeapon ? g_skinnedViewWeaponRenderItemIndicesAlphaDiscard : g_skinnedRenderItemIndicesAlphaDiscard;
        std::vector<uint32_t>& rigidDefaultIndices = viewWeapon ? g_skinnedNonDeformingViewWeaponRenderItemIndices : g_skinnedNonDeformingRenderItemIndices;
        std::vector<uint32_t>& rigidAlphaDiscardIndices = viewWeapon ? g_skinnedNonDeformingViewWeaponRenderItemIndicesAlphaDiscard : g_skinnedNonDeformingRenderItemIndicesAlphaDiscard;

        if (!animatedMeshNodes.RenderingEnabled() || EditorSession::Visibility::ShouldHide(animatedMeshNodes.m_parentId)) {
            skinnedGameObject->CommitRenderPoseHistory();
            return;
        }

        const std::vector<glm::mat4>& currentSkinningTransforms = skinnedGameObject->GetBoneSkinningMatrices();
        const std::map<std::string, float>& currentMorphTargetWeights = skinnedGameObject->GetMorphTargetWeights();
        const glm::mat4 currentModelMatrix = skinnedGameObject->GetModelMatrix();

        // Default previous pose to current pose
        const std::vector<glm::mat4>* previousSkinningTransformsPtr = &currentSkinningTransforms;
        const std::map<std::string, float>* previousMorphTargetWeightsPtr = &currentMorphTargetWeights;
        glm::mat4 previousModelMatrix = currentModelMatrix;

        // Use render history when the bone counts match
        if (skinnedGameObject->HasRenderPoseHistory()) {
            const std::vector<glm::mat4>& renderHistory = skinnedGameObject->GetPreviousRenderBoneSkinningMatrices();

            if (renderHistory.size() == currentSkinningTransforms.size()) {
                previousSkinningTransformsPtr = &renderHistory;
                previousMorphTargetWeightsPtr = &skinnedGameObject->GetPreviousRenderMorphTargetWeights();
                previousModelMatrix = skinnedGameObject->GetPreviousRenderModelMatrix();
            }
        }

        const std::vector<glm::mat4>& previousSkinningTransforms = *previousSkinningTransformsPtr;
        const std::map<std::string, float>& previousMorphTargetWeights = *previousMorphTargetWeightsPtr;

        // Cache the base indices before they're mutated
        uint32_t baseSkinningTransformIndex = g_skinningTransforms.size();

        // Append current and previous render-pose matrices at matching offsets.
        g_skinningTransforms.insert(g_skinningTransforms.end(), currentSkinningTransforms.begin(), currentSkinningTransforms.end());
        g_previousSkinningTransforms.insert(g_previousSkinningTransforms.end(), previousSkinningTransforms.begin(), previousSkinningTransforms.end());

        uint32_t transientRayQueryBLASIndex = UINT32_MAX;

        for (const AnimatedMeshNode& node : animatedMeshNodes.GetNodes()) {
            RenderItem renderItem = node.renderItem;
            if (node.blendingMode == BlendingMode::DO_NOT_RENDER) continue;

            if (node.excludeFromVulkanTLAS) {
                renderItem.vulkanFlags |= VULKAN_FLAG_EXCLUDE_FROM_TLAS;
            }

            Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("AssetGeometry");
            Hell::SkinnedMeshMetadata* metadata = meshBuffer.GetSkinnedMeshMetadataByMeshId(renderItem.meshId);
            if (!metadata) continue;
            const bool morphable = !metadata->morphTargets.empty();
            const bool useMorphDeformation = morphable && Hell::BackEnd::GetAPI() == API::OPENGL;

            // Deforming
            if (node.deforming || useMorphDeformation) {
                const int32_t rigidBoneIndex = node.deforming ? -1 : metadata->nonDeformingBoneIndex;
                if (!node.deforming &&
                    (rigidBoneIndex < 0 ||
                     rigidBoneIndex >= currentSkinningTransforms.size() ||
                     rigidBoneIndex >= previousSkinningTransforms.size())) {
                    continue;
                }

                renderItem.modelMatrix = currentModelMatrix;
                renderItem.inverseModelMatrix = glm::inverse(currentModelMatrix);
                renderItem.prevModelMatrix = previousModelMatrix;

                SkinningJob& skinningJob = g_skinningJobs.emplace_back();
                skinningJob.sourceBaseVertex = node.baseVertex;
                skinningJob.sourceBaseIndex = node.baseIndex;
                skinningJob.vertexCount = node.vertexCount;
                skinningJob.indexCount = node.indexCount;
                skinningJob.skinnedBaseVertex = g_baseSkinnedVertex;
                skinningJob.skinningTransformOffset = baseSkinningTransformIndex;
                skinningJob.sourceVertexWeightOffset = static_cast<uint32_t>(node.baseVertexWeight);

                SkinningMorphJob& skinningMorphJob = g_skinningMorphJobs.emplace_back();
                skinningMorphJob.morphTargetOffset = static_cast<uint32_t>(g_skinningMorphTargets.size());
                skinningMorphJob.rigidBoneIndex = rigidBoneIndex;

                if (useMorphDeformation) {
                    for (const Hell::MeshMorphTargetMetadata& morphTarget : metadata->morphTargets) {
                        SkinningMorphTarget& skinningMorphTarget = g_skinningMorphTargets.emplace_back();
                        skinningMorphTarget.positionDeltaOffset = morphTarget.positionDeltaOffset;
                        skinningMorphTarget.positionDeltaCount = morphTarget.positionDeltaCount;
                        skinningMorphTarget.normalDeltaOffset = morphTarget.normalDeltaOffset;
                        skinningMorphTarget.normalDeltaCount = morphTarget.normalDeltaCount;

                        const auto currentWeightIt = currentMorphTargetWeights.find(morphTarget.name);
                        const auto previousWeightIt = previousMorphTargetWeights.find(morphTarget.name);
                        skinningMorphTarget.currentWeight = currentWeightIt != currentMorphTargetWeights.end() ? currentWeightIt->second : 0.0f;
                        skinningMorphTarget.previousWeight = previousWeightIt != previousMorphTargetWeights.end() ? previousWeightIt->second : 0.0f;
                    }
                }
                skinningMorphJob.morphTargetCount = static_cast<uint32_t>(g_skinningMorphTargets.size()) - skinningMorphJob.morphTargetOffset;

                // Assign a new base vertex to be used by compute skinning
                renderItem.baseSkinningTransformIndex = baseSkinningTransformIndex;
                renderItem.baseVertex = g_baseSkinnedVertex;

                g_baseSkinnedVertex += renderItem.vertexCount;
                uint32_t renderItemIndex = AddSceneRenderItem(renderItem);
                AddRenderItemToTransientRayQueryBLAS(renderItemIndex, transientRayQueryBLASIndex);
                if (EditorSession::Selection::ShouldOutlineObject(skinnedGameObject->GetOwnerObjectId())) AddRenderItemToCategory(g_renderItemIndicesOutlineSkinned, renderItemIndex);

                switch (node.blendingMode) {
                case BlendingMode::DEFAULT:       AddRenderItemToCategory(skinnedDefaultIndices,     renderItemIndex); break;
                case BlendingMode::ALPHA_DISCARD: AddRenderItemToCategory(skinnedAlphaDiscardIndices, renderItemIndex); break;
                case BlendingMode::BLENDED:       AddRenderItemToCategory(g_skinnedRenderItemIndicesBlended,      renderItemIndex); break;
                case BlendingMode::HAIR:          AddRenderItemToCategory(g_skinnedRenderItemIndicesHair,         renderItemIndex); break;
                default: break;
                }
            }
            // Non deforming
            else {
                const int boneIndex = metadata->nonDeformingBoneIndex;

                if (boneIndex >= 0 &&
                    boneIndex < currentSkinningTransforms.size() &&
                    boneIndex < previousSkinningTransforms.size()) {
                    renderItem.modelMatrix = currentModelMatrix * currentSkinningTransforms[boneIndex];
                    renderItem.inverseModelMatrix = glm::inverse(renderItem.modelMatrix);
                    renderItem.prevModelMatrix = previousModelMatrix * previousSkinningTransforms[boneIndex];
                }
                else {
                    renderItem.prevModelMatrix = renderItem.modelMatrix;
                }

                uint32_t renderItemIndex = AddSceneRenderItem(renderItem);
                AddPersistentRayQueryRenderItem(renderItemIndex);

                switch (node.blendingMode) {
                case BlendingMode::ALPHA_DISCARD: AddRenderItemToCategory(rigidAlphaDiscardIndices, renderItemIndex); break;
                case BlendingMode::BLENDED:       AddRenderItemToCategory(g_skinnedNonDeformingRenderItemIndicesBlended,      renderItemIndex); break;
                case BlendingMode::DEFAULT:       AddRenderItemToCategory(rigidDefaultIndices,      renderItemIndex); break;
                case BlendingMode::HAIR:          AddRenderItemToCategory(g_skinnedNonDeformingRenderItemIndicesHair,         renderItemIndex); break;
                default: break;
                }
            }
        }

        skinnedGameObject->CommitRenderPoseHistory();
    }

    void SubmitMeshNodes(const MeshNodes& meshNodes, bool viewWeapon) {
        for (const MeshNode& node : meshNodes.GetNodes()) {
            SubmitRenderItem(node.renderItem, viewWeapon);
        }
    }

    void SubmitRenderItem(const RenderItem& renderItem, bool viewWeapon) {
        BlendingMode blendingMode = (BlendingMode)renderItem.blendingMode;

        if (blendingMode == BlendingMode::DO_NOT_RENDER) return;
        uint64_t objectId = 0;
        Hell::Bit::UnpackUint64(renderItem.objectIdLowerBit, renderItem.objectIdUpperBit, objectId);
        if (EditorSession::Visibility::ShouldHide(objectId)) return;
        if (!ValidateRenderItemMeshRange(renderItem, "RenderDataManager::SubmitRenderItem()")) return;

        uint32_t renderItemIndex = AddSceneRenderItem(renderItem);
        std::vector<uint32_t>& defaultIndices = viewWeapon ? g_viewWeaponRenderItemIndices : g_renderItemIndices;
        std::vector<uint32_t>& alphaDiscardIndices = viewWeapon ? g_viewWeaponRenderItemIndicesAlphaDiscarded : g_renderItemIndicesAlphaDiscarded;

        switch (blendingMode) {
        case BlendingMode::DEFAULT:       AddRenderItemToCategory(defaultIndices,        renderItemIndex); break;
        case BlendingMode::ALPHA_DISCARD: AddRenderItemToCategory(alphaDiscardIndices,   renderItemIndex); break;
        case BlendingMode::BLENDED:       AddRenderItemToCategory(g_renderItemIndicesBlended,        renderItemIndex); break;
        case BlendingMode::GLASS:         AddRenderItemToCategory(g_renderItemIndicesGlass,          renderItemIndex); break;
        case BlendingMode::HAIR:          AddRenderItemToCategory(g_renderItemIndicesHair,           renderItemIndex); break;
        case BlendingMode::MIRROR:        AddRenderItemToCategory(g_renderItemIndicesMirror,         renderItemIndex); break;
        case BlendingMode::TOILET_WATER:  AddRenderItemToCategory(g_renderItemIndicesToiletWater,    renderItemIndex); break;
        case BlendingMode::PLASTIC:       AddRenderItemToCategory(g_renderItemIndicesPlastic,        renderItemIndex); break;
        default: break;
        }

        // Emissive
        if (Material* material = ResourceManager::GetMaterialByIndex(renderItem.materialIndex)) {
            bool hasEmissiveMap = material->m_emissive != g_blackTextureIndex;
            bool hasEmissiveColor = renderItem.emissiveR != 0.0f || renderItem.emissiveG != 0.0f || renderItem.emissiveB != 0.0f;

            if (hasEmissiveMap || hasEmissiveColor) {
                AddRenderItemToCategory(g_renderItemIndicesEmissive, renderItemIndex);
            }
        }

        // Outline
        if (EditorSession::Selection::ShouldOutlineObject(objectId)) AddRenderItemToCategory(g_renderItemIndicesOutline, renderItemIndex);

        // Shadow Casting
        if ((renderItem.shadowFlags & SHADOW_FLAG_POINT_LIGHT) != 0u) {
            AddRenderItemToCategory(g_renderItemIndicesPointLightShadows, renderItemIndex);

            if (Hell::Bit::Contains(renderItem.miscFlags, MISC_FLAG_DYNAMIC_OBJECT)) {
                AddRenderItemToCategory(g_renderItemIndicesDynamicPointLightShadows, renderItemIndex);
            }
            else {
                AddRenderItemToCategory(g_renderItemIndicesStaticPointLightShadows, renderItemIndex);
            }
        }
        if ((renderItem.shadowFlags & SHADOW_FLAG_CSM) != 0u) {
            AddRenderItemToCategory(g_renderItemIndicesMoonLightShadows, renderItemIndex);
        }

        AddPersistentRayQueryRenderItem(renderItemIndex);
    }

    void SubmitRenderItems(const std::vector<RenderItem>& renderItems, bool viewWeapon) {
        for (const RenderItem& renderItem : renderItems) {
            SubmitRenderItem(renderItem, viewWeapon);
        }
    }

    void SubmitSpriteSheetRenderItem(const SpriteSheetRenderItem& renderItem) {
        g_spriteSheetRenderItems.push_back(renderItem);
    }

    // VULKAN RAYTRACING

    void AddRenderItemToTransientRayQueryBLAS(uint32_t renderItemIndex, uint32_t& groupIndex) {
        if (Hell::BackEnd::GetAPI() != API::VULKAN) return;

        const RenderItem& renderItem = g_sceneRenderItems[renderItemIndex];
        if ((renderItem.vulkanFlags & VULKAN_FLAG_EXCLUDE_FROM_TLAS) != 0u) return;

        if (groupIndex == UINT32_MAX) {
            groupIndex = static_cast<uint32_t>(g_transientRayQueryRenderItemGroups.size());
            g_transientRayQueryRenderItemGroups.emplace_back();
        }

        g_transientRayQueryRenderItemGroups[groupIndex].push_back(renderItemIndex);
    }

    void AddPersistentRayQueryRenderItem(uint32_t renderItemIndex) {
        if (Hell::BackEnd::GetAPI() != API::VULKAN) return;

        const RenderItem& renderItem = g_sceneRenderItems[renderItemIndex];
        if ((renderItem.vulkanFlags & VULKAN_FLAG_EXCLUDE_FROM_TLAS) != 0u) return;

        g_persistentRayQueryRenderItemIndices.push_back(renderItemIndex);
    }

    void AddProceduralRayQueryRenderItem(uint32_t renderItemIndex) {
        const RenderItem& renderItem = g_sceneRenderItems[renderItemIndex];
        if ((renderItem.vulkanFlags & VULKAN_FLAG_EXCLUDE_FROM_TLAS) != 0u) return;

        g_proceduralRayQueryRenderItemIndices.push_back(renderItemIndex);
    }

    // MISC HELPERS

    bool IsValidMesh(const Mesh* mesh) {
        return mesh && mesh->vertexCount > 0 && mesh->indexCount >= 3;
    }

    // GETTERS

    const DrawCommandsSet& GetDrawInfoSet()                                           { return g_drawCommandsSet; }
    const FlashLightShadowMapDrawInfo& GetFlashLightShadowMapDrawInfo()               { return g_flashLightShadowMapDrawInfo; }

    const std::vector<RenderItem>& GetSceneRenderItems()                              { return g_sceneRenderItems; }
    const std::vector<uint32_t>& GetDrawRenderItemIndices()                           { return g_drawRenderItemIndices; }
    const std::vector<GlassLightRange>& GetGlassLightRanges()                         { return g_glassLightRanges; }
    const std::vector<uint32_t>& GetGlassLightIndices()                               { return g_glassLightIndices; }
    const std::vector<GlassLightRange>& GetGlassSpotLightRanges()                     { return g_glassSpotLightRanges; }
    const std::vector<uint32_t>& GetGlassSpotLightIndices()                           { return g_glassSpotLightIndices; }
    const std::vector<SpriteSheetRenderItem>& GetSpriteSheetInstanceData()            { return g_spriteSheetInstanceData; }
    const std::vector<GPULight>& GetGPULights()                                       { return g_gpuLights; }
    const std::vector<GPUSpotLight>& GetGPUSpotLights()                               { return g_gpuSpotLights; }
    const std::vector<glm::mat4>& GetSkinningTransforms()                             { return g_skinningTransforms; }
    const std::vector<glm::mat4>& GetPreviousSkinningTransforms()                     { return g_previousSkinningTransforms; }
    const std::vector<DecalPaintingInfo>& GetDecalPaintingInfo()                      { return g_decalPaintingInfo; }
    const std::vector<BloodDecalInstanceData>& GetBloodScreenSpaceDecalInstanceData() { return g_bloodScreenSpaceDecalInstances; }

    uint32_t GetRequiredSkinnedVertexCount()                                          { return g_baseSkinnedVertex; }

}
