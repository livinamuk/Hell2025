#include "Unloved/Render/API/OpenGL/GL_renderer.h"

#include "Hell/Backend/BackEnd.h"
#include "Hell/Render/API/OpenGL/GL_back_end.h"
#include "Hell/UI/UIBackEnd.h"
#include "Unloved/Render/RenderDataManager.h"

namespace OpenGL::Renderer {
    namespace {
        void DrawUICanvas(UICanvas canvas) {
            const std::vector<DrawIndexedIndirectCommand>& drawCommands = Unloved::RenderDataManager::GetDrawCommandsUI(canvas);
            if (drawCommands.empty()) return;

            const glm::uvec2 resolution = UIBackEnd::GetCanvasResolution(canvas);

            OpenGL::BindShader("UI");
            OpenGL::BindSSBO(SSBO_IDX_SAMPLERS, "Samplers");
            OpenGL::BindSSBO(SSBO_IDX_UI_INSTANCE_DATA, "RenderItemsUI");
            OpenGL::SetUniformFloat("u_renderTargetWidth", static_cast<float>(resolution.x));
            OpenGL::SetUniformFloat("u_renderTargetHeight", static_cast<float>(resolution.y));
            OpenGL::SetUniformInt("u_flipY", canvas == UICanvas::NATIVE ? 1 : 0);

            glEnable(GL_BLEND);
            glBlendEquation(GL_FUNC_ADD);
            glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

            glEnable(GL_CLIP_DISTANCE0);
            glEnable(GL_CLIP_DISTANCE1);
            glEnable(GL_CLIP_DISTANCE2);
            glEnable(GL_CLIP_DISTANCE3);

            glDisable(GL_CULL_FACE);
            glDisable(GL_DEPTH_TEST);

            OpenGLGenericMesh& genericMesh = OpenGL::ResourceManager::GetGenericMesh("UI");
            glBindVertexArray(genericMesh.GetVAO());
            MultiDrawIndirect(drawCommands);

            glDisable(GL_CLIP_DISTANCE0);
            glDisable(GL_CLIP_DISTANCE1);
            glDisable(GL_CLIP_DISTANCE2);
            glDisable(GL_CLIP_DISTANCE3);

            glBindVertexArray(0);
        }
    }

    void GameUIPass() {
        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer& presentFbo = OpenGL::ResourceManager::GetFrameBuffer("Present");
        presentFbo.Bind();
        presentFbo.SetViewport();
        presentFbo.DrawBuffer("Color");
        DrawUICanvas(UICanvas::INTERNAL);
    }

    void EditorUIPass() {
        ProfilerOpenGLZoneFunction();

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDrawBuffer(GL_BACK);
        glViewport(0, 0, Hell::BackEnd::GetDrawableWidth(), Hell::BackEnd::GetDrawableHeight());
        glDisable(GL_SCISSOR_TEST);
        DrawUICanvas(UICanvas::NATIVE);
    }
}
