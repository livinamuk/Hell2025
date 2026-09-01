#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Unloved/Session/Session.h"
#include "Hell/Common/Random.h"
#include "Unloved/Render/Renderer.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Viewport/ViewportManager.h"
#include "Unloved/Systems/Bullets/BulletSystem.h"
#include "Unloved/World/World.h"

#include "Hell/Input.h"
#include "Hell/Time.h"

namespace Input = Hell::Input;


namespace OpenGL::Renderer {

    void DispatchTestPass();
    void BubblesPass();

    void UploadAnyNewParticles();
    void UpdateParticles();
    void DrawParticles();

    void ParticlePass() {
        ProfilerOpenGLZoneFunction();
        if (!Unloved::World::HasOcean()) return;

        UploadAnyNewParticles();
        UpdateParticles();
        DrawParticles();

        DispatchTestPass();
        BubblesPass();
    }

    void UploadAnyNewParticles() {

        std::vector<BulletTrailParticle>& particles = Unloved::BulletSystem::GetBulletTrailParticles();
        if (particles.empty()) return;

        std::vector<GpuParticle> gpuParticles;
        gpuParticles.reserve(particles.size());

        for (BulletTrailParticle& particle : particles) {
            GpuParticle& gpuParticle = gpuParticles.emplace_back();
            gpuParticle.position = glm::vec4(particle.position, 1.0f);
            gpuParticle.velocity = glm::vec4(particle.velocity, 1.0f);
            gpuParticle.rotation = particle.rotation;
            gpuParticle.rotationalVelocity = particle.rotationalVelocity;
            gpuParticle.lifeTime = particle.lifeTime;
        }

        if (gpuParticles.size() > MAX_GPU_PARTICLES) {
            gpuParticles.resize(MAX_GPU_PARTICLES);
        }

        particles.clear();

        OpenGL::UploadSSBOStatic("ParticleAdditions", gpuParticles.size() * sizeof(GpuParticle), gpuParticles.data());

        OpenGL::BindSSBO(SSBO_IDX_PARTICLE_ADD_ADDITIONS, "ParticleAdditions");
        OpenGL::BindSSBO(SSBO_IDX_PARTICLE_ADD_POOL, "ParticlePool");
        OpenGL::BindSSBO(SSBO_IDX_PARTICLE_ADD_COUNTER, "ParticleAdditionCounter");

        OpenGL::ClearSSBORange("ParticleAdditionCounter", 0, sizeof(uint32_t));

        OpenGL::BindShader("ParticleAdditions");
        OpenGL::SetUniformInt("u_newParticleCount", static_cast<int>(gpuParticles.size()));

        constexpr uint32_t localSize = 256;
        constexpr uint32_t groupCount = (MAX_GPU_PARTICLES + localSize - 1) / localSize;

        OpenGL::DispatchCompute(groupCount, 1, 1);

        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    void UpdateParticles() {
        OpenGL::BindSSBO(SSBO_IDX_PARTICLE_UPDATE_POOL, "ParticlePool");
        OpenGL::BindSSBO(SSBO_IDX_PARTICLE_UPDATE_ACTIVE_INDICES, "ParticleActiveIndices");
        OpenGL::BindSSBO(SSBO_IDX_PARTICLE_UPDATE_INDIRECT_COMMAND, "ParticleDrawCommand");

        OpenGL::BindShader("ParticleUpdate");
        OpenGL::SetUniformFloat("u_deltaTime", Hell::Time::DeltaTime());

        //OpenGL::DispatchCompute(1, 1, 1);

        OpenGL::ClearSSBORange("ParticleDrawCommand", offsetof(DrawArraysIndirectCommand, instanceCount), sizeof(uint32_t));

        constexpr uint32_t localSize = 256;
        constexpr uint32_t groupCount = (MAX_GPU_PARTICLES + localSize - 1) / localSize;

        OpenGL::DispatchCompute(groupCount, 1, 1);

        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_COMMAND_BARRIER_BIT);
    }

