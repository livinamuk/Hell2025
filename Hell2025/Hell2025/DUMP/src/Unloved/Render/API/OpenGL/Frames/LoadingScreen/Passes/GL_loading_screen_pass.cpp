#include "Hell/Render/API/OpenGL/GL_back_end.h"
#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Hell/UI/UIBackEnd.h"

namespace OpenGL::Renderer {

    void RenderBlackFrame() {
        OpenGLFrameBuffer& presentFbo = OpenGL::ResourceManager::GetFrameBuffer("Present");
        presentFbo.ClearAttachment("Color", 0.0f, 0.0f, 0.0f, 1.0f);
        PresentFinalImage(presentFbo);
    }

    void RenderLoadingScreen() {
        const std::vector<GLuint64>& samplers = OpenGL::BackEnd::GetBindlessTextureIDs();
        OpenGL::UpdateSSBO("Samplers", sizeof(GLuint64) * samplers.size(), samplers.data());

        const std::vector<RenderItemUI>& renderItemsUI = UIBackEnd::GetRenderItems();
        OpenGL::UpdateSSBO("RenderItemsUI", renderItemsUI.size() * sizeof(RenderItemUI), renderItemsUI.data());

        OpenGLFrameBuffer& presentFbo = OpenGL::ResourceManager::GetFrameBuffer("Present");
        presentFbo.ClearAttachment("Color", 0.0f, 0.0f, 0.0f, 0.0f);

        GameUIPass();

        PresentFinalImage(presentFbo);
    }
}
