#include "Unloved/Render/API/OpenGL/GL_renderer.h"

#include "Unloved/Systems/Ocean/Ocean.h"
#include "Unloved/World/World.h"

#include <string>

namespace OpenGL::Renderer {

    struct OceanFFTResources {
        OpenGLFrameBuffer* frameBuffers[Ocean::FFT_BAND_COUNT] = {};
        OpenGLSSBO* h0SSBOs[Ocean::FFT_BAND_COUNT] = {};
        OpenGLSSBO* spectrumInSSBO = nullptr;
        OpenGLSSBO* spectrumOutSSBO = nullptr;
        OpenGLSSBO* dispXInSSBO = nullptr;
        OpenGLSSBO* dispZInSSBO = nullptr;
        OpenGLSSBO* gradXInSSBO = nullptr;
        OpenGLSSBO* gradZInSSBO = nullptr;
        OpenGLSSBO* dispXOutSSBO = nullptr;
        OpenGLSSBO* dispZOutSSBO = nullptr;
        OpenGLSSBO* gradXOutSSBO = nullptr;
        OpenGLSSBO* gradZOutSSBO = nullptr;
    };

    bool LoadOceanFFTResources(OceanFFTResources& resources) {
        for (int i = 0; i < Ocean::FFT_BAND_COUNT; i++) {
            const std::string bandIndex = std::to_string(i);
            resources.frameBuffers[i] = OpenGL::ResourceManager::GetFrameBufferPtr("FFT_band" + bandIndex);
            resources.h0SSBOs[i] = OpenGL::ResourceManager::GetSSBOPtr("ffth0Band" + bandIndex);
            if (!resources.frameBuffers[i]) return false;
            if (!resources.h0SSBOs[i]) return false;
        }

        resources.spectrumInSSBO = OpenGL::ResourceManager::GetSSBOPtr("fftSpectrumInSSBO");
        resources.spectrumOutSSBO = OpenGL::ResourceManager::GetSSBOPtr("fftSpectrumOutSSBO");
        resources.dispXInSSBO = OpenGL::ResourceManager::GetSSBOPtr("fftDispInXSSBO");
        resources.dispZInSSBO = OpenGL::ResourceManager::GetSSBOPtr("fftDispZInSSBO");
        resources.gradXInSSBO = OpenGL::ResourceManager::GetSSBOPtr("fftGradXInSSBO");
        resources.gradZInSSBO = OpenGL::ResourceManager::GetSSBOPtr("fftGradZInSSBO");
        resources.dispXOutSSBO = OpenGL::ResourceManager::GetSSBOPtr("fftDispXOutSSBO");
        resources.dispZOutSSBO = OpenGL::ResourceManager::GetSSBOPtr("fftDispZOutSSBO");
        resources.gradXOutSSBO = OpenGL::ResourceManager::GetSSBOPtr("fftGradXOutSSBO");
        resources.gradZOutSSBO = OpenGL::ResourceManager::GetSSBOPtr("fftGradZOutSSBO");

        if (!resources.spectrumInSSBO) return false;
        if (!resources.spectrumOutSSBO) return false;
        if (!resources.dispXInSSBO) return false;
        if (!resources.dispZInSSBO) return false;
        if (!resources.gradXInSSBO) return false;
        if (!resources.gradZInSSBO) return false;
        if (!resources.dispXOutSSBO) return false;
        if (!resources.dispZOutSSBO) return false;
        if (!resources.gradXOutSSBO) return false;
        if (!resources.gradZOutSSBO) return false;
        if (!OpenGL::ResourceManager::GetShaderPtr("OceanCalculateSpectrum")) return false;
        if (!OpenGL::ResourceManager::GetShaderPtr("OceanUpdateTextures")) return false;
        if (!OpenGL::ResourceManager::GetShaderPtr("FttRadix64Vertical")) return false;
        if (!OpenGL::ResourceManager::GetShaderPtr("FttRadix8Vertical")) return false;
        if (!OpenGL::ResourceManager::GetShaderPtr("FttRadix64Horizontal")) return false;
        if (!OpenGL::ResourceManager::GetShaderPtr("FttRadix8Horizontal")) return false;

        return true;
    }

    void UploadChangedOceanSpectra(const OceanFFTResources& resources) {
        for (int i = 0; i < Ocean::FFT_BAND_COUNT; i++) {
            if (!Ocean::H0UploadRequired(i)) continue;

            const std::vector<std::complex<float>>& h0 = Ocean::GetH0(i);
            resources.h0SSBOs[i]->CopyFrom(h0.data(), sizeof(std::complex<float>) * h0.size());
            Ocean::MarkH0Uploaded(i);
        }
    }

