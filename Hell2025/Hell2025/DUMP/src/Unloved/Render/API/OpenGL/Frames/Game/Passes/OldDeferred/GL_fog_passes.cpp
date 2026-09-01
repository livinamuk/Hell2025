#include "Hell/Logging.h"
#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Hell/Render/API/OpenGL/GL_back_end.h"
#include "Unloved/EditorSession/EditorSession.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Render/Renderer.h"
#include "Unloved/Viewport/ViewportManager.h"
#include "Unloved/Session/Session.h"
#include "Hell/Time.h"
#include "Unloved/Config/Config.h"

namespace OpenGL::Renderer {

    void InitFog() {
        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("PerlinNoise3D");
        OpenGLTexture3D* perlinNoiseTexture = OpenGL::ResourceManager::GetTexture3DPtr("PerlinNoise");

        if (!shader) return;
        if (!perlinNoiseTexture) return;

        glBindImageTexture(0, perlinNoiseTexture->GetHandle(), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_R32F);

        int size = perlinNoiseTexture->GetSize();

        OpenGL::BindShader("PerlinNoise3D");
        OpenGL::SetUniformFloat("uScale", 8.0f);
        OpenGL::SetUniformVec3("uDimensions", glm::vec3(size));

        OpenGL::DispatchCompute((size + 7) / 8, (size + 7) / 8, (size + 7) / 8);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        perlinNoiseTexture->GenerateMipmaps();

        Logging::Init() << "Initialized the BERLIN NOISE";
    }

