#include "Unloved/Render/API/OpenGL/GL_renderer.h"

namespace OpenGL::Renderer {

    void GaussianBlur(OpenGLFrameBuffer& srcFrameBuffer, OpenGLFrameBuffer& dstFrameBuffer, const std::string& srcAttachmentName, const std::string& dstAttachmentName, BlitRect srcRect, BlitRect dstRect, int blurRadius, int passCount) {
        OpenGLFrameBuffer* gaussianFrameBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("GaussianBlur");
        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("GaussianBlurUtil");

        if (!gaussianFrameBuffer) return;
        if (!shader) return;

        if (passCount < 1) {
            std::cout << "GaussianBlur() failed: passCount of " << passCount << " must be greater than 0\n";
            return;
        }
        if (blurRadius < 1 || blurRadius > 5) {
            std::cout << "GaussianBlur() failed: blurRadius of " << blurRadius << " must be between 1 and 5\n";
            return;
        }

        // Initialize with src dimensions / 2
        unsigned int width = (srcRect.x1 - srcRect.x0) / 2;
        unsigned int height = (srcRect.y1 - srcRect.y0) / 2;
        width = std::max(1u, width);
        height = std::max(1u, height);

        // Ensure the gaussian framebuffer is big enough to peform this
        if (gaussianFrameBuffer->GetWidth() < width || gaussianFrameBuffer->GetHeight() < height) {
            gaussianFrameBuffer->Resize(width, height);
        }

        // Peform an initial 50% downscale of the source attachment into the gaussian fbo "ColorA" attachment
        BlitRect initialDownscaleRect;
        initialDownscaleRect.x0 = 0;
        initialDownscaleRect.y0 = 0;
        initialDownscaleRect.x1 = width;
        initialDownscaleRect.y1 = height;
        OpenGL::BlitFrameBuffer(&srcFrameBuffer, gaussianFrameBuffer, srcAttachmentName.c_str(), "ColorA", srcRect, initialDownscaleRect, GL_COLOR_BUFFER_BIT, GL_LINEAR);

        // Store gaussian attachment names as local strings for ping ponging
        std::string srcGaussianAtttachmentName = "ColorA";
        std::string dstGaussianAtttachmentName = "ColorB";

        // Begin the 2 pass blur sequence
        OpenGL::BindShader("GaussianBlurUtil");
        OpenGL::SetUniformInt("u_radius", blurRadius);

        for (int i = 0; i < passCount; i++) {
           OpenGL::SetUniformInt("u_width", width);
           OpenGL::SetUniformInt("u_height", height);

           // Horitontal pass
           OpenGL::SetUniformIVec2("u_direction", glm::ivec2(1, 0));
           glBindImageTexture(0, gaussianFrameBuffer->GetColorAttachmentHandleByName(srcGaussianAtttachmentName.c_str()), 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);
           glBindImageTexture(1, gaussianFrameBuffer->GetColorAttachmentHandleByName(dstGaussianAtttachmentName.c_str()), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
           OpenGL::DispatchCompute((width + 15) / 16, (height + 15) / 16, 1);
           glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

           std::swap(srcGaussianAtttachmentName, dstGaussianAtttachmentName);

           // Vertical pass
           OpenGL::SetUniformIVec2("u_direction", glm::ivec2(0, 1));
           glBindImageTexture(0, gaussianFrameBuffer->GetColorAttachmentHandleByName(srcGaussianAtttachmentName.c_str()), 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);
           glBindImageTexture(1, gaussianFrameBuffer->GetColorAttachmentHandleByName(dstGaussianAtttachmentName.c_str()), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
           OpenGL::DispatchCompute((width + 15) / 16, (height + 15) / 16, 1);
           glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

           BlitRect srcDownscaleRect;
           srcDownscaleRect.x0 = 0;
           srcDownscaleRect.y0 = 0;
           srcDownscaleRect.x1 = width;
           srcDownscaleRect.y1 = height;

           BlitRect dstDownscaleRect;
           dstDownscaleRect.x0 = 0;
           dstDownscaleRect.y0 = 0;
           dstDownscaleRect.x1 = width / 2;
           dstDownscaleRect.y1 = height / 2;

           // Still got more passes to do?
           if (i < passCount - 1) {
               OpenGL::BlitFrameBuffer(gaussianFrameBuffer, gaussianFrameBuffer, dstGaussianAtttachmentName.c_str(), srcGaussianAtttachmentName.c_str(), srcDownscaleRect, dstDownscaleRect, GL_COLOR_BUFFER_BIT, GL_LINEAR);
               width /= 2;
               height /= 2;
               width = std::max(1u, width);
               height = std::max(1u, height);
           }
           else {
               BlitRect composteRect;
               composteRect.x0 = 0;
               composteRect.y0 = 0;
               composteRect.x1 = width;
               composteRect.y1 = height;
               OpenGL::BlitFrameBuffer(gaussianFrameBuffer, &dstFrameBuffer, dstGaussianAtttachmentName.c_str(), dstAttachmentName.c_str(), composteRect, dstRect, GL_COLOR_BUFFER_BIT, GL_LINEAR);
            }
        }
    }
}