    void ComputeInverseFFT2D(GLuint handleA, GLuint handleB) {
        // Fixed 512 point radix 64 x 8 FFT
        OpenGL::BindShader("FttRadix64Vertical");
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, handleA);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, handleB);
        OpenGL::DispatchCompute(32, 8, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        OpenGL::BindShader("FttRadix8Vertical");
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, handleB);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, handleA);
        OpenGL::DispatchCompute(32, 8, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        OpenGL::BindShader("FttRadix64Horizontal");
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, handleA);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, handleB);
        OpenGL::DispatchCompute(1, Ocean::FFT_RESOLUTION, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        OpenGL::BindShader("FttRadix8Horizontal");
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, handleB);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, handleA);
        OpenGL::DispatchCompute(1, Ocean::FFT_RESOLUTION / 2, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    void GenerateOceanSpectrum(const OceanFFTResources& resources, int bandIndex, float simulationTime) {
        constexpr GLuint localSize = 16;
        constexpr GLuint dispatchSize = (Ocean::FFT_RESOLUTION + localSize - 1) / localSize;

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SSBO_IDX_OCEAN_SPECTRUM_H0, resources.h0SSBOs[bandIndex]->GetHandle());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SSBO_IDX_OCEAN_SPECTRUM_H, resources.spectrumInSSBO->GetHandle());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SSBO_IDX_OCEAN_SPECTRUM_DISPLACEMENT_X, resources.dispXInSSBO->GetHandle());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SSBO_IDX_OCEAN_SPECTRUM_DISPLACEMENT_Z, resources.dispZInSSBO->GetHandle());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SSBO_IDX_OCEAN_SPECTRUM_GRADIENT_X, resources.gradXInSSBO->GetHandle());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SSBO_IDX_OCEAN_SPECTRUM_GRADIENT_Z, resources.gradZInSSBO->GetHandle());

        OpenGL::BindShader("OceanCalculateSpectrum");
        OpenGL::SetUniformFloat("u_domainSize", Ocean::GetDomainSize(bandIndex));
        OpenGL::SetUniformFloat("u_gravity", Ocean::GetGravity());
        OpenGL::SetUniformFloat("u_time", simulationTime);
        OpenGL::DispatchCompute(dispatchSize, dispatchSize, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    void ComputeOceanInverseTransforms(const OceanFFTResources& resources) {
        ComputeInverseFFT2D(resources.spectrumInSSBO->GetHandle(), resources.spectrumOutSSBO->GetHandle());
        ComputeInverseFFT2D(resources.dispXInSSBO->GetHandle(), resources.dispXOutSSBO->GetHandle());
        ComputeInverseFFT2D(resources.dispZInSSBO->GetHandle(), resources.dispZOutSSBO->GetHandle());
        ComputeInverseFFT2D(resources.gradXInSSBO->GetHandle(), resources.gradXOutSSBO->GetHandle());
        ComputeInverseFFT2D(resources.gradZInSSBO->GetHandle(), resources.gradZOutSSBO->GetHandle());
    }

    void WriteOceanBandTextures(const OceanFFTResources& resources, int bandIndex) {
        constexpr GLuint localSize = 16;
        constexpr GLuint dispatchSize = (Ocean::FFT_RESOLUTION + localSize - 1) / localSize;

        OpenGL::BindImageTexture(0, resources.frameBuffers[bandIndex]->GetColorAttachmentHandleByName("Displacement"), GL_WRITE_ONLY, GL_RGBA32F);
        OpenGL::BindImageTexture(1, resources.frameBuffers[bandIndex]->GetColorAttachmentHandleByName("Slope"), GL_WRITE_ONLY, GL_RGBA32F);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SSBO_IDX_OCEAN_UPDATE_H, resources.spectrumInSSBO->GetHandle());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SSBO_IDX_OCEAN_UPDATE_DISPLACEMENT_X, resources.dispXInSSBO->GetHandle());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SSBO_IDX_OCEAN_UPDATE_DISPLACEMENT_Z, resources.dispZInSSBO->GetHandle());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SSBO_IDX_OCEAN_UPDATE_GRADIENT_X, resources.gradXInSSBO->GetHandle());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SSBO_IDX_OCEAN_UPDATE_GRADIENT_Z, resources.gradZInSSBO->GetHandle());

        OpenGL::BindShader("OceanUpdateTextures");
        OpenGL::SetUniformFloat("u_dispScale", Ocean::GetDisplacementScale());
        OpenGL::SetUniformFloat("u_heightScale", Ocean::GetHeightScale());
        OpenGL::DispatchCompute(dispatchSize, dispatchSize, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }

    void SimulateOceanBand(const OceanFFTResources& resources, int bandIndex, float simulationTime) {
        GenerateOceanSpectrum(resources, bandIndex, simulationTime);
        ComputeOceanInverseTransforms(resources);
        WriteOceanBandTextures(resources, bandIndex);
    }

    void GenerateOceanSlopeMipmaps(const OceanFFTResources& resources, int firstBandIndex, int bandCount) {
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_UPDATE_BARRIER_BIT);
        for (int i = 0; i < bandCount; i++) {
            glGenerateTextureMipmap(resources.frameBuffers[firstBandIndex + i]->GetColorAttachmentHandleByName("Slope"));
        }
        glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_TEXTURE_UPDATE_BARRIER_BIT);
    }

    void ComputeOceanFFTPass() {
        ProfilerOpenGLZoneFunction();
        if (!Unloved::World::HasOcean()) return;

        // Only rebuild H0 when a debug setting actually moves
        Ocean::UpdateSpectrum();

        OceanFFTResources resources;
        if (!LoadOceanFFTResources(resources)) return;
        UploadChangedOceanSpectra(resources);

        const float simulationTime = Ocean::UpdateSimulationTime();
        const bool alternateBands = Ocean::GetSettings().alternateBandUpdates;
        static bool texturesInitialized = false;
        static int nextBandIndex = 0;
        const int firstBandIndex = alternateBands && texturesInitialized ? nextBandIndex : 0;
        const int bandCount = alternateBands && texturesInitialized ? 1 : Ocean::FFT_BAND_COUNT;

        for (int i = 0; i < bandCount; i++) {
            SimulateOceanBand(resources, firstBandIndex + i, simulationTime);
        }
        GenerateOceanSlopeMipmaps(resources, firstBandIndex, bandCount);

        texturesInitialized = true;
        nextBandIndex = alternateBands ? (firstBandIndex + bandCount) % Ocean::FFT_BAND_COUNT : 0;
    }
}
