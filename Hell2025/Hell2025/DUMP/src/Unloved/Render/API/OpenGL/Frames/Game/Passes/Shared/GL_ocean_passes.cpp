#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Systems/Ocean/Ocean.h"
#include "Unloved/World/World.h"

#include "Unloved/Viewport/ViewportManager.h"

#include "Hell/Input.h"
namespace Input = Hell::Input;


namespace OpenGL::Renderer {

    namespace {
        struct OceanMeshConfig {
            int gridSize = 128;
            int lodLevelCount = 6;
            float baseSpacing = 0.175f;
            float lodScale = 2.0f;
            int holeMargin = 2;
            float lodDepthBias = 0.02f;
        };

        constexpr OceanMeshConfig OCEAN_MESH_CONFIG = {};
        constexpr int OCEAN_VERTICES_PER_QUAD = 6;
    }

    static const int g_readbackBufferCount = 3;
    GLuint readbackSSBOs[g_readbackBufferCount];
    GLsync readbackSyncs[g_readbackBufferCount] = { 0 };

    void SetOceanFFTBandUniforms() {
        for (int i = 0; i < Ocean::FFT_BAND_COUNT; i++) {
            const std::string uniformIndex = "[" + std::to_string(i) + "]";
            OpenGL::SetUniformFloat("u_domainSize" + uniformIndex, Ocean::GetDomainSize(i));
        }
    }

    static void SetOceanMeshUniforms() {
        OpenGL::SetUniformInt("u_mesh.gridSize", OCEAN_MESH_CONFIG.gridSize);
        OpenGL::SetUniformInt("u_mesh.lodLevelCount", OCEAN_MESH_CONFIG.lodLevelCount);
        OpenGL::SetUniformFloat("u_mesh.baseSpacing", OCEAN_MESH_CONFIG.baseSpacing);
        OpenGL::SetUniformFloat("u_mesh.lodScale", OCEAN_MESH_CONFIG.lodScale);
        OpenGL::SetUniformInt("u_mesh.holeMargin", OCEAN_MESH_CONFIG.holeMargin);
        OpenGL::SetUniformFloat("u_mesh.lodDepthBias", OCEAN_MESH_CONFIG.lodDepthBias);
    }

    void OceanGeometryPass() {
        ProfilerOpenGLZoneFunction();
        if (!Unloved::World::HasOcean()) return;

        OpenGLCubemapView& skyboxCubemapView = OpenGL::ResourceManager::GetCubemapView("SkyboxNightSky");
        OpenGLFrameBuffer& fftBand0Fbo = OpenGL::ResourceManager::GetFrameBuffer("FFT_band0");
        OpenGLFrameBuffer& fftBand1Fbo = OpenGL::ResourceManager::GetFrameBuffer("FFT_band1");
        OpenGLFrameBuffer& gBuffer = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
        OpenGLFrameBuffer& waterFbo = OpenGL::ResourceManager::GetFrameBuffer("Water");
        OpenGLShadowMap& flashlightShadowMaps = OpenGL::ResourceManager::GetShadowMap("FlashlightShadowMaps");

        constexpr int vertexCount = OCEAN_MESH_CONFIG.gridSize * OCEAN_MESH_CONFIG.gridSize * OCEAN_VERTICES_PER_QUAD * OCEAN_MESH_CONFIG.lodLevelCount;

        OpenGL::BindShader("OceanLighting");
        OpenGL::BindSSBO(SSBO_IDX_SAMPLERS, "Samplers");
        OpenGL::BindSSBO(SSBO_IDX_RENDERER_DATA, "RendererData");
        OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");
        OpenGL::BindSSBO(SSBO_IDX_SPOT_LIGHTS, "SpotLights");
        SetOceanFFTBandUniforms();
        SetOceanMeshUniforms();
        OpenGL::SetUniformFloat("u_time", Ocean::GetAnimationTime());

        OpenGL::BindTextureUnit(0, fftBand0Fbo.GetColorAttachmentHandleByName("Displacement"));
        OpenGL::BindTextureUnit(1, fftBand0Fbo.GetColorAttachmentHandleByName("Slope"));
        OpenGL::BindTextureUnit(2, fftBand1Fbo.GetColorAttachmentHandleByName("Displacement"));
        OpenGL::BindTextureUnit(3, fftBand1Fbo.GetColorAttachmentHandleByName("Slope"));
        OpenGL::BindTextureUnit(4, skyboxCubemapView.GetHandle());
        OpenGL::BindTextureUnit(5, GetTextureHandleByName("WaterNormals"));
        OpenGL::BindTextureUnit(6, flashlightShadowMaps.GetDepthTextureHandle());

        OpenGLRasterizerState state;
        state.depthTestEnabled = true;
        state.blendEnable = false;
        state.cullfaceEnable = false;
        state.depthMask = true;
        state.colorMask = true;
        state.depthFunc = GL_GREATER;
        OpenGL::RasterizerStateManager::SetRasterizerState(state);

        static bool lines = false;
        if (Input::KeyPressed(HELL_KEY_L)) {
            lines = !lines;
        }

        // Keep ocean behind scene geometry
        OpenGL::BlitFrameBufferDepth(&gBuffer, &waterFbo);

        waterFbo.Bind();
        waterFbo.DrawBuffers({ "Lighting", "OceanMask" });
        BindEmptyVAO();

        for (int viewportIndex = 0; viewportIndex < 4; viewportIndex++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(viewportIndex);
            if (!viewport || !viewport->IsVisible()) continue;

            OpenGL::Renderer::SetViewport(&waterFbo, viewport);
            OpenGL::SetUniformInt("u_viewportIndex", viewportIndex);

            if (lines) glDrawArrays(GL_LINES, 0, vertexCount);
            else       glDrawArrays(GL_TRIANGLES, 0, vertexCount);
        }

        glBindVertexArray(0);
    }

