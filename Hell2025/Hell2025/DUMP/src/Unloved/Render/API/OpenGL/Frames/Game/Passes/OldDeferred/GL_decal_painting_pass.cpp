#include "Hell/Math/GLM.h"
#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Hell/Render/API/OpenGL/GL_back_end.h"
#include "Unloved/EditorSession/EditorSession.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Hell/ResourceManagement/ResourceManager.h"


namespace OpenGL::Renderer {
    using namespace Unloved;

    namespace {
        glm::mat4 CreateDecalProjectionViewReverseZ(const DecalPaintingInfo& decalPaintingInfo) {
            const glm::vec2 decalSize = glm::vec2(0.15f * 0.5f);
            const float zNear = 0.001f;
            const float zFar = 50.1f;

            const glm::vec3 rayDirection = decalPaintingInfo.rayDirection;
            glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
            if (glm::abs(glm::dot(rayDirection, worldUp)) > 0.99f) {
                worldUp = glm::vec3(1.0f, 0.0f, 0.0f);
            }

            const glm::mat4 view = glm::lookAt(decalPaintingInfo.rayOrigin, decalPaintingInfo.rayOrigin + rayDirection, worldUp);
            const float halfW = decalSize.x * 0.5f;
            const float halfH = decalSize.y * 0.5f;
            const glm::mat4 projection = glm::orthoZO(-halfW, halfW, -halfH, halfH, zFar, zNear);

            return projection * view;
        }
    }


    void DecalPaintingPass() {
        ProfilerOpenGLZoneFunction();

        if (EditorSession::IsActive()) return;

        OpenGLFrameBuffer* decalPaintingFBO = OpenGL::ResourceManager::GetFrameBufferPtr("DecalPainting");
        OpenGLShader* uvShader = OpenGL::ResourceManager::GetShaderPtr("DecalPaintUVs");
        OpenGLShader* maskShader = OpenGL::ResourceManager::GetShaderPtr("DecalPaintMask");
        Hell::TextureArray* woundMaskArray = Hell::ResourceManager::GetTextureArrayPtr("WoundMasks");

        if (!decalPaintingFBO) return;
        if (!uvShader) return;
        if (!maskShader) return;
        if (!woundMaskArray) return;

        const std::vector<DecalPaintingInfo>& decalPaintingInfoSet = Unloved::RenderDataManager::GetDecalPaintingInfo();
        if (decalPaintingInfoSet.empty()) return;

        const std::vector<RenderItem>& sceneRenderItems = Unloved::RenderDataManager::GetSceneRenderItems();
        const std::vector<uint32_t>& skinnedRenderItemIndices = Unloved::RenderDataManager::GetCombinedSkinnedRenderItemIndices();
        if (skinnedRenderItemIndices.empty()) return;

        decalPaintingFBO->Bind();
        decalPaintingFBO->SetViewport();
        decalPaintingFBO->DrawBuffer("UVMap");

        OpenGLRasterizerState state;
        state.depthTestEnabled = true;
        state.depthMask = true;
        state.cullfaceEnable = false;
        state.blendEnable = false;
        state.colorMask = true;
        state.depthFunc = GL_GREATER;
        RasterizerStateManager::SetRasterizerState(state);

        glBindVertexArray(OpenGL::BackEnd::GetSkinnedVertexDataVAO());

        for (const DecalPaintingInfo& decalPaintingInfo : decalPaintingInfoSet) {
            const glm::mat4 projectionView = CreateDecalProjectionViewReverseZ(decalPaintingInfo);

            for (uint32_t renderItemIndex : skinnedRenderItemIndices) {
                const RenderItem& renderItem = sceneRenderItems[renderItemIndex];

                // Bail if this no wound mask texture index
                if (renderItem.woundMaskTextureIndex == -1) continue;

                // Bail if mesh is invalid
                Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(renderItem.meshId);
                if (!mesh) continue;

                // Clear depth and the UV map for every mesh
                decalPaintingFBO->ClearTexImage("UVMap", 0, 0, 0, 1);
                decalPaintingFBO->ClearDepthAttachment(0.0f);

                // Render the UVs
                BindShader("DecalPaintUVs");
                SetUniformMat4("u_projectionView", projectionView);
                SetUniformMat4("u_model", renderItem.modelMatrix);

                glDrawElementsInstancedBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (GLvoid*)(mesh->baseIndex * sizeof(GLuint)), 1, renderItem.baseVertex);
                glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

                // Render the mask
                BindShader("DecalPaintMask");
                SetUniformInt("u_layerIndex", renderItem.woundMaskTextureIndex);
                BindImageTextureArray(1, woundMaskArray->GetHandle(), GL_READ_WRITE, GL_R8);
                BindTextureUnit(1, decalPaintingFBO->GetColorAttachmentHandleByName("UVMap"));
                BindTextureUnit(2, GetTextureHandleByName("Decal_Wound0"));

                glDispatchCompute((decalPaintingFBO->GetWidth() + 7) / 8, (decalPaintingFBO->GetHeight() + 7) / 8, 1);
                glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
            }
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

}
