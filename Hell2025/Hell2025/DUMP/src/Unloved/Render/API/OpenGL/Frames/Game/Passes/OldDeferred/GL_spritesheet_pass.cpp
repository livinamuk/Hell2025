#include "Hell/Render/API/OpenGL/GL_back_end.h"
#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Viewport/ViewportManager.h"

#include "Hell/ResourceManagement/ResourceManager.h"

namespace OpenGL::Renderer {
    using namespace Unloved;

    void SpriteSheetPass() {
        ProfilerOpenGLZoneFunction();

        Mesh* mesh = Hell::ResourceManager::GetQuadMesh();
        if (!mesh) return;

        OpenGLFrameBuffer& gBuffer = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");

        gBuffer.Bind();
        gBuffer.DrawBuffer("Lighting");

        OpenGL::BindShader("SpriteSheet");
        OpenGL::BindSSBO(SSBO_IDX_SAMPLERS, "Samplers");
        OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");
        OpenGL::BindSSBO(SSBO_IDX_SPRITE_SHEET_INSTANCE_DATA, "SpriteSheetInstanceData");

        OpenGLRasterizerState rasterizerState;
        rasterizerState.depthTestEnabled = true;
        rasterizerState.depthMask = false;
        rasterizerState.depthFunc = GL_GEQUAL;
        rasterizerState.blendEnable = true;
        rasterizerState.blendFuncSrcfactor = GL_SRC_ALPHA;
        rasterizerState.blendFuncDstfactor = GL_ONE;
        rasterizerState.cullfaceEnable = false;
        rasterizerState.colorMask = true;
        rasterizerState.stencilTestEnabled = false;
        OpenGL::RasterizerStateManager::ForceRasterizerState(rasterizerState);

        glBindVertexArray(OpenGL::ResourceManager::GetMeshBuffer("AssetGeometry").GetVAO());

        const DrawCommandsSet& drawCommandsSet = Unloved::RenderDataManager::GetDrawInfoSet();

        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (!viewport || !viewport->IsVisible()) continue;

            OpenGL::Renderer::SetViewport(&gBuffer, viewport);
            OpenGL::SetUniformInt("u_viewportIndex", i);
            OpenGL::Renderer::MultiDrawIndirect(drawCommandsSet.spriteSheets[i]);
        }
    }
}
