#include "RenderDataManager.h"
#include "AssetManagement/AssetManager.h"
#include "HellConstants.h"
#include "BackEnd/BackEnd.h"
#include "Camera/Frustum.h"
#include "Core/Game.h"
#include "Config/Config.h"
#include "Editor/Editor.h"
#include "Input/Input.h"
#include "Managers/MirrorManager.h"
#include "Ocean/Ocean.h"
#include "Renderer/Renderer.h"
#include "Viewport/ViewportManager.h"
#include <span>
#include <unordered_map>

#include "HellLogging.h"
#include "Timer.hpp"

// Get me out of here
#include "World/World.h"
#include "API/OpenGL/Renderer/GL_renderer.h"
// Get me out of here

namespace RenderDataManager {
    DrawCommandsSet g_drawCommandsSet;
    FlashLightShadowMapDrawInfo g_flashLightShadowMapDrawInfo;
    RendererData g_rendererData;
    std::vector<GPULight> g_gpuLightsHighRes;

    std::vector<HouseRenderItem> g_houseRenderItems;
    std::vector<HouseRenderItem> g_houseOutlineRenderItems;
    std::vector<RenderItem> g_decalRenderItems;

    std::vector<RenderItem> g_glassRenderItems;
    std::vector<RenderItem> g_renderItems;
    std::vector<RenderItem> g_renderItemsBlended;
    std::vector<RenderItem> g_renderItemsAlphaDiscarded;
    std::vector<RenderItem> g_renderItemsHairTopLayer;
    std::vector<RenderItem> g_renderItemsHairBottomLayer;
    std::vector<RenderItem> g_renderItemsMirror;
    std::vector<RenderItem> g_stainedGlassRenderItems;

    std::vector<RenderItem> g_shadowCasterRenderItems;

    std::vector<RenderItem> g_outlineRenderItems;
    std::vector<RenderItem> g_shadowMapRenderItems;

    std::vector<RenderItem> g_instanceData;
    std::vector<ViewportData> g_viewportData;

    std::vector<DecalPaintingInfo> g_decalPaintingInfo;

    std::vector<BloodDecalInstanceData> g_screenSpaceBloodDecalInstances;

    std::vector<glm::mat4> g_skinningTransforms;
	std::vector<RenderItem> g_skinnedRenderItems;
	std::vector<RenderItem> g_nonDeformingSkinnedMeshRenderItems;
    std::vector<RenderItem> g_nonDeformingSkinnedMeshRenderItemsDepthPeeledTransparent;

    std::vector<glm::mat4> g_oceanPatchTransforms;
    std::vector<float> g_shadowCascadeLevels{ 5.0f, 10.0f, 20.0f, 40.0f }; // WARNING! YOU have a duplicate of this in GL_renderer.h

    void UpdateOceanPatchTransforms();
    void UpdateViewportData();
    void UpdateRendererData();
    void UpdateDrawCommandsSet();
    void CreateSkinningData();
    void CreateDrawCommands(std::vector<DrawIndexedIndirectCommand>& drawCommands, std::vector<RenderItem>& renderItems, Frustum* frustum, int viewportIndex, bool ignoreNonShadowCasters = false);
    void CreateDrawCommandsSkinned(std::vector<DrawIndexedIndirectCommand>& commands, std::vector<RenderItem>& renderItems, int viewportIndex);
    void CreateDrawCommandsNonDeformingSkinned(std::vector<DrawIndexedIndirectCommand>& commands, std::vector<RenderItem>& renderItems, int viewportIndex);

    void CreateMultiDrawIndirectCommands(std::vector<DrawIndexedIndirectCommand>& commands, std::span<RenderItem> renderItems, int viewportIndex, int instanceOffset);
    void CreateMultiDrawIndirectCommandsSkinned(std::vector<DrawIndexedIndirectCommand>& commands, std::span<RenderItem> renderItems, int viewportIndex, int instanceOffset);
    void CreateMultiDrawIndirectCommandsSkinnedNonDeforming(std::vector<DrawIndexedIndirectCommand>& commands, std::span<RenderItem> renderItems, int viewportIndex, int instanceOffset);

    void CreateShadowCubeMapMultiDrawIndirectCommands(std::vector<DrawIndexedIndirectCommand>& commands, uint32_t faceIndex, GPULight& gpuLight);
    void CreateMoonLightShadowMapDrawCommands();


    int EncodeBaseInstance(int playerIndex, int instanceOffset);
    void DecodeBaseInstance(int baseInstance, int& playerIndex, int& instanceOffset);

