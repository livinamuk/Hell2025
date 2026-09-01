#include "Hell/Render/API/OpenGL/GL_back_end.h"
#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Unloved/Render/RendererConstants.h"
#include "Unloved/Viewport/ViewportManager.h"

#include <array>

namespace {
    constexpr int BLOOM_MIP_COUNT = 3;

    glm::ivec2 GetHalfExtent(const glm::ivec2& extent) {
        return glm::max((extent + glm::ivec2(1)) / 2, glm::ivec2(1));
    }

    void DispatchEmissiveBloomFilter(GLuint sourceHandle, int sourceMip, const glm::ivec2& sourceOffset, const glm::ivec2& sourceExtent, GLuint outputHandle, int outputMip, const glm::ivec2& outputExtent, const glm::ivec2& direction, float filterScale) {
        OpenGL::BindShader("EmissiveBloomFilter");
        OpenGL::BindTextureUnit(0, sourceHandle);
        glBindImageTexture(1, outputHandle, outputMip, GL_FALSE, 0, GL_WRITE_ONLY, GL_R11F_G11F_B10F);

        OpenGL::SetUniformIVec2("u_sourceOffset", sourceOffset);
        OpenGL::SetUniformIVec2("u_sourceExtent", sourceExtent);
        OpenGL::SetUniformIVec2("u_outputExtent", outputExtent);
        OpenGL::SetUniformIVec2("u_direction", direction);
        OpenGL::SetUniformInt("u_sourceMip", sourceMip);
        OpenGL::SetUniformFloat("u_filterScale", filterScale);

        OpenGL::DispatchCompute((outputExtent.x + 15) / 16, (outputExtent.y + 7) / 8, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
    }
}

namespace OpenGL::Renderer {

    void EmissivePass() {
        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer& gBuffer = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
        OpenGLFrameBuffer& bloomPyramidFBO = OpenGL::ResourceManager::GetFrameBuffer("EmissiveBloomPyramid");

        const GLuint emissiveHandle = gBuffer.GetColorAttachmentHandleByName("Emissive");
        const GLuint lightingHandle = gBuffer.GetColorAttachmentHandleByName("Lighting");
        const GLuint bloomHandleA = bloomPyramidFBO.GetColorAttachmentHandleByName("ColorA");
        const GLuint bloomHandleB = bloomPyramidFBO.GetColorAttachmentHandleByName("ColorB");

        // Hair just wrote emissive
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT | GL_FRAMEBUFFER_BARRIER_BIT);

        for (int viewportIndex = 0; viewportIndex < 4; viewportIndex++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(viewportIndex);

            // Skip dead viewports
            if (!viewport || !viewport->IsVisible()) continue;

            const BlitRect viewportRect = OpenGL::Renderer::BlitRectFromFrameBufferViewport(&gBuffer, viewport);
            const glm::ivec2 viewportOffset(viewportRect.x0, viewportRect.y0);
            const glm::ivec2 viewportExtent(viewportRect.x1 - viewportRect.x0, viewportRect.y1 - viewportRect.y0);
            if (viewportExtent.x <= 0 || viewportExtent.y <= 0) continue;

            // Build the viewport local pyramid sizes
            std::array<glm::ivec2, BLOOM_MIP_COUNT> bloomExtents;
            bloomExtents[0] = GetHalfExtent(viewportExtent);

            for (int mip = 1; mip < BLOOM_MIP_COUNT; mip++) {
                bloomExtents[mip] = GetHalfExtent(bloomExtents[mip - 1]);
            }

            // Build the first half res band
            DispatchEmissiveBloomFilter(emissiveHandle, 0, viewportOffset, viewportExtent, bloomHandleA, 0, bloomExtents[0], glm::ivec2(1, 0), 1.0f);

            // Squash the old two vertical passes into one
            DispatchEmissiveBloomFilter(bloomHandleA, 0, glm::ivec2(0), bloomExtents[0], bloomHandleB, 0, bloomExtents[0], glm::ivec2(0, 1), 1.11803398875f);

            // Build the wider bands
            for (int mip = 1; mip < BLOOM_MIP_COUNT; mip++) {
                DispatchEmissiveBloomFilter(bloomHandleB, mip - 1, glm::ivec2(0), bloomExtents[mip - 1], bloomHandleA, mip, bloomExtents[mip], glm::ivec2(1, 0), 1.0f);
                DispatchEmissiveBloomFilter(bloomHandleA, mip, glm::ivec2(0), bloomExtents[mip], bloomHandleB, mip, bloomExtents[mip], glm::ivec2(0, 1), 1.0f);
            }

            // Composite direct emissive and bloom straight into lighting
            OpenGL::BindShader("EmissiveBloomComposite");
            OpenGL::BindSSBO(SSBO_IDX_RENDERER_DATA, "RendererData");

            OpenGL::BindImageTexture(0, lightingHandle, GL_READ_WRITE, GL_RGBA16F);
            OpenGL::BindTextureUnit(1, emissiveHandle);
            OpenGL::BindTextureUnit(2, bloomHandleB);
            OpenGL::SetUniformIVec2("u_viewportOffset", viewportOffset);
            OpenGL::SetUniformIVec2("u_viewportExtent", viewportExtent);
            OpenGL::SetUniformIVec2("u_bloomExtents[0]", bloomExtents[0]);
            OpenGL::SetUniformIVec2("u_bloomExtents[1]", bloomExtents[1]);
            OpenGL::SetUniformIVec2("u_bloomExtents[2]", bloomExtents[2]);

            OpenGL::DispatchCompute((viewportExtent.x + 15) / 16, (viewportExtent.y + 3) / 4, 1);
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT | GL_FRAMEBUFFER_BARRIER_BIT);
        }
    }
}
