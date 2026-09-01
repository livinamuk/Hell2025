#ifndef VULKAN_PUSH_CONSTANTS_GLSL
#define VULKAN_PUSH_CONSTANTS_GLSL

#include "VK_frame_address_table.glsl"

struct PushConstantsUI {
    FrameAddressTable frame;
    float renderTargetWidth;
    float renderTargetHeight;
};

struct PushConstantsVisibility {
    FrameAddressTable frame;
    uint64_t skinnedVerticesDeviceAddress;
    uint viewportIndex;
    uint useDepthOffset;
};

struct PointShadowFaceData {
    mat4 projectionView;
    vec4 lightPositionRadius;
    uint arrayLayer;
};

struct PushConstantsPointShadow {
    FrameAddressTable frame;
    uint64_t faceDataDeviceAddress;
    uint64_t drawFaceDataIndicesDeviceAddress;
};

struct PushConstantsSkinning {
    uint64_t outputVerticesDeviceAddress;
    uint64_t previousSkinnedPositionsDeviceAddress;
    uint64_t inputVerticesDeviceAddress;
    uint64_t skinningDispatchGroupsDeviceAddress;
    uint64_t skinningJobsDeviceAddress;

    uint64_t skinningTransformsDeviceAddress;
    uint64_t previousSkinningTransformsDeviceAddress;
    uint64_t vertexWeightsDeviceAddress;
    uint padding0;
    uint padding1;
};

struct PushConstantsDebugView {
    FrameAddressTable frame;
};

struct PushConstantsDebug3D {
    FrameAddressTable frame;
    uint viewportIndex;
    uint padding0;
};

struct PushConstantsDebug2D {
    float renderTargetWidth;
    float renderTargetHeight;
};

struct PushConstantsSkybox {
    FrameAddressTable frame;
};

struct PushConstantsMaterialResolve {
    FrameAddressTable frame;

    uint64_t vertexBufferDeviceAddress;
    uint64_t indexBufferDeviceAddress;
    uint64_t previousSkinnedPositionsDeviceAddress;
    uint vertexCount;
    uint indexCount;
    uint hasPreviousSkinnedPositions;
    uint padding0;
};

struct PushConstantsDeferredLighting {
    FrameAddressTable frame;

    uint64_t rayQueryBLASDataDeviceAddress;
    uint64_t rayQuerySceneRenderItemIndicesDeviceAddress;
    int brdfLutTextureIndex;
    uint viewportIndex;
};

struct PushConstantsIndirectSpecularAMDInput {
    FrameAddressTable frame;

    uint64_t rayQueryBLASDataDeviceAddress;
    uint64_t rayQuerySceneRenderItemIndicesDeviceAddress;
    int blueNoiseTextureIndex;
    uint frameIndex;
    uint samplesPerQuad;
    uint padding0;
    uint64_t ddgiReflectionVolumeDataDeviceAddress;
    uint enableDDGIReflections;
    uint padding1;
};

struct PushConstantsIndirectSpecularAMDReproject {
    FrameAddressTable frame;
    uint historyValid;
    uint padding0;
};

struct PushConstantsIndirectSpecularAMDPrefilter {
    FrameAddressTable frame;
};

struct PushConstantsSpriteSheet {
    FrameAddressTable frame;
    uint viewportIndex;
    uint padding0;
};

struct PushConstantsPostProcessing {
    FrameAddressTable frame;
};

struct PushConstantsHair {
    FrameAddressTable frame;
    uint viewportIndex;
    uint padding0;
};

struct PushConstantsEmissive {
    FrameAddressTable frame;
    uint viewportIndex;
    uint padding0;
};

struct PushConstantsEmissiveBloomFilter {
    ivec2 sourceOffset;
    ivec2 sourceExtent;
    ivec2 outputExtent;
    ivec2 direction;
    int sourceMip;
    float filterScale;
    uint sourceTextureIndex;
    uint outputImageIndex;
};

struct PushConstantsEmissiveBloomComposite {
    ivec2 viewportOffset;
    ivec2 viewportExtent;
    ivec2 bloomExtents[3];
};

struct PushConstantsTileWorldBounds {
    FrameAddressTable frame;
    int tileXCount;
    int tileYCount;
};

struct PushConstantsTileLightCulling {
    FrameAddressTable frame;
};

struct PushConstantsDebugTileView {
    FrameAddressTable frame;
};

struct PushConstantsDDGIRaytraceScene {
    FrameAddressTable frame;

    uint64_t houseVertexBufferDeviceAddress;
    uint64_t houseIndexBufferDeviceAddress;
    uint64_t doorVertexBufferDeviceAddress;
    uint64_t doorIndexBufferDeviceAddress;

    float maxRayDistance;
    uint clearOutput;
    uint padding0;
    uint padding1;
};

struct PushConstantsDDGIPointCloudBaseColor {
    uint64_t pointCloudDeviceAddress;
    uint64_t pointCloudTextureInfoDeviceAddress;
    uint pointCount;
    uint textureInfoCount;
};

struct PushConstantsDDGIPointCloudLighting {
    FrameAddressTable frame;

    uint64_t pointCloudDeviceAddress;
    uint64_t pointCloudDirtyFlagsDeviceAddress;
    uint pointCount;
    uint lightCount;
    uint forceUpdate;
    uint padding0;
};

struct PushConstantsDDGIPointCloudDebug {
    FrameAddressTable frame;

