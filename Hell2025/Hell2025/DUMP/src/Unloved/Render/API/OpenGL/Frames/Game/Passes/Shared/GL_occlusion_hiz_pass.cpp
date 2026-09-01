#include "Unloved/Render/API/OpenGL/GL_renderer.h"

#include <algorithm>
#include <cmath>

namespace {
    constexpr int OCCLUSION_HIZ_GROUP_SIZE = 8;

    int CalculateMipCount(const glm::ivec2& extent) {
        const int maximumDimension = std::max(extent.x, extent.y);
        return maximumDimension > 0 ? 1 + static_cast<int>(std::floor(std::log2(maximumDimension))) : 0;
    }

    glm::ivec2 GetHalfExtent(const glm::ivec2& extent) {
        return glm::max((extent + glm::ivec2(1)) / 2, glm::ivec2(1));
    }
}

namespace OpenGL::Renderer {

    void OcclusionHiZPass() {
        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer* gBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("GBuffer");
        OpenGLFrameBuffer* occlusionHiZFbo = OpenGL::ResourceManager::GetFrameBufferPtr("OcclusionHiZ");
        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("OcclusionHiZ");
        if (!gBuffer || !occlusionHiZFbo || !shader) return;
        if (!shader->GetHandle()) return;

        const GLuint gBufferDepthHandle = gBuffer->GetDepthAttachmentHandle();
        const GLuint occlusionHiZHandle = occlusionHiZFbo->GetColorAttachmentHandleByName("MinDepth");
        if (!gBufferDepthHandle || !occlusionHiZHandle) return;

        const glm::ivec2 baseExtent(occlusionHiZFbo->GetWidth(), occlusionHiZFbo->GetHeight());
        const int mipCount = CalculateMipCount(baseExtent);
        if (mipCount <= 0) return;

        glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
        OpenGL::BindShader("OcclusionHiZ");

        glm::ivec2 outputExtent = baseExtent;

        for (int outputMip = 0; outputMip < mipCount; outputMip++) {
            const bool reduceGBufferDepth = outputMip == 0;
            const GLuint sourceHandle = reduceGBufferDepth ? gBufferDepthHandle : occlusionHiZHandle;
            const int sourceMip = reduceGBufferDepth ? 0 : outputMip - 1;

            OpenGL::BindTextureUnit(0, sourceHandle);
            glBindImageTexture(0, occlusionHiZHandle, outputMip, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);
            OpenGL::SetUniformInt("u_sourceMip", sourceMip);

            OpenGL::DispatchCompute(
                (outputExtent.x + OCCLUSION_HIZ_GROUP_SIZE - 1) / OCCLUSION_HIZ_GROUP_SIZE,
                (outputExtent.y + OCCLUSION_HIZ_GROUP_SIZE - 1) / OCCLUSION_HIZ_GROUP_SIZE,
                1);

            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
            outputExtent = GetHalfExtent(outputExtent);
        }
    }
}
