#include "Hell/ResourceManagement/ResourceManager.h"

#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Viewport/ViewportManager.h"

namespace OpenGL::Renderer {

    void PhysicsShapesPass() {
        const DrawCommandsSet& drawCommandsSet = Unloved::RenderDataManager::GetDrawInfoSet();
        const std::vector<ViewportData>& viewportData = Unloved::RenderDataManager::GetViewportData();

        OpenGLFrameBuffer& gBuffer = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
        gBuffer.DrawBuffers({ "BaseColorMetallic", "NormalXYRoughnessMisc", "VelocityXYOcclusionSubSurface" });

        OpenGL::BindShader("PhysicsShapes");
        OpenGL::BindSSBO(SSBO_IDX_SCENE_RENDER_ITEMS, "SceneRenderItems");
        OpenGL::BindSSBO(SSBO_IDX_DRAW_RENDER_ITEM_INDICES, "DrawRenderItemIndices");

        OpenGLRasterizerState state;
        state.depthTestEnabled = true;
        state.blendEnable = false;
        state.cullfaceEnable = true;
        state.depthMask = true;
        state.colorMask = true;
        state.depthFunc = GL_GREATER;

        state.stencilTestEnabled = true;
        state.stencilFunc = GL_ALWAYS;
        state.stencilReadMask = 0xFF;
        state.stencilWriteMask = 0xFF;
        state.stencilFailOp = GL_KEEP;
        state.stencilDepthFailOp = GL_KEEP;
        state.stencilPassOp = GL_REPLACE;
        state.stencilRef = STENCIL_BIT_WORLD_LIGHTING;

        OpenGL::RasterizerStateManager::SetRasterizerState(state);

        OpenGLMeshBuffer& physicsShapeGeometry = OpenGL::ResourceManager::GetMeshBuffer("PhysicsShapeGeometry");
        glBindVertexArray(physicsShapeGeometry.GetVAO());

        for (int viewportIndex = 0; viewportIndex < 4; viewportIndex++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(viewportIndex);
            if (!viewport || !viewport->IsVisible()) continue;

            const std::vector<DrawIndexedIndirectCommand>& drawCommands = drawCommandsSet.physicsShapes[viewportIndex];
            if (drawCommands.empty()) continue;

            OpenGL::Renderer::SetViewport(&gBuffer, viewport);
            OpenGL::SetUniformMat4("u_projectionView", viewportData[viewportIndex].jitteredProjectionViewReverseZ);
            OpenGL::Renderer::MultiDrawIndirect(drawCommands);
        }
    }

}