    void OceanUnderWaterFlags() {
        ProfilerOpenGLZoneFunction();
        if (!Unloved::World::HasOcean()) return;

        OpenGLFrameBuffer& fftFrameBuffer_band0 = OpenGL::ResourceManager::GetFrameBuffer("FFT_band0");
        OpenGLFrameBuffer& fftFrameBuffer_band1 = OpenGL::ResourceManager::GetFrameBuffer("FFT_band1");
        OpenGLFrameBuffer& waterFrameBuffer = OpenGL::ResourceManager::GetFrameBuffer("Water");

        OpenGL::BindShader("OceanFlags");
        OpenGL::BindSSBO(SSBO_IDX_RENDERER_DATA, "RendererData");
        OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");
        SetOceanFFTBandUniforms();

        OpenGL::BindImageTexture(0, waterFrameBuffer.GetColorAttachmentHandleByName("OceanFlags"), GL_WRITE_ONLY, GL_R8UI);
        OpenGL::BindImageTexture(1, waterFrameBuffer.GetColorAttachmentHandleByName("OceanMask"), GL_READ_ONLY, GL_R8UI);
        OpenGL::BindTextureUnit(2, fftFrameBuffer_band0.GetColorAttachmentHandleByName("Displacement"));
        OpenGL::BindTextureUnit(3, fftFrameBuffer_band1.GetColorAttachmentHandleByName("Displacement"));

        glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        OpenGL::DispatchCompute((waterFrameBuffer.GetWidth() + 7) / 8, (waterFrameBuffer.GetHeight() + 7) / 8, 1);
    }

    void OceanSurfaceCompositePass() {
        ProfilerOpenGLZoneFunction();
        if (!Unloved::World::HasOcean()) return;

        OpenGLFrameBuffer& gBuffer = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");

        OpenGLFrameBuffer& waterFrameBuffer = OpenGL::ResourceManager::GetFrameBuffer("Water");
        OpenGLFrameBuffer& quaterSizeFrameBuffer = OpenGL::ResourceManager::GetFrameBuffer("QuarterSize");

        // Down sample the final lighting to 25%
        // TODO: try using Gaussian blur of final lighting. It's currently calculated in the underwater composite pass so will have to move it before
        OpenGL::BlitFrameBuffer(&gBuffer, &quaterSizeFrameBuffer, "Lighting", "DownsampledFinalLighting", GL_COLOR_BUFFER_BIT, GL_LINEAR);

        OpenGL::BindShader("OceanSurfaceComposite");
        OpenGL::BindSSBO(SSBO_IDX_RENDERER_DATA, "RendererData");
        OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");

        OpenGL::SetUniformFloat("u_time", Ocean::GetAnimationTime());

        OpenGL::BindImageTexture(0, gBuffer.GetColorAttachmentHandleByName("Lighting"), GL_READ_WRITE, GL_RGBA16F);
        OpenGL::BindImageTexture(1, waterFrameBuffer.GetColorAttachmentHandleByName("OceanMask"), GL_READ_ONLY, GL_R8UI);
        OpenGL::BindTextureUnit(2, waterFrameBuffer.GetColorAttachmentHandleByName("Lighting"));
        OpenGL::BindTextureUnit(3, GetTextureHandleByName("WaterDUDV"));
        OpenGL::BindTextureUnit(4, quaterSizeFrameBuffer.GetColorAttachmentHandleByName("DownsampledFinalLighting"));

        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        OpenGL::DispatchCompute((gBuffer.GetWidth() + 7) / 8, (gBuffer.GetHeight() + 7) / 8, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT | GL_FRAMEBUFFER_BARRIER_BIT);
    }

