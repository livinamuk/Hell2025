#include "Hell/Render/API/OpenGL/GL_back_end.h"
#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Viewport/ViewportManager.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Unloved/Objects/Lighting/Light.h"
#include "Unloved/Systems/ShadowMaps/ShadowMapManager.h"
#include "Unloved/World/World.h"
#include "World/LegacyWorld.h"

#include <cstddef>
#include <algorithm>
#include <initializer_list>

using namespace Hell;            // lil bit cursed dis

namespace OpenGL::Renderer {

    using namespace Unloved;     // lil bit cursed dis

    namespace {
        constexpr uint32_t POINT_SHADOW_FACE_COUNT = 6;

        struct alignas(16) OpenGLPointShadowFaceData {
            glm::mat4 projectionView = glm::mat4(1.0f);
            glm::vec4 lightPositionRadius = glm::vec4(0.0f);
            glm::uvec4 arrayLayer = glm::uvec4(0);
        };

        struct OpenGLPointShadowDrawBatch {
            size_t byteOffset = 0;
            uint32_t drawDataOffset = 0;
            GLsizei count = 0;
        };

        struct PointShadowTimingLabels {
            const char* prepare = nullptr;
            const char* upload = nullptr;
            const char* procedural = nullptr;
            const char* opaqueAsset = nullptr;
            const char* opaqueSkinned = nullptr;
            const char* alphaAsset = nullptr;
            const char* alphaSkinned = nullptr;
        };

        constexpr PointShadowTimingLabels POINT_SHADOW_STATIC_HI_TIMINGS = {
            "PointShadow Static Hi Prepare",
            "PointShadow Static Hi Upload",
            "PointShadow Static Hi Procedural",
            "PointShadow Static Hi OpaqueAsset",
            "PointShadow Static Hi OpaqueSkinned",
            "PointShadow Static Hi AlphaAsset",
            "PointShadow Static Hi AlphaSkinned"
        };

        constexpr PointShadowTimingLabels POINT_SHADOW_STATIC_LOW_TIMINGS = {
            "PointShadow Static Low Prepare",
            "PointShadow Static Low Upload",
            "PointShadow Static Low Procedural",
            "PointShadow Static Low OpaqueAsset",
            "PointShadow Static Low OpaqueSkinned",
            "PointShadow Static Low AlphaAsset",
            "PointShadow Static Low AlphaSkinned"
        };

        constexpr PointShadowTimingLabels POINT_SHADOW_HI_TIMINGS = {
            "PointShadow Hi Prepare",
            "PointShadow Hi Upload",
            "PointShadow Hi Procedural",
            "PointShadow Hi OpaqueAsset",
            "PointShadow Hi OpaqueSkinned",
            "PointShadow Hi AlphaAsset",
            "PointShadow Hi AlphaSkinned"
        };

        constexpr PointShadowTimingLabels POINT_SHADOW_LOW_TIMINGS = {
            "PointShadow Low Prepare",
            "PointShadow Low Upload",
            "PointShadow Low Procedural",
            "PointShadow Low OpaqueAsset",
            "PointShadow Low OpaqueSkinned",
            "PointShadow Low AlphaAsset",
            "PointShadow Low AlphaSkinned"
        };

        size_t GetPointShadowFaceDrawCommandCount(const PointLightShadowMapDrawCommands& drawCommands, uint32_t shadowMapIndex, uint32_t faceIndex) {
            return drawCommands.procedural[shadowMapIndex][faceIndex].size()
                + drawCommands.assetGeometry[shadowMapIndex][faceIndex].size()
                + drawCommands.assetGeometrySkinnedNonDeforming[shadowMapIndex][faceIndex].size()
                + drawCommands.assetGeometrySkinned[shadowMapIndex][faceIndex].size()
                + drawCommands.assetGeometryAlphaDiscard[shadowMapIndex][faceIndex].size()
                + drawCommands.assetGeometryHair[shadowMapIndex][faceIndex].size()
                + drawCommands.assetGeometrySkinnedNonDeformingAlphaDiscard[shadowMapIndex][faceIndex].size()
                + drawCommands.assetGeometrySkinnedNonDeformingHair[shadowMapIndex][faceIndex].size()
                + drawCommands.assetGeometrySkinnedAlphaDiscard[shadowMapIndex][faceIndex].size()
                + drawCommands.assetGeometrySkinnedHair[shadowMapIndex][faceIndex].size();
        }

