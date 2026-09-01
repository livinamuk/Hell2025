#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Hell/Render/API/OpenGL/GL_back_end.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Viewport/ViewportManager.h"
#include "World/LegacyWorld.h"
#include "Unloved/ObjectId.h"
#include "Unloved/Config/Config.h"
#include "Unloved/World/World.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/Time.h"

namespace OpenGL::Renderer {
    using namespace Unloved;


    void WinstonPass() {
        ProfilerOpenGLZoneFunction();

        const std::vector<ViewportData>& viewportData = Unloved::RenderDataManager::GetViewportData();

        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("Winston");
        OpenGLFrameBuffer* gBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("GBuffer");

        static float time = 0.0f;
        time += Hell::Time::DeltaTime();

        OpenGL::BindShader("Winston");
        OpenGL::SetUniformVec3("color", { 0, 0.9f, 1 });
        OpenGL::SetUniformFloat("alpha", 0.01f);
        OpenGL::SetUniformVec2("screensize", gBuffer->GetWidth(), gBuffer->GetHeight());
        OpenGL::SetUniformFloat("near", Config::GetNearPlane());
        OpenGL::SetUniformFloat("far", Config::GetFarPlane());
        OpenGL::SetUniformFloat("u_time", time);

        gBuffer->Bind();
        gBuffer->DrawBuffer("Lighting");

        OpenGLRasterizerState rasterizerState;
        rasterizerState.depthTestEnabled = true;
        rasterizerState.depthMask = false;
        rasterizerState.depthFunc = GL_EQUAL;
        rasterizerState.blendEnable = true;
        rasterizerState.blendFuncSrcfactor = GL_SRC_ALPHA;
        rasterizerState.blendFuncDstfactor = GL_ONE_MINUS_SRC_ALPHA;
        rasterizerState.cullfaceEnable = false;
        rasterizerState.colorMask = true;
        rasterizerState.stencilTestEnabled = false;
        OpenGL::RasterizerStateManager::ForceRasterizerState(rasterizerState);

        glBindTextureUnit(0, gBuffer->GetDepthAttachmentHandle());
        glBindVertexArray(OpenGL::ResourceManager::GetMeshBuffer("AssetGeometry").GetVAO());

        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            OpenGL::Renderer::SetViewport(gBuffer, viewport);

            glm::mat4 projectionViewMatrix = viewportData[i].jitteredProjectionViewReverseZ;
            glm::mat4 viewMatrix = viewportData[i].view;

            OpenGL::SetUniformMat4("projectionView", projectionViewMatrix);
            OpenGL::SetUniformMat4("view", viewMatrix);
            OpenGL::SetUniformBool("useUniformColor", false);

            Unloved::Player* player = Unloved::Session::GetLocalPlayerByViewportIndex(i);
            if (!player) continue;

            if (player->InteractFound()) {

                uint64_t interactObjectId = player->GetInteractObjectId();
                ObjectType interactObjectType = Unloved::GetObjectIdType(interactObjectId);

                if (interactObjectType == ObjectType::PICK_UP) {
                    PickUp* pickUp = Unloved::World::GetPickUpByObjectId(interactObjectId);
                    if (pickUp) {
                        const std::vector<RenderItem>& renderItems = pickUp->GetRenderItems();

                        for (const RenderItem& renderItem : renderItems) {

                            OpenGL::SetUniformMat4("model", renderItem.modelMatrix);

                            Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(renderItem.meshId);
                            if (!mesh) continue;

                            glDrawElementsBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), mesh->baseVertex);
                        }
                    }
                }
            }
        }

    }
}
