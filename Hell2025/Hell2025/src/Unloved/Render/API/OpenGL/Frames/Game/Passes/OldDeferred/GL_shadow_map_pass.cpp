#include "Hell/Render/API/OpenGL/GL_back_end.h"
#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Render/Renderer.h"
#include "Unloved/Viewport/ViewportManager.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Unloved/Objects/Lighting/Light.h"
#include "Unloved/Systems/ShadowMaps/ShadowMapManager.h"
#include "Unloved/World/World.h"
#include "World/LegacyWorld.h"

using namespace Hell;

namespace OpenGL::Renderer {
    using namespace Unloved;


    void RenderFlashLightShadowMaps();
    void RenderPointLightShadowMaps();
    void RenderPointLightShadowMapArray(OpenGLShadowCubeMapArray* shadowMaps, const std::vector<ShadowMapInfo>& shadowMapInfoSet, const PointLightShadowMapDrawCommands& drawCommands);
    void RenderMoonLightCascadedShadowMaps();

    void RenderShadowMaps() {
        RenderFlashLightShadowMaps();
        RenderPointLightShadowMaps();
        RenderMoonLightCascadedShadowMaps();
    }

    void RenderFlashLightShadowMaps() {
        ProfilerOpenGLZoneFunction();

        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("ShadowMap");
        OpenGLShadowMap* shadowMapsFBO = OpenGL::ResourceManager::GetShadowMapPtr("FlashlightShadowMaps");
        OpenGLHeightMapMesh& heightMapMesh = OpenGL::BackEnd::GetHeightMapMesh();
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

        for (int i = 0; i < Unloved::Session::GetLocalPlayerCount(); i++) {
            shadowMapsFBO->BindLayer(i);
            shadowMapsFBO->ClearLayer(i);

            glm::mat4 lightProjectionView = Unloved::Session::GetLocalPlayerByViewportIndex(i)->GetFlashlightProjectionView();
            OpenGL::SetUniformMat4("u_projectionView", lightProjectionView);

            Unloved::Frustum frustum;
            frustum.Update(lightProjectionView);

            // Scene geometry
            OpenGL::SetUniformBool("u_useInstanceData", true);
            glCullFace(GL_FRONT);
            glBindVertexArray(glMeshBufferAssets.GetVAO());

            MultiDrawIndirect(flashLightShadowMapDrawInfo.flashlightShadowMapGeometry[i]);

            // Heightfield chunks
            std::vector<HeightMapChunk>& chunks = LegacyWorld::GetHeightMapChunks();
            OpenGLHeightMapMesh& heightMapMesh = OpenGL::BackEnd::GetHeightMapMesh();
            glBindVertexArray(heightMapMesh.GetVAO());
            OpenGL::SetUniformMat4("u_modelMatrix", heightMapModelMatrix);
            OpenGL::SetUniformBool("u_useInstanceData", false);

            for (uint32_t chunkIndex : flashLightShadowMapDrawInfo.heightMapChunkIndices[i]) {
                HeightMapChunk& chunk = chunks[chunkIndex];
                int indexCount = INDICES_PER_CHUNK;
                int baseVertex = 0;
                int baseIndex = chunk.baseIndex;
                void* indexOffset = (GLvoid*)(baseIndex * sizeof(GLuint));
                int instanceCount = 1;
                int viewportIndex = i;
                if (indexCount > 0) {
                    glDrawElementsInstancedBaseVertexBaseInstance(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, indexOffset, instanceCount, baseVertex, viewportIndex);
                }
            }

            // Procedural
            OpenGL::SetUniformMat4("u_modelMatrix", glm::mat4(1.0f));

            glBindVertexArray(glMeshBufferProcedural.GetVAO());

            const std::vector<RenderItem>& renderItems = Unloved::RenderDataManager::GetRenderItemsProcedural();
            for (const RenderItem& renderItem : renderItems) {

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

    void RenderPointLightShadowMaps() {
        ProfilerOpenGLZoneFunction();

        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("ShadowCubeMap");
        if (!shader) return;

        const std::vector<ShadowMapInfo>& staticHiResShadowMaps = ShadowMapManager::GetStaticDirtyHiResShadowMaps();
        const std::vector<ShadowMapInfo>& staticLowResShadowMaps = ShadowMapManager::GetStaticDirtyLowResShadowMaps();
        const std::vector<ShadowMapInfo>& dynamicHiResShadowMaps = ShadowMapManager::GetDynamicDirtyHiResShadowMaps();
        const std::vector<ShadowMapInfo>& dynamicLowResShadowMaps = ShadowMapManager::GetDynamicDirtyLowResShadowMaps();

        if (staticHiResShadowMaps.empty() && staticLowResShadowMaps.empty() && dynamicHiResShadowMaps.empty() && dynamicHiResShadowMaps.empty()) return;

        OpenGL::BindShader("ShadowCubeMap");

		OpenGLRasterizerState state;
		state.depthMask = true;
		state.depthTestEnabled = true;
		state.blendEnable = false;
		state.cullfaceEnable = true;
		state.cullfaceMode = GL_FRONT;
        OpenGL::RasterizerStateManager::ForceRasterizerState(state);

        const DrawCommandsSet& drawInfoSet = Unloved::RenderDataManager::GetDrawInfoSet();


        RenderPointLightShadowMapArray(OpenGL::ResourceManager::GetShadowCubeMapArrayPtr("StaticHiRes"), staticHiResShadowMaps, drawInfoSet.staticHiResShadowMapDrawCommands);
        RenderPointLightShadowMapArray(OpenGL::ResourceManager::GetShadowCubeMapArrayPtr("StaticLowRes"), staticLowResShadowMaps, drawInfoSet.staticLowResShadowMapDrawCommands);

        // Now for any dynamic shadow maps, first copy their static partner into it

        for (const ShadowMapInfo& shadowMapInfo : dynamicHiResShadowMaps) {
            int32_t shadowMapIndex = shadowMapInfo.shadowMapIndex;

            OpenGLShadowCubeMapArray& src = OpenGL::ResourceManager::GetShadowCubeMapArray("StaticHiRes");
            OpenGLShadowCubeMapArray& dst = OpenGL::ResourceManager::GetShadowCubeMapArray("HiRes");

            BlitShadowCubeMapArray(src, dst, shadowMapIndex, shadowMapIndex);
        }

        for (const ShadowMapInfo& shadowMapInfo : dynamicLowResShadowMaps) {
            int32_t shadowMapIndex = shadowMapInfo.shadowMapIndex;

            OpenGLShadowCubeMapArray& src = OpenGL::ResourceManager::GetShadowCubeMapArray("StaticLowRes");
            OpenGLShadowCubeMapArray& dst = OpenGL::ResourceManager::GetShadowCubeMapArray("LowRes");

            BlitShadowCubeMapArray(src, dst, shadowMapIndex, shadowMapIndex);
        }


        // No render the dynamic objects on top

        RenderPointLightShadowMapArray(OpenGL::ResourceManager::GetShadowCubeMapArrayPtr("HiRes"), dynamicHiResShadowMaps, drawInfoSet.dynamicHiResShadowMapDrawCommands);
        RenderPointLightShadowMapArray(OpenGL::ResourceManager::GetShadowCubeMapArrayPtr("LowRes"), dynamicLowResShadowMaps, drawInfoSet.dynamicLowResShadowMapDrawCommands);


       // PointLightShadowMapDrawCommands staticHiResShadowMapDrawCommands;
       // PointLightShadowMapDrawCommands staticLowResShadowMapDrawCommands;
       // PointLightShadowMapDrawCommands dynamicHiResShadowMapDrawCommands;
       // PointLightShadowMapDrawCommands dynamicLowResShadowMapDrawCommands;

        //RenderPointLightShadowMapArray(OpenGL::ResourceManager::GetShadowCubeMapArrayPtr("HiRes"), hiResShadowMaps, drawInfoSet.hiResShadowMapDrawCommands);
        //RenderPointLightShadowMapArray(OpenGL::ResourceManager::GetShadowCubeMapArrayPtr("LowRes"), lowResShadowMaps, drawInfoSet.lowResShadowMapDrawCommands);
    }

    void RenderPointLightShadowMapArray(OpenGLShadowCubeMapArray* shadowMaps, const std::vector<ShadowMapInfo>& shadowMapInfoSet, const PointLightShadowMapDrawCommands& drawCommands) {
        if (!shadowMaps) return;
        if (shadowMapInfoSet.empty()) return;

        OpenGLMeshBuffer& meshBufferAssets = OpenGL::ResourceManager::GetMeshBuffer("AssetGeometry");
        OpenGLMeshBuffer& meshBufferProcedural = OpenGL::ResourceManager::GetMeshBuffer("Procedural");

        OpenGLRasterizerState solidShadowState;
        solidShadowState.depthMask = true;
        solidShadowState.depthTestEnabled = true;
        solidShadowState.blendEnable = false;
        solidShadowState.cullfaceEnable = true;
        solidShadowState.cullfaceMode = GL_FRONT;

        OpenGLRasterizerState alphaShadowState = solidShadowState;
        alphaShadowState.cullfaceEnable = false;

        // Clear any shadow map that needs redrawing
        for (const ShadowMapInfo& shadowMapInfo : shadowMapInfoSet) {
            int shadowMapIndex = shadowMapInfo.shadowMapIndex;
            if (shadowMapIndex == -1) continue;

            shadowMaps->ClearDepthLayer(shadowMapIndex, 1.0f);
        }

        glViewport(0, 0, shadowMaps->GetSize(), shadowMaps->GetSize());
        glBindFramebuffer(GL_FRAMEBUFFER, shadowMaps->GetHandle());

        for (const ShadowMapInfo& shadowMapInfo : shadowMapInfoSet) {
            int shadowMapIndex = shadowMapInfo.shadowMapIndex;
            if (shadowMapIndex == -1) continue;

            Light* light = Unloved::World::GetLightByObjectId(shadowMapInfo.lightId);
            if (!light) continue;

            OpenGL::SetUniformFloat("farPlane", light->GetRadius());
            OpenGL::SetUniformVec3("lightPosition", light->GetPosition());
            OpenGL::SetUniformMat4("shadowMatrices[0]", light->GetProjectionView(0));
            OpenGL::SetUniformMat4("shadowMatrices[1]", light->GetProjectionView(1));
            OpenGL::SetUniformMat4("shadowMatrices[2]", light->GetProjectionView(2));
            OpenGL::SetUniformMat4("shadowMatrices[3]", light->GetProjectionView(3));
            OpenGL::SetUniformMat4("shadowMatrices[4]", light->GetProjectionView(4));
            OpenGL::SetUniformMat4("shadowMatrices[5]", light->GetProjectionView(5));

            for (int face = 0; face < 6; ++face) {
                GLuint layer = shadowMapIndex * 6 + face;
                OpenGL::SetUniformInt("faceIndex", face);
                glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadowMaps->GetDepthTexture(), 0, layer);

                OpenGL::SetUniformBool("u_useInstanceData", true);

                OpenGL::RasterizerStateManager::SetRasterizerState(solidShadowState);
                glBindVertexArray(meshBufferProcedural.GetVAO());
                MultiDrawIndirect(drawCommands.procedural[shadowMapIndex][face]);
                glBindVertexArray(meshBufferAssets.GetVAO());
                MultiDrawIndirect(drawCommands.assetGeometry[shadowMapIndex][face]);

                OpenGL::RasterizerStateManager::SetRasterizerState(alphaShadowState);
                MultiDrawIndirect(drawCommands.assetGeometryAlphaDiscard[shadowMapIndex][face]);
                MultiDrawIndirect(drawCommands.assetGeometryHair[shadowMapIndex][face]);

                OpenGL::RasterizerStateManager::SetRasterizerState(solidShadowState);
                MultiDrawIndirect(drawCommands.assetGeometrySkinnedNonDeforming[shadowMapIndex][face]);

                OpenGL::RasterizerStateManager::SetRasterizerState(alphaShadowState);
                MultiDrawIndirect(drawCommands.assetGeometrySkinnedNonDeformingAlphaDiscard[shadowMapIndex][face]);
                MultiDrawIndirect(drawCommands.assetGeometrySkinnedNonDeformingHair[shadowMapIndex][face]);

                glBindVertexArray(OpenGL::BackEnd::GetSkinnedVertexDataVAO());
                glBindBuffer(GL_ARRAY_BUFFER, OpenGL::BackEnd::GetSkinnedVertexDataVBO());
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshBufferAssets.GetEBO());

                OpenGL::RasterizerStateManager::SetRasterizerState(solidShadowState);
                MultiDrawIndirect(drawCommands.assetGeometrySkinned[shadowMapIndex][face]);

                OpenGL::RasterizerStateManager::SetRasterizerState(alphaShadowState);
                MultiDrawIndirect(drawCommands.assetGeometrySkinnedAlphaDiscard[shadowMapIndex][face]);
                MultiDrawIndirect(drawCommands.assetGeometrySkinnedHair[shadowMapIndex][face]);
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
            OpenGL::SetUniformBool("u_useInstanceData", false);

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

                OpenGL::SetUniformBool("u_useInstanceData", true);
                MultiDrawIndirect(drawInfoSet.moonLightCascades[j][i]);

                OpenGL::SetUniformBool("u_useInstanceData", false);
                OpenGL::SetUniformMat4("u_modelMatrix", glm::mat4(1.0f));

                // Procedural
                glBindVertexArray(glMeshBufferProcedural.GetVAO());

                //glDisable(GL_CULL_FACE);
                const std::vector<RenderItem>& renderItems = Unloved::RenderDataManager::GetRenderItemsProcedural();
                for (const RenderItem& renderItem : renderItems) {
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
