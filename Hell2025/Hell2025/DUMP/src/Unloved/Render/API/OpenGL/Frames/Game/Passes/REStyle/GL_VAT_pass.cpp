#include "Hell/Debug/DebugDraw.h"
#include "Hell/Common/Color.h"
#include "Hell/Input/Input.h"
#include "Hell/Input/keycodes.h"
#include "Hell/Logging.h"
#include "Hell/Math/LocalFrame.h"
#include "Hell/Math/Transform.h"
#include "Hell/Render/API/OpenGL/GL_rasterizer_state_manager.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/Time.h"

#include "Unloved/Debug/Debug.h"
#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Unloved/Objects/Renderables/PointAnimationInstance.h"
#include "Unloved/Objects/Renderables/VATInstance.h"
#include "Unloved/Render/RendererConstants.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Render/Renderer.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Systems/Blood/BloodSystem.h"
#include "Unloved/Viewport/ViewportManager.h"


#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace OpenGL::Renderer {

    void VATPass() {
        ProfilerOpenGLZoneFunction();

        Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("AssetGeometry");

        OpenGLFrameBuffer* gBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("GBuffer");
        if (!gBuffer) return;

        gBuffer->Bind();
        gBuffer->DrawBuffers({ "BaseColorMetallic", "NormalXYRoughnessMisc", "VelocityXYOcclusionSubSurface" });

        BindShader("VAT");

        OpenGL::BindSSBO(SSBO_IDX_SAMPLERS, "Samplers");
        OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");
        OpenGLRasterizerState state;
        state.depthTestEnabled = true;
        state.blendEnable = false;
        state.cullfaceEnable = false;
        state.depthMask = true;
        state.depthFunc = GL_GREATER;

        OpenGL::RasterizerStateManager::SetRasterizerState(state);

        const float deltaTime = Hell::Time::DeltaTime();

        glBindVertexArray(OpenGL::ResourceManager::GetMeshBuffer("AssetGeometry").GetVAO());

        const std::vector<VATRenderItem>& renderItems = Unloved::BloodSystem::GetVATRenderItems();

        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            OpenGL::Renderer::SetViewport(gBuffer, viewport);
            OpenGL::SetUniformInt("u_viewportIndex", i);

            for (const VATRenderItem& renderItem : renderItems) {

                SetUniformMat4("u_modelMatrix", renderItem.modelMatrix);
                SetUniformMat4("u_inverseModelMatrix", renderItem.inverseModelMatrix);
                SetUniformFloat("u_time", renderItem.currentTime);
                SetUniformFloat("u_fps", renderItem.fps);
                SetUniformInt("u_frameCount", renderItem.frameCount);
                SetUniformVec3("u_boundsMin", renderItem.boundsMin);
                SetUniformVec3("u_boundsMax", renderItem.boundsMax);
                SetUniformInt("u_positionTextureIndex", renderItem.positionTextureIdx);
                SetUniformInt("u_rotationTextureIndex", renderItem.rotationTextureIdx);
                SetUniformInt("u_lookupTextureIndex", renderItem.lookupTextureIdx);
                SetUniformBool("u_mirror", renderItem.mirror);

                glDrawElementsBaseVertex(GL_TRIANGLES, renderItem.indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * renderItem.baseIndex), renderItem.baseVertex);
            }
        }
    }
}
