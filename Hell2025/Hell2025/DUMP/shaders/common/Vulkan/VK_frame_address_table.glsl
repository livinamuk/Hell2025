#ifndef VULKAN_FRAME_ADDRESS_TABLE_GLSL
#define VULKAN_FRAME_ADDRESS_TABLE_GLSL

#include "VK_buffer_references.glsl"

layout(buffer_reference, scalar, buffer_reference_align = 8)
readonly buffer FrameAddressTable {
    RenderItemBuffer sceneRenderItemBuffer;
    DrawRenderItemIndexBuffer drawRenderItemIndexBuffer;
    ViewportDataBuffer viewportDataBuffer;
    RendererDataBuffer rendererDataBuffer;
    MaterialBuffer materialBuffer;
    LightBuffer lightBuffer;
    SpriteSheetRenderItemBuffer spriteSheetRenderItemBuffer;
    RenderItemUIBuffer uiRenderItemBuffer;
    TileLightsBuffer tileLightBuffer;
    TileWorldBoundsBuffer tileWorldBoundsBuffer;
};

#endif