    void OceanUnderwaterBlurPass() {
        ProfilerOpenGLZoneFunction();
        if (!Unloved::World::HasOcean()) return;

        OpenGLFrameBuffer& miscFullSizeFrameBuffer = OpenGL::ResourceManager::GetFrameBuffer("MiscFullSize");

        OpenGLFrameBuffer& gBuffer = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");

        OpenGL::BindShader("GaussianBlur");

        OpenGL::SetUniformVec2("u_direction", glm::vec2(0, 1));
        glBindImageTexture(0, miscFullSizeFrameBuffer.GetColorAttachmentHandleByName("GaussianFinalLightingIntermediate"), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F); // WARNING! you WERE degrading your image quality by down sampling into a texture of lower bit resolution. Find out if this even matters at this point in the frame. But now you're not. But also. This is a shit load of VRAM so think about this.
        glBindTextureUnit(1, gBuffer.GetColorAttachmentHandleByName("Lighting"));
        OpenGL::DispatchCompute((miscFullSizeFrameBuffer.GetWidth() + 7) / 8, (miscFullSizeFrameBuffer.GetHeight() + 7) / 8, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        OpenGL::SetUniformVec2("u_direction", glm::vec2(1, 0));
        glBindImageTexture(0, miscFullSizeFrameBuffer.GetColorAttachmentHandleByName("GaussianFinalLighting"), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
        glBindTextureUnit(1, miscFullSizeFrameBuffer.GetColorAttachmentHandleByName("GaussianFinalLightingIntermediate"));
        OpenGL::DispatchCompute((miscFullSizeFrameBuffer.GetWidth() + 7) / 8, (miscFullSizeFrameBuffer.GetHeight() + 7) / 8, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }

    void OceanUnderwaterCompositePass() {
        ProfilerOpenGLZoneFunction();
        if (!Unloved::World::HasOcean()) return;

        OpenGLFrameBuffer& miscFullSizeFrameBuffer = OpenGL::ResourceManager::GetFrameBuffer("MiscFullSize");
        OpenGLFrameBuffer& waterFrameBuffer = OpenGL::ResourceManager::GetFrameBuffer("Water");

        OpenGLFrameBuffer& gBuffer = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");

        OpenGL::BindShader("OceanUnderwaterComposite");
        OpenGL::BindSSBO(SSBO_IDX_RENDERER_DATA, "RendererData");
        OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");

        OpenGL::SetUniformFloat("u_time", Ocean::GetAnimationTime());

        OpenGL::BindImageTexture(0, gBuffer.GetColorAttachmentHandleByName("Lighting"), GL_READ_WRITE, GL_RGBA16F);
        OpenGL::BindImageTexture(1, waterFrameBuffer.GetColorAttachmentHandleByName("OceanFlags"), GL_READ_ONLY, GL_R8UI);
        OpenGL::BindTextureUnit(2, miscFullSizeFrameBuffer.GetColorAttachmentHandleByName("GaussianFinalLighting"));
        OpenGL::BindTextureUnit(3, waterFrameBuffer.GetColorAttachmentHandleByName("Lighting"));
        OpenGL::BindTextureUnit(4, GetTextureHandleByName("WaterDUDV"));

        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        OpenGL::DispatchCompute((gBuffer.GetWidth() + 7) / 8, (gBuffer.GetHeight() + 7) / 8, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT | GL_FRAMEBUFFER_BARRIER_BIT);
    }

    void InitOceanHeightReadback() {
        const GLbitfield storageFlags = GL_MAP_READ_BIT | GL_CLIENT_STORAGE_BIT;
        //const GLbitfield storageFlags = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
        glGenBuffers(g_readbackBufferCount, readbackSSBOs);
        for (int i = 0; i < g_readbackBufferCount; ++i) {
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, readbackSSBOs[i]);
            glBufferStorage(GL_SHADER_STORAGE_BUFFER, sizeof(OceanReadbackData), nullptr, storageFlags);
        }
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    void OceanHeightReadback() {
        if (!Unloved::World::HasOcean()) return;

        OpenGLFrameBuffer* fftFrameBuffer_band0 = OpenGL::ResourceManager::GetFrameBufferPtr("FFT_band0");
        OpenGLFrameBuffer* fftFrameBuffer_band1 = OpenGL::ResourceManager::GetFrameBufferPtr("FFT_band1");
        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("OceanPositionReadback");

        if (!fftFrameBuffer_band0) return;
        if (!fftFrameBuffer_band1) return;
        if (!shader) return;

        static int frame = 0;
        int idx = frame % g_readbackBufferCount;

        if (readbackSyncs[idx]) {
            GLenum status = glClientWaitSync(readbackSyncs[idx], 0, 0);
            if (status == GL_ALREADY_SIGNALED || status == GL_CONDITION_SATISFIED) {
                glBindBuffer(GL_SHADER_STORAGE_BUFFER, readbackSSBOs[idx]);
                void* ptr = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(OceanReadbackData), GL_MAP_READ_BIT);

                if (ptr) {
                    const OceanReadbackData* gpuData = static_cast<const OceanReadbackData*>(ptr);
                    OceanReadbackData& cpuData = Ocean::GetOceanReadBackData();
                    cpuData.heightPlayer0 = gpuData->heightPlayer0;
                    cpuData.heightPlayer1 = gpuData->heightPlayer1;
                    cpuData.heightPlayer2 = gpuData->heightPlayer2;
                    cpuData.heightPlayer3 = gpuData->heightPlayer3;
                    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
                }
                else {
                    GLenum err = glGetError();
                    std::cerr << "ERROR: glMapBufferRange returned null for slot " << idx << ", glGetError()=0x" << std::hex << err << std::dec << "\n";
                    GLint flags = 0;
                    glGetNamedBufferParameteriv(readbackSSBOs[idx], GL_BUFFER_ACCESS_FLAGS, &flags);
                    std::cerr << "      Buffer access flags: 0x" << std::hex << flags << std::dec << "\n";
                    GLint buffer_size = 0;
                    glGetNamedBufferParameteriv(readbackSSBOs[idx], GL_BUFFER_SIZE, &buffer_size);
                    std::cerr << "      Buffer size: " << buffer_size << "\n";
                }
                glDeleteSync(readbackSyncs[idx]);
                readbackSyncs[idx] = 0;
            }
            else if (status == GL_TIMEOUT_EXPIRED) {
                // Not an error for timeout 0, just means not ready
            }
            else if (status == GL_WAIT_FAILED) {
                GLenum err = glGetError();
                std::cerr << "ERROR: glClientWaitSync failed for slot " << idx << ", glGetError()=0x" << std::hex << err << std::dec << "\n";
                glDeleteSync(readbackSyncs[idx]);
                readbackSyncs[idx] = 0;
            }
        }
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SSBO_IDX_OCEAN_POSITION_READBACK, readbackSSBOs[idx]);
        OpenGL::BindShader("OceanPositionReadback");
        SetOceanFFTBandUniforms();
        OpenGL::SetUniformFloat("u_oceanOriginY", Ocean::GetOceanOriginY());
        OpenGL::SetUniformInt("u_displayMode", static_cast<int>(Ocean::GetDisplayMode()));

        for (int i = 0; i < 4; ++i) {
            glm::vec3 position = glm::vec3(0.0f);
            if (Unloved::Session::GetLocalPlayerCount() > i) {
                position = Unloved::Session::GetLocalPlayerByViewportIndex(i)->GetFootPosition();
            }
            OpenGL::SetUniformVec3("positionPlayer" + std::to_string(i), position);
        }

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, fftFrameBuffer_band0->GetColorAttachmentHandleByName("Displacement"));
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, fftFrameBuffer_band1->GetColorAttachmentHandleByName("Displacement"));
        OpenGL::DispatchCompute(1, 1, 1);
        readbackSyncs[idx] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
        if (!readbackSyncs[idx]) {
            GLenum err = glGetError();
            std::cerr << "ERROR: glFenceSync failed for slot " << idx << ", glGetError()=0x" << std::hex << err << std::dec << "\n";
        }
        ++frame;
    }
}
