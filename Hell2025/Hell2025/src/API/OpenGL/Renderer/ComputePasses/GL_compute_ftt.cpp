#include "API/OpenGL/Renderer/GL_renderer.h"
#include "Core/Game.h"
#include "Ocean/Ocean.h"
#include "Input/Input.h"

namespace OpenGLRenderer {

    void ComputeInverseFFT2D(GLuint handleA, GLuint handleB);
    float g_globalTime = 50.0f;

    void ComputeOceanFFTPass() {
        OpenGLFrameBuffer* fftFrameBuffer_band0 = GetFrameBuffer("FFT_band0");
        OpenGLFrameBuffer* fftFrameBuffer_band1 = GetFrameBuffer("FFT_band1");
        OpenGLMeshPatch* oceanMeshPatch = GetOceanMeshPatch();

        OpenGLShader* oceanCalculateSpectrumShader = GetShader("OceanCalculateSpectrum");
        OpenGLShader* oceanUpdateTexturesShader = GetShader("OceanUpdateTextures");

        OpenGLSSBO* fftH0SSBO_band0 = GetSSBO("ffth0Band0");
        OpenGLSSBO* fftH0SSBO_band1 = GetSSBO("ffth0Band1");
        OpenGLSSBO* fftSpectrumInSSBO = GetSSBO("fftSpectrumInSSBO");
        OpenGLSSBO* fftSpectrumOutSSBO = GetSSBO("fftSpectrumOutSSBO");
        OpenGLSSBO* fftDispXInSSBO = GetSSBO("fftDispInXSSBO");
        OpenGLSSBO* fftDispZInSSBO = GetSSBO("fftDispZInSSBO");
        OpenGLSSBO* fftGradXInSSBO = GetSSBO("fftGradXInSSBO");
        OpenGLSSBO* fftGradZInSSBO = GetSSBO("fftGradZInSSBO");
        OpenGLSSBO* fftDispXOutSSBO = GetSSBO("fftDispXOutSSBO");
        OpenGLSSBO* fftDispZOutSSBO = GetSSBO("fftDispZOutSSBO");
        OpenGLSSBO* fftGradXOutSSBO = GetSSBO("fftGradXOutSSBO");
        OpenGLSSBO* fftGradZOutSSBO = GetSSBO("fftGradZOutSSBO");

        if (!fftFrameBuffer_band0) return;
        if (!fftFrameBuffer_band1) return;
        if (!fftH0SSBO_band0) return;
        if (!fftH0SSBO_band1) return;
        if (!fftSpectrumInSSBO) return;
        if (!fftSpectrumOutSSBO) return;
        if (!fftDispXInSSBO) return;
        if (!fftDispZInSSBO) return;
        if (!fftGradXInSSBO) return;
        if (!fftGradZInSSBO) return;
        if (!fftDispXOutSSBO) return;
        if (!fftDispZOutSSBO) return;
        if (!fftGradXOutSSBO) return;
        if (!fftGradZOutSSBO) return;
        if (!oceanMeshPatch) return;
        if (!oceanUpdateTexturesShader) return;
        if (!oceanCalculateSpectrumShader) return;

        static double lastTime = glfwGetTime();
        double currentTime = glfwGetTime();
        float deltaTime = float(currentTime - lastTime);
        lastTime = currentTime;

        static bool doTime = true;

        if (doTime) {
            g_globalTime += deltaTime;
        }
        else {
            g_globalTime = 50.0f;
        }
        if (Input::KeyPressed(HELL_KEY_T)) {
            doTime = !doTime;
        }

        int bandCount = 2;

        const float gravity = Ocean::GetGravity();

        for (int i = 0; i < bandCount; i++) {

            if (GetFftDisplayMode() == 1 && i == 1) {
                continue;
            }
            if (GetFftDisplayMode() == 2 && i == 0) {
                continue;
            }
            
            const glm::uvec2 fftResolution = Ocean::GetFFTResolution(i);
            const glm::vec2 patchSimSize = Ocean::GetPatchSimSize(i);

            const GLuint blocksPerSide = 16;
            const GLuint blockSizeX = fftResolution.x / blocksPerSide;
            const GLuint blockSizeY = fftResolution.y / blocksPerSide;

            // Generate spectrum on GPU
            if (i == 0) {
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, fftH0SSBO_band0->GetHandle());
            }
            if (i == 1) {
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, fftH0SSBO_band1->GetHandle());
            }

            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, fftSpectrumInSSBO->GetHandle());
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, fftDispXInSSBO->GetHandle());
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, fftDispZInSSBO->GetHandle());
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, fftGradXInSSBO->GetHandle());
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, fftGradZInSSBO->GetHandle());

            oceanCalculateSpectrumShader->Bind();
            oceanCalculateSpectrumShader->SetUvec2("u_fftGridSize", fftResolution);
            oceanCalculateSpectrumShader->SetVec2("u_patchSimSize", patchSimSize);
            oceanCalculateSpectrumShader->SetFloat("u_gravity", gravity);
            oceanCalculateSpectrumShader->SetFloat("u_time", g_globalTime);
            glDispatchCompute(blockSizeX, blockSizeY, 1);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

            // Perform FFT
            ComputeInverseFFT2D(fftSpectrumInSSBO->GetHandle(), fftSpectrumOutSSBO->GetHandle());
            ComputeInverseFFT2D(fftDispXInSSBO->GetHandle(), fftDispXOutSSBO->GetHandle());
            ComputeInverseFFT2D(fftDispZInSSBO->GetHandle(), fftDispZOutSSBO->GetHandle());
            ComputeInverseFFT2D(fftGradXInSSBO->GetHandle(), fftGradXOutSSBO->GetHandle());
            ComputeInverseFFT2D(fftGradZInSSBO->GetHandle(), fftGradZOutSSBO->GetHandle());

            // Update mesh position
            if (i == 0) {
                glBindImageTexture(0, fftFrameBuffer_band0->GetColorAttachmentHandleByName("Displacement"), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
                glBindImageTexture(1, fftFrameBuffer_band0->GetColorAttachmentHandleByName("Normals"), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
            }
            if (i == 1) {
                glBindImageTexture(0, fftFrameBuffer_band1->GetColorAttachmentHandleByName("Displacement"), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
                glBindImageTexture(1, fftFrameBuffer_band1->GetColorAttachmentHandleByName("Normals"), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
            }

            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, fftSpectrumInSSBO->GetHandle());
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, fftDispXInSSBO->GetHandle());
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, fftDispZInSSBO->GetHandle());
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, fftGradXInSSBO->GetHandle());
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, fftGradZInSSBO->GetHandle());
          
            oceanUpdateTexturesShader->Bind();
            oceanUpdateTexturesShader->SetUvec2("u_fftGridSize", fftResolution);
            oceanUpdateTexturesShader->SetFloat("u_dispScale", Ocean::GetDisplacementScale());
            oceanUpdateTexturesShader->SetFloat("u_heightScale", Ocean::GetHeightScale());

            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
            glDispatchCompute(blockSizeX, blockSizeY, 1);
        }
    }

    void ComputeInverseFFT2D(GLuint handleA, GLuint handleB) {
        OpenGLShader* radix64Vert = GetShader("FttRadix64Vertical");
        OpenGLShader* radix8Vert = GetShader("FttRadix8Vertical");
        OpenGLShader* radix64Hori = GetShader("FttRadix64Horizontal");
        OpenGLShader* radix8Hori = GetShader("FttRadix8Horizontal");

        if (!radix64Vert) return;
        if (!radix8Vert) return;
        if (!radix64Hori) return;
        if (!radix8Hori) return;

        radix64Vert->Bind();
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, handleA);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, handleB);
        glDispatchCompute(32, 8, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        radix8Vert->Bind();
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, handleB);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, handleA);
        glDispatchCompute(32, 8, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        radix64Hori->Bind();
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, handleA);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, handleB);
        glDispatchCompute(1, 512, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        radix8Hori->Bind();
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, handleB);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, handleA);
        glDispatchCompute(1, 256, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }
}
