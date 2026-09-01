#include "Unloved/Render/API/Vulkan/VK_renderer_internal.h"

#include "Hell/Profiling/CPUProfiler.h"
#include "Unloved/Render/Renderer.h"
#include "Unloved/Render/Renderer_settings.h"
#include "Unloved/Systems/DDGI/DDGIManager.h"

namespace VulkanRenderer {
    namespace {
        void SubmitDDGIGridDebugDraw() {
            const RendererSettings& rendererSettings = Unloved::Renderer::GetCurrentRendererSettings();
            if (!rendererSettings.debugDrawPointCloudGrid) return;

            Hell::SlotMap<Unloved::DDGIVolume>& ddgiVolumes = Unloved::DDGIManager::GetVolumes();
            for (Unloved::DDGIVolume& ddgiVolume : ddgiVolumes) {
                ddgiVolume.GetPointClound().DebugDrawGrid();
            }
        }
    }

    bool RecordGameFrame(SwapchainFrame& frame) {
        ProfilerCPUZoneFunction();

        if (Unloved::Renderer::DDGIEnabled()) {
            SubmitDDGIGridDebugDraw();
        }

        if (!UpdateBuffers()) return false;
        if (!UpdateBuffersUI()) return false;

        if (!UpdateFrameAddressTable()) return false;

        ComputeSkinningPass(frame.commandBuffer);
        PointLightShadowPass(frame.commandBuffer);
        UpdateRayTracing(frame.commandBuffer);

        if (Unloved::Renderer::DDGIEnabled()) {
            DDGIPointCloudPass(frame.commandBuffer);
        }

        VisibilityPass(frame.commandBuffer);
        MaterialResolvePass(frame.commandBuffer);
        EmissiveForwardPass(frame.commandBuffer);

        if (Unloved::Renderer::DDGIEnabled()) {
            DDGIProbeUpdatePass(frame.commandBuffer);
            DDGIIrradianceTexturePass(frame.commandBuffer);
        }

        ComputeTileWorldBounds(frame.commandBuffer);
        LightCullingPass(frame.commandBuffer);

        if (Unloved::Renderer::IndirectSpecularEnabled()) {
            IndirectSpecularClassifyTilesPass(frame.commandBuffer);
            IndirectSpecularInputPass(frame.commandBuffer);
            IndirectSpecularReprojectPass(frame.commandBuffer);
            IndirectSpecularPrefilterPass(frame.commandBuffer);
            IndirectSpecularResolveTemporalPass(frame.commandBuffer);
        }

        LightingPass(frame.commandBuffer);

        LightingForwardBlendedPass(frame.commandBuffer);
        SkyboxPass(frame.commandBuffer);
        HairPass(frame.commandBuffer);
        EmissiveBloomPass(frame.commandBuffer);
        SpriteSheetPass(frame.commandBuffer); // Muzzle flash, etc

        PostProcessingPass(frame.commandBuffer);

        DebugViewPass(frame.commandBuffer);
        DebugTileViewPass(frame.commandBuffer);

        if (Unloved::Renderer::DDGIEnabled()) {
            DDGIRaytraceScenePass(frame.commandBuffer);
            DDGIPointCloudDebugPass(frame.commandBuffer);
            DDGIProbeDebugPass(frame.commandBuffer);
        }

        DebugPass(frame.commandBuffer);

        BlitImage(frame.commandBuffer, "Lighting", "FinalImage", VK_FILTER_LINEAR);
        BlitImage(frame.commandBuffer, "FinalImage", "Present", VK_FILTER_NEAREST);

        RenderGameUIPass(frame.commandBuffer);
        PresentPass(frame.commandBuffer, frame.swapchainImageView, frame.extent);
        RenderEditorUIPass(frame.commandBuffer, frame.swapchainImage, frame.swapchainImageView, frame.extent);
        HiZPass(frame.commandBuffer);

        return true;
    }

    void RenderGame() {
        SwapchainFrame frame;
        if (!BeginSwapchainFrame(frame)) return;

        if (!RecordGameFrame(frame)) {
            EndSwapchainFrame(frame);
            return;
        }

        EndSwapchainFrame(frame);
    }
}
