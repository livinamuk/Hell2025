#include "Hell/Common/Constants.h"

#include "Unloved/Config/Config.h"
#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Unloved/Render/Renderer.h"
#include "Unloved/Render/RendererConstants.h"
#include "Unloved/Render/RendererTypes.h"
#include "Unloved/Systems/Ocean/Ocean.h"

namespace OpenGL::Renderer {

    void CreateFramebuffers() {
        const Resolutions& resolutions = Config::GetResolutions();

        OpenGLFrameBuffer& gBuffer = OpenGL::ResourceManager::CreateFrameBuffer("GBuffer");
        gBuffer.Create(resolutions.gBuffer);
        gBuffer.CreateAttachment("BaseColorMetallic", GL_RGBA8);
        gBuffer.CreateAttachment("NormalXYRoughnessMisc", GL_RGB10_A2);
        gBuffer.CreateAttachment("VelocityXYOcclusionSubSurface", GL_RGBA16F);
        gBuffer.CreateAttachment("Lighting", GL_RGBA16F, GL_LINEAR, GL_LINEAR);
        gBuffer.CreateAttachment("Emissive", GL_RGBA8, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE);
        gBuffer.CreateAttachment("Glass", GL_RGBA16F);
        gBuffer.CreateAttachment("Visibility", GL_RG32UI);
        gBuffer.CreateDepthAttachment(GL_DEPTH32F_STENCIL8);

        // Reverse-Z minimum pyramid used for conservative occlusion tests.
        // Mip zero is already a 2x2 reduction of the full-resolution depth.
        const glm::ivec2 occlusionHiZResolution = glm::max((resolutions.gBuffer + 1) / 2, glm::ivec2(1));
        OpenGLFrameBuffer& occlusionHiZFbo = OpenGL::ResourceManager::CreateFrameBuffer("OcclusionHiZ");
        occlusionHiZFbo.Create(occlusionHiZResolution);
        occlusionHiZFbo.CreateAttachment("MinDepth", GL_R32F, GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST, GL_CLAMP_TO_EDGE, true);
        glTextureParameteri(occlusionHiZFbo.GetColorAttachmentHandleByName("MinDepth"), GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);

        OpenGLFrameBuffer& hairFboRE = OpenGL::ResourceManager::CreateFrameBuffer("HairRE");
        hairFboRE.Create(resolutions.gBuffer, 4);
        hairFboRE.CreateAttachment("Lighting", GL_RGBA16F);
        hairFboRE.CreateDepthAttachment(GL_DEPTH32F_STENCIL8);

        OpenGLFrameBuffer& indirectDiffuseFbo = OpenGL::ResourceManager::CreateFrameBuffer("IndirectDiffuse");
        indirectDiffuseFbo.Create(resolutions.gBufferHalfRes);
        indirectDiffuseFbo.CreateAttachment("Color", GL_R11F_G11F_B10F);
        indirectDiffuseFbo.CreateAttachment("Surface", GL_RGBA16F, GL_NEAREST, GL_NEAREST);

        OpenGLFrameBuffer& taaFbo = OpenGL::ResourceManager::CreateFrameBuffer("TAA");
        taaFbo.Create(resolutions.gBuffer);
        taaFbo.CreateAttachment("History", GL_RGBA16F);
        taaFbo.CreateAttachment("Output", GL_RGBA16F);

        OpenGLFrameBuffer& scratchFbo = OpenGL::ResourceManager::CreateFrameBuffer("Scratch");
        scratchFbo.Create(resolutions.gBuffer);
        scratchFbo.CreateAttachment("RGBA16F", GL_RGBA16F);

        OpenGL::ResourceManager::CreateSSBO("BubblePositions").Create(sizeof(glm::vec4) * 100, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("BubblePositionCount").Create(sizeof(uint64_t), GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("BubbleDrawCommand").Create(sizeof(DrawArraysIndirectCommand), GL_DYNAMIC_STORAGE_BIT);

        OpenGL::ClearSSBO("BubblePositions");
        OpenGL::ClearSSBO("BubblePositionCount");

        // Particles
        OpenGL::ResourceManager::CreateSSBO("ParticlePool").Create(sizeof(GpuParticle) * MAX_GPU_PARTICLES, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("ParticleAdditions").Create(sizeof(GpuParticle) * MAX_GPU_PARTICLES, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("ParticleAdditionCounter").Create(sizeof(uint32_t), GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("ParticleActiveIndices").Create(sizeof(uint32_t) * MAX_GPU_PARTICLES, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("ParticleDrawCommand").Create(sizeof(DrawArraysIndirectCommand), GL_DYNAMIC_STORAGE_BIT);

        OpenGL::ClearSSBO("ParticlePool");
        OpenGL::ClearSSBO("ParticleAdditions");

        // Zero out QUAD draw commands
        DrawArraysIndirectCommand particleDrawCommand;
        particleDrawCommand.vertexCount = 6;
        particleDrawCommand.instanceCount = 0;
        particleDrawCommand.firstVertex = 0;
        particleDrawCommand.baseInstance = 0;

        OpenGL::UploadSSBOStatic("BubbleDrawCommand", sizeof(DrawArraysIndirectCommand), &particleDrawCommand);
        OpenGL::UploadSSBOStatic("ParticleDrawCommand", sizeof(DrawArraysIndirectCommand), &particleDrawCommand);








        OpenGLCubemapFrameBuffer& lightAABBfbo = OpenGL::ResourceManager::CreateCubemapFrameBuffer("LightAABB");
        lightAABBfbo.Create(512);
        lightAABBfbo.CreateAttachment(GL_RGBA32F, GL_NEAREST);
        lightAABBfbo.CreateDepthAttachment(GL_DEPTH_COMPONENT32F);



        OpenGLFrameBuffer& waterFbo = OpenGL::ResourceManager::CreateFrameBuffer("Water");
        waterFbo.Create(resolutions.gBuffer);
        waterFbo.CreateAttachment("Lighting", GL_RGBA16F);
        waterFbo.CreateAttachment("OceanFlags", GL_R8UI);
        waterFbo.CreateAttachment("OceanMask", GL_R8UI);
        waterFbo.CreateDepthAttachment(GL_DEPTH32F_STENCIL8);

        // One half-resolution emissive bloom pyramid is reused for every
        // visible viewport, so split-screen views can never filter into each other.
        constexpr int EMISSIVE_BLOOM_MIP_ALIGNMENT = 4;
        const glm::ivec2 halfGBufferResolution = glm::max((resolutions.gBuffer + 1) / 2, glm::ivec2(1));
        const glm::ivec2 emissiveBloomResolution = ((halfGBufferResolution + EMISSIVE_BLOOM_MIP_ALIGNMENT - 1) / EMISSIVE_BLOOM_MIP_ALIGNMENT) * EMISSIVE_BLOOM_MIP_ALIGNMENT;
        OpenGLFrameBuffer& emissiveBloomPyramidFbo = OpenGL::ResourceManager::CreateFrameBuffer("EmissiveBloomPyramid");
        emissiveBloomPyramidFbo.Create(emissiveBloomResolution);
        emissiveBloomPyramidFbo.CreateAttachment("ColorA", GL_R11F_G11F_B10F, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, true);
        emissiveBloomPyramidFbo.CreateAttachment("ColorB", GL_R11F_G11F_B10F, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, true);

        OpenGLFrameBuffer& depthPeeledTransparencyFbo = OpenGL::ResourceManager::CreateFrameBuffer("DepthPeeledTransparency");
        depthPeeledTransparencyFbo.Create(resolutions.gBuffer);
        depthPeeledTransparencyFbo.CreateAttachment("Color", GL_RGBA16F);
        depthPeeledTransparencyFbo.CreateAttachment("ViewspaceDepth", GL_R32F);
        depthPeeledTransparencyFbo.CreateAttachment("ViewspaceDepthPrevious", GL_R32F);
        depthPeeledTransparencyFbo.CreateAttachment("Composite", GL_RGBA16F);
        depthPeeledTransparencyFbo.CreateDepthAttachment(GL_DEPTH32F_STENCIL8);

        OpenGLFrameBuffer& gaussianBlurFbo = OpenGL::ResourceManager::CreateFrameBuffer("GaussianBlur");
        gaussianBlurFbo.Create(resolutions.gBuffer.x / 2, resolutions.gBuffer.y / 2);
        gaussianBlurFbo.CreateAttachment("ColorA", GL_RGBA16F, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE);
        gaussianBlurFbo.CreateAttachment("ColorB", GL_RGBA16F, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE);

        OpenGLFrameBuffer& decalPaintingFbo = OpenGL::ResourceManager::CreateFrameBuffer("DecalPainting");
        decalPaintingFbo.Create(512, 512);
        decalPaintingFbo.CreateAttachment("UVMap", GL_RGBA8, GL_LINEAR, GL_LINEAR);
        decalPaintingFbo.CreateDepthAttachment(GL_DEPTH_COMPONENT24);

        OpenGL::ResourceManager::CreateFrameBuffer("DecalMasks").Create(WOUND_MASK_TEXTURE_SIZE, WOUND_MASK_TEXTURE_SIZE);

        OpenGLFrameBuffer& gBufferBackupFbo = OpenGL::ResourceManager::CreateFrameBuffer("GBufferBackup");
        gBufferBackupFbo.Create(resolutions.gBuffer);
        gBufferBackupFbo.CreateDepthAttachment(GL_DEPTH32F_STENCIL8); // do you really need this? you have WIP below

        OpenGLFrameBuffer& wipFbo = OpenGL::ResourceManager::CreateFrameBuffer("WIP");
        wipFbo.Create(resolutions.gBuffer);
        wipFbo.CreateDepthAttachment(GL_DEPTH32F_STENCIL8);

        OpenGLFrameBuffer& fogFbo = OpenGL::ResourceManager::CreateFrameBuffer("Fog");
        fogFbo.Create(resolutions.gBuffer / 2);
        fogFbo.CreateAttachment("Color", GL_RGBA16F, GL_LINEAR, GL_LINEAR);

        OpenGLFrameBuffer& quarterSizeFbo = OpenGL::ResourceManager::CreateFrameBuffer("QuarterSize");
        quarterSizeFbo.Create(resolutions.gBuffer.x / 4, resolutions.gBuffer.y / 4);
        quarterSizeFbo.CreateAttachment("DownsampledFinalLighting", GL_RGBA16F);

        OpenGLFrameBuffer& halfSizeFbo = OpenGL::ResourceManager::CreateFrameBuffer("HalfSize");
        halfSizeFbo.Create(resolutions.gBuffer.x / 2, resolutions.gBuffer.y / 2);
        halfSizeFbo.CreateAttachment("DownsampledFinalLighting", GL_RGBA16F, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, true);
        halfSizeFbo.CreateAttachment("SSRHistoryA", GL_RGBA16F);
        halfSizeFbo.CreateAttachment("SSRHistoryB", GL_RGBA16F);
        halfSizeFbo.CreateAttachment("SSRCurrent", GL_RGBA16F);

        OpenGLFrameBuffer& miscFullSizeFbo = OpenGL::ResourceManager::CreateFrameBuffer("MiscFullSize");
        miscFullSizeFbo.Create(resolutions.gBuffer);
        miscFullSizeFbo.CreateAttachment("GaussianFinalLightingIntermediate", GL_RGBA16F);
        miscFullSizeFbo.CreateAttachment("GaussianFinalLighting", GL_RGBA16F);
        miscFullSizeFbo.CreateAttachment("BloodScreenSpaceDecalMask", GL_R8);
        miscFullSizeFbo.CreateAttachment("ViewspaceDepth", GL_R32F, GL_NEAREST, GL_NEAREST);
        miscFullSizeFbo.CreateAttachment("FinalLightingCopy", GL_RGBA16F, GL_LINEAR, GL_LINEAR);

        OpenGLFrameBuffer& outlineFbo = OpenGL::ResourceManager::CreateFrameBuffer("Outline");
        outlineFbo.Create(resolutions.gBuffer);
        outlineFbo.CreateAttachment("Mask", GL_R8);
        outlineFbo.CreateAttachment("Result", GL_R8);

        OpenGLFrameBuffer& hairFbo = OpenGL::ResourceManager::CreateFrameBuffer("Hair");
        hairFbo.Create(resolutions.hair);
        hairFbo.CreateDepthAttachment(GL_DEPTH32F_STENCIL8);
        hairFbo.CreateAttachment("Lighting", GL_RGBA16F);
        hairFbo.CreateAttachment("ViewspaceDepth", GL_R32F);
        hairFbo.CreateAttachment("ViewspaceDepthPrevious", GL_R32F);
        hairFbo.CreateAttachment("Composite", GL_RGBA16F);

        OpenGLFrameBuffer& finalImageFbo = OpenGL::ResourceManager::CreateFrameBuffer("FinalImage");
        finalImageFbo.Create(resolutions.finalImage);
        finalImageFbo.CreateAttachment("Color", GL_RGBA16F);

        OpenGLFrameBuffer& presentFbo = OpenGL::ResourceManager::CreateFrameBuffer("Present");
        presentFbo.Create(resolutions.ui);
        presentFbo.CreateAttachment("Color", GL_RGBA16F, GL_NEAREST, GL_NEAREST);

        OpenGLFrameBuffer& worldFbo = OpenGL::ResourceManager::CreateFrameBuffer("World");
        worldFbo.Create(1, 1);
        worldFbo.CreateAttachment("HeightMap", GL_R32F);
        worldFbo.CreateAttachment("TerrainControl", GL_R32UI, GL_NEAREST, GL_NEAREST);

        OpenGLFrameBuffer& roadFbo = OpenGL::ResourceManager::CreateFrameBuffer("Road");
        roadFbo.Create(1, 1);
        roadFbo.CreateAttachment("RoadMask", GL_R16F);

        OpenGLFrameBuffer& fftFrameBufferBand0 = OpenGL::ResourceManager::CreateFrameBuffer("FFT_band0");
        fftFrameBufferBand0.Create(Ocean::FFT_RESOLUTION, Ocean::FFT_RESOLUTION);
        fftFrameBufferBand0.CreateAttachment("Displacement", GL_RGBA32F, GL_LINEAR, GL_LINEAR, GL_REPEAT);
        fftFrameBufferBand0.CreateAttachment("Slope", GL_RGBA32F, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_REPEAT, true);

        OpenGLFrameBuffer& fftFrameBufferBand1 = OpenGL::ResourceManager::CreateFrameBuffer("FFT_band1");
        fftFrameBufferBand1.Create(Ocean::FFT_RESOLUTION, Ocean::FFT_RESOLUTION);
        fftFrameBufferBand1.CreateAttachment("Displacement", GL_RGBA32F, GL_LINEAR, GL_LINEAR, GL_REPEAT, true);
        fftFrameBufferBand1.CreateAttachment("Slope", GL_RGBA32F, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_REPEAT, true);
    }
}
