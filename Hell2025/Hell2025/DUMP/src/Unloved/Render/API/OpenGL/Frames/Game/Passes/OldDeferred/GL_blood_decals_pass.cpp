#include "Hell/Audio.h"
#include "Hell/Debug/DebugDraw.h"
#include "Hell/Input.h"
#include "Hell/ResourceManagement/ResourceManager.h"

#include "Unloved/EditorSession/EditorSession.h"
#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Systems/Blood/BloodSystem.h"
#include "Unloved/Viewport/ViewportManager.h"

#include "World/LegacyWorld.h"

namespace OpenGL::Renderer {
    void BloodDecalTileCulling();
    void BloodDecalDraw();
    void BloodDecalComposite();
    void DecalTestPass();
    void NewBloodsDecalsPass();

    void BloodDecalsPass() {
        DecalTestPass();
        //NewBloodsDecalsPass();
        //
        //// DO NOT RENDER OLD BLOOD
        //return;

        if (Unloved::RenderDataManager::GetBloodScreenSpaceDecalInstanceData().empty()) {
            return;
        }

        BloodDecalTileCulling();
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        BloodDecalDraw();
        glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

        BloodDecalComposite();
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
    }

    void BloodDecalDraw() {
        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer* miscFullSizeFBO = OpenGL::ResourceManager::GetFrameBufferPtr("MiscFullSize");
        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("BloodDecalsDraw");
        OpenGLFrameBuffer* gBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("GBuffer");

        if (!miscFullSizeFBO) return;
        if (!shader) return;
        if (!gBuffer) return;

        miscFullSizeFBO->Bind();
        miscFullSizeFBO->SetViewport();
        miscFullSizeFBO->DrawBuffers({ "BloodScreenSpaceDecalMask" });

        OpenGL::BindShader("BloodDecalsDraw");
        OpenGL::BindSSBO(SSBO_IDX_SAMPLERS, "Samplers");
        OpenGL::BindSSBO(SSBO_IDX_RENDERER_DATA, "RendererData");
        OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");
        OpenGL::SetUniformInt("u_tileXCount", GetTileCountX());
        OpenGL::SetUniformInt("u_tileYCount", GetTileCountY());

        OpenGLRasterizerState state;
        state.depthTestEnabled = false;
        state.depthMask = false;
        state.blendEnable = false;
        state.cullfaceEnable = false;
        state.colorMask = true;
        OpenGL::RasterizerStateManager::ForceRasterizerState(state);

        OpenGL::BindSSBO(SSBO_IDX_BLOOD_DRAW_TILE_DECALS, "TileBloodDecals");
        OpenGL::BindSSBO(SSBO_IDX_BLOOD_DRAW_DECALS, "BloodDecalInstances");
        OpenGL::BindSSBO(SSBO_IDX_BLOOD_DRAW_INDEX_POOL, "BloodDecalIndices");

        //glBindTextureUnit(0, gBuffer->GetColorAttachmentHandleByName("RMA"));
        glBindTextureUnit(1, gBuffer->GetColorAttachmentHandleByName("NormalXYRoughnessMisc"));
        glBindTextureUnit(2, gBuffer->GetDepthAttachmentHandle());

        // Draw full screen triangle
        BindEmptyVAO();
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }

