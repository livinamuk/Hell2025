#pragma once

#include "Hell/Math/GLM.h"

#include <cstdint>

// Geometry

struct PushConstantsMaterialResolve {
    uint64_t frameAddressTableDeviceAddress = 0;

    uint64_t vertexBufferDeviceAddress = 0;
    uint64_t indexBufferDeviceAddress = 0;
    uint64_t previousSkinnedPositionsDeviceAddress = 0;
    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;
    uint32_t hasPreviousSkinnedPositions = 0;
    uint32_t padding0 = 0;
};

struct PushConstantsSkinning {
    uint64_t outputVerticesDeviceAddress = 0;
    uint64_t previousSkinnedPositionsDeviceAddress = 0;
    uint64_t inputVerticesDeviceAddress = 0;
    uint64_t skinningDispatchGroupsDeviceAddress = 0;
    uint64_t skinningJobsDeviceAddress = 0;

    uint64_t skinningTransformsDeviceAddress = 0;
    uint64_t previousSkinningTransformsDeviceAddress = 0;
    uint64_t vertexWeightsDeviceAddress = 0;
    uint32_t padding0 = 0;
    uint32_t padding1 = 0;
};

struct PushConstantsVisibility {
    uint64_t frameAddressTableDeviceAddress = 0;
    uint64_t skinnedVerticesDeviceAddress = 0;
    uint32_t viewportIndex = 0;
    uint32_t useDepthOffset = 0;
};

struct PointShadowFaceData {
    glm::mat4 projectionView = glm::mat4(1.0f);
    glm::vec4 lightPositionRadius = glm::vec4(0.0f);
    uint32_t arrayLayer = 0;
};

static_assert(sizeof(PointShadowFaceData) == 84);

struct PushConstantsPointShadow {
    uint64_t frameAddressTableDeviceAddress = 0;
    uint64_t faceDataDeviceAddress = 0;
    uint64_t drawFaceDataIndicesDeviceAddress = 0;
};

static_assert(sizeof(PushConstantsPointShadow) == 24);

// Indirect specular

struct PushConstantsIndirectSpecularAMDInput {
    uint64_t frameAddressTableDeviceAddress = 0;

    uint64_t rayQueryBLASDataDeviceAddress = 0;
    uint64_t rayQuerySceneRenderItemIndicesDeviceAddress = 0;
    int32_t blueNoiseTextureIndex = -1;
    uint32_t frameIndex = 0;
    uint32_t samplesPerQuad = 1;
    uint32_t padding0 = 0;
    uint64_t ddgiReflectionVolumeDataDeviceAddress = 0;
    uint32_t enableDDGIReflections = 0;
    uint32_t padding1 = 0;
};

static_assert(sizeof(PushConstantsIndirectSpecularAMDInput) == 56);

struct PushConstantsIndirectSpecularAMDReproject {
    uint64_t frameAddressTableDeviceAddress = 0;
    uint32_t historyValid = 0;
    uint32_t padding0 = 0;
};

struct PushConstantsIndirectSpecularAMDPrefilter {
    uint64_t frameAddressTableDeviceAddress = 0;
};

// Lighting

struct PushConstantsDeferredLighting {
    uint64_t frameAddressTableDeviceAddress = 0;

    uint64_t rayQueryBLASDataDeviceAddress = 0;
    uint64_t rayQuerySceneRenderItemIndicesDeviceAddress = 0;
    int32_t brdfLutTextureIndex = -1;
    uint32_t viewportIndex = 0;
};

struct PushConstantsHair {
    uint64_t frameAddressTableDeviceAddress = 0;
    uint32_t viewportIndex = 0;
    uint32_t padding0 = 0;
};

static_assert(sizeof(PushConstantsHair) == 16);

struct PushConstantsEmissive {
    uint64_t frameAddressTableDeviceAddress = 0;
    uint32_t viewportIndex = 0;
    uint32_t padding0 = 0;
};

static_assert(sizeof(PushConstantsEmissive) == 16);

struct PushConstantsEmissiveBloomFilter {
    glm::ivec2 sourceOffset = glm::ivec2(0);
    glm::ivec2 sourceExtent = glm::ivec2(0);
    glm::ivec2 outputExtent = glm::ivec2(0);
    glm::ivec2 direction = glm::ivec2(0);
    int32_t sourceMip = 0;
    float filterScale = 1.0f;
    uint32_t sourceTextureIndex = 0;
    uint32_t outputImageIndex = 0;
};

static_assert(sizeof(PushConstantsEmissiveBloomFilter) == 48);

struct PushConstantsEmissiveBloomComposite {
    glm::ivec2 viewportOffset = glm::ivec2(0);
    glm::ivec2 viewportExtent = glm::ivec2(0);
    glm::ivec2 bloomExtents[3] = {};
};

static_assert(sizeof(PushConstantsEmissiveBloomComposite) == 40);

// Tile culling

struct PushConstantsTileLightCulling {
    uint64_t frameAddressTableDeviceAddress = 0;
};