    void DrawParticles() {
        const std::vector<ViewportData>& viewportData = Unloved::RenderDataManager::GetViewportData();

        OpenGLCubemapView& skyboxCubemapView = OpenGL::ResourceManager::GetCubemapView("SkyboxNightSky");
        OpenGLFrameBuffer& fbo = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");

        fbo.Bind();
        fbo.SetViewport();
        fbo.DrawBuffers({ "Lighting" });

        OpenGLRasterizerState state;
        state.depthTestEnabled = true;
        state.cullfaceEnable = false;
        state.depthMask = false;
        state.colorMask = true;
        state.depthFunc = GL_GREATER;

        state.blendEnable = true;
        state.blendFuncSrcfactor = GL_SRC_ALPHA;
        state.blendFuncDstfactor = GL_ONE_MINUS_SRC_ALPHA;

        OpenGL::RasterizerStateManager::SetRasterizerState(state);

        OpenGL::BindShader("ParticleColor");
        OpenGL::BindSSBO(SSBO_IDX_SAMPLERS, "Samplers");
        OpenGL::BindSSBO(SSBO_IDX_RENDERER_DATA, "RendererData");
        OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");
        OpenGL::BindSSBO(SSBO_IDX_SPOT_LIGHTS, "SpotLights");
        OpenGL::SetUniformFloat("u_time", Unloved::Session::GetSessionTime());

        OpenGL::BindTextureUnit(0, GetTextureHandleByName("UnderwaterBulletBubble"));

        glMemoryBarrier(GL_COMMAND_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

        OpenGL::BindSSBO(SSBO_IDX_PARTICLE_DRAW_POOL, "ParticlePool");
        OpenGL::BindSSBO(SSBO_IDX_PARTICLE_DRAW_ACTIVE_INDICES, "ParticleActiveIndices");

        BindEmptyVAO();
        OpenGL::BindDrawIndirectBuffer("ParticleDrawCommand");

        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            OpenGL::Renderer::SetViewport(&fbo, viewport);
            OpenGL::SetUniformInt("u_viewportIndex", i);
            OpenGL::SetUniformMat4("u_projectionView", viewportData[i].jitteredProjectionViewReverseZ);
            OpenGL::SetUniformMat4("u_view", viewportData[i].view);
            OpenGL::SetUniformVec3("u_viewPos", viewportData[i].viewPos);

            glDrawArraysIndirect(GL_TRIANGLES, 0);
        }

        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
    }


    void DispatchTestPass() {
        static std::vector<glm::vec4> positions;

        if (Input::KeyPressed(HELL_KEY_U)) {
            glm::vec3 origin = glm::vec3(36.0f, 32.5f, 37.0f);
            positions.clear();

            for (int i = 0; i < 20; i++) {
                glm::vec3 pos = origin;
                pos.x -= Hell::Random::Float(-2.0f, 2.0f);
                pos.y -= Hell::Random::Float(-2.0f, 2.0f);
                pos.z -= Hell::Random::Float(-2.0f, 2.0f);
                positions.push_back(glm::vec4(pos, 1.0f));
            }
            OpenGL::UploadSSBOStatic("BubblePositions", positions.size() * sizeof(glm::vec4), positions.data());

            uint64_t count = positions.size();
            OpenGL::UpdateSSBO("BubblePositionCount", sizeof(uint64_t), &count);

            OpenGL::BindShader("BubbleDrawCommandArgs");
            OpenGL::BindSSBO(SSBO_IDX_BUBBLE_DRAW_POSITIONS, "BubblePositions");
            OpenGL::BindSSBO(SSBO_IDX_BUBBLE_DRAW_PARTICLE_COUNT, "BubblePositionCount");
            OpenGL::BindSSBO(SSBO_IDX_BUBBLE_DRAW_INDIRECT_COMMAND, "BubbleDrawCommand");
            OpenGL::DispatchCompute(1, 1, 1);
        }
    }


    void BubblesPass() {
        //ProfilerOpenGLZoneFunction();

        const std::vector<ViewportData>& viewportData = Unloved::RenderDataManager::GetViewportData();

        OpenGLCubemapView& skyboxCubemapView = OpenGL::ResourceManager::GetCubemapView("SkyboxNightSky");
        OpenGLFrameBuffer& fbo = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");

        fbo.Bind();
        fbo.SetViewport();
        fbo.DrawBuffers({ "Lighting" });

        OpenGLRasterizerState state;
        state.depthTestEnabled = true;
        state.cullfaceEnable = false;
        state.depthMask = false;
        state.colorMask = true;
        state.depthFunc = GL_GREATER;

        state.blendEnable = true;
        state.blendFuncSrcfactor = GL_SRC_ALPHA;
        state.blendFuncDstfactor = GL_ONE_MINUS_SRC_ALPHA;

        OpenGL::RasterizerStateManager::SetRasterizerState(state);

        OpenGL::BindShader("Bubbles");
        OpenGL::BindSSBO(SSBO_IDX_SAMPLERS, "Samplers");
        OpenGL::BindSSBO(SSBO_IDX_RENDERER_DATA, "RendererData");
        OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");
        OpenGL::BindSSBO(SSBO_IDX_SPOT_LIGHTS, "SpotLights");
        OpenGL::SetUniformFloat("u_time", Unloved::Session::GetSessionTime());
        OpenGL::BindTextureUnit(0, skyboxCubemapView.GetHandle());

        glMemoryBarrier(GL_COMMAND_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

        OpenGL::BindSSBO(SSBO_IDX_BUBBLE_DRAW_POSITIONS, "BubblePositions");

        BindEmptyVAO();
        OpenGL::BindDrawIndirectBuffer("BubbleDrawCommand");

        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            OpenGL::Renderer::SetViewport(&fbo, viewport);
            OpenGL::SetUniformInt("u_viewportIndex", i);
            OpenGL::SetUniformMat4("u_projectionView", viewportData[i].jitteredProjectionViewReverseZ);
            OpenGL::SetUniformMat4("u_view", viewportData[i].view);
            OpenGL::SetUniformVec3("u_viewPos", viewportData[i].viewPos);

            glDrawArraysIndirect(GL_TRIANGLES, 0);
        }

        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
    }

}