    void BloodDecalComposite() {
        ProfilerOpenGLZoneFunction();

        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("BloodDecalsComposite");
        OpenGLFrameBuffer* miscFullSizeFBO = OpenGL::ResourceManager::GetFrameBufferPtr("MiscFullSize");
        OpenGLFrameBuffer* gBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("GBuffer");

        if (!miscFullSizeFBO) return;
        if (!shader) return;
        if (!gBuffer) return;

        OpenGL::BindShader("BloodDecalsComposite");

        glBindImageTexture(0, gBuffer->GetColorAttachmentHandleByName("BaseColorMetallic"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
        glBindImageTexture(1, gBuffer->GetColorAttachmentHandleByName("NormalXYRoughnessMisc"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGB10_A2);
        glBindImageTexture(2, gBuffer->GetColorAttachmentHandleByName("VelocityXYOcclusionSubSurface"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
        glBindTextureUnit(3, miscFullSizeFBO->GetColorAttachmentHandleByName("BloodScreenSpaceDecalMask"));
        OpenGL::DispatchCompute((gBuffer->GetWidth() + 7) / 8, (gBuffer->GetHeight() + 7) / 8, 1);
    }


    void DecalTestPass() {
        ProfilerOpenGLZoneFunction();

        struct TestDecal {
            glm::vec3 position = glm::vec3(0.0f);
            glm::vec3 normal = glm::vec3(0.0f, 0.0f, 1.0f);
            float scale = 0.5f;
        };

        static std::vector<TestDecal> decals;

        Unloved::Player* player = Unloved::Session::GetLocalPlayerByViewportIndex(0);
        if (!player) return;

        glm::vec3 pos = player->GetInteractHitPosition();
        glm::vec3 normal = player->GetInteractHitNormal();

        if (Hell::Input::KeyPressed(HELL_KEY_T) && Unloved::EditorSession::IsInactive()) {
            Hell::Audio::PlayAudio("Spray.wav", 1.0f);
            TestDecal& decal = decals.emplace_back();
            decal.position = pos;
            decal.normal = normal;
        }

        OpenGLFrameBuffer* gBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("GBuffer");
        if (!gBuffer) return;

        gBuffer->Bind();
        gBuffer->DrawBuffers({ "BaseColorMetallic", "NormalXYRoughnessMisc", "VelocityXYOcclusionSubSurface" });

        Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("AssetGeometry");

        Model* model = Hell::ResourceManager::GetModelByName("Cube");
        if (!model) return;
        if (model->GetMeshIndices().empty()) return;

        Mesh* mesh = meshBuffer.GetMeshById(model->GetMeshIndices()[0]);
        if (!mesh) return;

        BindShader("DecalTest");

        glBindTextureUnit(1, gBuffer->GetDepthAttachmentHandle());
        OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");

        OpenGLRasterizerState state;
        state.depthTestEnabled = true;
        state.blendEnable = false;
        state.cullfaceEnable = false;
        state.depthMask = false;
        state.depthFunc = GL_GREATER;

        OpenGL::RasterizerStateManager::SetRasterizerState(state);

        // Replace the decal material channels directly in the G-buffer.
        glColorMaski(0, GL_TRUE,  GL_TRUE,  GL_TRUE,  GL_TRUE);
        glColorMaski(1, GL_TRUE,  GL_TRUE,  GL_TRUE,  GL_TRUE);
        glColorMaski(2, GL_FALSE, GL_FALSE, GL_TRUE,  GL_FALSE);

        glBindVertexArray(OpenGL::ResourceManager::GetMeshBuffer("AssetGeometry").GetVAO());

        Texture* texture = Hell::ResourceManager::GetTextureByName("HalfLife");
        if (!texture) return;

        glBindTextureUnit(0, texture->GetGLTexture().GetHandle());

        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            OpenGL::Renderer::SetViewport(gBuffer, viewport);
            OpenGL::SetUniformInt("u_viewportIndex", i);

            for (TestDecal& decal : decals) {

                Hell::LocalFrame localFrame = Hell::LocalFrame(decal.normal);
                Hell::QuatTransform transform = Hell::QuatTransform(decal.position, localFrame, glm::vec3(decal.scale));

                const glm::mat4 modelMatrix = transform.ToMat4();
                const glm::mat4 inverseModelMatrix = glm::inverse(modelMatrix);

                SetUniformMat4("u_modelMatrix", modelMatrix);
                SetUniformMat4("u_inverseModelMatrix", inverseModelMatrix);

                glDrawElementsBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int)* mesh->baseIndex), mesh->baseVertex);
            }
        }

        glColorMaski(0, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glColorMaski(1, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glColorMaski(2, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

        //for (TestDecal& decal : decals) {
        //    Hell::DebugDraw::DrawPoint(decal.position, RED);
        //    Hell::DebugDraw::DrawLine(decal.position, decal.position + (decal.normal * 0.1f), RED);
        //}
    }


    void NewBloodsDecalsPass() {
        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer* gBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("GBuffer");
        if (!gBuffer) return;

        gBuffer->Bind();
        gBuffer->DrawBuffers({ "BaseColorMetallic", "NormalXYRoughnessMisc", "VelocityXYOcclusionSubSurface" });

        Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("AssetGeometry");

        Model* model = Hell::ResourceManager::GetModelByName("Cube");
        if (!model) return;
        if (model->GetMeshIndices().empty()) return;

        Mesh* mesh = meshBuffer.GetMeshById(model->GetMeshIndices()[0]);
        if (!mesh) return;

        BindShader("BloodDecalsNew");

        glBindTextureUnit(1, gBuffer->GetDepthAttachmentHandle());
        OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");

        OpenGLRasterizerState state;
        state.depthTestEnabled = true;
        state.blendEnable = false;
        state.cullfaceEnable = false;
        state.depthMask = false;
        state.depthFunc = GL_GREATER;

        OpenGL::RasterizerStateManager::SetRasterizerState(state);

        // Replace the decal material channels directly in the G-buffer.
        glColorMaski(0, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glColorMaski(1, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glColorMaski(2, GL_FALSE, GL_FALSE, GL_TRUE, GL_FALSE);

        glBindVertexArray(OpenGL::ResourceManager::GetMeshBuffer("AssetGeometry").GetVAO());

        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            OpenGL::Renderer::SetViewport(gBuffer, viewport);
            OpenGL::SetUniformInt("u_viewportIndex", i);

            for (const TestBloodDecal& decal : Unloved::BloodSystem::GetBloodDecals()) {
                Texture* texture = Hell::ResourceManager::GetTextureByBindlessIndex(static_cast<int32_t>(decal.textureIdx));

                glBindTextureUnit(0, texture->GetGLTexture().GetHandle());

                SetUniformMat4("u_modelMatrix", decal.modelMatrix);
                SetUniformMat4("u_inverseModelMatrix", decal.inverseModelMatrix);

                glDrawElementsBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), mesh->baseVertex);
            }
        }

        glColorMaski(0, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glColorMaski(1, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glColorMaski(2, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    }
}
