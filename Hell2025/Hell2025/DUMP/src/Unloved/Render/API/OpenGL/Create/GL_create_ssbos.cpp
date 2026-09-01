#include "Hell/Common/Constants.h"

#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Unloved/Render/Renderer.h"
#include "Unloved/Render/RendererConstants.h"
#include "Unloved/Render/RendererTypes.h"
#include "Unloved/Systems/Ocean/Ocean.h"

#include <cstddef>
#include <string>

namespace OpenGL::Renderer{

    void CreateSSBOs() {
        GLbitfield staticFlags = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
        GLbitfield dynamicFlags = GL_DYNAMIC_STORAGE_BIT | GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;

        // Ocean
        const size_t oceanFFTByteSize = Ocean::FFT_RESOLUTION * Ocean::FFT_RESOLUTION * sizeof(std::complex<float>);
        for (int i = 0; i < Ocean::FFT_BAND_COUNT; i++) {
            OpenGL::ResourceManager::CreateSSBO("ffth0Band" + std::to_string(i)).Create(oceanFFTByteSize, staticFlags);
        }
        OpenGL::ResourceManager::CreateSSBO("fftSpectrumInSSBO").Create(oceanFFTByteSize, dynamicFlags);
        OpenGL::ResourceManager::CreateSSBO("fftSpectrumOutSSBO").Create(oceanFFTByteSize, dynamicFlags);
        OpenGL::ResourceManager::CreateSSBO("fftDispInXSSBO").Create(oceanFFTByteSize, dynamicFlags);
        OpenGL::ResourceManager::CreateSSBO("fftDispZInSSBO").Create(oceanFFTByteSize, dynamicFlags);
        OpenGL::ResourceManager::CreateSSBO("fftGradXInSSBO").Create(oceanFFTByteSize, dynamicFlags);
        OpenGL::ResourceManager::CreateSSBO("fftGradZInSSBO").Create(oceanFFTByteSize, dynamicFlags);
        OpenGL::ResourceManager::CreateSSBO("fftDispXOutSSBO").Create(oceanFFTByteSize, dynamicFlags);
        OpenGL::ResourceManager::CreateSSBO("fftDispZOutSSBO").Create(oceanFFTByteSize, dynamicFlags);
        OpenGL::ResourceManager::CreateSSBO("fftGradXOutSSBO").Create(oceanFFTByteSize, dynamicFlags);
        OpenGL::ResourceManager::CreateSSBO("fftGradZOutSSBO").Create(oceanFFTByteSize, dynamicFlags);

        int dummySize = 64;

        // Core
        OpenGL::ResourceManager::CreateSSBO("Samplers").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("ViewportData").Create(sizeof(ViewportData) * 4, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("RendererData").Create(sizeof(RendererData), GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("GlassLightRanges").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("GlassLightIndices").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("GlassSpotLightRanges").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("GlassSpotLightIndices").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("SceneRenderItems").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("DrawRenderItemIndices").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("SpriteSheetInstanceData").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("Lights").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("SpotLights").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("PointShadowStaticHiResFaceData").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("PointShadowStaticLowResFaceData").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("PointShadowHiResFaceData").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("PointShadowLowResFaceData").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("PointShadowStaticHiResDrawCommands").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("PointShadowStaticLowResDrawCommands").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("PointShadowHiResDrawCommands").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("PointShadowLowResDrawCommands").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("PointShadowDrawFaceIndices").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);

        // Skinning
        OpenGL::ResourceManager::CreateSSBO("SkinningDispatchGroups").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("SkinningJobs").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("SkinningMorphJobs").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("SkinningMorphTargets").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("SkinningTransforms").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("PreviousSkinningTransforms").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);

        OpenGL::ResourceManager::CreateSSBO("Materials").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);

        OpenGL::ResourceManager::CreateSSBO("RenderItemsUI").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);

        // Vertices
        OpenGL::ResourceManager::CreateSSBO("Indices2");
        OpenGL::ResourceManager::CreateSSBO("Vertices2");

        // Raytracing
        OpenGL::ResourceManager::CreateSSBO("TriangleData").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("SceneBvh").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("MeshesBvh").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("EntityInstances").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("PointGridBuffer").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("PointIndicesBuffer").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);

        // DDGI
        OpenGL::ResourceManager::CreateSSBO("DDGIVolume").Create(sizeof(DDGIVolumeGPU), GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("DirtyDoorAABBs").Create(sizeof(GPUAABB), GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("PointCloudGridCounts").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("PointCloudDirtyFlags").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("PointCloudGridOffsets").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("PointCloudTextureInfo").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("ProbeDistanceCounter").Create(sizeof(uint32_t), GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("ProbeDistanceDispatchArgs").Create(sizeof(DispatchIndirectCommand), GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("ProbeDistanceIndices").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("ProbeIndexCounter").Create(sizeof(uint32_t), GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("ProbeIrradianceCounter").Create(sizeof(uint32_t), GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("ProbeIrradianceDispatchArgs").Create(sizeof(DispatchIndirectCommand), GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("ProbeIrradianceIndices").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("ProbePointIndices").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("ProbePointOffsets").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("ProbePointCounts").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("ProbeStates").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);

        OpenGL::ResourceManager::CreateSSBO("LightAABBs").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);

        // Tile data
        OpenGL::ResourceManager::CreateSSBO("TileChristmasLights").Create(GetTileCount() * sizeof(TileInstanceData), HELL_NONE_BIT);
        OpenGL::ResourceManager::CreateSSBO("TileBloodDecals").Create(GetTileCount() * sizeof(TileInstanceData), HELL_NONE_BIT);
        OpenGL::ResourceManager::CreateSSBO("TileLights").Create(GetTileCount() * sizeof(TileLights), HELL_NONE_BIT);
        OpenGL::ResourceManager::CreateSSBO("TileSpotLights").Create(GetTileCount() * sizeof(TileSpotLights), HELL_NONE_BIT);
        OpenGL::ResourceManager::CreateSSBO("TileWorldBounds").Create(GetTileCount() * sizeof(TileWorldBounds), HELL_NONE_BIT);

        // Instance data
        OpenGL::ResourceManager::CreateSSBO("BloodDecalCounter").Create(sizeof(uint32_t), GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("BloodDecalIndices").Create(sizeof(uint32_t) * GetTileCount() * 256, HELL_NONE_BIT);
        OpenGL::ResourceManager::CreateSSBO("BloodDecalInstances").Create(sizeof(BloodDecalInstanceData) * MAX_SCREEN_SPACE_BLOOD_DECAL_COUNT, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("ChristmasLightCounter").Create(sizeof(uint32_t), GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("ChristmasLightIndices").Create(sizeof(uint32_t) * GetTileCount() * 256, HELL_NONE_BIT);
        OpenGL::ResourceManager::CreateSSBO("ChristmasLightInstances").Create(MAX_CHRISTMAS_LIGHTS * sizeof(GPUChristmasLight), GL_DYNAMIC_STORAGE_BIT);

        // Remove me at some point
        OpenGL::ResourceManager::CreateSSBO("MetaBalls").Create(sizeof(glm::vec4) * 1000, GL_DYNAMIC_STORAGE_BIT);

        // Preallocate the indirect command buffer
        IndirectBuffer& indirectBuffer = GetIndirectBuffer();
        indirectBuffer.PreAllocate(sizeof(DrawIndexedIndirectCommand) * MAX_INDIRECT_DRAW_COMMAND_COUNT);
    }
}
