#include "Unloved/Render/API/Vulkan/VK_renderer_internal.h"

namespace VulkanRenderer {

    void RenderGame() {
        SwapchainFrame frame;
        if (!BeginSwapchainFrame(frame)) return;

        UpdateBuffers();
        UpdateBuffersUI();

        ComputeSkinningPass(frame.commandBuffer);
        UpdateRayTracing(frame.commandBuffer);

        VisibilityPass(frame.commandBuffer);
        MaterialResolvePass(frame.commandBuffer);
        LightingPass(frame.commandBuffer);
        LightingForwardBlendedPass(frame.commandBuffer);
        SkyboxPass(frame.commandBuffer);
        // HairPass(frame.commandBuffer);
        PostProcessingPass(frame.commandBuffer);

        DebugViewPass(frame.commandBuffer);
        ComputeDebugTileViewPass(frame.commandBuffer);

        BlitImage(frame.commandBuffer, "Lighting", "FinalImage", VK_FILTER_LINEAR);
        BlitImage(frame.commandBuffer, "FinalImage", "Present", VK_FILTER_NEAREST);

        RenderUIPass(frame.commandBuffer);

        PresentPass(frame.commandBuffer, frame.swapchainImageView);
        EndSwapchainFrame(frame);
    }
}
