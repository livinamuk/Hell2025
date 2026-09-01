#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Unloved/EditorSession/HeightMap/EditorHeightMap.h"
#include "Unloved/EditorSession/EditorSession.h"
#include "Unloved/Viewport/ViewportManager.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Render/Renderer.h"
#include "Unloved/Render/RendererConstants.h"

#include "Unloved/EditorSession/Gizmo/Gizmo.h"

namespace OpenGL::Renderer {
    using namespace Unloved;

    void HeightMapBrushPreviewPass() {
        if (EditorSession::IsInactive()) return;

        const EditorSession::HeightMapEditor::BrushPreview& preview = EditorSession::HeightMapEditor::GetBrushPreview();
        if (!preview.visible || preview.viewportIndex < 0 || preview.viewportIndex >= 4) return;

        OpenGLFrameBuffer* gBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("GBuffer");
        OpenGLFrameBuffer* worldFbo = OpenGL::ResourceManager::GetFrameBufferPtr("World");
        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("HeightMapBrushPreview");
        Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(preview.viewportIndex);
        if (!gBuffer || !worldFbo || !shader || !viewport || !viewport->IsVisible()) return;

        gBuffer->Bind();
        gBuffer->DrawBuffer("Lighting");
        OpenGL::Renderer::SetViewport(gBuffer, viewport);

        OpenGL::BindShader("HeightMapBrushPreview");
        OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");
        OpenGL::BindTextureUnit(0, gBuffer->GetDepthAttachmentHandle());
        OpenGL::BindTextureUnit(1, worldFbo->GetColorAttachmentHandleByName("HeightMap"));
        OpenGL::SetUniformInt("u_viewportIndex", preview.viewportIndex);
        OpenGL::SetUniformVec3("u_brushPosition", preview.position);
        OpenGL::SetUniformFloat("u_brushRadius", preview.radius);
        OpenGL::SetUniformBool("u_validateTerrainHeight", Unloved::Renderer::GetRendererMode() == RendererMode::OLD_DEFERRED);

        OpenGLRasterizerState state;
        state.depthTestEnabled = false;
        state.depthMask = false;
        state.cullfaceEnable = false;
        state.blendEnable = true;
        state.blendFuncSrcfactor = GL_SRC_ALPHA;
        state.blendFuncDstfactor = GL_ONE_MINUS_SRC_ALPHA;
        state.stencilTestEnabled = Unloved::Renderer::GetRendererMode() == RendererMode::RE_STYLE;
        state.stencilFunc = GL_EQUAL;
        state.stencilRef = STENCIL_VERTEX_BUFFER_HEIGHT_MAP;
        state.stencilReadMask = STENCIL_VERTEX_BUFFER_MASK;
        state.stencilWriteMask = 0x00;
        OpenGL::RasterizerStateManager::SetRasterizerState(state);
        DrawFullscreenTriangle();

        state.blendEnable = false;
        state.stencilTestEnabled = false;
        OpenGL::RasterizerStateManager::SetRasterizerState(state);
    }

    void EditorPass() {
        const std::vector<ViewportData>& viewportData = Unloved::RenderDataManager::GetViewportData();

        std::string gBufferName = (Unloved::Renderer::GetRendererMode() == RendererMode::RE_STYLE) ? "GBuffer" : "GBuffer";
        OpenGLFrameBuffer& gBuffer = OpenGL::ResourceManager::GetFrameBuffer(gBufferName);

        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("SolidColor");

        if (!shader) return;
        if (EditorSession::IsInactive()) return;

        OpenGLRasterizerState state;
        state.depthMask = true;
        state.depthTestEnabled = true;
        state.blendEnable = true;
        state.depthFunc = GL_GREATER;
        OpenGL::RasterizerStateManager::ForceRasterizerState(state);

        gBuffer.Bind();
        gBuffer.DrawBuffers({ "Lighting" });
        gBuffer.SetViewport();
        gBuffer.ClearDepthAttachment(0.0f);

        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (viewport->IsVisible()) {

                OpenGL::Renderer::SetViewport(&gBuffer, viewport);

                OpenGL::BindShader("SolidColor");
                OpenGL::SetUniformMat4("projection", viewportData[i].projectionReverseZ);
                OpenGL::SetUniformMat4("view", viewportData[i].view);
                OpenGL::SetUniformBool("useUniformColor", true);

                for (GizmoRenderItem& renderItem : Gizmo::GetRenderItemsByViewportIndex(i)) {
                    MeshBufferOLD* mesh = Gizmo::GetMeshBufferByIndex(renderItem.meshIndex);
                    if (mesh) {
                        OpenGLMeshBufferOLD glMesh = mesh->GetGLMeshBuffer();
                        OpenGL::SetUniformMat4("model", renderItem.modelMatrix);
                        OpenGL::SetUniformVec4("uniformColor", renderItem.color);
                        glBindVertexArray(glMesh.GetVAO());
                        glDrawElements(GL_TRIANGLES, glMesh.GetIndexCount(), GL_UNSIGNED_INT, 0);
                    }
                }
            }
        }

        // Cleanup
        glDisable(GL_BLEND);
    }
}
