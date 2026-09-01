#ifndef VULKAN_TYPES_GLSL
#define VULKAN_TYPES_GLSL

struct RenderItemUI {
    uint baseVertex;
    uint baseIndex;
    uint indexCount;
    uint textureIndex;

    int clipMinX;
    int clipMinY;
    int clipMaxX;
    int clipMaxY;

    uint filterMode;
    int padding0;
    int padding1;
    int padding2;
};

#endif