    uint64_t pointCloudDeviceAddress;
    uint64_t pointCloudDirtyFlagsDeviceAddress;
    uint pointCount;
    uint viewportIndex;
};

struct PushConstantsDDGIProbeDebug {
    FrameAddressTable frame;
    uint64_t probeStatesDeviceAddress;
    vec3 volumeOrigin;
    float probeSpacing;
    ivec3 probeCounts;
    uint viewportIndex;
    uint probeOffset;
    uint probeDebugState;
    uint padding0;
    uint padding1;
};

struct PushConstantsDDGIProbePointIndices {
    uint64_t pointCloudDeviceAddress;
    uint64_t pointCloudGridOffsetsDeviceAddress;
    uint64_t pointCloudGridCountsDeviceAddress;
    uint64_t probePointIndicesDeviceAddress;
    uint64_t probePointOffsetsDeviceAddress;
    uint64_t probePointCountsDeviceAddress;
    uint64_t probeIndexCounterDeviceAddress;
    vec3 volumeOrigin;
    float probeSpacing;
    ivec3 probeCounts;
    uint probeOffset;
    vec3 gridMin;
    float gridCellSize;
    ivec3 gridDimensions;
    uint totalProbes;
};

struct PushConstantsDDGIProbeStateUpdate {
    uint64_t probeStatesDeviceAddress;
    uint64_t dirtyDoorAABBsDeviceAddress;
    vec3 volumeOrigin;
    float probeSpacing;
    ivec3 probeCounts;
    uint probeOffset;
    uint totalProbes;
    uint dirtyDoorAABBCount;
    float time;
    uint padding0;
};

struct PushConstantsDDGIProbeRelevance {
    FrameAddressTable frame;
    uint64_t probeStatesDeviceAddress;
    vec3 volumeOrigin;
    float probeSpacing;
    ivec3 probeCounts;
    uint probeOffset;
    uint totalProbes;
    uint padding0;
    uint padding1;
    uint padding2;
};

struct PushConstantsDDGIProbeDistanceList {
    uint64_t probeStatesDeviceAddress;
    uint64_t probeDistanceCounterDeviceAddress;
    uint64_t probeDistanceIndicesDeviceAddress;
    uint totalProbes;
    uint probeOffset;
};

struct PushConstantsDDGIProbeDistanceDispatchArgs {
    uint64_t probeDistanceCounterDeviceAddress;
    uint64_t probeDistanceDispatchArgsDeviceAddress;
};

struct PushConstantsDDGIProbeIrradianceDirtyPointCheck {
    uint64_t probeStatesDeviceAddress;
    uint64_t probePointIndicesDeviceAddress;
    uint64_t probePointOffsetsDeviceAddress;
    uint64_t probePointCountsDeviceAddress;
    uint64_t pointCloudDirtyFlagsDeviceAddress;
    uint totalProbes;
    uint probeOffset;
};

struct PushConstantsDDGIProbeIrradianceList {
    uint64_t probeStatesDeviceAddress;
    uint64_t probeIrradianceCounterDeviceAddress;
    uint64_t probeIrradianceIndicesDeviceAddress;
    uint totalProbes;
    uint probeOffset;
};

struct PushConstantsDDGIProbeIrradianceDispatchArgs {
    uint64_t probeIrradianceCounterDeviceAddress;
    uint64_t probeIrradianceDispatchArgsDeviceAddress;
};

struct PushConstantsDDGIProbeDistance {
    uint64_t probeStatesDeviceAddress;
    uint64_t probeDistanceCounterDeviceAddress;
    uint64_t probeDistanceIndicesDeviceAddress;
    vec3 volumeOrigin;
    float probeSpacing;
    ivec3 probeCounts;
    uint probeOffset;
    uint totalProbes;
    uint distanceAtlasStorageImageIndex;
    uint padding1;
    uint padding2;
};

struct PushConstantsDDGIProbeIrradiance {
    uint64_t pointCloudDeviceAddress;
    uint64_t probeStatesDeviceAddress;
    uint64_t probeIrradianceCounterDeviceAddress;
    uint64_t probeIrradianceIndicesDeviceAddress;
    uint64_t probePointIndicesDeviceAddress;
    uint64_t probePointOffsetsDeviceAddress;
    uint64_t probePointCountsDeviceAddress;
    vec3 volumeOrigin;
    float probeSpacing;
    ivec3 probeCounts;
    uint probeOffset;
    uint totalProbes;
    float pointCloudSpacing;
    uint irradianceAtlasStorageImageIndex;
    uint padding1;
};

struct PushConstantsDDGIProbeIrradianceTexture {
    FrameAddressTable frame;
    uint64_t probeStatesDeviceAddress;
    vec3 volumeOrigin;
    float probeSpacing;
    ivec3 probeCounts;
    uint totalProbes;
    vec3 worldBoundsMin;
    uint probeOffset;
    vec3 worldBoundsMax;
    uint probeAtlasImageIndex;
    uint indirectDiffuseStorageImageIndex;
    uint indirectDiffuseSurfaceStorageImageIndex;
    uint padding0;
};

struct PushConstantsDDGIProbeBorder {
    vec3 volumeOrigin;
    float probeSpacing;
    ivec3 probeCounts;
    uint totalProbes;
    uint probeOffset;
    uint distanceAtlasStorageImageIndex;
    uint irradianceAtlasStorageImageIndex;
    uint padding2;
};

#endif
