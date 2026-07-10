#include "RenderDataManager.h"

#include "Hell/Backend/BackEnd.h"
#include "Hell/Common/Bit.h"
#include "Hell/Logging.h"
#include "Hell/Math/Math.h"
#include "Hell/Math/Transform.h"
#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/UI/UIBackEnd.h"

#include "Unloved/Camera/Frustum.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Config/Config.h"
#include "Unloved/Editor/Editor.h"
#include "Unloved/Objects/Renderables/AnimatedGameObject.h"
#include "Unloved/Systems/Mirrors/MirrorManager.h"
#include "Unloved/Systems/Blood/BloodSystem.h"
#include "Unloved/Systems/Ocean/Ocean.h"
#include "Unloved/Systems/ShadowMaps/ShadowMapManager.h"
#include "Unloved/Viewport/ViewportManager.h"
#include "Unloved/World/World.h"

#include <span>
#include <unordered_map>

// Get me out of here
#include "World/LegacyWorld.h"
#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include <vector>
#include "Hell/Input.h"
#include "Unloved/Render/RendererConstants.h"
#include "Timer.hpp"
#include "Unloved/Common/Constants.h"
#include "Unloved/Render/Renderer.h"
#include "Unloved/Render/RendererUtil.h"
//
#include "../../../res/shaders/common/flags.glsl"

namespace Input = Hell::Input;

using namespace Hell;

namespace Unloved::RenderDataManager {
    using namespace Unloved;

    DrawCommandsSet g_drawCommandsSet;
    FlashLightShadowMapDrawInfo g_flashLightShadowMapDrawInfo;
    RendererData g_rendererData;
    std::vector<DrawIndexedIndirectCommand> g_drawCommandsUI;
    std::vector<GPULight> g_gpuLights;

	std::vector<RenderItem> g_renderItemsProcedural;
    std::vector<RenderItem> g_renderItems;
    std::vector<RenderItem> g_renderItemsBlended;
    std::vector<RenderItem> g_renderItemsAlphaDiscarded;
    std::vector<RenderItem> g_renderItemsHair;
    std::vector<RenderItem> g_renderItemsGlass;
	std::vector<RenderItem> g_renderItemsMirror;
    std::vector<RenderItem> g_renderItemsPlastic;
    std::vector<RenderItem> g_renderItemsStainedGlass;
    std::vector<RenderItem> g_renderItemsToiletWater;

    std::vector<RenderItem> g_renderItemsStaticPointLightShadows;
    std::vector<RenderItem> g_renderItemsDynamicPointLightShadows;


    std::vector<RenderItem> g_renderItemsMoonLightShadows;

    // Emissive
    std::vector<RenderItem> g_renderItemsEmissive;

    std::vector<RenderItem> g_shadowCasterRenderItems;

    std::vector<RenderItem> g_renderItemsOutline;
    std::vector<RenderItem> g_shadowMapRenderItems;

    std::vector<RenderItem> g_instanceData;
    std::vector<ViewportData> g_viewportData;

    std::vector<DecalPaintingInfo> g_decalPaintingInfo;

    std::vector<BloodDecalInstanceData> g_bloodScreenSpaceDecalInstances;

    std::vector<glm::mat4> g_skinningTransforms;
    std::vector<RenderItem> g_combinedSkinnedRenderItems;

    std::vector<RenderItem> g_skinnedRenderItemsDefault;
    std::vector<RenderItem> g_skinnedRenderItemsAlphaDiscard;
    std::vector<RenderItem> g_skinnedRenderItemsBlended;
    std::vector<RenderItem> g_skinnedRenderItemsHair;

    std::vector<RenderItem> g_skinnedNonDeformingSkinnedMeshRenderItems;
    std::vector<RenderItem> g_skinnedNonDeformingSkinnedMeshRenderItemsAlphaDiscard;
    std::vector<RenderItem> g_skinnedNonDeformingSkinnedMeshRenderItemsBlended;
    std::vector<RenderItem> g_skinnedNonDeformingSkinnedMeshRenderItemsHair;

    std::vector<RenderItem> g_nonDeformingSkinnedMeshRenderItemsDepthPeeledTransparent;
    uint32_t g_baseSkinnedVertex = 0;
    int32_t g_blackTextureIndex = -1;

    std::vector<glm::mat4> g_oceanPatchTransforms;
    std::vector<float> g_shadowCascadeLevels{ 5.0f, 10.0f, 20.0f, 40.0f }; // WARNING! YOU have a duplicate of this in GL_renderer.h

    std::vector<SkinningJob> g_skinningJobs;
    std::vector<SkinnedRayTracingGroup> g_skinnedRayTracingGroups;
    std::vector<StaticRayTracingInstance> g_staticRayTracingInstances;

    const std::vector<SkinningJob>& GetSkinningJobs()                   { return g_skinningJobs; }
    const std::vector<SkinnedRayTracingGroup>& GetSkinnedRayTracingGroups() { return g_skinnedRayTracingGroups; }
    const std::vector<StaticRayTracingInstance>& GetStaticRayTracingInstances() { return g_staticRayTracingInstances; }

    void CreateGPULights();
    void UpdateOceanPatchTransforms();
    void UpdateViewportData();
    void UpdateRendererData();
    void UpdateDrawCommandsSet();
    void UpdatePointLightShadowMapDrawCommands();

    void CreateSkinningData();
    void CreateDrawCommands(std::vector<DrawIndexedIndirectCommand>& drawCommands, std::vector<RenderItem>& renderItems, Unloved::Frustum* frustum, int viewportIndex, bool ignoreNonShadowCasters = false);
    void CreateDrawCommandsSkinned(std::vector<DrawIndexedIndirectCommand>& commands, std::vector<RenderItem>& renderItems, int viewportIndex, Unloved::Frustum* frustum = nullptr);
    void CreateDrawCommandsNonDeformingSkinned(std::vector<DrawIndexedIndirectCommand>& commands, std::vector<RenderItem>& renderItems, int viewportIndex, Unloved::Frustum* frustum = nullptr);

    void CreateMultiDrawIndirectCommands(std::vector<DrawIndexedIndirectCommand>& commands, std::span<RenderItem> renderItems, int viewportIndex, int instanceOffset);
    void CreateMultiDrawIndirectCommandsSkinned(std::vector<DrawIndexedIndirectCommand>& commands, std::span<RenderItem> renderItems, int viewportIndex, int instanceOffset);
    void CreateMultiDrawIndirectCommandsSkinnedNonDeforming(std::vector<DrawIndexedIndirectCommand>& commands, std::span<RenderItem> renderItems, int viewportIndex, int instanceOffset);

    void CreateDrawCommandProcedural(std::vector<DrawIndexedIndirectCommand>& drawCommands, std::vector<RenderItem>& renderItems, Unloved::Frustum* frustum, int viewportIndex);
	void CreateHouseMultiDrawIndirectCommands(std::vector<DrawIndexedIndirectCommand>& commands, std::span<RenderItem> renderItems, int viewportIndex, int instanceOffset);

    void CreateShadowCubeMapMultiDrawIndirectCommands(std::vector<DrawIndexedIndirectCommand>& commands, std::vector<RenderItem>& renderItems, uint32_t faceIndex, Light* light, BlendingMode blendingModeFilter);
    void CreateMoonLightShadowMapDrawCommands();


    int EncodeBaseInstance(int playerIndex, int instanceOffset);
    void DecodeBaseInstance(int baseInstance, int& playerIndex, int& instanceOffset);

    void BeginFrame() {
        g_skinningJobs.clear();
        g_skinningTransforms.clear();
        g_baseSkinnedVertex = 0;

        // Ray query (Vulkan only)
        g_skinnedRayTracingGroups.clear();
        g_staticRayTracingInstances.clear();

        // Skinned (deforming)
        g_combinedSkinnedRenderItems.clear();
        g_skinnedRenderItemsDefault.clear();
        g_skinnedRenderItemsAlphaDiscard.clear();
        g_skinnedRenderItemsBlended.clear();
        g_skinnedRenderItemsHair.clear();

        // Skinned (non deforming)
        g_skinnedNonDeformingSkinnedMeshRenderItems.clear();
        g_skinnedNonDeformingSkinnedMeshRenderItemsAlphaDiscard.clear();
        g_skinnedNonDeformingSkinnedMeshRenderItemsBlended.clear();
        g_skinnedNonDeformingSkinnedMeshRenderItemsHair.clear();

		g_nonDeformingSkinnedMeshRenderItemsDepthPeeledTransparent.clear();

		g_renderItemsProcedural.clear();

        g_renderItems.clear();
		g_renderItemsMirror.clear();
		g_renderItemsPlastic.clear();
        g_renderItemsBlended.clear();
        g_renderItemsAlphaDiscarded.clear();
        g_renderItemsHair.clear();
        g_renderItemsEmissive.clear();

        g_renderItemsStaticPointLightShadows.clear();
        g_renderItemsDynamicPointLightShadows.clear();

        g_renderItemsMoonLightShadows.clear();

        // Think about better names for these containers below
        g_renderItemsOutline.clear();
        g_decalPaintingInfo.clear();
		g_shadowCasterRenderItems.clear();
		g_renderItemsStainedGlass.clear();
		g_renderItemsGlass.clear();

        g_drawCommandsUI.clear();

        g_blackTextureIndex = ResourceManager::GetTextureBindlessIndexByName("Black");
    }

    void Update() {
        CreateGPULights();

        UpdateViewportData();
        UpdateRendererData();
        UpdateDrawCommandsSet();
        UpdateDrawCommandsUI();
        UpdatePointLightShadowMapDrawCommands();
    }

    void UpdateDrawCommandsUI() {
        const std::vector<RenderItemUI>& renderItems = UIBackEnd::GetRenderItems();
        g_drawCommandsUI.resize(renderItems.size());

        for (uint32_t i = 0; i < renderItems.size(); i++) {
            const RenderItemUI& renderItem = renderItems[i];
            DrawIndexedIndirectCommand& command = g_drawCommandsUI[i];
            command.indexCount = renderItem.indexCount;
            command.instanceCount = 1;
            command.firstIndex = renderItem.baseIndex;
            command.baseVertex = renderItem.baseVertex;
            command.baseInstance = i;
        }
    }

    const std::vector<DrawIndexedIndirectCommand>& GetDrawCommandsUI() {
        return g_drawCommandsUI;
    }

