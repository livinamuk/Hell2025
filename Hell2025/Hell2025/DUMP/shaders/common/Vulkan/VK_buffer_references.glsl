#ifndef VULKAN_BUFFER_REFERENCES_GLSL
#define VULKAN_BUFFER_REFERENCES_GLSL

#include "../types.glsl"
#include "VK_types.glsl"

layout(buffer_reference, scalar) readonly buffer RenderItemBuffer {
    RenderItem renderItems[];
};

layout(buffer_reference, scalar) readonly buffer DrawRenderItemIndexBuffer {
    uint renderItemIndices[];
};

layout(buffer_reference, scalar) readonly buffer ViewportDataBuffer {
    ViewportData viewportData[];
};

layout(buffer_reference, scalar) readonly buffer RendererDataBuffer {
    RendererData rendererData;
};

layout(buffer_reference, scalar) readonly buffer MaterialBuffer {
    Material materials[];
};

layout(buffer_reference, scalar) readonly buffer LightBuffer {
    Light lights[];
};

layout(buffer_reference, scalar) readonly buffer SpriteSheetRenderItemBuffer {
    SpriteSheetRenderItem spriteSheetRenderItems[];
};

layout(buffer_reference, scalar) readonly buffer RenderItemUIBuffer {
    RenderItemUI uiRenderItems[];
};

layout(buffer_reference, scalar) buffer TileLightsBuffer {
    TileLights tileLights[];
};

layout(buffer_reference, scalar) buffer TileWorldBoundsBuffer {
    TileWorldBounds tileWorldBounds[];
};

#endif