    void BeginFrame() {
        g_skinningTransforms.clear();
		g_skinnedRenderItems.clear();
		g_nonDeformingSkinnedMeshRenderItems.clear();
		g_nonDeformingSkinnedMeshRenderItemsDepthPeeledTransparent.clear();

        g_decalRenderItems.clear();
        g_houseOutlineRenderItems.clear();
        g_houseRenderItems.clear();

        g_renderItems.clear();
        g_renderItemsMirror.clear();
        g_glassRenderItems.clear();
        g_renderItemsBlended.clear();
        g_renderItemsAlphaDiscarded.clear();
        g_renderItemsHairTopLayer.clear();
        g_renderItemsHairBottomLayer.clear();
        g_stainedGlassRenderItems.clear();
        g_outlineRenderItems.clear();
        g_gpuLightsHighRes.clear();
        g_decalPaintingInfo.clear();
        g_shadowCasterRenderItems.clear();
    }

    void Update() {
        UpdateViewportData();
        UpdateRendererData();
        UpdateDrawCommandsSet();
    }

    void UpdateViewportData() {
        const Resolutions& resolutions = Config::GetResolutions();
        g_viewportData.resize(4);
        for (int i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
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
            }
            else {
                g_viewportData[i].orthoSize = 0.0f;
                g_viewportData[i].isOrtho = false;
                g_viewportData[i].fov = Game::GetLocalPlayerFovByIndex(i);

                Player* player = Game::GetLocalPlayerByIndex(i);
                if (player) {
                    g_viewportData[i].colorTint = glm::vec4(player->GetViewportColorTint(), 1.0f);
                    g_viewportData[i].colorContrast = player->GetViewportContrast();

                    if (player->IsDead()) {
                        viewMatrix = player->m_deathCamViewMatrix;
                    }
                    else {
                        viewMatrix = Game::GetLocalPlayerCameraByIndex(i)->GetViewMatrix();
                    }

                    g_viewportData[i].isInShop = player->IsInShop();
                }
            }
            glm::mat4 inverseView = glm::inverse(viewMatrix);
            glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
            glm::vec3 cameraRight = glm::vec3(inverseView[0]);
            glm::vec3 cameraUp = glm::vec3(inverseView[1]);
            glm::vec3 cameraForward = -glm::vec3(inverseView[2]);

            // Is there any previous data?
            bool previousDataExists = !Util::Mat4NearlyEqual(g_viewportData[i].previousProjectionView, glm::mat4(1.0f));

            // Previous
            if (previousDataExists) {
                g_viewportData[i].previousProjectionView = g_viewportData[i].projectionView;
            }

            // Store them (no need to normalize, they should already be unit vectors)
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
                Player* player = Game::GetLocalPlayerByIndex(i);
                if (player) {
                    g_viewportData[i].flashlightProjectionView = player->GetFlashlightProjectionView();
                    g_viewportData[i].flashlightDir = glm::vec4(player->GetFlashlightDirection(), 0.0f);
                    g_viewportData[i].flashlightPosition = glm::vec4(player->GetFlashlightPosition(), 0.0f);
                    g_viewportData[i].flashlightModifer = player->GetFlashLightModifer();
                }
            }

            // CSM matrices
            glm::vec3 lightDir = Game::GetMoonlightDirection();
            float viewportWidth = g_viewportData[i].width;
            float viewportHeight = g_viewportData[i].height;
            float fov = g_viewportData[i].fov;
            const std::vector<glm::mat4> lightProjectionViews = Util::GetLightProjectionViews(viewMatrix, lightDir, g_shadowCascadeLevels, viewportWidth, viewportHeight, fov);