        size_t GetPointShadowDrawCommandCount(const PointLightShadowMapDrawCommands& drawCommands, const std::vector<ShadowMapInfo>& shadowMapInfos, uint32_t shadowMapCount) {
            size_t count = 0;
            for (const ShadowMapInfo& shadowMapInfo : shadowMapInfos) {
                if (shadowMapInfo.shadowMapIndex < 0 || shadowMapInfo.shadowMapIndex >= MAX_SHADOW_MAP_ARRAY_LEVELS || shadowMapInfo.shadowMapIndex >= static_cast<int32_t>(shadowMapCount)) continue;
                const uint32_t shadowMapIndex = static_cast<uint32_t>(shadowMapInfo.shadowMapIndex);
                for (uint32_t faceIndex = 0; faceIndex < POINT_SHADOW_FACE_COUNT; faceIndex++) {
                    if ((shadowMapInfo.faceMask & uint8_t(1u << faceIndex)) == 0) continue;
                    count += GetPointShadowFaceDrawCommandCount(drawCommands, shadowMapIndex, faceIndex);
                }
            }
            return count;
        }

        void AppendPointShadowDrawCommands(std::vector<DrawIndexedIndirectCommand>& destinationCommands, std::vector<uint32_t>& destinationFaceDataIndices, uint32_t faceDataIndex, std::initializer_list<const std::vector<DrawIndexedIndirectCommand>*> sources) {
            for (const std::vector<DrawIndexedIndirectCommand>* source : sources) {
                if (!source || source->empty()) continue;
                for (const DrawIndexedIndirectCommand& command : *source) {
                    destinationCommands.push_back(command);
                    destinationFaceDataIndices.push_back(faceDataIndex);
                }
            }
        }

        OpenGLPointShadowDrawBatch AppendPointShadowDrawBatch(std::vector<DrawIndexedIndirectCommand>& destinationCommands, std::vector<uint32_t>& destinationFaceDataIndices, const std::vector<DrawIndexedIndirectCommand>& sourceCommands, const std::vector<uint32_t>& sourceFaceDataIndices) {
            OpenGLPointShadowDrawBatch batch;
            if (sourceCommands.size() != sourceFaceDataIndices.size()) return batch;

            batch.byteOffset = destinationCommands.size() * sizeof(DrawIndexedIndirectCommand);
            batch.drawDataOffset = static_cast<uint32_t>(destinationFaceDataIndices.size());
            batch.count = static_cast<GLsizei>(sourceCommands.size());
            destinationCommands.insert(destinationCommands.end(), sourceCommands.begin(), sourceCommands.end());
            destinationFaceDataIndices.insert(destinationFaceDataIndices.end(), sourceFaceDataIndices.begin(), sourceFaceDataIndices.end());
            return batch;
        }

