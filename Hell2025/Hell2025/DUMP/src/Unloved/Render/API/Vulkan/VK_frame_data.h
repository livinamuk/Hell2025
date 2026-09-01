#pragma once
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <vector>

inline constexpr size_t VULKAN_MAX_UI_VERTICES = 262144;
inline constexpr size_t VULKAN_MAX_UI_INDICES = 393216;
inline constexpr size_t VULKAN_MAX_UI_RENDER_ITEMS = 16384;

struct alignas(8) FrameAddressTable {
    uint64_t sceneRenderItemBuffer = 0;
    uint64_t drawRenderItemIndexBuffer = 0;
    uint64_t viewportDataBuffer = 0;
    uint64_t rendererDataBuffer = 0;
    uint64_t materialBuffer = 0;
    uint64_t lightBuffer = 0;
    uint64_t spriteSheetRenderItemBuffer = 0;
    uint64_t uiRenderItemBuffer = 0;
    uint64_t tileLightBuffer = 0;
    uint64_t tileWorldBoundsBuffer = 0;
};

static_assert(std::is_standard_layout_v<FrameAddressTable>);
static_assert(std::is_trivially_copyable_v<FrameAddressTable>);
static_assert(alignof(FrameAddressTable) == alignof(uint64_t));
static_assert(sizeof(FrameAddressTable) == sizeof(uint64_t) * 10);
static_assert(offsetof(FrameAddressTable, sceneRenderItemBuffer) == sizeof(uint64_t) * 0);
static_assert(offsetof(FrameAddressTable, drawRenderItemIndexBuffer) == sizeof(uint64_t) * 1);
static_assert(offsetof(FrameAddressTable, viewportDataBuffer) == sizeof(uint64_t) * 2);
static_assert(offsetof(FrameAddressTable, rendererDataBuffer) == sizeof(uint64_t) * 3);
static_assert(offsetof(FrameAddressTable, materialBuffer) == sizeof(uint64_t) * 4);
static_assert(offsetof(FrameAddressTable, lightBuffer) == sizeof(uint64_t) * 5);
static_assert(offsetof(FrameAddressTable, spriteSheetRenderItemBuffer) == sizeof(uint64_t) * 6);
static_assert(offsetof(FrameAddressTable, uiRenderItemBuffer) == sizeof(uint64_t) * 7);
static_assert(offsetof(FrameAddressTable, tileLightBuffer) == sizeof(uint64_t) * 8);
static_assert(offsetof(FrameAddressTable, tileWorldBoundsBuffer) == sizeof(uint64_t) * 9);

struct VulkanFrameData {
    struct Buffers {
        uint64_t frameAddressTable = 0;
        uint64_t sceneRenderItems = 0;
        uint64_t drawRenderItemIndices = 0;
        uint64_t viewportData = 0;
        uint64_t rendererData = 0;
        uint64_t lights = 0;
        uint64_t materials = 0;
        uint64_t spriteSheetInstanceData = 0;
        uint64_t drawCommands = 0;
        uint64_t pointShadowFaceData = 0;
        uint64_t skinningDispatchGroups = 0;
        uint64_t skinningJobs = 0;
        uint64_t skinningTransforms = 0;
        uint64_t previousSkinningTransforms = 0;
        uint64_t skinnedVertices = 0;
        uint64_t previousSkinnedPositions = 0;
        uint64_t rayQueryInstances = 0;
        uint64_t rayQueryBLASData = 0;
        uint64_t rayQuerySceneRenderItemIndices = 0;
        uint64_t rayQueryScratch = 0;
        uint64_t ddgiRayQueryScratch = 0;
        uint64_t uiRenderItems = 0;
        uint64_t tileLights = 0;
        uint64_t tileWorldBounds = 0;
    } buffers;

    struct DDGI {
        uint64_t dirtyDoorAABBs = 0;
        uint32_t dirtyDoorAABBCount = 0;
        uint64_t probeIndexCounter = 0;

        uint64_t probeDistanceCounter = 0;
        uint64_t probeDistanceIndices = 0;
        uint64_t probeDistanceDispatchArgs = 0;

        uint64_t probeIrradianceCounter = 0;
        uint64_t probeIrradianceIndices = 0;
        uint64_t probeIrradianceDispatchArgs = 0;
    } ddgi;

    struct GenericMeshes {
        uint64_t ui = 0;
        uint64_t debugLines2D = 0;
        uint64_t debugLines3D = 0;
        uint64_t debugPoints2D = 0;
        uint64_t debugPoints3D = 0;
    } genericMeshes;

    struct AccelerationStructures {
        struct SkinnedBLASSlot {
            uint64_t id = 0;
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
