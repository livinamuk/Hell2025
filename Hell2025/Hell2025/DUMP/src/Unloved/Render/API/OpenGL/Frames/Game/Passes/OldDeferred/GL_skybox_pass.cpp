#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Hell/Render/API/OpenGL/GL_back_end.h"
#include "Unloved/EditorSession/EditorSession.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Viewport/ViewportManager.h"
#include "Hell/ResourceManagement/ResourceManager.h"

namespace OpenGL::Renderer {

    void SkyBoxPass() {
        if (Unloved::EditorSession::IsActive()) return;

        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer& gBuffer = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
        OpenGLCubemapView* skyboxCubemapView = OpenGL::ResourceManager::GetCubemapViewPtr("SkyboxNightSky");

        gBuffer.Bind();
        gBuffer.DrawBuffers({ "Lighting" });

        OpenGL::BindShader("SkyboxRE");
        OpenGL::BindSSBO(SSBO_IDX_RENDERER_DATA, "RendererData");
        OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");

        OpenGLRasterizerState state;
        state.depthTestEnabled = false;
        state.blendEnable = false;
        state.cullfaceEnable = false;
        state.depthMask = false;
        state.depthFunc = GL_GREATER;

        OpenGL::RasterizerStateManager::SetRasterizerState(state);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxCubemapView->GetHandle());
        glBindVertexArray(OpenGL::ResourceManager::GetMeshBuffer("AssetGeometry").GetVAO());

        for (int32_t viewportIndex = 0; viewportIndex < 4; viewportIndex++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(viewportIndex);
            if (!viewport || !viewport->IsVisible()) continue;

            SetViewport(&gBuffer, viewport);
            RenderFullscreenTriangle();
        }
    }
}
