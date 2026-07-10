#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

inline constexpr size_t VULKAN_MAX_UI_VERTICES = 262144;
inline constexpr size_t VULKAN_MAX_UI_INDICES = 393216;
inline constexpr size_t VULKAN_MAX_UI_RENDER_ITEMS = 16384;

struct VulkanFrameData {
    struct Buffers {
        uint64_t instanceData = 0;
        uint64_t viewportData = 0;
        uint64_t rendererData = 0;
        uint64_t lights = 0;
        uint64_t materials = 0;
        uint64_t drawCommands = 0;
        uint64_t rayQueryInstances = 0;
        uint64_t rayQueryInstanceData = 0;
        uint64_t rayQueryGeometryData = 0;
        uint64_t rayQueryScratch = 0;
        uint64_t skinningTransforms = 0;
        uint64_t skinnedVertices = 0;
        uint64_t tileLights = 0;
        uint64_t tileWorldBounds = 0;
        uint64_t uiRenderItems = 0;
      } buffers;

    struct GenericMeshes {
        uint64_t ui = 0;
    } genericMeshes;

    struct AccelerationStructures {
        struct SkinnedBLASSlot {
            uint64_t id = 0;
            uint32_t baseVertex = 0;
            uint32_t baseIndex = 0;
            uint32_t vertexCount = 0;
            uint32_t indexCount = 0;
            uint32_t geometryCount = 0;
            uint64_t geometryHash = 0;
            uint64_t accelerationStructureSize = 0;
            uint64_t buildScratchSize = 0;
            uint64_t updateScratchSize = 0;
            bool built = false;
        };

        uint64_t rayQueryTLAS = 0;
        uint32_t rayQueryTLASInstanceCapacity = 0;
        uint64_t rayQueryTLASScratchSize = 0;
        uint64_t skinnedVertexBufferAddress = 0;
        std::vector<SkinnedBLASSlot> skinnedBLAS;
    } accelerationStructures;
};