        void DrawPointShadowBatch(const OpenGLPointShadowDrawBatch& batch, const char* timingLabel) {
            if (batch.count == 0) return;
            ProfilerOpenGLZone(timingLabel);
            OpenGL::SetUniformInt("u_drawDataOffset", static_cast<int>(batch.drawDataOffset));
            glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, reinterpret_cast<const void*>(batch.byteOffset), batch.count, 0);
        }
    }


    void RenderFlashLightShadowMaps();
    void PointLightShadowPass();
    void PreparePointLightShadowMapArray(OpenGLShadowCubeMapArray* destination, OpenGLShadowCubeMapArray* source, const std::vector<ShadowMapInfo>& shadowMapInfoSet);
    void RenderPointLightShadowMapArray(OpenGLShadowCubeMapArray* shadowMaps, const std::vector<ShadowMapInfo>& shadowMapInfoSet, const PointLightShadowMapDrawCommands& drawCommands, const char* faceDataBufferName, const char* drawCommandBufferName, const PointShadowTimingLabels& timingLabels);
    void RenderMoonLightCascadedShadowMaps();

    void RenderShadowMaps() {
        RenderFlashLightShadowMaps();
        PointLightShadowPass();
        RenderMoonLightCascadedShadowMaps();
    }

    void RenderFlashLightShadowMaps() {
        ProfilerOpenGLZoneFunction();

        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("ShadowMap");
        OpenGLShadowMap* shadowMapsFBO = OpenGL::ResourceManager::GetShadowMapPtr("FlashlightShadowMaps");
        //const DrawCommandsSet& drawInfoSet = Unloved::RenderDataManager::GetDrawInfoSet();
        const FlashLightShadowMapDrawInfo& flashLightShadowMapDrawInfo = Unloved::RenderDataManager::GetFlashLightShadowMapDrawInfo();
        MeshBuffer& meshBufferProcedural = Hell::ResourceManager::GetMeshBuffer("Procedural");
        OpenGLMeshBuffer& glMeshBufferAssets = OpenGL::ResourceManager::GetMeshBuffer("AssetGeometry");
        OpenGLMeshBuffer& glMeshBufferProcedural = OpenGL::ResourceManager::GetMeshBuffer("Procedural");

        glm::mat4 heightMapModelMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(HEIGHTMAP_SCALE_XZ, HEIGHTMAP_SCALE_Y, HEIGHTMAP_SCALE_XZ)); // move to height map manager

        glEnable(GL_DEPTH_TEST);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        shadowMapsFBO->Bind();
        shadowMapsFBO->SetViewport();

        OpenGL::BindShader("ShadowMap");
        OpenGL::BindSSBO(SSBO_IDX_SCENE_RENDER_ITEMS, "SceneRenderItems");
        OpenGL::BindSSBO(SSBO_IDX_DRAW_RENDER_ITEM_INDICES, "DrawRenderItemIndices");

        for (int i = 0; i < MAX_SHADOWED_SPOT_LIGHTS; i++) {
            shadowMapsFBO->BindLayer(i);
            shadowMapsFBO->ClearLayer(i);
            if (!flashLightShadowMapDrawInfo.active[i]) continue;

            const glm::mat4& lightProjectionView = flashLightShadowMapDrawInfo.projectionView[i];
            OpenGL::SetUniformMat4("u_projectionView", lightProjectionView);

            // Scene geometry
            OpenGL::SetUniformBool("u_useDrawRenderItemIndices", true);
            glCullFace(GL_FRONT);
            glBindVertexArray(glMeshBufferAssets.GetVAO());

            MultiDrawIndirect(flashLightShadowMapDrawInfo.flashlightShadowMapGeometry[i]);

            // Heightfield chunks
            std::vector<HeightMapChunk>& chunks = LegacyWorld::GetHeightMapChunks();
            if (!chunks.empty()) {
                MeshBuffer& heightMapMeshBuffer = Hell::ResourceManager::GetMeshBuffer("HeightMapGeometry");
                OpenGLMeshBuffer& glHeightMapMeshBuffer = OpenGL::ResourceManager::GetMeshBuffer("HeightMapGeometry");
                OpenGLFrameBuffer& worldFbo = OpenGL::ResourceManager::GetFrameBuffer("World");
                Hell::TextureArray* displacementBuffer = Hell::ResourceManager::GetTextureArrayPtr("TerrainDisplacement");

                OpenGL::BindShader("ShadowHeightMap");
                OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");
                OpenGL::SetUniformMat4("u_projectionView", lightProjectionView);
                OpenGL::SetUniformMat4("u_modelMatrix", heightMapModelMatrix);
                OpenGL::SetUniformInt("u_viewportIndex", std::max(flashLightShadowMapDrawInfo.ownerViewportIndex[i], 0));
                OpenGL::BindTextureUnit(5, worldFbo.GetColorAttachmentHandleByName("HeightMap"));
                if (displacementBuffer) OpenGL::BindTextureUnit(6, displacementBuffer->GetHandle());
                glBindVertexArray(glHeightMapMeshBuffer.GetVAO());
                glPatchParameteri(GL_PATCH_VERTICES, 3);

                for (uint32_t chunkIndex : flashLightShadowMapDrawInfo.heightMapChunkIndices[i]) {
                    HeightMapChunk& chunk = chunks[chunkIndex];
                    Mesh* mesh = heightMapMeshBuffer.GetMeshById(chunk.meshId);
                    if (!mesh) continue;

                    int indexCount = mesh->indexCount;
                    int baseVertex = mesh->baseVertex;
                    int baseIndex = mesh->baseIndex;
                    void* indexOffset = (GLvoid*)(baseIndex * sizeof(GLuint));
                    if (indexCount > 0) {
                        glDrawElementsBaseVertex(GL_PATCHES, indexCount, GL_UNSIGNED_INT, indexOffset, baseVertex);
                    }
                }
            }

            // Procedural
            OpenGL::BindShader("ShadowMap");
            OpenGL::SetUniformMat4("u_projectionView", lightProjectionView);
            OpenGL::SetUniformBool("u_useDrawRenderItemIndices", false);
            OpenGL::SetUniformMat4("u_modelMatrix", glm::mat4(1.0f));

            glBindVertexArray(glMeshBufferProcedural.GetVAO());

            const std::vector<RenderItem>& sceneRenderItems = Unloved::RenderDataManager::GetSceneRenderItems();
            const std::vector<uint32_t>& renderItemIndices = Unloved::RenderDataManager::GetRenderItemIndicesProcedural();
            for (uint32_t renderItemIndex : renderItemIndices) {
                const RenderItem& renderItem = sceneRenderItems[renderItemIndex];

                Mesh* mesh = meshBufferProcedural.GetMeshById(renderItem.meshId);
                if (!mesh) continue;

                int indexCount = mesh->indexCount;
                int baseVertex = renderItem.baseVertex;
                int baseIndex = renderItem.baseIndex;
                glDrawElementsBaseVertex(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * baseIndex), baseVertex);
            }
        }

        glBindVertexArray(0);
        glCullFace(GL_BACK);
    }

    void PointLightShadowPass() {
        // Keep these zones flat: the on-screen profiler does not visualize parent/child relationships.
        OpenGLShader* opaqueShader = OpenGL::ResourceManager::GetShaderPtr("ShadowCubeMap");
        OpenGLShader* alphaDiscardShader = OpenGL::ResourceManager::GetShaderPtr("ShadowCubeMapAlphaDiscard");
        if (!opaqueShader || !alphaDiscardShader) return;

        const std::vector<ShadowMapInfo>& staticHiResShadowMaps = ShadowMapManager::GetStaticDirtyHiResShadowMaps();
        const std::vector<ShadowMapInfo>& staticLowResShadowMaps = ShadowMapManager::GetStaticDirtyLowResShadowMaps();
        const std::vector<ShadowMapInfo>& compositeHiResShadowMaps = ShadowMapManager::GetCompositeDirtyHiResShadowMaps();
        const std::vector<ShadowMapInfo>& compositeLowResShadowMaps = ShadowMapManager::GetCompositeDirtyLowResShadowMaps();
        if (staticHiResShadowMaps.empty() && staticLowResShadowMaps.empty() && compositeHiResShadowMaps.empty() && compositeLowResShadowMaps.empty()) return;

        OpenGL::BindShader("ShadowCubeMap");
        OpenGL::BindSSBO(SSBO_IDX_SAMPLERS, "Samplers");
        OpenGL::BindSSBO(SSBO_IDX_MATERIALS, "Materials");
        OpenGL::BindSSBO(SSBO_IDX_SCENE_RENDER_ITEMS, "SceneRenderItems");
        OpenGL::BindSSBO(SSBO_IDX_DRAW_RENDER_ITEM_INDICES, "DrawRenderItemIndices");

		OpenGLRasterizerState state;
		state.depthMask = true;
		state.depthTestEnabled = true;
		state.depthFunc = GL_LESS;
		state.blendEnable = false;
		state.cullfaceEnable = true;
		state.cullfaceMode = GL_FRONT;
        OpenGL::RasterizerStateManager::ForceRasterizerState(state);

        const DrawCommandsSet& drawInfoSet = Unloved::RenderDataManager::GetDrawInfoSet();
        OpenGLShadowCubeMapArray* staticHiRes = OpenGL::ResourceManager::GetShadowCubeMapArrayPtr("StaticHiRes");
        OpenGLShadowCubeMapArray* staticLowRes = OpenGL::ResourceManager::GetShadowCubeMapArrayPtr("StaticLowRes");
        OpenGLShadowCubeMapArray* compositeHiRes = OpenGL::ResourceManager::GetShadowCubeMapArrayPtr("HiRes");
        OpenGLShadowCubeMapArray* compositeLowRes = OpenGL::ResourceManager::GetShadowCubeMapArrayPtr("LowRes");
        const bool staticCacheEnabled = ShadowMapManager::StaticCacheEnabled();

        if (staticCacheEnabled && (!staticHiRes || !staticLowRes)) return;

        if (!staticHiResShadowMaps.empty()) {
            ProfilerOpenGLZone("PointShadow Static Clear Hi");
            PreparePointLightShadowMapArray(staticHiRes, nullptr, staticHiResShadowMaps);
        }
        if (!staticLowResShadowMaps.empty()) {
            ProfilerOpenGLZone("PointShadow Static Clear Low");
            PreparePointLightShadowMapArray(staticLowRes, nullptr, staticLowResShadowMaps);
        }
        RenderPointLightShadowMapArray(staticHiRes, staticHiResShadowMaps, drawInfoSet.staticHiResShadowMapDrawCommands, "PointShadowStaticHiResFaceData", "PointShadowStaticHiResDrawCommands", POINT_SHADOW_STATIC_HI_TIMINGS);
        RenderPointLightShadowMapArray(staticLowRes, staticLowResShadowMaps, drawInfoSet.staticLowResShadowMapDrawCommands, "PointShadowStaticLowResFaceData", "PointShadowStaticLowResDrawCommands", POINT_SHADOW_STATIC_LOW_TIMINGS);

        OpenGLShadowCubeMapArray* hiResSource = staticCacheEnabled ? staticHiRes : nullptr;
        OpenGLShadowCubeMapArray* lowResSource = staticCacheEnabled ? staticLowRes : nullptr;

        if (!compositeHiResShadowMaps.empty()) {
            ProfilerOpenGLZone("PointShadow Seed Hi");
            PreparePointLightShadowMapArray(compositeHiRes, hiResSource, compositeHiResShadowMaps);
        }
        if (!compositeLowResShadowMaps.empty()) {
            ProfilerOpenGLZone("PointShadow Seed Low");
            PreparePointLightShadowMapArray(compositeLowRes, lowResSource, compositeLowResShadowMaps);
        }
        RenderPointLightShadowMapArray(compositeHiRes, compositeHiResShadowMaps, drawInfoSet.compositeHiResShadowMapDrawCommands, "PointShadowHiResFaceData", "PointShadowHiResDrawCommands", POINT_SHADOW_HI_TIMINGS);
        RenderPointLightShadowMapArray(compositeLowRes, compositeLowResShadowMaps, drawInfoSet.compositeLowResShadowMapDrawCommands, "PointShadowLowResFaceData", "PointShadowLowResDrawCommands", POINT_SHADOW_LOW_TIMINGS);
    }

    void PreparePointLightShadowMapArray(OpenGLShadowCubeMapArray* destination, OpenGLShadowCubeMapArray* source, const std::vector<ShadowMapInfo>& shadowMapInfoSet) {
        if (!destination) return;

        // A source seeds a cached composite
        // No source selects the uncached "clear and render" path

        for (const ShadowMapInfo& shadowMapInfo : shadowMapInfoSet) {
            const int shadowMapIndex = shadowMapInfo.shadowMapIndex;
            if (shadowMapIndex == -1) continue;

            if (source) {
                BlitShadowCubeMapArray(*source, *destination, shadowMapIndex, shadowMapIndex, shadowMapInfo.faceMask);
            }
            else {
                destination->ClearDepthFaces(shadowMapIndex, shadowMapInfo.faceMask, 1.0f);
            }
        }
    }

    void RenderPointLightShadowMapArray(OpenGLShadowCubeMapArray* shadowMaps, const std::vector<ShadowMapInfo>& shadowMapInfoSet, const PointLightShadowMapDrawCommands& drawCommands, const char* faceDataBufferName, const char* drawCommandBufferName, const PointShadowTimingLabels& timingLabels) {
        if (!shadowMaps || shadowMapInfoSet.empty()) return;

        OpenGLSSBO* faceDataBuffer = OpenGL::ResourceManager::GetSSBOPtr(faceDataBufferName);
        OpenGLSSBO* drawCommandBuffer = OpenGL::ResourceManager::GetSSBOPtr(drawCommandBufferName);
        OpenGLSSBO* drawFaceDataIndexBuffer = OpenGL::ResourceManager::GetSSBOPtr("PointShadowDrawFaceIndices");
        if (!faceDataBuffer || !drawCommandBuffer || !drawFaceDataIndexBuffer) return;

        OpenGLMeshBuffer& meshBufferAssets = OpenGL::ResourceManager::GetMeshBuffer("AssetGeometry");
        OpenGLMeshBuffer& meshBufferProcedural = OpenGL::ResourceManager::GetMeshBuffer("Procedural");

        // This zone is explicitly ended before upload/draw so every displayed point-shadow timing is exclusive.
        GetTimer().BeginZone(timingLabels.prepare, OpenGLFrameTimer::s_DefaultColor);

        OpenGLRasterizerState solidShadowState;
        solidShadowState.depthMask = true;
        solidShadowState.depthTestEnabled = true;
        solidShadowState.depthFunc = GL_LESS;
        solidShadowState.blendEnable = false;
        solidShadowState.cullfaceEnable = true;
        solidShadowState.cullfaceMode = GL_FRONT;

        OpenGLRasterizerState alphaShadowState = solidShadowState;
        alphaShadowState.cullfaceEnable = false;

        std::vector<OpenGLPointShadowFaceData> faceData;
        std::vector<DrawIndexedIndirectCommand> proceduralCommands;
        std::vector<DrawIndexedIndirectCommand> opaqueAssetCommands;
        std::vector<DrawIndexedIndirectCommand> opaqueSkinnedCommands;
        std::vector<DrawIndexedIndirectCommand> alphaTestedAssetCommands;
        std::vector<DrawIndexedIndirectCommand> alphaTestedSkinnedCommands;
        std::vector<uint32_t> proceduralFaceDataIndices;
        std::vector<uint32_t> opaqueAssetFaceDataIndices;
        std::vector<uint32_t> opaqueSkinnedFaceDataIndices;
        std::vector<uint32_t> alphaTestedAssetFaceDataIndices;
        std::vector<uint32_t> alphaTestedSkinnedFaceDataIndices;
        faceData.reserve(shadowMapInfoSet.size() * POINT_SHADOW_FACE_COUNT);

        const size_t drawCommandCount = GetPointShadowDrawCommandCount(drawCommands, shadowMapInfoSet, shadowMaps->GetLayerCount());
        proceduralCommands.reserve(drawCommandCount / 5);
        opaqueAssetCommands.reserve(drawCommandCount / 5);
        opaqueSkinnedCommands.reserve(drawCommandCount / 5);
        alphaTestedAssetCommands.reserve(drawCommandCount / 5);
        alphaTestedSkinnedCommands.reserve(drawCommandCount / 5);
        proceduralFaceDataIndices.reserve(drawCommandCount / 5);
        opaqueAssetFaceDataIndices.reserve(drawCommandCount / 5);
        opaqueSkinnedFaceDataIndices.reserve(drawCommandCount / 5);
        alphaTestedAssetFaceDataIndices.reserve(drawCommandCount / 5);
        alphaTestedSkinnedFaceDataIndices.reserve(drawCommandCount / 5);

        for (const ShadowMapInfo& shadowMapInfo : shadowMapInfoSet) {
            if (shadowMapInfo.shadowMapIndex < 0 || shadowMapInfo.shadowMapIndex >= MAX_SHADOW_MAP_ARRAY_LEVELS || shadowMapInfo.shadowMapIndex >= static_cast<int32_t>(shadowMaps->GetLayerCount())) continue;

            Light* light = Unloved::World::GetLightByObjectId(shadowMapInfo.lightId);
            if (!light) continue;

            const uint32_t shadowMapIndex = static_cast<uint32_t>(shadowMapInfo.shadowMapIndex);
            for (uint32_t faceIndex = 0; faceIndex < POINT_SHADOW_FACE_COUNT; faceIndex++) {
                if ((shadowMapInfo.faceMask & uint8_t(1u << faceIndex)) == 0) continue;
                const uint32_t faceDataIndex = static_cast<uint32_t>(faceData.size());
                OpenGLPointShadowFaceData& currentFaceData = faceData.emplace_back();
                currentFaceData.projectionView = light->GetProjectionView(faceIndex);
                currentFaceData.lightPositionRadius = glm::vec4(light->GetPosition(), light->GetRadius());
                currentFaceData.arrayLayer.x = shadowMapIndex * POINT_SHADOW_FACE_COUNT + faceIndex;

                AppendPointShadowDrawCommands(proceduralCommands, proceduralFaceDataIndices, faceDataIndex, { &drawCommands.procedural[shadowMapIndex][faceIndex] });
                AppendPointShadowDrawCommands(opaqueAssetCommands, opaqueAssetFaceDataIndices, faceDataIndex, { &drawCommands.assetGeometry[shadowMapIndex][faceIndex], &drawCommands.assetGeometrySkinnedNonDeforming[shadowMapIndex][faceIndex] });
                AppendPointShadowDrawCommands(opaqueSkinnedCommands, opaqueSkinnedFaceDataIndices, faceDataIndex, { &drawCommands.assetGeometrySkinned[shadowMapIndex][faceIndex] });
                AppendPointShadowDrawCommands(alphaTestedAssetCommands, alphaTestedAssetFaceDataIndices, faceDataIndex, { &drawCommands.assetGeometryAlphaDiscard[shadowMapIndex][faceIndex], &drawCommands.assetGeometryHair[shadowMapIndex][faceIndex], &drawCommands.assetGeometrySkinnedNonDeformingAlphaDiscard[shadowMapIndex][faceIndex], &drawCommands.assetGeometrySkinnedNonDeformingHair[shadowMapIndex][faceIndex] });
                AppendPointShadowDrawCommands(alphaTestedSkinnedCommands, alphaTestedSkinnedFaceDataIndices, faceDataIndex, { &drawCommands.assetGeometrySkinnedAlphaDiscard[shadowMapIndex][faceIndex], &drawCommands.assetGeometrySkinnedHair[shadowMapIndex][faceIndex] });
            }
        }

        if (faceData.empty() || drawCommandCount == 0) {
            GetTimer().EndZone(timingLabels.prepare);
            return;
        }

        std::vector<DrawIndexedIndirectCommand> combinedCommands;
        std::vector<uint32_t> combinedFaceDataIndices;
        combinedCommands.reserve(drawCommandCount);
        combinedFaceDataIndices.reserve(drawCommandCount);
        const OpenGLPointShadowDrawBatch proceduralBatch = AppendPointShadowDrawBatch(combinedCommands, combinedFaceDataIndices, proceduralCommands, proceduralFaceDataIndices);
        const OpenGLPointShadowDrawBatch opaqueAssetBatch = AppendPointShadowDrawBatch(combinedCommands, combinedFaceDataIndices, opaqueAssetCommands, opaqueAssetFaceDataIndices);
        const OpenGLPointShadowDrawBatch opaqueSkinnedBatch = AppendPointShadowDrawBatch(combinedCommands, combinedFaceDataIndices, opaqueSkinnedCommands, opaqueSkinnedFaceDataIndices);
        const OpenGLPointShadowDrawBatch alphaTestedAssetBatch = AppendPointShadowDrawBatch(combinedCommands, combinedFaceDataIndices, alphaTestedAssetCommands, alphaTestedAssetFaceDataIndices);
        const OpenGLPointShadowDrawBatch alphaTestedSkinnedBatch = AppendPointShadowDrawBatch(combinedCommands, combinedFaceDataIndices, alphaTestedSkinnedCommands, alphaTestedSkinnedFaceDataIndices);

        if (combinedCommands.empty()) {
            GetTimer().EndZone(timingLabels.prepare);
            return;
        }

        GetTimer().EndZone(timingLabels.prepare);

        {
            ProfilerOpenGLZone(timingLabels.upload);
            faceDataBuffer->Update(sizeof(OpenGLPointShadowFaceData) * faceData.size(), faceData.data());
            drawCommandBuffer->Update(sizeof(DrawIndexedIndirectCommand) * combinedCommands.size(), combinedCommands.data());
            drawFaceDataIndexBuffer->Update(sizeof(uint32_t) * combinedFaceDataIndices.size(), combinedFaceDataIndices.data());
        }
        faceDataBuffer->Bind(SSBO_IDX_POINT_SHADOW_FACE_DATA);
        drawFaceDataIndexBuffer->Bind(SSBO_IDX_POINT_SHADOW_DRAW_FACE_INDICES);
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, drawCommandBuffer->GetHandle());

        glViewport(0, 0, shadowMaps->GetSize(), shadowMaps->GetSize());
        glBindFramebuffer(GL_FRAMEBUFFER, shadowMaps->GetHandle());
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadowMaps->GetDepthTexture(), 0);

        OpenGL::BindShader("ShadowCubeMap");
        OpenGL::RasterizerStateManager::SetRasterizerState(solidShadowState);

        if (proceduralBatch.count > 0) {
            glBindVertexArray(meshBufferProcedural.GetVAO());
            DrawPointShadowBatch(proceduralBatch, timingLabels.procedural);
        }
        if (opaqueAssetBatch.count > 0) {
            glBindVertexArray(meshBufferAssets.GetVAO());
            DrawPointShadowBatch(opaqueAssetBatch, timingLabels.opaqueAsset);
        }
        if (opaqueSkinnedBatch.count > 0) {
            glBindVertexArray(OpenGL::BackEnd::GetSkinnedVertexDataVAO());
            glBindBuffer(GL_ARRAY_BUFFER, OpenGL::BackEnd::GetSkinnedVertexDataVBO());
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshBufferAssets.GetEBO());
            DrawPointShadowBatch(opaqueSkinnedBatch, timingLabels.opaqueSkinned);
        }

        if (alphaTestedAssetBatch.count > 0 || alphaTestedSkinnedBatch.count > 0) {
            OpenGL::BindShader("ShadowCubeMapAlphaDiscard");
            OpenGL::RasterizerStateManager::SetRasterizerState(alphaShadowState);

            if (alphaTestedAssetBatch.count > 0) {
                glBindVertexArray(meshBufferAssets.GetVAO());
                DrawPointShadowBatch(alphaTestedAssetBatch, timingLabels.alphaAsset);
            }
            if (alphaTestedSkinnedBatch.count > 0) {
                glBindVertexArray(OpenGL::BackEnd::GetSkinnedVertexDataVAO());
                glBindBuffer(GL_ARRAY_BUFFER, OpenGL::BackEnd::GetSkinnedVertexDataVBO());
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshBufferAssets.GetEBO());
                DrawPointShadowBatch(alphaTestedSkinnedBatch, timingLabels.alphaSkinned);
            }
        }
    }


    void RenderMoonLightCascadedShadowMaps() {
        ProfilerOpenGLZoneFunction();

        const DrawCommandsSet& drawInfoSet = Unloved::RenderDataManager::GetDrawInfoSet();

        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("ShadowMap");
        OpenGLShadowMapArray* shadowMapArray = OpenGL::ResourceManager::GetShadowMapArrayPtr("MoonlightCSM");

        if (!shader) return;
        if (!shadowMapArray) return;

        MeshBuffer& meshBufferProcedural = Hell::ResourceManager::GetMeshBuffer("Procedural");
        OpenGLMeshBuffer& glMeshBufferAssets = OpenGL::ResourceManager::GetMeshBuffer("AssetGeometry");
        OpenGLMeshBuffer& glMeshBufferProcedural = OpenGL::ResourceManager::GetMeshBuffer("Procedural");

        int viewportCount = std::min(4, Unloved::Session::GetLocalPlayerCount());

        OpenGLRasterizerState state;
        state.depthMask = true;
        state.depthTestEnabled = true;
        state.blendEnable = false;
        state.cullfaceEnable = false;
        OpenGL::RasterizerStateManager::ForceRasterizerState(state);

        for (int j = 0; j < viewportCount; j++) {
            Unloved::Player* player = Unloved::Session::GetLocalPlayerByViewportIndex(j);
            if (!player || !player->ViewportIsVisible()) continue;

            const ViewportData& viewportData = Unloved::RenderDataManager::GetViewportData()[j];

            OpenGL::BindShader("ShadowMap");
            OpenGL::BindSSBO(SSBO_IDX_SCENE_RENDER_ITEMS, "SceneRenderItems");
            OpenGL::BindSSBO(SSBO_IDX_DRAW_RENDER_ITEM_INDICES, "DrawRenderItemIndices");
            OpenGL::SetUniformBool("u_useDrawRenderItemIndices", false);

            size_t numLayers = SHADOW_CASCADE_COUNT;

            shadowMapArray->Bind();
            shadowMapArray->SetViewport();

            //glEnable(GL_CULL_FACE);
            //glCullFace(GL_FRONT);  // peter panning

            for (size_t i = 0; i < numLayers; ++i) {

                //int textureLayer = i + (viewportCount * j * numLayers);
                int textureLayer = int(i) + (j * int(numLayers)); // numLayers == SHADOW_CASCADE_COUNT

                shadowMapArray->SetTextureLayer(textureLayer);
                shadowMapArray->ClearDepth();

                const glm::mat4& lightProjectionView = viewportData.csmLightProjectionView[i];

                OpenGL::SetUniformMat4("u_projectionView", lightProjectionView);

                // Geometry
                glBindVertexArray(glMeshBufferAssets.GetVAO());

                OpenGL::SetUniformBool("u_useDrawRenderItemIndices", true);
                MultiDrawIndirect(drawInfoSet.moonLightCascades[j][i]);

                OpenGL::SetUniformBool("u_useDrawRenderItemIndices", false);
                OpenGL::SetUniformMat4("u_modelMatrix", glm::mat4(1.0f));

                // Procedural
                glBindVertexArray(glMeshBufferProcedural.GetVAO());

                //glDisable(GL_CULL_FACE);
                const std::vector<RenderItem>& sceneRenderItems = Unloved::RenderDataManager::GetSceneRenderItems();
                const std::vector<uint32_t>& renderItemIndices = Unloved::RenderDataManager::GetRenderItemIndicesProcedural();
                for (uint32_t renderItemIndex : renderItemIndices) {
                    const RenderItem& renderItem = sceneRenderItems[renderItemIndex];
                    Mesh* mesh = meshBufferProcedural.GetMeshById(renderItem.meshId);
                    if (!mesh) continue;

                    int indexCount = mesh->indexCount;
                    int baseVertex = renderItem.baseVertex;
                    int baseIndex = renderItem.baseIndex;
                    glDrawElementsBaseVertex(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * baseIndex), baseVertex);
                }

                // Weather boards
                //MeshBuffer weatherboardMeshBuffer = LegacyWorld::GetWeatherBoardMeshBuffer();
                //glBindVertexArray(weatherboardMeshBuffer.GetGLMeshBuffer().GetVAO());
                //int indexCount = weatherboardMeshBuffer.GetGLMeshBuffer().GetIndexCount();
                //if (indexCount > 0) {
                //    int baseIndex = 0;
                //    int baseVertex = 0;
                //    glDrawElementsBaseVertex(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * baseIndex), baseVertex);
                //}
            }
        }
        glCullFace(GL_BACK);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}
