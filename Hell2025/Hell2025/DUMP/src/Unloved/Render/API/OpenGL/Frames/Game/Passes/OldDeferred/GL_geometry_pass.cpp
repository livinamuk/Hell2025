#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Unloved/Render/Renderer.h"
#include "Unloved/Render/RenderDataManager.h"
#include "World/LegacyWorld.h"

#include "Hell/Backend/BackEnd.h"
#include "Hell/Logging.h"
#include "Hell/Physics/Physics.h"
#include "Hell/Render/API/OpenGL/GL_back_end.h"
#include "Hell/ResourceManagement/ResourceManager.h"

#include "Unloved/Session/Session.h"
#include "Unloved/Systems/Mirrors/MirrorManager.h"
#include "Unloved/Viewport/ViewportManager.h"
#include "Unloved/World/World.h"

// get me out of here

using namespace Hell;

namespace OpenGL::Renderer {
    using namespace Unloved;

    void RenderNonDeformingSkinnedGameObjects();

	void ProceduralGeometryPass() {
		ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer* gBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("GBuffer");
        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("GBuffer");

        if (!gBuffer) return;
        if (!shader) return;

        gBuffer->Bind();
        //gBuffer->DrawBuffers({ "BaseColor", "Normal", "RMA", "Emissive", "VelocityXYOcclusionSubSurface" });
        gBuffer->DrawBuffers({ "BaseColorMetallic", "NormalXYRoughnessMisc", "Emissive", "VelocityXYOcclusionSubSurface" });
        OpenGL::RasterizerStateManager::ForceRasterizerState("GeometryPass_Default");

        OpenGL::BindShader("GBuffer");
        OpenGL::BindSSBO(SSBO_IDX_SAMPLERS, "Samplers");
        OpenGL::BindSSBO(SSBO_IDX_MATERIALS, "Materials");
        OpenGL::BindSSBO(SSBO_IDX_RENDERER_DATA, "RendererData");
        OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");
        OpenGL::BindSSBO(SSBO_IDX_SCENE_RENDER_ITEMS, "SceneRenderItems");
        OpenGL::BindSSBO(SSBO_IDX_DRAW_RENDER_ITEM_INDICES, "DrawRenderItemIndices");
        OpenGL::SetUniformMat4("u_model", glm::mat4(1));
        OpenGL::SetUniformBool("u_flipNormalMapY", ShouldFlipNormalMapY());
        OpenGL::SetUniformBool("u_alphaDiscard", false);

        //MeshBuffer& houseMeshBuffer = LegacyWorld::GetHouseMeshBuffer();
        //OpenGLMeshBuffer& glHouseMeshBuffer = houseMeshBuffer.GetGLMeshBuffer();
        //glBindVertexArray(glHouseMeshBuffer.GetVAO());

        OpenGLMeshBuffer& meshBuffer = OpenGL::ResourceManager::GetMeshBuffer("Procedural");
        glBindVertexArray(meshBuffer.GetVAO());

        const DrawCommandsSet& drawInfoSet = Unloved::RenderDataManager::GetDrawInfoSet();
        const std::vector<ViewportData>& viewportData = Unloved::RenderDataManager::GetViewportData();

        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            OpenGL::Renderer::SetViewport(gBuffer, viewport);
            OpenGL::SetUniformInt("u_viewportIndex", i);
            MultiDrawIndirect(drawInfoSet.procedural[i]);
        }
    }


    void RenderNonDeformingSkinnedGameObjects() {
        OpenGLFrameBuffer* gBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("GBuffer");
        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("GBuffer");

        if (!gBuffer) return;
        if (!shader) return;

        gBuffer->Bind();
        //gBuffer->DrawBuffers({ "BaseColor", "Normal", "RMA", "Emissive", "VelocityXYOcclusionSubSurface" });
        gBuffer->DrawBuffers({ "BaseColorMetallic", "NormalXYRoughnessMisc", "Emissive", "VelocityXYOcclusionSubSurface" });

        OpenGL::BindShader("GBuffer");
        OpenGL::BindSSBO(SSBO_IDX_SAMPLERS, "Samplers");
        OpenGL::BindSSBO(SSBO_IDX_MATERIALS, "Materials");
        OpenGL::BindSSBO(SSBO_IDX_RENDERER_DATA, "RendererData");
        OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");
        OpenGL::BindSSBO(SSBO_IDX_SCENE_RENDER_ITEMS, "SceneRenderItems");
        OpenGL::BindSSBO(SSBO_IDX_DRAW_RENDER_ITEM_INDICES, "DrawRenderItemIndices");

        //glBindVertexArray(OpenGL::BackEnd::GetWeightedVertexDataVAO());
        //glBindBuffer(GL_ARRAY_BUFFER, OpenGL::BackEnd::GetWeightedVertexDataVBO());
        //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, OpenGL::BackEnd::GetWeightedVertexDataEBO());
        OpenGLMeshBuffer& meshBuffer = OpenGL::ResourceManager::GetMeshBuffer("AssetGeometry");
        glBindVertexArray(meshBuffer.GetVAO());
        glBindBuffer(GL_ARRAY_BUFFER, meshBuffer.GetVBO());
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshBuffer.GetEBO());

        const DrawCommandsSet& drawInfoSet = Unloved::RenderDataManager::GetDrawInfoSet();
        const std::vector<ViewportData>& viewportData = Unloved::RenderDataManager::GetViewportData();

        // Default
        OpenGL::SetUniformBool("u_alphaDiscard", false);
        OpenGL::RasterizerStateManager::ForceRasterizerState("GeometryPass_Default");
        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            OpenGL::Renderer::SetViewport(gBuffer, viewport);
            OpenGL::SetUniformInt("u_viewportIndex", i);
            MultiDrawIndirect(drawInfoSet.skinnedNonDeformingStandard[i]);
            MultiDrawIndirect(drawInfoSet.skinnedNonDeformingViewWeaponStandard[i]);
        }

        // Alpha Discard
        OpenGL::SetUniformBool("u_alphaDiscard", true);
        OpenGL::RasterizerStateManager::ForceRasterizerState("GeometryPass_AlphaDiscard");
        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            OpenGL::Renderer::SetViewport(gBuffer, viewport);
            OpenGL::SetUniformInt("u_viewportIndex", i);
            MultiDrawIndirect(drawInfoSet.skinnedNonDeformingAlphaDiscard[i]);
            MultiDrawIndirect(drawInfoSet.skinnedNonDeformingViewWeaponAlphaDiscard[i]);

            // Hair
            glDisable(GL_CULL_FACE);
            MultiDrawIndirect(drawInfoSet.skinnedNonDeformingHair[i]);
        }

        // Blended
        OpenGL::SetUniformBool("u_alphaDiscard", false);
        gBuffer->DrawBuffers({ "BaseColorMetallic" });
        //gBuffer->DrawBuffers({ "BaseColor" });
        OpenGL::RasterizerStateManager::ForceRasterizerState("GeometryPass_Blended");
        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            OpenGL::Renderer::SetViewport(gBuffer, viewport);
            OpenGL::SetUniformInt("u_viewportIndex", i);
            MultiDrawIndirect(drawInfoSet.skinnedNonDeformingBlended[i]);
        }
    }


    void GeometryPass() {
        ProfilerOpenGLZoneFunction();

        const DrawCommandsSet& drawInfoSet = Unloved::RenderDataManager::GetDrawInfoSet();
        const std::vector<ViewportData>& viewportData = Unloved::RenderDataManager::GetViewportData();

        OpenGLFrameBuffer* gBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("GBuffer");
        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("GBuffer");
        OpenGLShader* editorMeshShader = OpenGL::ResourceManager::GetShaderPtr("EditorMesh");
        Hell::TextureArray* woundMaskArray = Hell::ResourceManager::GetTextureArrayPtr("WoundMasks");

        if (!gBuffer) return;
        if (!shader) return;
        if (!editorMeshShader) return;
        if (!woundMaskArray) return;

        {
            OpenGLMeshBuffer& meshBuffer = OpenGL::ResourceManager::GetMeshBuffer("AssetGeometry");
            glBindVertexArray(meshBuffer.GetVAO());
        }
        glActiveTexture(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_2D_ARRAY, woundMaskArray->GetHandle());


        OpenGL::BindShader("GBuffer");
        OpenGL::BindSSBO(SSBO_IDX_SAMPLERS, "Samplers");
        OpenGL::BindSSBO(SSBO_IDX_MATERIALS, "Materials");
        OpenGL::BindSSBO(SSBO_IDX_RENDERER_DATA, "RendererData");
        OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");
        OpenGL::BindSSBO(SSBO_IDX_SCENE_RENDER_ITEMS, "SceneRenderItems");
        OpenGL::BindSSBO(SSBO_IDX_DRAW_RENDER_ITEM_INDICES, "DrawRenderItemIndices");
        OpenGL::SetUniformBool("u_flipNormalMapY", ShouldFlipNormalMapY());

        OpenGLFrameBuffer* decalMasksFBO = OpenGL::ResourceManager::GetFrameBufferPtr("DecalMasks");

        gBuffer->Bind();
        //gBuffer->DrawBuffers({ "BaseColor", "Normal", "RMA", "Emissive", "VelocityXYOcclusionSubSurface" });
        gBuffer->DrawBuffers({ "BaseColorMetallic", "NormalXYRoughnessMisc", "Emissive", "VelocityXYOcclusionSubSurface" });

        // Default (Non blended)
        OpenGL::SetUniformBool("u_alphaDiscard", false);
        OpenGL::RasterizerStateManager::ForceRasterizerState("GeometryPass_Default");

        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (viewport->IsVisible()) {
                OpenGL::Renderer::SetViewport(gBuffer, viewport);
                OpenGL::SetUniformInt("u_viewportIndex", i);
                MultiDrawIndirect(drawInfoSet.standard[i]);
                MultiDrawIndirect(drawInfoSet.viewWeaponStandard[i]);
            }
        }

        // Alpha discard
        OpenGL::SetUniformBool("u_alphaDiscard", true);
        OpenGL::RasterizerStateManager::ForceRasterizerState("GeometryPass_AlphaDiscard");

        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (viewport->IsVisible()) {
                OpenGL::Renderer::SetViewport(gBuffer, viewport);
                OpenGL::SetUniformInt("u_viewportIndex", i);
                MultiDrawIndirect(drawInfoSet.alphaDiscard[i]);
                MultiDrawIndirect(drawInfoSet.viewWeaponAlphaDiscard[i]);

                // Hair
                glDisable(GL_CULL_FACE);
                MultiDrawIndirect(drawInfoSet.hair[i]);
            }
        }

        // Blended
        OpenGL::SetUniformBool("u_alphaDiscard", false);
        //gBuffer->DrawBuffers({ "BaseColor" });
        gBuffer->DrawBuffers({ "BaseColorMetallic" });
        OpenGL::RasterizerStateManager::ForceRasterizerState("GeometryPass_Blended");

        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (viewport->IsVisible()) {
                OpenGL::Renderer::SetViewport(gBuffer, viewport);
                OpenGL::SetUniformInt("u_viewportIndex", i);
                MultiDrawIndirect(drawInfoSet.blended[i]);
            }
        }


        glBindVertexArray(OpenGL::BackEnd::GetSkinnedVertexDataVAO());
        glBindBuffer(GL_ARRAY_BUFFER, OpenGL::BackEnd::GetSkinnedVertexDataVBO());
        //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, OpenGL::BackEnd::GetWeightedVertexDataEBO());
        {
            OpenGLMeshBuffer& meshBuffer = OpenGL::ResourceManager::GetMeshBuffer("AssetGeometry");
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshBuffer.GetEBO());
        }

        OpenGL::BindShader("GBuffer");
        //gBuffer->DrawBuffers({ "BaseColor", "Normal", "RMA", "Emissive", "VelocityXYOcclusionSubSurface" });
        gBuffer->DrawBuffers({ "BaseColorMetallic", "NormalXYRoughnessMisc", "Emissive", "VelocityXYOcclusionSubSurface" });

        // Skinned mesh (non blended)
        OpenGL::SetUniformBool("u_alphaDiscard", false);
        OpenGL::RasterizerStateManager::ForceRasterizerState("GeometryPass_Default");
        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            OpenGL::Renderer::SetViewport(gBuffer, viewport);
            OpenGL::SetUniformInt("u_viewportIndex", i);
            MultiDrawIndirect(drawInfoSet.skinnedStandard[i]);
            MultiDrawIndirect(drawInfoSet.skinnedViewWeaponStandard[i]);
        }

        // Skinned mesh (alpha discard)
        OpenGL::SetUniformBool("u_alphaDiscard", true);
        OpenGL::RasterizerStateManager::ForceRasterizerState("GeometryPass_AlphaDiscard");
        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            OpenGL::Renderer::SetViewport(gBuffer, viewport);
            OpenGL::SetUniformInt("u_viewportIndex", i);
            MultiDrawIndirect(drawInfoSet.skinnedAlphaDiscard[i]);
            MultiDrawIndirect(drawInfoSet.skinnedViewWeaponAlphaDiscard[i]);

            // Hair
            glDisable(GL_CULL_FACE);
            MultiDrawIndirect(drawInfoSet.skinnedHair[i]);
        }

        // Skinned mesh (alpha blended)
        OpenGL::SetUniformBool("u_alphaDiscard", false);
        gBuffer->DrawBuffers({ "BaseColorMetallic" });
        //gBuffer->DrawBuffers({ "BaseColor" });
        OpenGL::RasterizerStateManager::ForceRasterizerState("GeometryPass_Blended");
        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            OpenGL::Renderer::SetViewport(gBuffer, viewport);
            OpenGL::SetUniformInt("u_viewportIndex", i);
            MultiDrawIndirect(drawInfoSet.skinnedBlended[i]);
        }

        gBuffer->DrawBuffers({ "BaseColorMetallic", "NormalXYRoughnessMisc", "Emissive", "VelocityXYOcclusionSubSurface" });

        // Debug draw ragdolls
        //if (Unloved::Renderer::GetCurrentRendererSettings().debugDrawRagdolls) {
        //    OpenGLShader* physicsShapesShader = OpenGL::ResourceManager::GetShaderPtr("PhysicsShapes");
        //    OpenGL::BindShader("PhysicsShapes");
        //    OpenGL::RasterizerStateManager::ForceRasterizerState("GeometryPass_Default");
        //
        //    MeshBuffer& physicsShapeGeometry = Hell::ResourceManager::GetMeshBuffer("PhysicsShapeGeometry");
        //    OpenGLMeshBuffer& glPhysicsShapeGeometry = OpenGL::ResourceManager::GetMeshBuffer("PhysicsShapeGeometry");
        //    if (physicsShapeGeometry.GetMeshCount() > 0) {
        //        for (int i = 0; i < 4; i++) {
        //            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
        //            if (viewport->IsVisible()) {
        //                OpenGL::Renderer::SetViewport(gBuffer, viewport);
        //
        //                Unloved::Player* player = Unloved::Session::GetLocalPlayerByViewportIndex(i);
        //                if (!player) continue;
        //
        //                OpenGL::SetUniformInt("u_playerIndex", i);
        //                OpenGL::SetUniformMat4("u_projectionView", viewportData[i].jitteredProjectionViewReverseZ);
        //
        //                glBindVertexArray(glPhysicsShapeGeometry.GetVAO());
        //
        //                // Ragdoll
        //                auto& ragdolls = Hell::Physics::GetRagdolls();
        //
        //                for (auto it = ragdolls.begin(); it != ragdolls.end(); ) {
        //                    Ragdoll& ragdoll = it->second;
        //
        //                    // Dont render current viewports ragdoll. It blocks the screen.
        //                    bool skipRendering = (player->GetRagdollId() == it->first);
        //
        //                    if (!skipRendering) {
        //                        for (uint32_t rigidIndex = 0; rigidIndex < ragdoll.m_pxRigidDynamics.size(); rigidIndex++) {
        //                            const uint32_t meshId = ragdoll.GetMarkerMeshIdByRigidIndex(rigidIndex);
        //                            if (meshId == 0) continue;
        //
        //                            Mesh* mesh = physicsShapeGeometry.GetMeshById(meshId);
        //                            if (!mesh) continue;
        //
        //                            glm::mat4 modelMatrix = ragdoll.GetModelMatrixByRigidIndex(rigidIndex);
        //                            OpenGL::SetUniformMat4("u_model", modelMatrix);
        //                            OpenGL::SetUniformVec3("u_color", ragdoll.GetMarkerColorByRigidIndex(rigidIndex));
        //
        //                            glDrawElementsBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), mesh->baseVertex);
        //                        }
        //                    }
        //                    it++;
        //                }
        //            }
        //        }
        //    }
        //}

        glBindVertexArray(0);

        RenderNonDeformingSkinnedGameObjects();
    }

    void MirrorGeometryPass() {
        ProfilerOpenGLZoneFunction();

        const DrawCommandsSet& drawInfoSet = Unloved::RenderDataManager::GetDrawInfoSet();
        const std::vector<ViewportData>& viewportData = Unloved::RenderDataManager::GetViewportData();

        OpenGLFrameBuffer* gBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("GBuffer");
        OpenGLFrameBuffer* gBufferBackup = OpenGL::ResourceManager::GetFrameBufferPtr("GBufferBackup");
        OpenGLShader* geometryShader = OpenGL::ResourceManager::GetShaderPtr("GBuffer");
        OpenGLShader* houseGeometryShader = OpenGL::ResourceManager::GetShaderPtr("DebugTextured");
        OpenGLShader* solidColorShader = OpenGL::ResourceManager::GetShaderPtr("DebugSolidColor");

        if (!gBuffer) return;
        if (!gBufferBackup) return;
        if (!geometryShader) return;
        if (!houseGeometryShader) return;
        if (!solidColorShader) return;

        // Render the mirror mask
        // - First you copy the depth buffer from the GBuffer so you can render your mirror plane against scene depth
        // - Then you just do a standard stencil buffer mask write for each viewport
        OpenGL::BlitFrameBufferDepth(gBuffer, gBufferBackup);

        gBuffer->Bind();
        gBuffer->BindDepthAttachmentFrom(*gBufferBackup);

        OpenGL::BindShader("DebugSolidColor");

        //gBuffer->DrawBuffer("BaseColor");

        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

        glEnable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glDepthMask(GL_FALSE); // Test depth, but don't write it
        glDepthFunc(GL_GEQUAL);

        glEnable(GL_STENCIL_TEST);
        glStencilMask(0xFF);
        glClearStencil(0);
        glClear(GL_STENCIL_BUFFER_BIT);
        glStencilFunc(GL_ALWAYS, 1, 0xFF);
        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

        //glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        //glDisable(GL_DEPTH_TEST);

        MeshBuffer& meshBufferProcedural = Hell::ResourceManager::GetMeshBuffer("Procedural");
        OpenGLMeshBuffer& glMeshBufferAssets = OpenGL::ResourceManager::GetMeshBuffer("AssetGeometry");
        OpenGLMeshBuffer& glMeshBufferProcedural = OpenGL::ResourceManager::GetMeshBuffer("Procedural");

        glBindVertexArray(glMeshBufferAssets.GetVAO());

        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (viewport->IsVisible()) {
                OpenGL::Renderer::SetViewport(gBuffer, viewport);

                Mirror* mirror = Unloved::MirrorManager::GetMirrorByObjectId(viewport->GetMirrorId());
                if (!mirror) continue;

                //mirror->DebugDraw();

                Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(mirror->GetGlobalMeshIndex());
                if (!mesh) continue;

                glm::mat4 modelMatrix = mirror->GetWorldMatrix();

                OpenGL::SetUniformMat4("u_projectionView", viewportData[i].jitteredProjectionViewReverseZ);
                OpenGL::SetUniformMat4("u_model", modelMatrix);

                glDrawElementsBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (GLvoid*)(mesh->baseIndex * sizeof(GLuint)), mesh->baseVertex);
            }
        }

        glDepthMask(GL_TRUE);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

        gBuffer->Bind();
        //gBuffer->DrawBuffers({ "BaseColor", "Normal", "RMA", "Emissive" });
        gBuffer->DrawBuffers({ "BaseColorMetallic", "NormalXYRoughnessMisc", "Emissive", "VelocityXYOcclusionSubSurface" });

        // Clear the depth buffer so that the mirror world has a clean depth state to test against
        gBuffer->ClearDepthAttachment(0.0);

        OpenGL::RasterizerStateManager::ForceRasterizerState("GeometryPass_Default");

        glEnable(GL_CLIP_DISTANCE0);
        glFrontFace(GL_CW);

        glEnable(GL_STENCIL_TEST);
        glStencilFunc(GL_EQUAL, 1, 0xFF);
        glStencilMask(0x00);

        OpenGL::BindShader("GBuffer");
        OpenGL::BindSSBO(SSBO_IDX_SAMPLERS, "Samplers");
        OpenGL::BindSSBO(SSBO_IDX_MATERIALS, "Materials");
        OpenGL::BindSSBO(SSBO_IDX_RENDERER_DATA, "RendererData");
        OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");
        OpenGL::BindSSBO(SSBO_IDX_SCENE_RENDER_ITEMS, "SceneRenderItems");
        OpenGL::BindSSBO(SSBO_IDX_DRAW_RENDER_ITEM_INDICES, "DrawRenderItemIndices");
        OpenGL::SetUniformBool("u_flipNormalMapY", ShouldFlipNormalMapY());

        // Regular geometry
        glBindVertexArray(glMeshBufferAssets.GetVAO());

        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (viewport->IsVisible()) {
                OpenGL::Renderer::SetViewport(gBuffer, viewport);

                Mirror* mirror = Unloved::MirrorManager::GetMirrorByObjectId(viewport->GetMirrorId());
                if (!mirror) continue;

                OpenGL::SetUniformBool("u_useMirrorMatrix", true);
                OpenGL::SetUniformMat4("u_mirrorViewMatrix", mirror->GetViewMatrix(i));
                OpenGL::SetUniformVec4("u_mirrorClipPlane", mirror->GetClipPlane(i));
                OpenGL::SetUniformInt("u_viewportIndex", i);

                OpenGL::Renderer::SetViewport(gBuffer, viewport);
                MultiDrawIndirect(drawInfoSet.mirrorRenderItems[i]);
            }
        }
        OpenGL::SetUniformBool("u_useMirrorMatrix", false);

        // House geometry
        OpenGL::BindShader("DebugTextured");
        OpenGL::SetUniformMat4("u_model", glm::mat4(1));
        OpenGL::SetUniformBool("u_flipNormalMapY", ShouldFlipNormalMapY());

        glBindVertexArray(glMeshBufferProcedural.GetVAO());

        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            Mirror* mirror = Unloved::MirrorManager::GetMirrorByObjectId(viewport->GetMirrorId());
            if (!mirror) continue;

            OpenGL::Renderer::SetViewport(gBuffer, viewport);

            OpenGL::SetUniformInt("u_viewportIndex", i);
            OpenGL::SetUniformBool("u_useMirrorMatrix", true);
            OpenGL::SetUniformMat4("u_mirrorViewMatrix", mirror->GetViewMatrix(i));
            OpenGL::SetUniformVec4("u_mirrorClipPlane", mirror->GetClipPlane(i));

            const std::vector<RenderItem>& sceneRenderItems = Unloved::RenderDataManager::GetSceneRenderItems();
            const std::vector<uint32_t>& renderItemIndices = Unloved::RenderDataManager::GetRenderItemIndicesProcedural();
            for (uint32_t renderItemIndex : renderItemIndices) {
                const RenderItem& renderItem = sceneRenderItems[renderItemIndex];

                Mesh* mesh = meshBufferProcedural.GetMeshById(renderItem.meshId);
                if (!mesh) continue;

                Material* material = Hell::ResourceManager::GetMaterialByIndex(renderItem.materialIndex);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(material->m_basecolor)->GetGLTexture().GetHandle());
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(material->m_normal)->GetGLTexture().GetHandle());
                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(material->m_rma)->GetGLTexture().GetHandle());

                int indexCount = mesh->indexCount;
                int baseVertex = renderItem.baseVertex;
                int baseIndex = renderItem.baseIndex;

                glDrawElementsBaseVertex(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * baseIndex), baseVertex);
            }
        }
        OpenGL::SetUniformBool("u_useMirrorMatrix", false);

        // Clean up
        glStencilMask(0xFF);
        glDisable(GL_CLIP_DISTANCE0);
        glFrontFace(GL_CCW);
        glDisable(GL_STENCIL_TEST);

        gBuffer->BindDepthAttachmentFrom(*gBuffer);
    }
}