struct PushConstantsTileWorldBounds {
    uint64_t frameAddressTableDeviceAddress = 0;
    int32_t tileXCount;
    int32_t tileYCount;
};

// Scene rendering

struct PushConstantsSkybox {
    uint64_t frameAddressTableDeviceAddress = 0;
};

struct PushConstantsSpriteSheet {
    uint64_t frameAddressTableDeviceAddress = 0;
    uint32_t viewportIndex = 0;
    uint32_t padding0 = 0;
};

struct PushConstantsPostProcessing {
    uint64_t frameAddressTableDeviceAddress = 0;
};

// UI

struct PushConstantsUI {
    uint64_t frameAddressTableDeviceAddress = 0;
    float renderTargetWidth = 1.0f;
    float renderTargetHeight = 1.0f;
};

// Debug

struct PushConstantsDebug2D {
    float renderTargetWidth = 1.0f;
    float renderTargetHeight = 1.0f;
};

struct PushConstantsDebug3D {
    uint64_t frameAddressTableDeviceAddress = 0;
    uint32_t viewportIndex = 0;
    uint32_t padding0 = 0;
};

struct PushConstantsDebugView {
    uint64_t frameAddressTableDeviceAddress = 0;
};

struct PushConstantsDebugTileView {
    uint64_t frameAddressTableDeviceAddress = 0;
};

// DDGI

struct PushConstantsDDGIPointCloudBaseColor {
    uint64_t pointCloudDeviceAddress = 0;
    uint64_t pointCloudTextureInfoDeviceAddress = 0;
    uint32_t pointCount = 0;
    uint32_t textureInfoCount = 0;
};

struct PushConstantsDDGIPointCloudDebug {
    uint64_t frameAddressTableDeviceAddress = 0;

    uint64_t pointCloudDeviceAddress = 0;
    uint64_t pointCloudDirtyFlagsDeviceAddress = 0;
    uint32_t pointCount = 0;
    uint32_t viewportIndex = 0;
};

struct PushConstantsDDGIPointCloudLighting {
    uint64_t frameAddressTableDeviceAddress = 0;

    uint64_t pointCloudDeviceAddress = 0;
    uint64_t pointCloudDirtyFlagsDeviceAddress = 0;
    uint32_t pointCount = 0;
    uint32_t lightCount = 0;
    uint32_t forceUpdate = 0;
    uint32_t padding0 = 0;
};

struct PushConstantsDDGIProbeBorder {
    glm::vec3 volumeOrigin = glm::vec3(0.0f);
    float probeSpacing = 0.0f;
    glm::ivec3 probeCounts = glm::ivec3(0);
    uint32_t totalProbes = 0;
    uint32_t probeOffset = 0;
    uint32_t distanceAtlasStorageImageIndex = 0;
    uint32_t irradianceAtlasStorageImageIndex = 0;
    uint32_t padding2 = 0;
};

struct PushConstantsDDGIProbeDebug {
    uint64_t frameAddressTableDeviceAddress = 0;
    uint64_t probeStatesDeviceAddress = 0;
    glm::vec3 volumeOrigin = glm::vec3(0.0f);
    float probeSpacing = 0.0f;
    glm::ivec3 probeCounts = glm::ivec3(0);
    uint32_t viewportIndex = 0;
    uint32_t probeOffset = 0;
    uint32_t probeDebugState = 0;
    uint32_t padding0 = 0;
    uint32_t padding1 = 0;
};

struct PushConstantsDDGIProbeDistance {
    uint64_t probeStatesDeviceAddress = 0;
    uint64_t probeDistanceCounterDeviceAddress = 0;
    uint64_t probeDistanceIndicesDeviceAddress = 0;
    glm::vec3 volumeOrigin = glm::vec3(0.0f);
    float probeSpacing = 0.0f;
    glm::ivec3 probeCounts = glm::ivec3(0);
    uint32_t probeOffset = 0;
    uint32_t totalProbes = 0;
    uint32_t distanceAtlasStorageImageIndex = 0;
    uint32_t padding1 = 0;
    uint32_t padding2 = 0;
};

struct PushConstantsDDGIProbeDistanceDispatchArgs {
    uint64_t probeDistanceCounterDeviceAddress = 0;
    uint64_t probeDistanceDispatchArgsDeviceAddress = 0;
};

struct PushConstantsDDGIProbeDistanceList {
    uint64_t probeStatesDeviceAddress = 0;
    uint64_t probeDistanceCounterDeviceAddress = 0;
    uint64_t probeDistanceIndicesDeviceAddress = 0;
    uint32_t totalProbes = 0;
    uint32_t probeOffset = 0;
};