            if (lightProjectionViews.size() != SHADOW_CASCADE_COUNT) Logging::Error() << "INCORRECT SIZE: " << lightProjectionViews.size();
            for (int j = 0; j < SHADOW_CASCADE_COUNT && j < lightProjectionViews.size(); j++) {
                g_viewportData[i].csmLightProjectionView[j] = lightProjectionViews[j];
            }
        }
    }

    void UpdateRendererData() {
        const RendererSettings& rendererSettings = Renderer::GetCurrentRendererSettings();
        const Resolutions& resolutions = Config::GetResolutions();
        g_rendererData.nearPlane = NEAR_PLANE;
        g_rendererData.farPlane = FAR_PLANE;
        g_rendererData.gBufferWidth = (float)resolutions.gBuffer.x;
        g_rendererData.gBufferHeight = (float)resolutions.gBuffer.y;
        g_rendererData.hairBufferWidth = (float)resolutions.hair.x;
        g_rendererData.hairBufferHeight = (float)resolutions.hair.y;
        g_rendererData.splitscreenMode = (int)Game::GetSplitscreenMode();
        g_rendererData.time = Game::GetTotalTime();
        g_rendererData.rendererOverrideState = (int)rendererSettings.rendererOverrideState;
        g_rendererData.normalizedMouseX = Util::MapRange(Input::GetMouseX(), 0, BackEnd::GetCurrentWindowWidth(), 0.0f, 1.0f);
        g_rendererData.normalizedMouseY = Util::MapRange(Input::GetMouseY(), 0, BackEnd::GetCurrentWindowHeight(), 0.0f, 1.0f);
        g_rendererData.tileCountX = resolutions.gBuffer.x / TILE_SIZE;
        g_rendererData.tileCountY = resolutions.gBuffer.y / TILE_SIZE;
    }

    void SortRenderItems(std::vector<RenderItem>& renderItems) {
        std::sort(renderItems.begin(), renderItems.end(), [](const RenderItem& a, const RenderItem& b) {
            return a.meshIndex < b.meshIndex;
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

        std::vector<RenderItem> potentialRenderItems = g_renderItems; // First start with everything in the scene
        potentialRenderItems.insert(potentialRenderItems.end(), g_shadowCasterRenderItems.begin(), g_shadowCasterRenderItems.end());

        Frustum frustum;

        for (int i = 0; i < viewportCount; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (!viewport || !viewport->IsVisible()) continue;

            for (int j = 0; j < cascadeCount; j++) {
                frustum.Update(g_viewportData[i].csmLightProjectionView[j]);

                // Store the instance offset for this player
                int instanceStart = g_instanceData.size();

                // Preallocate an estimate
                g_instanceData.reserve(g_instanceData.size() + potentialRenderItems.size());

                // Append new render items to the global instance data if its within this cascade's frustum
                for (const RenderItem& renderItem : potentialRenderItems) {
                    if (renderItem.castCSMShadows && frustum.IntersectsAABBFast(renderItem)) {
                        g_instanceData.push_back(renderItem);
                        //Renderer::DrawAABB(AABB(renderItem.aabbMin, renderItem.aabbMax), YELLOW);
                    }
                }

                // Create indirect draw commands using the stored offset
                std::span<RenderItem> instanceView(g_instanceData.begin() + instanceStart, g_instanceData.end());
                CreateMultiDrawIndirectCommands(set.moonLightCascades[i][j], instanceView, -1, instanceStart);
            }
        }
    }

	const std::vector<RenderItem>& GetNonDeformingSkinnedMeshRenderItems() {
		return g_nonDeformingSkinnedMeshRenderItems;
	}

	const std::vector<RenderItem>& GetNonDeformingSkinnedMeshRenderItemsDepthPeeledTransparent() {
		return g_nonDeformingSkinnedMeshRenderItemsDepthPeeledTransparent;
	}


    void UpdateDrawCommandsSet() {
        g_instanceData.clear();
        auto& set = g_drawCommandsSet;

        // Clear any commands from last frame
        for (int i = 0; i < 4; i++) {
            set.geometry[i].clear();
            set.geometryBlended[i].clear();
            set.geometryAlphaDiscarded[i].clear();
            set.hairTopLayer[i].clear();
            set.hairBottomLayer[i].clear();
            set.mirrorRenderItems[i].clear();

            g_flashLightShadowMapDrawInfo.flashlightShadowMapGeometry[i].clear();
            g_flashLightShadowMapDrawInfo.heightMapChunkIndices[i].clear();
            g_flashLightShadowMapDrawInfo.houseMeshRenderItems[i].clear();
        }



        SortRenderItems(g_renderItems);
        SortRenderItems(g_renderItemsBlended);
        SortRenderItems(g_renderItemsAlphaDiscarded);
        SortRenderItems(g_renderItemsHairTopLayer);
        SortRenderItems(g_renderItemsHairBottomLayer);

        // Lil hack to include bullet decals in mirrors
        int count = g_renderItems.size() + g_renderItemsAlphaDiscarded.size();
        std::vector<RenderItem> potentialMirrorItems;
        potentialMirrorItems.reserve(count);
        potentialMirrorItems.insert(potentialMirrorItems.end(), g_renderItems.begin(), g_renderItems.end());
        potentialMirrorItems.insert(potentialMirrorItems.end(), g_renderItemsAlphaDiscarded.begin(), g_renderItemsAlphaDiscarded.end());


        for (int i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            Frustum& frustum = viewport->GetFrustum();
            CreateDrawCommands(set.geometry[i], g_renderItems, &frustum, i);
            CreateDrawCommands(set.geometryBlended[i], g_renderItemsBlended, &frustum, i);
            CreateDrawCommands(set.geometryAlphaDiscarded[i], g_renderItemsAlphaDiscarded, &frustum, i);
            CreateDrawCommands(set.hairTopLayer[i], g_renderItemsHairTopLayer, &frustum, i);
            CreateDrawCommands(set.hairBottomLayer[i], g_renderItemsHairBottomLayer, &frustum, i);

            if (Mirror* mirror = MirrorManager::GetMirrorByObjectId(viewport->GetMirrorId())) {
                CreateDrawCommands(set.mirrorRenderItems[i], potentialMirrorItems, mirror->GetFrustum(i), i);
            }
        }


        CreateSkinningData();

        for (int i = 0; i < g_gpuLightsHighRes.size(); i++) {
            GPULight& gpuLight = g_gpuLightsHighRes[i];
            for (uint32_t j = 0; j < 6; j++) {
                CreateShadowCubeMapMultiDrawIndirectCommands(set.shadowMapHiRes[i][j], j, gpuLight);
            }
        }

        // CSM render items (moon light shadowmaps)
        CreateMoonLightShadowMapDrawCommands();

        // Flashlight stuff
        for (int playerIndex = 0; playerIndex < Game::GetLocalPlayerCount(); playerIndex++) {
            Player* player = Game::GetLocalPlayerByIndex(playerIndex);
            if (!player) continue;

            Frustum flashLightFrustum = player->GetFlashlightFrustum();

            // Build multi draw commands for regular geometry
            CreateDrawCommands(g_flashLightShadowMapDrawInfo.flashlightShadowMapGeometry[playerIndex], g_renderItems, &flashLightFrustum, playerIndex, true);

            // Frustum cull the heightmap chunks
            std::vector<HeightMapChunk>& chunks = World::GetHeightMapChunks();
            for (int i = 0; i < chunks.size(); i++) {
                HeightMapChunk& chunk = chunks[i];
                if (flashLightFrustum.IntersectsAABBFast(AABB(chunk.aabbMin, chunk.aabbMax))) {
                    g_flashLightShadowMapDrawInfo.heightMapChunkIndices[playerIndex].push_back(i);
                }
            }

            // Frustum cull the house mesh
            g_flashLightShadowMapDrawInfo.houseMeshRenderItems->reserve(g_houseRenderItems.size());
            for (int i = 0; i < g_houseRenderItems.size(); i++) {
                HouseRenderItem& renderItem = g_houseRenderItems[i];
                if (flashLightFrustum.IntersectsAABBFast(renderItem)) {
                    g_flashLightShadowMapDrawInfo.houseMeshRenderItems[playerIndex].push_back(renderItem);
                }
            }
        }

        // Screenspace blood decals
        std::sort(World::GetScreenSpaceBloodDecals().begin(), World::GetScreenSpaceBloodDecals().end(), [](const ScreenSpaceBloodDecal& a, const ScreenSpaceBloodDecal& b) {
            return a.m_type < b.m_type;
        });

        int instanceCount = World::GetScreenSpaceBloodDecals().size();
        g_screenSpaceBloodDecalInstances.resize(instanceCount);

        for (int i = 0; i < instanceCount; i++) {
            ScreenSpaceBloodDecal& decal = World::GetScreenSpaceBloodDecals()[i];
            g_screenSpaceBloodDecalInstances[i].modelMatrix = decal.GetModelMatrix();
            g_screenSpaceBloodDecalInstances[i].inverseModelMatrix = decal.GetInverseModelMatrix();
            g_screenSpaceBloodDecalInstances[i].type = decal.GetType();

            switch (decal.GetType()) {
                case 0: g_screenSpaceBloodDecalInstances[i].textureIndex = AssetManager::GetTextureIndexByName("BloodDecal4"); break;
                case 1: g_screenSpaceBloodDecalInstances[i].textureIndex = AssetManager::GetTextureIndexByName("BloodDecal6"); break;
                case 2: g_screenSpaceBloodDecalInstances[i].textureIndex = AssetManager::GetTextureIndexByName("BloodDecal7"); break;
                case 3: g_screenSpaceBloodDecalInstances[i].textureIndex = AssetManager::GetTextureIndexByName("BloodDecal9"); break;
                default: continue;
            }
        }
        UpdateOceanPatchTransforms();
    }

    void CreateDrawCommands(std::vector<DrawIndexedIndirectCommand>& drawCommands, std::vector<RenderItem>& renderItems, Frustum* frustum, int viewportIndex, bool ignoreNonShadowCasters) {
        // Store the instance offset for this list of commands
        int instanceStart = g_instanceData.size();

        // Preallocate an estimate
        g_instanceData.reserve(g_instanceData.size() + renderItems.size());

        // Append new render items to the global instance data
        for (const RenderItem& renderItem : renderItems) {
            if (ignoreNonShadowCasters && !renderItem.castShadows) continue;
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

    void CreateDrawCommandsSkinned(std::vector<DrawIndexedIndirectCommand>& commands, std::vector<RenderItem>& renderItems, int viewportIndex) {
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

            g_instanceData.push_back(renderItem);
        }

        // Create indirect draw commands using the stored offset
        std::span<RenderItem> instanceView(g_instanceData.begin() + instanceStart, g_instanceData.end());
        CreateMultiDrawIndirectCommandsSkinned(commands, instanceView, viewportIndex, instanceStart);
    }

    void CreateDrawCommandsNonDeformingSkinned(std::vector<DrawIndexedIndirectCommand>& commands, std::vector<RenderItem>& renderItems, int viewportIndex) {
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

            g_instanceData.push_back(renderItem);
        }

        // Create indirect draw commands using the stored offset
        std::span<RenderItem> instanceView(g_instanceData.begin() + instanceStart, g_instanceData.end());
        CreateMultiDrawIndirectCommandsSkinnedNonDeforming(commands, instanceView, viewportIndex, instanceStart);
    }

    void CreateShadowCubeMapMultiDrawIndirectCommands(std::vector<DrawIndexedIndirectCommand>& drawCommands, uint32_t faceIndex, GPULight& gpuLight) {
        drawCommands.clear();

        //std::string name = "light " + std::to_string(gpuLight.lightIndex);
        //Timer timer(name);

        Light* light = World::GetLightByIndex(gpuLight.lightIndex);
        if (!light) return;

        Frustum* frustum = light->GetFrustumByFaceIndex(faceIndex);
        if (!frustum) return;

        // Store the instance offset for this player
        int instanceStart = g_instanceData.size();

        // Preallocate an estimate
        g_instanceData.reserve(g_instanceData.size() + g_renderItems.size());

        // Append new render items to the global instance data if it's within light frustum
        // g_renderItems is already sorted by this point
        // but if anything breaks, check here! (maybe you re-ordered things)
        for (const RenderItem& renderItem : g_renderItems) {

            //AABB aabb(renderItem.aabbMin, renderItem.aabbMax);
            //if (frustum->IntersectsAABB(aabb)) {

            if (renderItem.castShadows && frustum->IntersectsAABBFast(renderItem)) {
                g_instanceData.push_back(renderItem);

                //std::cout << gpuLight.lightIndex << " " << AssetManager::GetMeshByIndex(renderItem.meshIndex)->name << "\n";
            }
        }

        // Create indirect draw commands using the stored offset
        std::span<RenderItem> instanceView(g_instanceData.begin() + instanceStart, g_instanceData.end());
        CreateMultiDrawIndirectCommands(drawCommands, instanceView, -1, instanceStart);
    }

    void CreateMultiDrawIndirectCommands(std::vector<DrawIndexedIndirectCommand>& commands, std::span<RenderItem> renderItems, int viewportIndex, int instanceOffset) {
        std::unordered_map<int, std::size_t> commandMap;
        commands.reserve(renderItems.size());

        for (const RenderItem& renderItem : renderItems) {
            int meshIndex = renderItem.meshIndex;
            Mesh* mesh = AssetManager::GetMeshByIndex(meshIndex);

            // If the command exists, increment its instance count
            auto it = commandMap.find(meshIndex);
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

                commandMap[meshIndex] = index;
            }
            instanceOffset++;
        }
    }

    void CreateMultiDrawIndirectCommandsSkinned(std::vector<DrawIndexedIndirectCommand>& commands, std::span<RenderItem> renderItems, int viewportIndex, int instanceOffset) {
        commands.reserve(renderItems.size());

        for (const RenderItem& renderItem : renderItems) {
            SkinnedMesh* mesh = AssetManager::GetSkinnedMeshByIndex(renderItem.meshIndex);
            if (!mesh) {
                instanceOffset++;
                continue;
            }

            auto& cmd = commands.emplace_back();
            cmd.indexCount = mesh->indexCount;
            cmd.firstIndex = mesh->baseIndex;
            cmd.baseVertex = renderItem.baseSkinnedVertex;
            cmd.baseInstance = EncodeBaseInstance(viewportIndex, instanceOffset);
            cmd.instanceCount = 1;
            instanceOffset++;
        }
    }

    void CreateMultiDrawIndirectCommandsSkinnedNonDeforming(std::vector<DrawIndexedIndirectCommand>& commands, std::span<RenderItem> renderItems, int viewportIndex, int instanceOffset) {
        commands.reserve(renderItems.size());

        for (const RenderItem& renderItem : renderItems) {
            SkinnedMesh* mesh = AssetManager::GetSkinnedMeshByIndex(renderItem.meshIndex);
            if (!mesh) {
                instanceOffset++;
                continue;
            }

            auto& cmd = commands.emplace_back();
            cmd.indexCount = mesh->indexCount;
            cmd.firstIndex = mesh->baseIndex;
            cmd.baseVertex = mesh->baseVertexGlobal;
            cmd.baseInstance = EncodeBaseInstance(viewportIndex, instanceOffset);
            cmd.instanceCount = 1;
            instanceOffset++;
        }
    }

    void CreateSkinningData() {
        auto& set = g_drawCommandsSet;

        // Sort render items by mesh index
        SortRenderItems(g_skinnedRenderItems);

        // Create the transforms buffer
        std::unordered_map<uint64_t, uint32_t> transformIndexMap; // Maps an AnimatedGameObject Id to its base transform index
        uint32_t baseTransformIndex = 0;
        int32_t baseSkinnedVertex = 0;
        std::vector<RenderItem> validSkinnedRenderItems;
        validSkinnedRenderItems.reserve(g_skinnedRenderItems.size());

        for (RenderItem renderItem : g_skinnedRenderItems) {
            uint64_t id = 0;
            Util::UnpackUint64(renderItem.objectIdLowerBit, renderItem.objectIdUpperBit, id);

            AnimatedGameObject* animatedGameObject = World::GetAnimatedGameObjectByObjectId(id);
            if (!animatedGameObject) {
                continue;
            }

            const std::vector<glm::mat4>& boneSkinningMatrices = animatedGameObject->GetBoneSkinningMatrices();
            if (boneSkinningMatrices.empty()) {
                continue;
            }

            SkinnedMesh* mesh = AssetManager::GetSkinnedMeshByIndex(renderItem.meshIndex);
            if (!mesh || mesh->vertexCount == 0) {
                continue;
            }

            // Add the id if aint in the map yet
            if (transformIndexMap.find(id) == transformIndexMap.end()) {
                transformIndexMap[id] = baseTransformIndex;

                // Append skinning matrices to global array
                g_skinningTransforms.insert(g_skinningTransforms.end(), boneSkinningMatrices.begin(), boneSkinningMatrices.end());

                // Set the base transform index for the next animated game object
                baseTransformIndex = static_cast<uint32_t>(g_skinningTransforms.size());
            }

            // Update render item with the base transform index
            renderItem.baseSkinningTransformIndex = static_cast<int32_t>(transformIndexMap[id]);

            renderItem.baseSkinnedVertex = baseSkinnedVertex;
            baseSkinnedVertex += static_cast<int32_t>(mesh->vertexCount);
            validSkinnedRenderItems.push_back(renderItem);
        }

        g_skinnedRenderItems.swap(validSkinnedRenderItems);

        // Create the per viewport draw commands
        for (int i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            CreateDrawCommandsSkinned(set.skinnedGeometry[i], g_skinnedRenderItems, i);
        }

        // Gather all non deforming render items
        for (AnimatedGameObject& animatedGameObject : World::GetAnimatedGameObjects()) {
			if (animatedGameObject.RenderingEnabled()) {
				g_nonDeformingSkinnedMeshRenderItems.insert(g_nonDeformingSkinnedMeshRenderItems.end(), animatedGameObject.GetNonDeformingRenderItems().begin(), animatedGameObject.GetNonDeformingRenderItems().end());
                g_nonDeformingSkinnedMeshRenderItemsDepthPeeledTransparent.insert(g_nonDeformingSkinnedMeshRenderItemsDepthPeeledTransparent.end(), animatedGameObject.GetNonDeformingRenderItemsDepthPeeledTransparent().begin(), animatedGameObject.GetNonDeformingRenderItemsDepthPeeledTransparent().end());
            }
        }

        // Sort by mesh index
        SortRenderItems(g_nonDeformingSkinnedMeshRenderItems);

        for (int i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            CreateDrawCommandsNonDeformingSkinned(set.nonDeformingSkinnedGeometry[i], g_nonDeformingSkinnedMeshRenderItems, i);
        }
    }

    void UpdateOceanPatchTransforms() {
        g_oceanPatchTransforms.clear();

        g_oceanPatchTransforms.push_back(glm::mat4(1.0f));

        return;
        OpenGLMeshPatch* oceanMeshPatch = OpenGLRenderer::GetOceanMeshPatch();

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

        Transform tesseleationTransform;
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
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            Frustum& frustum = viewport->GetFrustum();

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


    void SubmitGPULightHighRes(Light& light) {

    }

    int EncodeBaseInstance(int playerIndex, int instanceOffset) {
        return (playerIndex << VIEWPORT_INDEX_SHIFT) | instanceOffset;
    }

    void DecodeBaseInstance(int baseInstance, int& playerIndex, int& instanceOffset) {
        playerIndex = baseInstance >> VIEWPORT_INDEX_SHIFT;
        instanceOffset = baseInstance & ((1 << VIEWPORT_INDEX_SHIFT) - 1);
    }

    //void SubmitAnimatedGameObjectForSkinning(AnimatedGameObject* animatedGameObject) {
    //    g_animatedGameObjectsToSkin.push_back(animatedGameObject);
    //}
    //
    //void ResetBaseSkinnedVertex() {
    //    g_baseSkinnedVertex = 0;
    //}
    //
    //void IncrementBaseSkinnedVertex(uint32_t vertexCount) {
    //    g_baseSkinnedVertex += vertexCount;
    //}
    //
    //uint32_t GetBaseSkinnedVertex() {
    //    return g_baseSkinnedVertex;
    //}
    //
    //std::vector<AnimatedGameObject*>& GetAnimatedGameObjectToSkin() {
    //    return g_animatedGameObjectsToSkin;
    //}

    const RendererData& GetRendererData() {
        return g_rendererData;
    }

    const std::vector<HouseRenderItem>& GetHouseRenderItems() {
        return g_houseRenderItems;
    }

    const std::vector<HouseRenderItem>& GetHouseOutlineRenderItems() {
        return g_houseOutlineRenderItems;
    }

    const std::vector<ViewportData>& GetViewportData() {
        return g_viewportData;
    }

    const std::vector<RenderItem>& GetRenderItems() {
        return g_renderItems;
    }

    const std::vector<RenderItem>& GetGlassRenderItems() {
        return g_glassRenderItems;
    }

    const std::vector<RenderItem>& GetDecalRenderItems() {
        return g_decalRenderItems;
    }

    const std::vector<RenderItem>& GetMirrorRenderItems() {
        return g_renderItemsMirror;
    }

    const std::vector<RenderItem>& GetStainedGlassRenderItems() {
        return g_stainedGlassRenderItems;
    }

    const std::vector<RenderItem>& GetSkinnedRenderItems() {
        return g_skinnedRenderItems;
    }

    const std::vector<RenderItem>& GetInstanceData() {
        return g_instanceData;
    }

    const std::vector<RenderItem>& GetOutlineRenderItems() {
        return g_outlineRenderItems;
    }

    const DrawCommandsSet& GetDrawInfoSet() {
        return g_drawCommandsSet;
    }

    const FlashLightShadowMapDrawInfo& GetFlashLightShadowMapDrawInfo() {
        return g_flashLightShadowMapDrawInfo;
    }

    const std::vector<GPULight>& GetGPULightsHighRes() {
        return g_gpuLightsHighRes;
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

    const std::vector<BloodDecalInstanceData>& GetScreenSpaceBloodDecalInstanceData() {
        return g_screenSpaceBloodDecalInstances;
    }

    // Submissions
    void SubmitGPULightHighRes(uint32_t lightIndex) {
        if (g_gpuLightsHighRes.size() >= SHADOWMAP_HI_RES_COUNT) return;

        Light* light = World::GetLightByIndex(lightIndex);
        if (!light) return;

        GPULight& gpuLight = g_gpuLightsHighRes.emplace_back();
        gpuLight.colorR = light->GetColor().r;
        gpuLight.colorG = light->GetColor().g;
        gpuLight.colorB = light->GetColor().b;
        gpuLight.posX = light->GetPosition().x;
        gpuLight.posY = light->GetPosition().y;
        gpuLight.posZ = light->GetPosition().z;
        gpuLight.radius = light->GetRadius();
        gpuLight.strength = light->GetStrength();
        gpuLight.shadowMapDirty = true;
        gpuLight.lightIndex = lightIndex;
    }

    void SubmitDecalRenderItem(const RenderItem& renderItem) {
        g_decalRenderItems.push_back(renderItem);
    }

    void SubmitRenderItem(const RenderItem& renderItem) {
        g_renderItems.push_back(renderItem);
    }

    void SubmitGlassRenderItem(const RenderItem& renderItem) {
        g_glassRenderItems.push_back(renderItem);
    }

    void SubmitRenderItemsGlass(const std::vector<RenderItem>& renderItems) {
        g_glassRenderItems.insert(g_glassRenderItems.begin(), renderItems.begin(), renderItems.end());
    }

    void SubmitRenderItemsStainedGlass(const std::vector<RenderItem>& renderItems) {
        g_stainedGlassRenderItems.insert(g_stainedGlassRenderItems.begin(), renderItems.begin(), renderItems.end());
    }

    void SubmitHouseRenderItem(const HouseRenderItem& renderItem) {
        g_houseRenderItems.push_back(renderItem);
    }

    void SubmitRenderItems(const std::vector<RenderItem>& renderItems) {
        g_renderItems.insert(g_renderItems.begin(), renderItems.begin(), renderItems.end());
    }

    void SubmitShadowCasterRenderItems(const std::vector<RenderItem>& renderItems) {
        g_shadowCasterRenderItems.insert(g_shadowCasterRenderItems.begin(), renderItems.begin(), renderItems.end());
    }

    void SubmitRenderItemsMirror(const std::vector<RenderItem>& renderItems) {
        g_renderItemsMirror.insert(g_renderItemsMirror.begin(), renderItems.begin(), renderItems.end());
    }

    void SubmitRenderItemsBlended(const std::vector<RenderItem>& renderItems) {
        g_renderItemsBlended.insert(g_renderItemsBlended.begin(), renderItems.begin(), renderItems.end());
    }

    void SubmitRenderItemsAlphaDiscard(const std::vector<RenderItem>& renderItems) {
        g_renderItemsAlphaDiscarded.insert(g_renderItemsAlphaDiscarded.begin(), renderItems.begin(), renderItems.end());
    }

    void SubmitRenderItemsAlphaHairTopLayer(const std::vector<RenderItem>& renderItems) {
        g_renderItemsHairTopLayer.insert(g_renderItemsHairTopLayer.begin(), renderItems.begin(), renderItems.end());
    }

    void SubmitRenderItemsAlphaHairBottomLayer(const std::vector<RenderItem>& renderItems) {
        g_renderItemsHairBottomLayer.insert(g_renderItemsHairBottomLayer.begin(), renderItems.begin(), renderItems.end());
    }

    void SubmitHouseRenderItems(const std::vector<HouseRenderItem>& renderItems) {
        g_houseRenderItems.insert(g_houseRenderItems.begin(), renderItems.begin(), renderItems.end());
    }

    void SubmitOutlineRenderItem(const RenderItem& renderItem) {
        g_outlineRenderItems.push_back(renderItem);
    }

    void SubmitOutlineRenderItem(const HouseRenderItem& renderItem) {
        g_houseOutlineRenderItems.push_back(renderItem);
    }

    void SubmitOutlineRenderItems(const std::vector<RenderItem>& renderItems) {
        g_outlineRenderItems.insert(g_outlineRenderItems.begin(), renderItems.begin(), renderItems.end());
    }

    void SubmitOutlineRenderItems(const std::vector<HouseRenderItem>& renderItems) {
        g_houseOutlineRenderItems.insert(g_houseOutlineRenderItems.begin(), renderItems.begin(), renderItems.end());
    }

    void SubmitDecalPaintingInfo(DecalPaintingInfo decalPaintingInfo) {
        g_decalPaintingInfo.push_back(decalPaintingInfo);
    }

    void SubmitSkinnedRenderItems(const std::vector<RenderItem>& renderItems) {
        g_skinnedRenderItems.insert(g_skinnedRenderItems.begin(), renderItems.begin(), renderItems.end());
    }

       //void SubmitDecalPaintingInfo(DecalPaintingInfo& decalPaintingInfo)  {
   //    g_decalPaintingInfo.push_back(decalPaintingInfo);
   //}
}
