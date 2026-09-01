#include "Unloved/Render/API/OpenGL/GL_renderer.h"

namespace OpenGL::Renderer {

    void DownSampleFinalImage() {
        OpenGLFrameBuffer* gBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("GBuffer");
        OpenGLFrameBuffer* finalImageFbo = OpenGL::ResourceManager::GetFrameBufferPtr("FinalImage");

        int dstWidth = finalImageFbo->GetWidth();
        int dstHeight = finalImageFbo->GetHeight();

        int groupSizeX = 8;
        int groupSizeY = 8;

        int dispatchGroupCountX = (dstWidth + groupSizeX - 1) / groupSizeX;
        int dispatchGroupCountY = (dstHeight + groupSizeY - 1) / groupSizeY;

        OpenGL::BindShader("DownSample2xBox");
        OpenGL::BindImageTexture(0, gBuffer->GetColorAttachmentHandleByName("Lighting"), GL_READ_ONLY, GL_RGBA16F);
        OpenGL::BindImageTexture(1, finalImageFbo->GetColorAttachmentHandleByName("Color"), GL_WRITE_ONLY, GL_RGBA16F);
        OpenGL::BindTextureUnit(2, gBuffer->GetColorAttachmentHandleByName("Lighting"));

        OpenGL::DispatchCompute(dispatchGroupCountX, dispatchGroupCountY, 1);

        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }
}