struct PushConstantsDDGIProbeIrradiance {
    uint64_t pointCloudDeviceAddress = 0;
    uint64_t probeStatesDeviceAddress = 0;
    uint64_t probeIrradianceCounterDeviceAddress = 0;
    uint64_t probeIrradianceIndicesDeviceAddress = 0;
    uint64_t probePointIndicesDeviceAddress = 0;
    uint64_t probePointOffsetsDeviceAddress = 0;
    uint64_t probePointCountsDeviceAddress = 0;
    glm::vec3 volumeOrigin = glm::vec3(0.0f);
    float probeSpacing = 0.0f;
    glm::ivec3 probeCounts = glm::ivec3(0);
    uint32_t probeOffset = 0;
    uint32_t totalProbes = 0;
    float pointCloudSpacing = 0.0f;
    uint32_t irradianceAtlasStorageImageIndex = 0;
    uint32_t padding1 = 0;
};

struct PushConstantsDDGIProbeIrradianceDirtyPointCheck {
    uint64_t probeStatesDeviceAddress = 0;
    uint64_t probePointIndicesDeviceAddress = 0;
    uint64_t probePointOffsetsDeviceAddress = 0;
    uint64_t probePointCountsDeviceAddress = 0;
    uint64_t pointCloudDirtyFlagsDeviceAddress = 0;
    uint32_t totalProbes = 0;
    uint32_t probeOffset = 0;
};

struct PushConstantsDDGIProbeIrradianceDispatchArgs {
    uint64_t probeIrradianceCounterDeviceAddress = 0;
    uint64_t probeIrradianceDispatchArgsDeviceAddress = 0;
};

struct PushConstantsDDGIProbeIrradianceList {
    uint64_t probeStatesDeviceAddress = 0;
    uint64_t probeIrradianceCounterDeviceAddress = 0;
    uint64_t probeIrradianceIndicesDeviceAddress = 0;
    uint32_t totalProbes = 0;
    uint32_t probeOffset = 0;
};

struct PushConstantsDDGIProbeIrradianceTexture {
    uint64_t frameAddressTableDeviceAddress = 0;
    uint64_t probeStatesDeviceAddress = 0;
    glm::vec3 volumeOrigin = glm::vec3(0.0f);
    float probeSpacing = 0.0f;
    glm::ivec3 probeCounts = glm::ivec3(0);
    uint32_t totalProbes = 0;
    glm::vec3 worldBoundsMin = glm::vec3(0.0f);
    uint32_t probeOffset = 0;
    glm::vec3 worldBoundsMax = glm::vec3(0.0f);
    uint32_t probeAtlasImageIndex = 0;
    uint32_t indirectDiffuseStorageImageIndex = 0;
    uint32_t indirectDiffuseSurfaceStorageImageIndex = 0;
    uint32_t padding0 = 0;
};

struct PushConstantsDDGIProbePointIndices {
    uint64_t pointCloudDeviceAddress = 0;
    uint64_t pointCloudGridOffsetsDeviceAddress = 0;
    uint64_t pointCloudGridCountsDeviceAddress = 0;
    uint64_t probePointIndicesDeviceAddress = 0;
    uint64_t probePointOffsetsDeviceAddress = 0;
    uint64_t probePointCountsDeviceAddress = 0;
    uint64_t probeIndexCounterDeviceAddress = 0;
    glm::vec3 volumeOrigin = glm::vec3(0.0f);
    float probeSpacing = 0.0f;
    glm::ivec3 probeCounts = glm::ivec3(0);
    uint32_t probeOffset = 0;
    glm::vec3 gridMin = glm::vec3(0.0f);
    float gridCellSize = 0.0f;
    glm::ivec3 gridDimensions = glm::ivec3(0);
    uint32_t totalProbes = 0;
};

struct PushConstantsDDGIProbeRelevance {
    uint64_t frameAddressTableDeviceAddress = 0;
    uint64_t probeStatesDeviceAddress = 0;
    glm::vec3 volumeOrigin = glm::vec3(0.0f);
    float probeSpacing = 0.0f;
    glm::ivec3 probeCounts = glm::ivec3(0);
    uint32_t probeOffset = 0;
    uint32_t totalProbes = 0;
    uint32_t padding0 = 0;
    uint32_t padding1 = 0;
    uint32_t padding2 = 0;
};

struct PushConstantsDDGIProbeStateUpdate {
    uint64_t probeStatesDeviceAddress = 0;
    uint64_t dirtyDoorAABBsDeviceAddress = 0;
    glm::vec3 volumeOrigin = glm::vec3(0.0f);
    float probeSpacing = 0.0f;
    glm::ivec3 probeCounts = glm::ivec3(0);
    uint32_t probeOffset = 0;
    uint32_t totalProbes = 0;
    uint32_t dirtyDoorAABBCount = 0;
    float time = 0.0f;
    uint32_t padding0 = 0;
};

struct PushConstantsDDGIRaytraceScene {
    uint64_t frameAddressTableDeviceAddress = 0;

    uint64_t houseVertexBufferDeviceAddress = 0;
    uint64_t houseIndexBufferDeviceAddress = 0;
    uint64_t doorVertexBufferDeviceAddress = 0;
    uint64_t doorIndexBufferDeviceAddress = 0;

    float maxRayDistance = 100.0f;
    uint32_t clearOutput = 0;
    uint32_t padding0 = 0;
    uint32_t padding1 = 0;
};