    void RayMarchFog() {
        ProfilerOpenGLZoneFunction();

        if (Unloved::EditorSession::IsActive()) return;

        const std::vector<ViewportData>& viewportData = Unloved::RenderDataManager::GetViewportData();

        OpenGLFrameBuffer* fogFbo = OpenGL::ResourceManager::GetFrameBufferPtr("Fog");
        OpenGLTexture3D* perlinNoiseTexture = OpenGL::ResourceManager::GetTexture3DPtr("PerlinNoise");

        std::string gBufferName = (Unloved::Renderer::GetRendererMode() == RendererMode::RE_STYLE) ? "GBuffer" : "GBuffer";
        OpenGLFrameBuffer& gBuffer = OpenGL::ResourceManager::GetFrameBuffer(gBufferName);

        if (!fogFbo) return;
        if (!perlinNoiseTexture) return;

        static float time = 0.0f;
        time += Hell::Time::DeltaTime();

        static int noiseSeed = 0;
        noiseSeed++;

        // Ray march the fog
        OpenGL::BindShader("FogRayMarch");
        OpenGL::BindSSBO(SSBO_IDX_SAMPLERS, "Samplers");
        OpenGL::BindSSBO(SSBO_IDX_RENDERER_DATA, "RendererData");
        OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");
        OpenGL::BindSSBO(SSBO_IDX_SPOT_LIGHTS, "SpotLights");
        OpenGL::SetUniformFloat("u_time", time);
        OpenGL::SetUniformInt("u_noiseSeed", noiseSeed);

        const Config::Fog::Settings& fog = Config::Fog::GetSettings();
        OpenGL::SetUniformInt("u_fogEnabled", fog.enabled ? 1 : 0);
        OpenGL::SetUniformVec3("u_fogColor", fog.color);
        OpenGL::SetUniformFloat("u_fogColorStrength", fog.colorStrength);
        OpenGL::SetUniformFloat("u_maxRayDistance", fog.maxRayDistance);
        OpenGL::SetUniformInt("u_stepCount", fog.stepCount);
        OpenGL::SetUniformFloat("u_densityBias", fog.densityBias);
        OpenGL::SetUniformFloat("u_densityScale", fog.densityScale);
        OpenGL::SetUniformFloat("u_extinctionScale", fog.extinctionScale);
        OpenGL::SetUniformFloat("u_ambientStartDistance", fog.ambientStartDistance);
        OpenGL::SetUniformFloat("u_ambientEndDistance", fog.ambientEndDistance);
        OpenGL::SetUniformFloat("u_ambientExponent", fog.ambientExponent);
        OpenGL::SetUniformFloat("u_heightFadeStart", fog.heightFadeStart);
        OpenGL::SetUniformFloat("u_heightFadeEnd", fog.heightFadeEnd);
        OpenGL::SetUniformFloat("u_heightExponent", fog.heightExponent);
        OpenGL::SetUniformFloat("u_lowHeightScatterMultiplier", fog.lowHeightScatterMultiplier);
        OpenGL::SetUniformFloat("u_distanceFogStart", fog.distanceFogStart);
        OpenGL::SetUniformFloat("u_distanceFogEnd", fog.distanceFogEnd);
        OpenGL::SetUniformFloat("u_distanceFogExponent", fog.distanceFogExponent);
        OpenGL::SetUniformFloat("u_distanceFogStrength", fog.distanceFogStrength);
        OpenGL::SetUniformFloat("u_clumpSizeXZ", fog.clumpSizeXZ);
        OpenGL::SetUniformFloat("u_clumpSizeY", fog.clumpSizeY);
        OpenGL::SetUniformFloat("u_noiseScaleNearMultiplier", fog.noiseScaleNearMultiplier);
        OpenGL::SetUniformFloat("u_noiseScaleFarMultiplier", fog.noiseScaleFarMultiplier);
        OpenGL::SetUniformFloat("u_noiseScaleStartDistance", fog.noiseScaleStartDistance);
        OpenGL::SetUniformFloat("u_noiseScaleEndDistance", fog.noiseScaleEndDistance);
        OpenGL::SetUniformFloat("u_noiseScaleExponent", fog.noiseScaleExponent);
        OpenGL::SetUniformFloat("u_noiseMipBias", fog.noiseMipBias);
        OpenGL::SetUniformFloat("u_noiseMipScale", fog.noiseMipScale);
        OpenGL::SetUniformFloat("u_noiseMinMip", fog.noiseMinMip);
        OpenGL::SetUniformFloat("u_noiseMaxMip", fog.noiseMaxMip);
        OpenGL::SetUniformFloat("u_noiseNearMip", fog.noiseNearMip);
        OpenGL::SetUniformFloat("u_noiseFarMip", fog.noiseFarMip);
        OpenGL::SetUniformFloat("u_noiseMipNearDistance", fog.noiseMipNearDistance);
        OpenGL::SetUniformFloat("u_noiseMipFarDistance", fog.noiseMipFarDistance);
        OpenGL::SetUniformFloat("u_noiseMipExponent", fog.noiseMipExponent);
        OpenGL::SetUniformFloat("u_noiseMipRespectStep", fog.noiseMipRespectStep);
        OpenGL::SetUniformVec3("u_windVelocity", fog.windVelocity);
        OpenGL::SetUniformFloat("u_timeScrollSpeed", fog.timeScrollSpeed);
        OpenGL::SetUniformFloat("u_xMorphSpeed", fog.xMorphSpeed);
        OpenGL::SetUniformFloat("u_zMorphSpeed", fog.zMorphSpeed);
        OpenGL::SetUniformFloat("u_yScrollSpeed", fog.yScrollSpeed);

        OpenGL::BindImageTexture(4, fogFbo->GetColorAttachmentHandleByName("Color"), GL_WRITE_ONLY, GL_RGBA16F);
        OpenGL::BindTextureUnit(1, gBuffer.GetDepthAttachmentHandle());
        OpenGL::BindTextureUnit(2, perlinNoiseTexture->GetHandle());

        OpenGL::DispatchCompute((fogFbo->GetWidth() + 15) / 16, (fogFbo->GetHeight() + 15) / 16, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        // Composite
        OpenGL::BindShader("FogComposite");
        OpenGL::BindSSBO(SSBO_IDX_RENDERER_DATA, "RendererData");
        OpenGL::BindImageTexture(0, gBuffer.GetColorAttachmentHandleByName("Lighting"), GL_READ_WRITE, GL_RGBA16F);
        OpenGL::BindTextureUnit(1, fogFbo->GetColorAttachmentHandleByName("Color"));

        OpenGL::DispatchCompute((gBuffer.GetWidth() + 7) / 8, (gBuffer.GetHeight() + 7) / 8, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }


    void BlitFog() {
        OpenGLTexture3D* perlinNoiseTexture = OpenGL::ResourceManager::GetTexture3DPtr("PerlinNoise");
        if (!perlinNoiseTexture) return;

        GLuint fbo;
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);

        static int z = 0;
        z = (z + 1) % 128;
        glFramebufferTextureLayer(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, perlinNoiseTexture->GetHandle(), 0, z);

        GLenum status = glCheckFramebufferStatus(GL_READ_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "FBO incomplete: " << status << "\n";
        }

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

        int sliceSize = 128;

        glReadBuffer(GL_COLOR_ATTACHMENT0);
        glDrawBuffer(GL_BACK);

        glBlitFramebuffer(
            0, 0, sliceSize, sliceSize,
            0, 0, sliceSize * 4, sliceSize * 4,
            GL_COLOR_BUFFER_BIT,
            GL_NEAREST
        );

        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &fbo);
    }


}