    void UpdateViewportData() {
        const Resolutions& resolutions = Config::GetResolutions();
        g_viewportData.resize(4);
        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            g_viewportData[i].colorTint = WHITE;
            g_viewportData[i].colorContrast = 1.0f;
            g_viewportData[i].isInShop = false;

            glm::mat4 viewMatrix = glm::mat4(1);
            if (Editor::IsOpen()) {
                viewMatrix = Editor::GetViewportViewMatrix(i);
                g_viewportData[i].orthoSize = Editor::GetEditorOrthoSize(i);
				g_viewportData[i].isOrtho = true;
				g_viewportData[i].fov = 1.0f;

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
            if (Editor::IsOpen()) {
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
        g_rendererData.nearPlane = Config::GetNearPlane();
        g_rendererData.farPlane = Config::GetFarPlane();
        g_rendererData.gBufferWidth = (float)resolutions.gBuffer.x;
        g_rendererData.gBufferHeight = (float)resolutions.gBuffer.y;
        g_rendererData.hairBufferWidth = (float)resolutions.hair.x;
        g_rendererData.hairBufferHeight = (float)resolutions.hair.y;
        g_rendererData.splitscreenMode = (int)Unloved::Session::GetSplitscreenMode();
        g_rendererData.time = Unloved::Session::GetSessionTime();
        g_rendererData.rendererOverrideState = (int)rendererSettings.rendererOverrideState;
        g_rendererData.normalizedMouseX = Hell::Math::MapRange(Input::GetMouseX(), 0, Hell::BackEnd::GetCurrentWindowWidth(), 0.0f, 1.0f);
        g_rendererData.normalizedMouseY = Hell::Math::MapRange(Input::GetMouseY(), 0, Hell::BackEnd::GetCurrentWindowHeight(), 0.0f, 1.0f);
        g_rendererData.tileCountX = resolutions.gBuffer.x / TILE_SIZE;
        g_rendererData.tileCountY = resolutions.gBuffer.y / TILE_SIZE;
        g_rendererData.moonLightDir = glm::vec4(Unloved::World::GetMoonlightDirection(), 0.0f);
    }

    void SortRenderItems(std::vector<RenderItem>& renderItems) {
        std::sort(renderItems.begin(), renderItems.end(), [](const RenderItem& a, const RenderItem& b) {
            return a.meshId < b.meshId;
        });
    }

    void SortRenderItemsByMeshId(std::vector<RenderItem>& renderItems) {
        std::sort(renderItems.begin(), renderItems.end(), [](const RenderItem& a, const RenderItem& b) {
            return a.meshId < b.meshId;
        });
    }

    void CreateMoonLightShadowMapDrawCommands() {
        auto& set = g_drawCommandsSet;
        int viewportCount = 4;
        int cascadeCount = SHADOW_CASCADE_COUNT;

        // Clear last frames draw commands
        for (int x = 0; x < viewportCount; x++) {
            for (int y = 0; y < cascadeCount; y++) {
                set.moonLightCascades[x][y].clear();
            }
        }

        //std::vector<RenderItem> potentialRenderItems = g_renderItems; // First start with everything in the scene
        //potentialRenderItems.insert(potentialRenderItems.end(), g_shadowCasterRenderItems.begin(), g_shadowCasterRenderItems.end());

        Unloved::Frustum frustum;

        for (int i = 0; i < viewportCount; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (!viewport || !viewport->IsVisible()) continue;

            for (int j = 0; j < cascadeCount; j++) {
                frustum.Update(g_viewportData[i].csmLightProjectionView[j]);

                // Store the instance offset for this player
                int instanceStart = g_instanceData.size();

                // Preallocate an estimate
                g_instanceData.reserve(g_instanceData.size() + g_renderItemsMoonLightShadows.size());

                // Append new render items to the global instance data if its within this cascade's frustum
                for (const RenderItem& renderItem : g_renderItemsMoonLightShadows) {
                    if (frustum.IntersectsAABBFast(renderItem)) {
                        g_instanceData.push_back(renderItem);
                        //DebugDraw::DrawAABB(AABB(renderItem.aabbMin, renderItem.aabbMax), YELLOW);
                    }
                }

                // Create indirect draw commands using the stored offset
                std::span<RenderItem> instanceView(g_instanceData.begin() + instanceStart, g_instanceData.end());
                CreateMultiDrawIndirectCommands(set.moonLightCascades[i][j], instanceView, -1, instanceStart);
            }
        }
    }

	const std::vector<RenderItem>& GetNonDeformingSkinnedMeshRenderItems() {
		return g_skinnedNonDeformingSkinnedMeshRenderItems;
	}

	const std::vector<RenderItem>& GetNonDeformingSkinnedMeshRenderItemsAlphaDiscard() {
		return g_skinnedNonDeformingSkinnedMeshRenderItemsAlphaDiscard;
	}

	const std::vector<RenderItem>& GetNonDeformingSkinnedMeshRenderItemsDepthPeeledTransparent() {
		return g_nonDeformingSkinnedMeshRenderItemsDepthPeeledTransparent;
	}


    void FrustumCullGlassRenderItemsPerViewport() {
        auto& set = g_drawCommandsSet;

        for (int i = 0; i < 4; i++) {
            std::vector<RenderItem>& renderItems = set.glass[i];
            renderItems.clear();

            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            Unloved::Player* player = Unloved::Session::GetLocalPlayerByViewportIndex(i);
            if (!player) continue;


            Unloved::Frustum& frustum = viewport->GetFrustum();

            // First frustum cull
            for (RenderItem& renderItem : g_renderItemsGlass) {
                if (frustum.IntersectsAABB(AABB(renderItem.aabbMin, renderItem.aabbMax))) {
                    renderItems.push_back(renderItem);
                }
            }

            // Now sort by distance to camera
            std::sort(renderItems.begin(), renderItems.end(), [player](RenderItem& a, RenderItem& b) {
                float distA = glm::distance(player->GetCameraPosition(), glm::vec3(a.modelMatrix[3]));
                float distB = glm::distance(player->GetCameraPosition(), glm::vec3(b.modelMatrix[3]));
                return distA > distB;
            });
        }
    }

    void UpdateDrawCommandsSet() {
        g_instanceData.clear();
        auto& set = g_drawCommandsSet;

        // Clear any commands from last frame
        for (int i = 0; i < 4; i++) {
            set.standard[i].clear();
            set.blended[i].clear();
            set.alphaDiscard[i].clear();
            set.glass[i].clear();
            set.hair[i].clear();
			set.mirrorRenderItems[i].clear();
            set.plastic[i].clear();
            set.procedural[i].clear();
            set.emissive[i].clear();

            g_flashLightShadowMapDrawInfo.flashlightShadowMapGeometry[i].clear();
            g_flashLightShadowMapDrawInfo.heightMapChunkIndices[i].clear();
            //g_flashLightShadowMapDrawInfo.houseMeshRenderItems[i].clear();
        }

        FrustumCullGlassRenderItemsPerViewport();

        SortRenderItems(g_renderItems);
        SortRenderItems(g_renderItemsBlended);
        SortRenderItems(g_renderItemsAlphaDiscarded);
        SortRenderItems(g_renderItemsHair);
        SortRenderItems(g_renderItemsPlastic);
        SortRenderItemsByMeshId(g_renderItemsProcedural);

        SortRenderItems(g_renderItemsStaticPointLightShadows);
        SortRenderItems(g_renderItemsDynamicPointLightShadows);

        SortRenderItems(g_renderItemsMoonLightShadows);

        SortRenderItems(g_renderItemsEmissive);

        // Lil hack to include bullet decals in mirrors
        int count = g_renderItems.size() + g_renderItemsAlphaDiscarded.size();
        std::vector<RenderItem> potentialMirrorItems;
        potentialMirrorItems.reserve(count);
        potentialMirrorItems.insert(potentialMirrorItems.end(), g_renderItems.begin(), g_renderItems.end());
        potentialMirrorItems.insert(potentialMirrorItems.end(), g_renderItemsAlphaDiscarded.begin(), g_renderItemsAlphaDiscarded.end());


        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            Unloved::Frustum& frustum = viewport->GetFrustum();
            CreateDrawCommands(set.standard[i], g_renderItems, &frustum, i);
            CreateDrawCommands(set.standard[i], g_renderItemsMirror, &frustum, i);
            CreateDrawCommands(set.blended[i], g_renderItemsBlended, &frustum, i);
            CreateDrawCommands(set.alphaDiscard[i], g_renderItemsAlphaDiscarded, &frustum, i);
			CreateDrawCommands(set.hair[i], g_renderItemsHair, &frustum, i);
            CreateDrawCommands(set.plastic[i], g_renderItemsPlastic, &frustum, i);
            CreateDrawCommands(set.emissive[i], g_renderItemsEmissive, &frustum, i);
			CreateDrawCommandProcedural(set.procedural[i], g_renderItemsProcedural, &frustum, i);

            if (Mirror* mirror = Unloved::MirrorManager::GetMirrorByObjectId(viewport->GetMirrorId())) {
                CreateDrawCommands(set.mirrorRenderItems[i], potentialMirrorItems, mirror->GetFrustum(i), i);
            }
        }


        CreateSkinningData();


        // CSM render items (moon light shadow maps)
        CreateMoonLightShadowMapDrawCommands();

        // Flashlight stuff
        for (int playerIndex = 0; playerIndex < Unloved::Session::GetLocalPlayerCount(); playerIndex++) {
            Unloved::Player* player = Unloved::Session::GetLocalPlayerByViewportIndex(playerIndex);
            if (!player) continue;

            Unloved::Frustum flashLightFrustum = player->GetFlashlightFrustum();

            // Build multi draw commands for regular geometry
            CreateDrawCommands(g_flashLightShadowMapDrawInfo.flashlightShadowMapGeometry[playerIndex], g_renderItems, &flashLightFrustum, playerIndex, true);

            // Frustum cull the heightmap chunks
            std::vector<HeightMapChunk>& chunks = LegacyWorld::GetHeightMapChunks();
            for (int i = 0; i < chunks.size(); i++) {
                HeightMapChunk& chunk = chunks[i];
                if (flashLightFrustum.IntersectsAABBFast(AABB(chunk.aabbMin, chunk.aabbMax))) {
                    g_flashLightShadowMapDrawInfo.heightMapChunkIndices[playerIndex].push_back(i);
                }
            }

            // Frustum cull the house mesh
            //g_flashLightShadowMapDrawInfo.houseMeshRenderItems->reserve(g_houseRenderItemsOLD.size());
            //for (int i = 0; i < g_houseRenderItemsOLD.size(); i++) {
            //    HouseRenderItem& renderItem = g_houseRenderItemsOLD[i];
            //    if (flashLightFrustum.IntersectsAABBFast(renderItem)) {
            //        g_flashLightShadowMapDrawInfo.houseMeshRenderItems[playerIndex].push_back(renderItem);
            //    }
            //}
        }

        // Screenspace blood decals
        std::vector<BloodScreenSpaceDecal>& bloodScreenSpaceDecals = Unloved::BloodSystem::GetBloodScreenSpaceDecals();
        std::sort(bloodScreenSpaceDecals.begin(), bloodScreenSpaceDecals.end(), [](const BloodScreenSpaceDecal& a, const BloodScreenSpaceDecal& b) {
            return a.m_type < b.m_type;
        });

        int instanceCount = static_cast<int>(bloodScreenSpaceDecals.size());
        g_bloodScreenSpaceDecalInstances.resize(instanceCount);

        for (int i = 0; i < instanceCount; i++) {
            BloodScreenSpaceDecal& decal = bloodScreenSpaceDecals[i];
            g_bloodScreenSpaceDecalInstances[i].modelMatrix = decal.GetModelMatrix();
            g_bloodScreenSpaceDecalInstances[i].inverseModelMatrix = decal.GetInverseModelMatrix();
            g_bloodScreenSpaceDecalInstances[i].type = decal.GetType();

            switch (decal.GetType()) {
                case 0: g_bloodScreenSpaceDecalInstances[i].textureIndex = Hell::ResourceManager::GetTextureBindlessIndexByName("BloodDecal4"); break;
                case 1: g_bloodScreenSpaceDecalInstances[i].textureIndex = Hell::ResourceManager::GetTextureBindlessIndexByName("BloodDecal6"); break;
                case 2: g_bloodScreenSpaceDecalInstances[i].textureIndex = Hell::ResourceManager::GetTextureBindlessIndexByName("BloodDecal7"); break;
                case 3: g_bloodScreenSpaceDecalInstances[i].textureIndex = Hell::ResourceManager::GetTextureBindlessIndexByName("BloodDecal9"); break;
                default: continue;
            }
        }
        UpdateOceanPatchTransforms();
    }

    void UpdatePointLightShadowMapDrawCommands() {
        auto& set = g_drawCommandsSet;

        // Clear all existing draw commands
        for (int shadowMapIndex = 0; shadowMapIndex < MAX_SHADOW_MAP_ARRAY_LEVELS; shadowMapIndex++) {
            for (int faceIndex = 0; faceIndex < 6; faceIndex++) {
                set.staticHiResShadowMapDrawCommands.assetGeometry[shadowMapIndex][faceIndex].clear();
                set.staticHiResShadowMapDrawCommands.assetGeometryAlphaDiscard[shadowMapIndex][faceIndex].clear();
                set.staticHiResShadowMapDrawCommands.assetGeometryHair[shadowMapIndex][faceIndex].clear();
                set.staticHiResShadowMapDrawCommands.assetGeometrySkinned[shadowMapIndex][faceIndex].clear();
                set.staticHiResShadowMapDrawCommands.assetGeometrySkinnedAlphaDiscard[shadowMapIndex][faceIndex].clear();
                set.staticHiResShadowMapDrawCommands.assetGeometrySkinnedHair[shadowMapIndex][faceIndex].clear();
                set.staticHiResShadowMapDrawCommands.assetGeometrySkinnedNonDeforming[shadowMapIndex][faceIndex].clear();
                set.staticHiResShadowMapDrawCommands.assetGeometrySkinnedNonDeformingAlphaDiscard[shadowMapIndex][faceIndex].clear();
                set.staticHiResShadowMapDrawCommands.assetGeometrySkinnedNonDeformingHair[shadowMapIndex][faceIndex].clear();
                set.staticHiResShadowMapDrawCommands.procedural[shadowMapIndex][faceIndex].clear();

                set.staticLowResShadowMapDrawCommands.assetGeometry[shadowMapIndex][faceIndex].clear();
                set.staticLowResShadowMapDrawCommands.assetGeometryAlphaDiscard[shadowMapIndex][faceIndex].clear();
                set.staticLowResShadowMapDrawCommands.assetGeometryHair[shadowMapIndex][faceIndex].clear();
                set.staticLowResShadowMapDrawCommands.assetGeometrySkinned[shadowMapIndex][faceIndex].clear();
                set.staticLowResShadowMapDrawCommands.assetGeometrySkinnedAlphaDiscard[shadowMapIndex][faceIndex].clear();
                set.staticLowResShadowMapDrawCommands.assetGeometrySkinnedHair[shadowMapIndex][faceIndex].clear();
                set.staticLowResShadowMapDrawCommands.assetGeometrySkinnedNonDeforming[shadowMapIndex][faceIndex].clear();
                set.staticLowResShadowMapDrawCommands.assetGeometrySkinnedNonDeformingAlphaDiscard[shadowMapIndex][faceIndex].clear();
                set.staticLowResShadowMapDrawCommands.assetGeometrySkinnedNonDeformingHair[shadowMapIndex][faceIndex].clear();
                set.staticLowResShadowMapDrawCommands.procedural[shadowMapIndex][faceIndex].clear();

                set.dynamicHiResShadowMapDrawCommands.assetGeometry[shadowMapIndex][faceIndex].clear();
                set.dynamicHiResShadowMapDrawCommands.assetGeometryAlphaDiscard[shadowMapIndex][faceIndex].clear();
                set.dynamicHiResShadowMapDrawCommands.assetGeometryHair[shadowMapIndex][faceIndex].clear();
                set.dynamicHiResShadowMapDrawCommands.assetGeometrySkinned[shadowMapIndex][faceIndex].clear();
                set.dynamicHiResShadowMapDrawCommands.assetGeometrySkinnedAlphaDiscard[shadowMapIndex][faceIndex].clear();
                set.dynamicHiResShadowMapDrawCommands.assetGeometrySkinnedHair[shadowMapIndex][faceIndex].clear();
                set.dynamicHiResShadowMapDrawCommands.assetGeometrySkinnedNonDeforming[shadowMapIndex][faceIndex].clear();
                set.dynamicHiResShadowMapDrawCommands.assetGeometrySkinnedNonDeformingAlphaDiscard[shadowMapIndex][faceIndex].clear();
                set.dynamicHiResShadowMapDrawCommands.assetGeometrySkinnedNonDeformingHair[shadowMapIndex][faceIndex].clear();
                set.dynamicHiResShadowMapDrawCommands.procedural[shadowMapIndex][faceIndex].clear();

                set.dynamicLowResShadowMapDrawCommands.assetGeometry[shadowMapIndex][faceIndex].clear();
                set.dynamicLowResShadowMapDrawCommands.assetGeometryAlphaDiscard[shadowMapIndex][faceIndex].clear();
                set.dynamicLowResShadowMapDrawCommands.assetGeometryHair[shadowMapIndex][faceIndex].clear();
                set.dynamicLowResShadowMapDrawCommands.assetGeometrySkinned[shadowMapIndex][faceIndex].clear();
                set.dynamicLowResShadowMapDrawCommands.assetGeometrySkinnedAlphaDiscard[shadowMapIndex][faceIndex].clear();
                set.dynamicLowResShadowMapDrawCommands.assetGeometrySkinnedHair[shadowMapIndex][faceIndex].clear();
                set.dynamicLowResShadowMapDrawCommands.assetGeometrySkinnedNonDeforming[shadowMapIndex][faceIndex].clear();
                set.dynamicLowResShadowMapDrawCommands.assetGeometrySkinnedNonDeformingAlphaDiscard[shadowMapIndex][faceIndex].clear();
                set.dynamicLowResShadowMapDrawCommands.assetGeometrySkinnedNonDeformingHair[shadowMapIndex][faceIndex].clear();
                set.dynamicLowResShadowMapDrawCommands.procedural[shadowMapIndex][faceIndex].clear();
            }
        }





        // S T A T I C




        // Create shadow map draw commands for dirty hi res shadow mapped lights

        for (const ShadowMapInfo& shadowMapInfo : ShadowMapManager::GetStaticDirtyHiResShadowMaps()) {
            Light* light = Unloved::World::GetLightByObjectId(shadowMapInfo.lightId);
            if (!light) continue;
            int shadowMapIndex = shadowMapInfo.shadowMapIndex;
            if (shadowMapIndex == -1) continue;

            for (uint32_t i = 0; i < 6; i++) {
                Unloved::Frustum* frustum = light->GetFrustumByFaceIndex(i);
                if (!frustum) continue;

                CreateShadowCubeMapMultiDrawIndirectCommands(set.staticHiResShadowMapDrawCommands.assetGeometry[shadowMapIndex][i], g_renderItemsStaticPointLightShadows, i, light, BlendingMode::DEFAULT);
                CreateShadowCubeMapMultiDrawIndirectCommands(set.staticHiResShadowMapDrawCommands.assetGeometryAlphaDiscard[shadowMapIndex][i], g_renderItemsStaticPointLightShadows, i, light, BlendingMode::ALPHA_DISCARD);
                CreateShadowCubeMapMultiDrawIndirectCommands(set.staticHiResShadowMapDrawCommands.assetGeometryHair[shadowMapIndex][i], g_renderItemsStaticPointLightShadows, i, light, BlendingMode::HAIR);
                CreateDrawCommandProcedural(set.staticHiResShadowMapDrawCommands.procedural[shadowMapIndex][i], g_renderItemsProcedural, frustum, -1);
            }
        }

        // Create shadow map draw commands for dirty low res shadow mapped lights
        for (const ShadowMapInfo& shadowMapInfo : ShadowMapManager::GetStaticDirtyLowResShadowMaps()) {
            Light* light = Unloved::World::GetLightByObjectId(shadowMapInfo.lightId);
            if (!light) continue;
            int shadowMapIndex = shadowMapInfo.shadowMapIndex;
            if (shadowMapIndex == -1) continue;

            for (uint32_t i = 0; i < 6; i++) {
                Unloved::Frustum* frustum = light->GetFrustumByFaceIndex(i);
                if (!frustum) continue;

                CreateShadowCubeMapMultiDrawIndirectCommands(set.staticLowResShadowMapDrawCommands.assetGeometry[shadowMapIndex][i], g_renderItemsStaticPointLightShadows, i, light, BlendingMode::DEFAULT);
                CreateShadowCubeMapMultiDrawIndirectCommands(set.staticLowResShadowMapDrawCommands.assetGeometryAlphaDiscard[shadowMapIndex][i], g_renderItemsStaticPointLightShadows, i, light, BlendingMode::ALPHA_DISCARD);
                CreateShadowCubeMapMultiDrawIndirectCommands(set.staticLowResShadowMapDrawCommands.assetGeometryHair[shadowMapIndex][i], g_renderItemsStaticPointLightShadows, i, light, BlendingMode::HAIR);
                CreateDrawCommandProcedural(set.staticLowResShadowMapDrawCommands.procedural[shadowMapIndex][i], g_renderItemsProcedural, frustum, -1);
            }
        }








         // D Y N A M I C




        // Create shadow map draw commands for dirty hi res shadow mapped lights

        for (const ShadowMapInfo& shadowMapInfo : ShadowMapManager::GetDynamicDirtyHiResShadowMaps()) {
            Light* light = Unloved::World::GetLightByObjectId(shadowMapInfo.lightId);
            if (!light) continue;
            int shadowMapIndex = shadowMapInfo.shadowMapIndex;
            if (shadowMapIndex == -1) continue;

            for (uint32_t i = 0; i < 6; i++) {
                Unloved::Frustum* frustum = light->GetFrustumByFaceIndex(i);
                if (!frustum) continue;

                CreateShadowCubeMapMultiDrawIndirectCommands(set.dynamicHiResShadowMapDrawCommands.assetGeometry[shadowMapIndex][i], g_renderItemsDynamicPointLightShadows, i, light, BlendingMode::DEFAULT);
                CreateShadowCubeMapMultiDrawIndirectCommands(set.dynamicHiResShadowMapDrawCommands.assetGeometryAlphaDiscard[shadowMapIndex][i], g_renderItemsDynamicPointLightShadows, i, light, BlendingMode::ALPHA_DISCARD);
                CreateShadowCubeMapMultiDrawIndirectCommands(set.dynamicHiResShadowMapDrawCommands.assetGeometryHair[shadowMapIndex][i], g_renderItemsDynamicPointLightShadows, i, light, BlendingMode::HAIR);
                CreateDrawCommandsSkinned(set.dynamicHiResShadowMapDrawCommands.assetGeometrySkinned[shadowMapIndex][i], g_skinnedRenderItemsDefault, -1, frustum);
                CreateDrawCommandsSkinned(set.dynamicHiResShadowMapDrawCommands.assetGeometrySkinnedAlphaDiscard[shadowMapIndex][i], g_skinnedRenderItemsAlphaDiscard, -1, frustum);
                CreateDrawCommandsSkinned(set.dynamicHiResShadowMapDrawCommands.assetGeometrySkinnedHair[shadowMapIndex][i], g_skinnedRenderItemsHair, -1, frustum);
                CreateDrawCommandsNonDeformingSkinned(set.dynamicHiResShadowMapDrawCommands.assetGeometrySkinnedNonDeforming[shadowMapIndex][i], g_skinnedNonDeformingSkinnedMeshRenderItems, -1, frustum);
                CreateDrawCommandsNonDeformingSkinned(set.dynamicHiResShadowMapDrawCommands.assetGeometrySkinnedNonDeformingAlphaDiscard[shadowMapIndex][i], g_skinnedNonDeformingSkinnedMeshRenderItemsAlphaDiscard, -1, frustum);
                CreateDrawCommandsNonDeformingSkinned(set.dynamicHiResShadowMapDrawCommands.assetGeometrySkinnedNonDeformingHair[shadowMapIndex][i], g_skinnedNonDeformingSkinnedMeshRenderItemsHair, -1, frustum);
            }
        }

        // Create shadow map draw commands for dirty low res shadow mapped lights
        for (const ShadowMapInfo& shadowMapInfo : ShadowMapManager::GetDynamicDirtyLowResShadowMaps()) {
            Light* light = Unloved::World::GetLightByObjectId(shadowMapInfo.lightId);
            if (!light) continue;
            int shadowMapIndex = shadowMapInfo.shadowMapIndex;
            if (shadowMapIndex == -1) continue;

            for (uint32_t i = 0; i < 6; i++) {
                Unloved::Frustum* frustum = light->GetFrustumByFaceIndex(i);
                if (!frustum) continue;

                CreateShadowCubeMapMultiDrawIndirectCommands(set.dynamicLowResShadowMapDrawCommands.assetGeometry[shadowMapIndex][i], g_renderItemsDynamicPointLightShadows, i, light, BlendingMode::DEFAULT);
                CreateShadowCubeMapMultiDrawIndirectCommands(set.dynamicLowResShadowMapDrawCommands.assetGeometryAlphaDiscard[shadowMapIndex][i], g_renderItemsDynamicPointLightShadows, i, light, BlendingMode::ALPHA_DISCARD);
                CreateShadowCubeMapMultiDrawIndirectCommands(set.dynamicLowResShadowMapDrawCommands.assetGeometryHair[shadowMapIndex][i], g_renderItemsDynamicPointLightShadows, i, light, BlendingMode::HAIR);
                CreateDrawCommandsSkinned(set.dynamicLowResShadowMapDrawCommands.assetGeometrySkinned[shadowMapIndex][i], g_skinnedRenderItemsDefault, -1, frustum);
                CreateDrawCommandsSkinned(set.dynamicLowResShadowMapDrawCommands.assetGeometrySkinnedAlphaDiscard[shadowMapIndex][i], g_skinnedRenderItemsAlphaDiscard, -1, frustum);
                CreateDrawCommandsSkinned(set.dynamicLowResShadowMapDrawCommands.assetGeometrySkinnedHair[shadowMapIndex][i], g_skinnedRenderItemsHair, -1, frustum);
                CreateDrawCommandsNonDeformingSkinned(set.dynamicLowResShadowMapDrawCommands.assetGeometrySkinnedNonDeforming[shadowMapIndex][i], g_skinnedNonDeformingSkinnedMeshRenderItems, -1, frustum);
                CreateDrawCommandsNonDeformingSkinned(set.dynamicLowResShadowMapDrawCommands.assetGeometrySkinnedNonDeformingAlphaDiscard[shadowMapIndex][i], g_skinnedNonDeformingSkinnedMeshRenderItemsAlphaDiscard, -1, frustum);
                CreateDrawCommandsNonDeformingSkinned(set.dynamicLowResShadowMapDrawCommands.assetGeometrySkinnedNonDeformingHair[shadowMapIndex][i], g_skinnedNonDeformingSkinnedMeshRenderItemsHair, -1, frustum);
            }
        }










    }

    void CreateDrawCommandProcedural(std::vector<DrawIndexedIndirectCommand>& drawCommands, std::vector<RenderItem>& renderItems, Unloved::Frustum* frustum, int viewportIndex) {
		// Store the instance offset for this list of commands
		int instanceStart = g_instanceData.size();

		// Preallocate an estimate
		g_instanceData.reserve(g_instanceData.size() + renderItems.size());

		// Append new render items to the global instance data
		for (const RenderItem& renderItem : renderItems) {
			// Skip culling if no frustum is bound, otherwise test intersection
			if (!frustum || frustum->IntersectsAABBFast(renderItem)) {
				g_instanceData.push_back(renderItem);
			}
		}

		// Create indirect draw commands using the stored offset
		std::span<RenderItem> instanceView(g_instanceData.begin() + instanceStart, g_instanceData.end());
		CreateHouseMultiDrawIndirectCommands(drawCommands, instanceView, viewportIndex, instanceStart);
    }

    void CreateHouseMultiDrawIndirectCommands(std::vector<DrawIndexedIndirectCommand>& commands, std::span<RenderItem> renderItems, int viewportIndex, int instanceOffset) {
        std::unordered_map<uint32_t, std::size_t> commandMap;
        commands.reserve(renderItems.size());

        MeshBuffer& meshBuffer = ResourceManager::GetMeshBuffer("Procedural");

        for (const RenderItem& renderItem : renderItems) {
            uint32_t meshId = renderItem.meshId;

            Mesh* mesh = meshBuffer.GetMeshById(meshId);
            if (!mesh) {
                instanceOffset++;
                continue;
            }

            // If the command exists, increment its instance count
            auto it = commandMap.find(meshId);
            if (it != commandMap.end()) {
                commands[it->second].instanceCount++;
            }
            // Otherwise create a new command
            else {
                std::size_t index = commands.size();
                auto& cmd = commands.emplace_back();
                cmd.indexCount = mesh->indexCount;
                cmd.firstIndex = mesh->baseIndex;
                cmd.baseVertex = mesh->baseVertex;
                cmd.baseInstance = EncodeBaseInstance(viewportIndex, instanceOffset);
                cmd.instanceCount = 1;

                commandMap[meshId] = index;
            }
            instanceOffset++;
        }
    }


    void CreateDrawCommands(std::vector<DrawIndexedIndirectCommand>& drawCommands, std::vector<RenderItem>& renderItems, Unloved::Frustum* frustum, int viewportIndex, bool ignoreNonShadowCasters) {
        // Store the instance offset for this list of commands
        int instanceStart = g_instanceData.size();

        // Preallocate an estimate
        g_instanceData.reserve(g_instanceData.size() + renderItems.size());

        // Append new render items to the global instance data
        for (const RenderItem& renderItem : renderItems) {
            bool shadowCasting = ((renderItem.shadowFlags & SHADOW_FLAG_POINT_LIGHT) != 0u);

            if (ignoreNonShadowCasters && !shadowCasting) continue;
            if (renderItem.ignoredViewportIndex != -1 && renderItem.ignoredViewportIndex == viewportIndex) continue;
            if (renderItem.exclusiveViewportIndex != -1 && renderItem.exclusiveViewportIndex != viewportIndex) continue;

            // If you supplied no frustum, then it passes no matter what
            if (!frustum) {
                g_instanceData.push_back(renderItem);
            }
            // Frustum cull it
            else if (frustum->IntersectsAABBFast(renderItem)) {
                g_instanceData.push_back(renderItem);
            }
        }

        // Create indirect draw commands using the stored offset
        std::span<RenderItem> instanceView(g_instanceData.begin() + instanceStart, g_instanceData.end());
        CreateMultiDrawIndirectCommands(drawCommands, instanceView, viewportIndex, instanceStart);
    }

    void CreateDrawCommandsSkinned(std::vector<DrawIndexedIndirectCommand>& commands, std::vector<RenderItem>& renderItems, int viewportIndex, Unloved::Frustum* frustum) {
        // Clear any commands from last frame
        commands.clear();

        // Iterate the viewports and build the draw commands
        int instanceStart = g_instanceData.size();

        // Preallocate an estimate
        g_instanceData.reserve(g_instanceData.size() + renderItems.size());

        // Append new render items to the global instance data
        for (const RenderItem& renderItem : renderItems) {
            if (renderItem.ignoredViewportIndex != -1 && renderItem.ignoredViewportIndex == viewportIndex) continue;
            if (renderItem.exclusiveViewportIndex != -1 && renderItem.exclusiveViewportIndex != viewportIndex) continue;

            // Cull the whole animated object for shadow maps
            if (frustum) {
                uint64_t objectId = 0;
                Hell::Bit::UnpackUint64(renderItem.objectIdLowerBit, renderItem.objectIdUpperBit, objectId);

                AnimatedGameObject* animatedGameObject = World::GetAnimatedGameObjectByObjectId(objectId);
                if (!animatedGameObject) continue;
                if (!frustum->IntersectsAABBFast(animatedGameObject->GetSkinnedAABB())) continue;
            }

            g_instanceData.push_back(renderItem);
        }

        // Create indirect draw commands using the stored offset
        std::span<RenderItem> instanceView(g_instanceData.begin() + instanceStart, g_instanceData.end());
        CreateMultiDrawIndirectCommandsSkinned(commands, instanceView, viewportIndex, instanceStart);
    }

    void CreateDrawCommandsNonDeformingSkinned(std::vector<DrawIndexedIndirectCommand>& commands, std::vector<RenderItem>& renderItems, int viewportIndex, Unloved::Frustum* frustum) {
        // Clear any commands from last frame
        commands.clear();

        // Iterate the viewports and build the draw commands
        int instanceStart = g_instanceData.size();

        // Preallocate an estimate
        g_instanceData.reserve(g_instanceData.size() + renderItems.size());

        // Append new render items to the global instance data
        for (const RenderItem& renderItem : renderItems) {
            if (renderItem.ignoredViewportIndex != -1 && renderItem.ignoredViewportIndex == viewportIndex) continue;
            if (renderItem.exclusiveViewportIndex != -1 && renderItem.exclusiveViewportIndex != viewportIndex) continue;

            // Cull the whole animated object for shadow maps
            if (frustum) {
                uint64_t objectId = 0;
                Hell::Bit::UnpackUint64(renderItem.objectIdLowerBit, renderItem.objectIdUpperBit, objectId);

                AnimatedGameObject* animatedGameObject = World::GetAnimatedGameObjectByObjectId(objectId);
                if (!animatedGameObject) continue;
                if (!frustum->IntersectsAABBFast(animatedGameObject->GetSkinnedAABB())) continue;
            }

            g_instanceData.push_back(renderItem);
        }

        // Create indirect draw commands using the stored offset
        std::span<RenderItem> instanceView(g_instanceData.begin() + instanceStart, g_instanceData.end());
        CreateMultiDrawIndirectCommandsSkinnedNonDeforming(commands, instanceView, viewportIndex, instanceStart);
    }

    void CreateShadowCubeMapMultiDrawIndirectCommands(std::vector<DrawIndexedIndirectCommand>& drawCommands, std::vector<RenderItem>& renderItems, uint32_t faceIndex, Light* light, BlendingMode blendingModeFilter) {
        drawCommands.clear();

        if (!light) return;

        // Get face frustum, bail if invalid
        Unloved::Frustum* frustum = light->GetFrustumByFaceIndex(faceIndex);
        if (!frustum) return;

        // Store the instance offset for this player
        int instanceStart = g_instanceData.size();

        // Preallocate an estimate
        g_instanceData.reserve(g_instanceData.size() + renderItems.size());

        // Append new render items to the global instance data if it's within light frustum
        // renderItems is already sorted by this point
        // but if anything breaks, check here! (maybe you re-ordered things)
        for (const RenderItem& renderItem : renderItems) {
            if ((renderItem.shadowFlags & SHADOW_FLAG_POINT_LIGHT) == 0u) continue;

            if ((BlendingMode)renderItem.blendingMode != blendingModeFilter) {
                continue;
            }

            if (frustum->IntersectsAABBFast(renderItem)) {
                g_instanceData.push_back(renderItem);
            }
        }

        // Create indirect draw commands using the stored offset
        std::span<RenderItem> instanceView(g_instanceData.begin() + instanceStart, g_instanceData.end());
        CreateMultiDrawIndirectCommands(drawCommands, instanceView, -1, instanceStart);
    }

    void CreateMultiDrawIndirectCommands(std::vector<DrawIndexedIndirectCommand>& commands, std::span<RenderItem> renderItems, int viewportIndex, int instanceOffset) {
        std::unordered_map<uint32_t, std::size_t> commandMap;
        Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("AssetGeometry");
        commands.reserve(renderItems.size());

        for (const RenderItem& renderItem : renderItems) {
            uint32_t meshId = renderItem.meshId;
            Mesh* mesh = meshBuffer.GetMeshById(meshId);
            if (!mesh) {
                instanceOffset++;
                continue;
            }

            // If the command exists, increment its instance count
            auto it = commandMap.find(meshId);
            if (it != commandMap.end()) {
                commands[it->second].instanceCount++;
            }
            // Otherwise create a new command
            else {
                std::size_t index = commands.size();
                auto& cmd = commands.emplace_back();
                cmd.indexCount = mesh->indexCount;
                cmd.firstIndex = mesh->baseIndex;
                cmd.baseVertex = mesh->baseVertex;
                cmd.baseInstance = EncodeBaseInstance(viewportIndex, instanceOffset);
                cmd.instanceCount = 1;

                commandMap[meshId] = index;
            }
            instanceOffset++;
        }
    }

    void CreateMultiDrawIndirectCommandsSkinned(std::vector<DrawIndexedIndirectCommand>& commands, std::span<RenderItem> renderItems, int viewportIndex, int instanceOffset) {
        commands.reserve(renderItems.size());
        Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("AssetGeometry");

        for (const RenderItem& renderItem : renderItems) {
            Mesh* mesh = meshBuffer.GetMeshById(renderItem.meshId);
            if (!mesh) {
                instanceOffset++;
                continue;
            }
            std::size_t index = commands.size();
            auto& cmd = commands.emplace_back();
            cmd.indexCount = mesh->indexCount;
            cmd.firstIndex = mesh->baseIndex;
            cmd.baseVertex = renderItem.baseVertex;
            cmd.baseInstance = EncodeBaseInstance(viewportIndex, instanceOffset);
            cmd.instanceCount = 1;
            instanceOffset++;
        }
    }

    void CreateMultiDrawIndirectCommandsSkinnedNonDeforming(std::vector<DrawIndexedIndirectCommand>& commands, std::span<RenderItem> renderItems, int viewportIndex, int instanceOffset) {
        commands.reserve(renderItems.size());
        Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("AssetGeometry");

        for (const RenderItem& renderItem : renderItems) {
            Mesh* mesh = meshBuffer.GetMeshById(renderItem.meshId);
            if (!mesh) {
                instanceOffset++;
                continue;
            }
            std::size_t index = commands.size();
            auto& cmd = commands.emplace_back();
            cmd.indexCount = mesh->indexCount;
            cmd.firstIndex = mesh->baseIndex;
            cmd.baseVertex = mesh->baseVertex;
            cmd.baseInstance = EncodeBaseInstance(viewportIndex, instanceOffset);
            cmd.instanceCount = 1;
            instanceOffset++;
        }
    }

    void CreateSkinningData() {
        auto& set = g_drawCommandsSet;

        // Sort render items by mesh index
        SortRenderItems(g_skinnedRenderItemsDefault);
        SortRenderItems(g_skinnedRenderItemsAlphaDiscard);
        SortRenderItems(g_skinnedRenderItemsBlended);
        SortRenderItems(g_skinnedRenderItemsHair);

        SortRenderItems(g_skinnedNonDeformingSkinnedMeshRenderItems);
        SortRenderItems(g_skinnedNonDeformingSkinnedMeshRenderItemsAlphaDiscard);
        SortRenderItems(g_skinnedNonDeformingSkinnedMeshRenderItemsBlended);
        SortRenderItems(g_skinnedNonDeformingSkinnedMeshRenderItemsHair);

        // Create the per viewport draw commands
        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            CreateDrawCommandsSkinned(set.skinnedStandard[i], g_skinnedRenderItemsDefault, i);
            CreateDrawCommandsSkinned(set.skinnedAlphaDiscard[i], g_skinnedRenderItemsAlphaDiscard, i);
            CreateDrawCommandsSkinned(set.skinnedBlended[i], g_skinnedRenderItemsBlended, i);
            CreateDrawCommandsSkinned(set.skinnedHair[i], g_skinnedRenderItemsHair, i);
        }

        // Combine all into a single vector for the compute skinning pass
        g_combinedSkinnedRenderItems.clear();
        g_combinedSkinnedRenderItems.insert(g_combinedSkinnedRenderItems.end(), g_skinnedRenderItemsDefault.begin(), g_skinnedRenderItemsDefault.end());
        g_combinedSkinnedRenderItems.insert(g_combinedSkinnedRenderItems.end(), g_skinnedRenderItemsAlphaDiscard.begin(), g_skinnedRenderItemsAlphaDiscard.end());
        g_combinedSkinnedRenderItems.insert(g_combinedSkinnedRenderItems.end(), g_skinnedRenderItemsBlended.begin(), g_skinnedRenderItemsBlended.end());
        g_combinedSkinnedRenderItems.insert(g_combinedSkinnedRenderItems.end(), g_skinnedRenderItemsHair.begin(), g_skinnedRenderItemsHair.end());

        // Gather all non deforming render items
        //for (AnimatedGameObject& animatedGameObject : LegacyWorld::GetAnimatedGameObjects()) {
		//	if (animatedGameObject.RenderingEnabled()) {
		//		g_skinnedNonDeformingSkinnedMeshRenderItems.insert(g_skinnedNonDeformingSkinnedMeshRenderItems.end(), animatedGameObject.GetNonDeformingRenderItems().begin(), animatedGameObject.GetNonDeformingRenderItems().end());
        //        g_nonDeformingSkinnedMeshRenderItemsDepthPeeledTransparent.insert(g_nonDeformingSkinnedMeshRenderItemsDepthPeeledTransparent.end(), animatedGameObject.GetNonDeformingRenderItemsDepthPeeledTransparent().begin(), animatedGameObject.GetNonDeformingRenderItemsDepthPeeledTransparent().end());
        //    }
        //}

        // Sort by mesh index

        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            CreateDrawCommandsNonDeformingSkinned(set.skinnedNonDeformingAlphaDiscard[i], g_skinnedNonDeformingSkinnedMeshRenderItemsAlphaDiscard, i);
            CreateDrawCommandsNonDeformingSkinned(set.skinnedNonDeformingBlended[i], g_skinnedNonDeformingSkinnedMeshRenderItemsBlended, i);
            CreateDrawCommandsNonDeformingSkinned(set.skinnedNonDeformingStandard[i], g_skinnedNonDeformingSkinnedMeshRenderItems, i);
            CreateDrawCommandsNonDeformingSkinned(set.skinnedNonDeformingHair[i], g_skinnedNonDeformingSkinnedMeshRenderItemsHair, i);
        }
    }

    void UpdateOceanPatchTransforms() {
        g_oceanPatchTransforms.clear();

        g_oceanPatchTransforms.push_back(glm::mat4(1.0f));

        return;
        OpenGLMeshPatch* oceanMeshPatch = OpenGL::Renderer::GetOceanMeshPatch();

        static bool test = false;
        static bool swap = false;

        if (Input::KeyPressed(HELL_KEY_8)) {
            test = !test;
        }
        if (Input::KeyPressed(HELL_KEY_0)) {
            swap = !swap;
        }

        float scale = 0.05;

        float patchOffset = Ocean::GetBaseFFTResolution().y * scale;

        Hell::Transform tesseleationTransform;
        tesseleationTransform.scale = glm::vec3(scale);

        int min = -20;
        int max = 20;
        float offset = (max - min) * Ocean::GetBaseFFTResolution().x * scale;

        if (test) {
            min = 0;
            max = 1;
            offset = Ocean::GetBaseFFTResolution().x * scale;
        }


        for (int i = 0; i < 1; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            Unloved::Frustum& frustum = viewport->GetFrustum();

            for (int x = min; x < max; x++) {
                for (int z = min; z < max; z++) {
                    tesseleationTransform.position = glm::vec3(patchOffset * x, Ocean::GetOceanOriginY(), patchOffset * z);
                    if (swap) {
                        tesseleationTransform.position += glm::vec3(offset, 0.0f, 0.0f);
                    }

                    float threshold = 1.0f;
                    glm::vec3 aabbMin = tesseleationTransform.position - glm::vec3(0, threshold / 2, 0);
                    glm::vec3 aabbMax = tesseleationTransform.position + glm::vec3(patchOffset, threshold / 2, patchOffset);
                    AABB aabb(aabbMin, aabbMax);
                    //DrawAABB(aabb, BLUE);

                    if (frustum.IntersectsAABB(aabb)) {
                        g_oceanPatchTransforms.push_back(tesseleationTransform.to_mat4());
                    }
                }
            }
        }

        // ALL THIS WORKS BUT U COMMENTED IT OUT DURING THE START OF YOUR PORT OF THE NEW OCEAN CODE
        // ALL THIS WORKS BUT U COMMENTED IT OUT DURING THE START OF YOUR PORT OF THE NEW OCEAN CODE
        // ALL THIS WORKS BUT U COMMENTED IT OUT DURING THE START OF YOUR PORT OF THE NEW OCEAN CODE


        // Offset water origin when in heightmap editor
        //glm::vec3 originOffset = glm::vec3(0.0f);
        //if (Editor::IsOpen() && Editor::GetEditorMode() == EditorMode::MAP_HEIGHT_EDITOR) {
        //    originOffset = glm::vec3(64.0f, 0.0f, 64.0f);
        //}
        //
        //const float waterHeight = Ocean::GetWaterHeight();
        //int patchCount = 16;
        //float scale = 0.03125f;
        //float patchOffset = Ocean::GetOceanSize().y * scale;
        //
        //Transform patchTransform;
        //patchTransform.scale = glm::vec3(scale);
        //
        //g_oceanPatchTransforms.clear();
        //
        //Viewport* viewport = ViewportManager::GetViewportByIndex(0);
        //Frustum& frustum = viewport->GetFrustum();
        //
        //for (int x = 0; x < patchCount; x++) {
        //    for (int z = 0; z < patchCount; z++) {
        //        patchTransform.position = glm::vec3(patchOffset * x, waterHeight, patchOffset * z);
        //        patchTransform.position += originOffset;
        //
        //        float threshold = 0.25f;
        //        glm::vec3 aabbMin = patchTransform.position;
        //        glm::vec3 aabbMax = aabbMin;
        //        aabbMin.x += Ocean::GetOceanSize().x * scale;
        //        aabbMin.z += Ocean::GetOceanSize().y * scale;
        //        aabbMin.y -= threshold;
        //        aabbMax.y += threshold;
        //        AABB aabb(aabbMin, aabbMax);
        //
        //        if (frustum.IntersectsAABB(aabb)) {
        //            g_oceanPatchTransforms.push_back(patchTransform.to_mat4());
        //        }
        //    }
        //}
    }

    int EncodeBaseInstance(int playerIndex, int instanceOffset) {
        return (playerIndex << VIEWPORT_INDEX_SHIFT) | instanceOffset;
    }

    void DecodeBaseInstance(int baseInstance, int& playerIndex, int& instanceOffset) {
        playerIndex = baseInstance >> VIEWPORT_INDEX_SHIFT;
        instanceOffset = baseInstance & ((1 << VIEWPORT_INDEX_SHIFT) - 1);
    }

    const RendererData& GetRendererData() {
        return g_rendererData;
    }

    const std::vector<ViewportData>& GetViewportData() {
        return g_viewportData;
    }

    const std::vector<RenderItem>& GetRenderItems()             { return g_renderItems; }
    const std::vector<RenderItem>& GetRenderItemsAlphaDiscard() { return g_renderItemsAlphaDiscarded; }
    const std::vector<RenderItem>& GetRenderItemsBlended()      { return g_renderItemsBlended; }
    const std::vector<RenderItem>& GetRenderItemsGlass()        { return g_renderItemsGlass; }
    const std::vector<RenderItem>& GetRenderItemsHair()         { return g_renderItemsHair; }
    const std::vector<RenderItem>& GetRenderItemsMirror()       { return g_renderItemsMirror; }
    const std::vector<RenderItem>& GetRenderItemsOutline()      { return g_renderItemsOutline; }
    const std::vector<RenderItem>& GetRenderItemsPlastic()      { return g_renderItemsPlastic; }
    const std::vector<RenderItem>& GetRenderItemsProcedural()   { return g_renderItemsProcedural; }
    const std::vector<RenderItem>& GetRenderItemsStainedGlass() { return g_renderItemsStainedGlass; }
    const std::vector<RenderItem>& GetRenderItemsToiletWater() { return g_renderItemsToiletWater; }
    // const std::vector<RenderItem>& GetRenderItemsDynamicPointLightShadows() { return g_renderItemsDynamicPointLightShadows; }
    // const std::vector<RenderItem>& GetRenderItemsStaticPointLightShadows()  { return g_renderItemsStaticPointLightShadows; }

    const std::vector<RenderItem>& GetSkinnedRenderItemsAlphaDiscard() { return g_skinnedRenderItemsAlphaDiscard; }
    const std::vector<RenderItem>& GetSkinnedRenderItemsBlended()      { return g_skinnedRenderItemsBlended; }
    const std::vector<RenderItem>& GetSkinnedRenderItemsDefault()      { return g_skinnedRenderItemsDefault; }
    const std::vector<RenderItem>& GetSkinnedRenderItemsHair()         { return g_skinnedRenderItemsHair; }

    const std::vector<RenderItem>& GetCombinedSkinnedRenderItems() {
        return g_combinedSkinnedRenderItems;
    }

    uint32_t GetRequiredSkinnedVertexCount() {
        return g_baseSkinnedVertex;
    }


    const std::vector<RenderItem>& GetInstanceData() {
        return g_instanceData;
    }

    const DrawCommandsSet& GetDrawInfoSet() {
        return g_drawCommandsSet;
    }

    const FlashLightShadowMapDrawInfo& GetFlashLightShadowMapDrawInfo() {
        return g_flashLightShadowMapDrawInfo;
    }

    const std::vector<GPULight>& GetGPULights() {
        return g_gpuLights;
    }

    const std::vector<glm::mat4>& GetOceanPatchTransforms() {
        return g_oceanPatchTransforms;
    }

    const std::vector<glm::mat4>& GetSkinningTransforms() {
        return g_skinningTransforms;
    }

    const std::vector<DecalPaintingInfo>& GetDecalPaintingInfo() {
        return g_decalPaintingInfo;
    }

    const std::vector<BloodDecalInstanceData>& GetBloodScreenSpaceDecalInstanceData() {
        return g_bloodScreenSpaceDecalInstances;
    }

    // Submissions
    void CreateGPULights() {
        g_gpuLights.clear();

        // todo: remove me when u can
        int lightIndex = 0;

        for (Light& light : World::GetLights()) {
            GPULight& gpuLight = g_gpuLights.emplace_back();
            gpuLight.colorR = light.GetColor().r;
            gpuLight.colorG = light.GetColor().g;
            gpuLight.colorB = light.GetColor().b;
            gpuLight.posX = light.GetPosition().x;
            gpuLight.posY = light.GetPosition().y;
            gpuLight.posZ = light.GetPosition().z;
            gpuLight.radius = light.GetRadius();
            gpuLight.strength = light.GetStrength();
            gpuLight.shadowMapDirty = true;
            gpuLight.lightIndex = lightIndex;
            gpuLight.isDirtyForRaytracing = light.IsDirtyForRaytracing();
            gpuLight.hiResShadowMapIndex = ShadowMapManager::GetHiResShadowMapIndex(light.GetObjectId());
            gpuLight.lowResShadowMapIndex = ShadowMapManager::GetLowResShadowMapIndex(light.GetObjectId());

            // Pack in the light ID
            Hell::Bit::PackUint64(light.GetObjectId(), gpuLight.lightIdLowerBit, gpuLight.lightIdUpperBit);

            // todo: remove me when u can
            lightIndex++;

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

	void SubmitRenderItemProcedural(const RenderItem& renderItem) {
		g_renderItemsProcedural.push_back(renderItem);

        if (Hell::BackEnd::GetAPI() == API::VULKAN && (renderItem.vulkanFlags & VULKAN_FLAG_EXCLUDE_FROM_TLAS) == 0u) {
            Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("Procedural");
            Mesh* mesh = meshBuffer.GetMeshById(renderItem.meshId);
            uint64_t vulkanMeshBufferId = meshBuffer.GetVulkanId();
            VulkanMeshBuffer* vulkanMeshBuffer = vulkanMeshBufferId != 0 && VulkanResourceManager::MeshBufferExists(vulkanMeshBufferId) ? VulkanResourceManager::GetMeshBuffer(vulkanMeshBufferId) : nullptr;
            uint64_t vertexBufferDeviceAddress = vulkanMeshBuffer ? vulkanMeshBuffer->GetVertexBufferAddress() : 0;
            uint64_t indexBufferDeviceAddress = vulkanMeshBuffer ? vulkanMeshBuffer->GetIndexBufferAddress() : 0;

            if (mesh && mesh->vulkanBlasId != 0 && vertexBufferDeviceAddress != 0 && indexBufferDeviceAddress != 0 && mesh->vertexCount != 0 && mesh->indexCount >= 3) {
                StaticRayTracingInstance& instance = g_staticRayTracingInstances.emplace_back();
                instance.vulkanBlasId = mesh->vulkanBlasId;
                instance.vertexBufferDeviceAddress = vertexBufferDeviceAddress;
                instance.indexBufferDeviceAddress = indexBufferDeviceAddress;
                instance.modelMatrix = glm::mat4(1.0f);
                instance.range.baseVertex = renderItem.baseVertex;
                instance.range.baseIndex = renderItem.baseIndex;
                instance.range.vertexCount = mesh->vertexCount;
                instance.range.indexCount = mesh->indexCount;
                instance.range.blendingMode = renderItem.blendingMode;
                instance.range.materialIndex = renderItem.materialIndex;
                instance.range.shadowBit = renderItem.shadowFlags;
            }
        }
	}

    void SubmitRenderItemsMirror(const std::vector<RenderItem>& renderItems) {
        g_renderItemsMirror.insert(g_renderItemsMirror.begin(), renderItems.begin(), renderItems.end());
    }

    void SubmitDecalPaintingInfo(DecalPaintingInfo decalPaintingInfo) {
        g_decalPaintingInfo.push_back(decalPaintingInfo);
    }

    void SubmitSkinnedRenderItems(const std::vector<RenderItem>& renderItems) {
        g_skinnedRenderItemsDefault.insert(g_skinnedRenderItemsDefault.begin(), renderItems.begin(), renderItems.end());
    }


    //void PrepareAnimatedMeshNodesForVulkan(const AnimatedMeshNodes& animatedMeshNodes) {
    //    if (!animatedMeshNodes.RenderingEnabled()) return;
    //
    //    // Get parent
    //    AnimatedGameObject* animatedGameObject = Unloved::World::GetAnimatedGameObjectByObjectId(animatedMeshNodes.m_parentId);
    //    if (!animatedGameObject) return;
    //
    //    // Cache the base indices before they're mutated
    //    uint32_t baseSkinningTransformIndex = g_skinningTransforms.size();
    //
    //    // Append skinning matrices to global array
    //    g_skinningTransforms.insert(g_skinningTransforms.end(), animatedGameObject->GetBoneSkinningMatrices().begin(), animatedGameObject->GetBoneSkinningMatrices().end());
    //
    //    // Ray query group (Vulkan ONLY)
    //    SkinnedRayTracingGroup& group = g_skinnedRayTracingGroups.emplace_back();
    //    group.modelMatrix = animatedGameObject->GetModelMatrix();
    //
    //    for (const AnimatedMeshNode& node : animatedMeshNodes.GetNodes()) {
    //        RenderItem renderItem = node.renderItem;
    //        if (node.blendingMode == BlendingMode::DO_NOT_RENDER) continue;
    //
    //        if (node.excludeFromVulkanTLAS) {
    //            renderItem.vulkanFlags |= VULKAN_FLAG_EXCLUDE_FROM_TLAS;
    //        }
    //
    //        // Deforming
    //        if (node.deforming) {
    //
    //            // Vulkan raytracing instance
    //            if (Hell::BackEnd::GetAPI() == API::VULKAN && !Hell::Bit::Contains(renderItem.vulkanFlags, VULKAN_FLAG_EXCLUDE_FROM_TLAS)) {
    //                RayTracingGeometryRange& range = group.ranges.emplace_back();
    //                range.baseVertex = g_baseSkinnedVertex;
    //                range.baseIndex = node.baseIndex;
    //                range.vertexCount = node.vertexCount;
    //                range.indexCount = node.indexCount;
    //                range.blendingMode = static_cast<uint32_t>(node.blendingMode);
    //                range.materialIndex = node.materialIndex;
    //                range.shadowBit = renderItem.shadowFlags;
    //            }
    //
    //            // Assign a new base vertex to be used by compute skinning
    //            renderItem.baseSkinningTransformIndex = baseSkinningTransformIndex;
    //            renderItem.baseVertex = g_baseSkinnedVertex;
    //
    //            g_baseSkinnedVertex += renderItem.vertexCount;
    //
    //            switch (node.blendingMode) {
    //                case BlendingMode::DEFAULT:       g_skinnedRenderItemsDefault.push_back(renderItem);      break;
    //                case BlendingMode::ALPHA_DISCARD: g_skinnedRenderItemsAlphaDiscard.push_back(renderItem); break;
    //                case BlendingMode::BLENDED:       g_skinnedRenderItemsBlended.push_back(renderItem);      break;
    //                case BlendingMode::HAIR:          g_skinnedRenderItemsHair.push_back(renderItem);         break;
    //                default: break;
    //            }
    //        }
    //        // Non deforming
    //        else {
    //            // Vulkan raytracing instance
    //            if (Hell::BackEnd::GetAPI() == API::VULKAN && !Hell::Bit::Contains(renderItem.vulkanFlags, VULKAN_FLAG_EXCLUDE_FROM_TLAS)) {
    //                Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("AssetGeometry");
    //                Mesh* mesh = meshBuffer.GetMeshById(renderItem.meshId);
    //                uint64_t vulkanMeshBufferId = meshBuffer.GetVulkanId();
    //                VulkanMeshBuffer* vulkanMeshBuffer = vulkanMeshBufferId != 0 && VulkanResourceManager::MeshBufferExists(vulkanMeshBufferId) ? VulkanResourceManager::GetMeshBuffer(vulkanMeshBufferId) : nullptr;
    //                uint64_t vertexBufferDeviceAddress = vulkanMeshBuffer ? vulkanMeshBuffer->GetVertexBufferAddress() : 0;
    //                uint64_t indexBufferDeviceAddress = vulkanMeshBuffer ? vulkanMeshBuffer->GetIndexBufferAddress() : 0;
    //
    //                if (mesh && mesh->vulkanBlasId != 0 && vertexBufferDeviceAddress != 0 && indexBufferDeviceAddress != 0 && mesh->vertexCount != 0 && mesh->indexCount >= 3) {
    //                    StaticRayTracingInstance& instance = g_staticRayTracingInstances.emplace_back();
    //                    instance.vulkanBlasId = mesh->vulkanBlasId;
    //                    instance.vertexBufferDeviceAddress = vertexBufferDeviceAddress;
    //                    instance.indexBufferDeviceAddress = indexBufferDeviceAddress;
    //                    instance.modelMatrix = renderItem.modelMatrix;
    //                    instance.range.baseVertex = renderItem.baseVertex;
    //                    instance.range.baseIndex = renderItem.baseIndex;
    //                    instance.range.vertexCount = mesh->vertexCount;
    //                    instance.range.indexCount = mesh->indexCount;
    //                    instance.range.blendingMode = renderItem.blendingMode;
    //                    instance.range.materialIndex = renderItem.materialIndex;
    //                    instance.range.shadowBit = renderItem.shadowFlags;
    //                }
    //            }
    //
    //            switch (node.blendingMode) {
    //                case BlendingMode::ALPHA_DISCARD: g_skinnedNonDeformingSkinnedMeshRenderItemsAlphaDiscard.push_back(renderItem); break;
    //                case BlendingMode::BLENDED:       g_skinnedNonDeformingSkinnedMeshRenderItemsBlended.push_back(renderItem);      break;
    //                case BlendingMode::DEFAULT:       g_skinnedNonDeformingSkinnedMeshRenderItems.push_back(renderItem);             break;
    //                case BlendingMode::HAIR:          g_skinnedNonDeformingSkinnedMeshRenderItemsHair.push_back(renderItem);         break;
    //                default: break;
    //            }
    //        }
    //    }
    //
    //    if (group.ranges.empty()) {
    //        g_skinnedRayTracingGroups.pop_back();
    //    }
    //}

    void SubmitAnimatedMeshNodes(const AnimatedMeshNodes& animatedMeshNodes) {
        if (!animatedMeshNodes.RenderingEnabled()) return;

        // Get parent
        AnimatedGameObject* animatedGameObject = Unloved::World::GetAnimatedGameObjectByObjectId(animatedMeshNodes.m_parentId);
        if (!animatedGameObject) return;

        // Cache the base indices before they're mutated
        uint32_t baseSkinningTransformIndex = g_skinningTransforms.size();

        // Append skinning matrices to global array
        g_skinningTransforms.insert(g_skinningTransforms.end(), animatedGameObject->GetBoneSkinningMatrices().begin(), animatedGameObject->GetBoneSkinningMatrices().end());

        // Ray query group (Vulkan ONLY)
        SkinnedRayTracingGroup& group = g_skinnedRayTracingGroups.emplace_back();
        group.modelMatrix = animatedGameObject->GetModelMatrix();

        for (const AnimatedMeshNode& node : animatedMeshNodes.GetNodes()) {
            RenderItem renderItem = node.renderItem;
            if (node.blendingMode == BlendingMode::DO_NOT_RENDER) continue;

            if (node.excludeFromVulkanTLAS) {
                renderItem.vulkanFlags |= VULKAN_FLAG_EXCLUDE_FROM_TLAS;
            }

            // Deforming
            if (node.deforming) {

                // Vulkan raytracing instance
                if (Hell::BackEnd::GetAPI() == API::VULKAN && !Hell::Bit::Contains(renderItem.vulkanFlags, VULKAN_FLAG_EXCLUDE_FROM_TLAS)) {
                    RayTracingGeometryRange& range = group.ranges.emplace_back();
                    range.baseVertex = g_baseSkinnedVertex;
                    range.baseIndex = node.baseIndex;
                    range.vertexCount = node.vertexCount;
                    range.indexCount = node.indexCount;
                    range.blendingMode = static_cast<uint32_t>(node.blendingMode);
                    range.materialIndex = node.materialIndex;
                    range.shadowBit = renderItem.shadowFlags;
                }

                SkinningJob& skinningJob = g_skinningJobs.emplace_back();
                skinningJob.baseVertex = node.baseVertex;
                skinningJob.baseIndex = node.baseIndex;
                skinningJob.vertexCount = node.vertexCount;
                skinningJob.indexCount = node.baseIndex;
                skinningJob.baseSkinningVertex = g_baseSkinnedVertex;
                skinningJob.baseSkinningTransformIndex = baseSkinningTransformIndex;
                skinningJob.baseVertexWeight = node.baseVertexWeight;

                // Assign a new base vertex to be used by compute skinning
                renderItem.baseSkinningTransformIndex = baseSkinningTransformIndex;
                renderItem.baseVertex = g_baseSkinnedVertex;

                g_baseSkinnedVertex += renderItem.vertexCount;

                switch (node.blendingMode) {
                case BlendingMode::DEFAULT:       g_skinnedRenderItemsDefault.push_back(renderItem);      break;
                case BlendingMode::ALPHA_DISCARD: g_skinnedRenderItemsAlphaDiscard.push_back(renderItem); break;
                case BlendingMode::BLENDED:       g_skinnedRenderItemsBlended.push_back(renderItem);      break;
                case BlendingMode::HAIR:          g_skinnedRenderItemsHair.push_back(renderItem);         break;
                default: break;
                }
            }
            // Non deforming
            else {
                // Vulkan raytracing instance
                if (Hell::BackEnd::GetAPI() == API::VULKAN && !Hell::Bit::Contains(renderItem.vulkanFlags, VULKAN_FLAG_EXCLUDE_FROM_TLAS)) {
                    Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("AssetGeometry");
                    Mesh* mesh = meshBuffer.GetMeshById(renderItem.meshId);
                    uint64_t vulkanMeshBufferId = meshBuffer.GetVulkanId();
                    VulkanMeshBuffer* vulkanMeshBuffer = vulkanMeshBufferId != 0 && VulkanResourceManager::MeshBufferExists(vulkanMeshBufferId) ? VulkanResourceManager::GetMeshBuffer(vulkanMeshBufferId) : nullptr;
                    uint64_t vertexBufferDeviceAddress = vulkanMeshBuffer ? vulkanMeshBuffer->GetVertexBufferAddress() : 0;
                    uint64_t indexBufferDeviceAddress = vulkanMeshBuffer ? vulkanMeshBuffer->GetIndexBufferAddress() : 0;

                    if (mesh && mesh->vulkanBlasId != 0 && vertexBufferDeviceAddress != 0 && indexBufferDeviceAddress != 0 && mesh->vertexCount != 0 && mesh->indexCount >= 3) {
                        StaticRayTracingInstance& instance = g_staticRayTracingInstances.emplace_back();
                        instance.vulkanBlasId = mesh->vulkanBlasId;
                        instance.vertexBufferDeviceAddress = vertexBufferDeviceAddress;
                        instance.indexBufferDeviceAddress = indexBufferDeviceAddress;
                        instance.modelMatrix = renderItem.modelMatrix;
                        instance.range.baseVertex = renderItem.baseVertex;
                        instance.range.baseIndex = renderItem.baseIndex;
                        instance.range.vertexCount = mesh->vertexCount;
                        instance.range.indexCount = mesh->indexCount;
                        instance.range.blendingMode = renderItem.blendingMode;
                        instance.range.materialIndex = renderItem.materialIndex;
                        instance.range.shadowBit = renderItem.shadowFlags;
                    }
                }

                switch (node.blendingMode) {
                case BlendingMode::ALPHA_DISCARD: g_skinnedNonDeformingSkinnedMeshRenderItemsAlphaDiscard.push_back(renderItem); break;
                case BlendingMode::BLENDED:       g_skinnedNonDeformingSkinnedMeshRenderItemsBlended.push_back(renderItem);      break;
                case BlendingMode::DEFAULT:       g_skinnedNonDeformingSkinnedMeshRenderItems.push_back(renderItem);             break;
                case BlendingMode::HAIR:          g_skinnedNonDeformingSkinnedMeshRenderItemsHair.push_back(renderItem);         break;
                default: break;
                }
            }
        }

        if (group.ranges.empty()) {
            g_skinnedRayTracingGroups.pop_back();
        }
    }

    void SubmitMeshNodes(const MeshNodes& meshNodes) {
        for (const MeshNode& node : meshNodes.GetNodes()) {
            RenderItem renderItem = node.renderItem;
            if (node.excludeFromVulkanTLAS) {
                renderItem.vulkanFlags |= VULKAN_FLAG_EXCLUDE_FROM_TLAS;
            }
            SubmitRenderItem(renderItem);
        }
    }

    void SubmitRenderItem(const RenderItem& renderItem) {
        BlendingMode blendingMode = (BlendingMode)renderItem.blendingMode;

        if (blendingMode == BlendingMode::DO_NOT_RENDER) return;

        switch (blendingMode) {
        case BlendingMode::DEFAULT:       g_renderItems.push_back(renderItem); break;
        case BlendingMode::ALPHA_DISCARD: g_renderItemsAlphaDiscarded.push_back(renderItem); break;
        case BlendingMode::BLENDED:       g_renderItemsBlended.push_back(renderItem); break;
        case BlendingMode::GLASS:         g_renderItemsGlass.push_back(renderItem); break;
        case BlendingMode::HAIR:          g_renderItemsHair.push_back(renderItem); break;
        case BlendingMode::MIRROR:        g_renderItemsMirror.push_back(renderItem); break;
        case BlendingMode::TOILET_WATER:  g_renderItemsToiletWater.push_back(renderItem); break;
        case BlendingMode::STAINED_GLASS: g_renderItemsStainedGlass.push_back(renderItem); break;
        case BlendingMode::PLASTIC:       g_renderItemsPlastic.push_back(renderItem); break;
        default: break;
        }

        uint64_t objectId = 0;
        Hell::Bit::UnpackUint64(renderItem.objectIdLowerBit, renderItem.objectIdUpperBit, objectId);

        // Emissive
        if (Material* material = ResourceManager::GetMaterialByIndex(renderItem.materialIndex)) {
            bool hasEmissiveMap = material->m_emissive != g_blackTextureIndex;
            bool hasEmissiveColor = renderItem.emissiveR != 0.0f || renderItem.emissiveG != 0.0f || renderItem.emissiveB != 0.0f;

            if (hasEmissiveMap || hasEmissiveColor) {
                g_renderItemsEmissive.push_back(renderItem);
            }
        }

        // Outline
        if (objectId != 0 && objectId == Editor::GetSelectedObjectId()) g_renderItemsOutline.push_back(renderItem);

        // Shadow Casting
        if ((renderItem.shadowFlags & SHADOW_FLAG_POINT_LIGHT) != 0u) {

            bool isDynamic = Hell::Bit::Contains(renderItem.miscFlags, MISC_FLAG_DYNAMIC_OBJECT);

            if (isDynamic) {
                g_renderItemsDynamicPointLightShadows.push_back(renderItem);
            }
            else {
                g_renderItemsStaticPointLightShadows.push_back(renderItem);
            }
        }

        if ((renderItem.shadowFlags & SHADOW_FLAG_CSM)  != 0u) g_renderItemsMoonLightShadows.push_back(renderItem);

        // Vulkan raytracing instance
        if (Hell::BackEnd::GetAPI() == API::VULKAN && (renderItem.vulkanFlags & VULKAN_FLAG_EXCLUDE_FROM_TLAS) == 0u) {
            Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("AssetGeometry");
            Mesh* mesh = meshBuffer.GetMeshById(renderItem.meshId);
            uint64_t vulkanMeshBufferId = meshBuffer.GetVulkanId();
            VulkanMeshBuffer* vulkanMeshBuffer = vulkanMeshBufferId != 0 && VulkanResourceManager::MeshBufferExists(vulkanMeshBufferId) ? VulkanResourceManager::GetMeshBuffer(vulkanMeshBufferId) : nullptr;
            uint64_t vertexBufferDeviceAddress = vulkanMeshBuffer ? vulkanMeshBuffer->GetVertexBufferAddress() : 0;
            uint64_t indexBufferDeviceAddress = vulkanMeshBuffer ? vulkanMeshBuffer->GetIndexBufferAddress() : 0;

            if (mesh && mesh->vulkanBlasId != 0 && vertexBufferDeviceAddress != 0 && indexBufferDeviceAddress != 0 && mesh->vertexCount != 0 && mesh->indexCount >= 3) {
                StaticRayTracingInstance& instance = g_staticRayTracingInstances.emplace_back();
                instance.vulkanBlasId = mesh->vulkanBlasId;
                instance.vertexBufferDeviceAddress = vertexBufferDeviceAddress;
                instance.indexBufferDeviceAddress = indexBufferDeviceAddress;
                instance.modelMatrix = renderItem.modelMatrix;
                instance.range.baseVertex = renderItem.baseVertex;
                instance.range.baseIndex = renderItem.baseIndex;
                instance.range.vertexCount = mesh->vertexCount;
                instance.range.indexCount = mesh->indexCount;
                instance.range.blendingMode = renderItem.blendingMode;
                instance.range.materialIndex = renderItem.materialIndex;
                instance.range.shadowBit = renderItem.shadowFlags;
            }
        }
    }

    void SubmitRenderItems(const std::vector<RenderItem>& renderItems) {
        for (const RenderItem& renderItem : renderItems) {
            SubmitRenderItem(renderItem);
        }
    }
}
