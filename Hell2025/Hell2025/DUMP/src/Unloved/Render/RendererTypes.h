#pragma once

#include "Unloved/Render/RendererConstants.h"
#include "Unloved/Render/RendererEnums.h"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <cstdint>

struct ViewportData {
    glm::mat4 projectionReverseZ;
    glm::mat4 inverseProjectionReverseZ;
    glm::mat4 projectionViewReverseZ;
    glm::mat4 prevProjectionViewReverseZ;
    glm::mat4 inverseProjectionViewReverseZ;
    glm::mat4 projection;
    glm::mat4 inverseProjection;
    glm::mat4 view;
    glm::mat4 inverseView;
    glm::mat4 projectionView;
    glm::mat4 prevProjectionView;
    glm::mat4 inverseProjectionView;
    glm::mat4 skyboxProjectionView;
    glm::mat4 flashlightProjectionView;
    glm::mat4 previousProjectionView = glm::mat4(1.0f);

    glm::mat4 jitteredProjectionViewReverseZ;
    glm::mat4 inverseJitteredProjectionViewReverseZ;

    glm::mat4 csmLightProjectionView[5]; // Is this right?

    int xOffset;
    int yOffset;
    int width;
    int height;

    float posX;
    float posY;
    float sizeX;
    float sizeY;

    glm::vec4 frustumPlane0;
    glm::vec4 frustumPlane1;
    glm::vec4 frustumPlane2;
    glm::vec4 frustumPlane3;
    glm::vec4 frustumPlane4;
    glm::vec4 frustumPlane5;
    glm::vec4 flashlightDir;
    glm::vec4 flashlightPosition;

    float flashlightModifer;
    bool isOrtho;
    float orthoSize;
    float fov;

    glm::vec4 viewPos;
    glm::vec4 cameraForward;
    glm::vec4 cameraUp;
    glm::vec4 cameraRight;
    glm::vec4 colorTint;

    float colorContrast;
    int isInShop;
    float padding1;
    float vignetteIntensityScalar;

    glm::vec4 vignetteColor;
};

struct RendererData {
    glm::vec4 moonLightDir = glm::vec4(0.0f);
    glm::vec4 moonLightColorStrength = glm::vec4(0.0f);

    float nearPlane;
    float farPlane;
    float gBufferWidth;
    float gBufferHeight;

    float hairBufferWidth;
    float hairBufferHeight;
    float time;
    int viewportLayout;

    int rendererOverrideState;
    float normalizedMouseX;
    float normalizedMouseY;
    int tileCountX;

    int tileCountY;
    uint32_t lightCount; // Boolean
    uint32_t enableDDGI; // Boolean
    uint32_t enableIndirectSpecular; // Boolean

    uint32_t enableTAA;  // Boolean
    float indirectSpecularFactor = 1.0f;
    float indirectSpecularRoughnessDampening = 1.0;
    uint32_t directPointShadowMode = 0;

    glm::vec2 taaJitterPx = glm::vec2(0.0f);
    float viewportSplitX;
    float viewportSplitY;

    glm::vec4 flashlightColor = glm::vec4(0.780f, 0.778f, 0.797f, 1.0f);

    float flashlightRange = 19.1f;
    float flashlightFalloffExponent = 4.29f;
    float flashlightBrightness = 1.0f;
    float flashlightIESConeScale = 1.0f;

    float flashlightIESInnerAngle = 14.0f;
    float flashlightIESOuterAngle = 40.0f;
    float flashlightIESContrast = 0.2f;
    float flashlightIESVerticalScale = 0.0f;

    float flashlightIESVerticalBias = 0.0f;
    float flashlightIESHorizontalBias = 0.0f;
    int32_t flashlightIESTextureIndex = -1;
    uint32_t flashlightIESEnabled = 1;

    float flashlightCenterSpotRange = 15.0f;
    float flashlightCenterSpotFalloffExponent = 4.0f;
    float flashlightCenterSpotBrightness = 1.0f;
    float flashlightCenterSpotInnerAngle = 1.5f;

    float flashlightCenterSpotOuterAngle = 5.0f;
    uint32_t flashlightCenterSpotEnabled = 1;
    uint32_t padding3 = 0;
    uint32_t enableDDGIReflections = 0;

    uint32_t spotLightCount = 0;
    uint32_t activeViewportMask = 1;
    uint32_t padding5 = 0;
    uint32_t padding6 = 0;

    glm::vec4 oceanSurfaceAlbedo = glm::vec4(0.0f);
    glm::vec4 oceanSurfaceFogColor = glm::vec4(0.0f);
    glm::vec4 oceanSurfaceRippleVelocity = glm::vec4(0.0f);
    glm::vec4 oceanUnderwaterTint = glm::vec4(0.0f);
    glm::vec4 oceanUnderwaterRayFogColor = glm::vec4(0.0f);

    float oceanOriginY = 0.0f;
    int32_t oceanDisplayMode = 0;
    uint32_t oceanSurfaceSpecularAntiAliasing = 0;
    uint32_t padding7 = 0;

    float oceanSurfaceNormalScale = 0.0f;
    float oceanSurfaceNormalConvergeStartDistance = 0.0f;
    float oceanSurfaceNormalConvergeEndDistance = 0.0f;
    float oceanSurfaceNormalConvergeMaxFactor = 0.0f;

    float oceanSurfaceNormalConvergeExponent = 0.0f;
    float oceanSurfaceNormalSoftening = 0.0f;
    float oceanSurfaceRippleTiling = 0.0f;
    float oceanSurfaceRippleStrength = 0.0f;

    float oceanSurfaceRippleSecondLayerScale = 0.0f;
    float oceanSurfaceRoughness = 0.0f;
    float oceanSurfaceReflectance = 0.0f;
    float oceanSurfaceReflectionGamma = 0.0f;

    float oceanSurfaceDiffuseStrength = 0.0f;
    float oceanSurfaceSssHeightRange = 0.0f;
    float oceanSurfaceSssStrength = 0.0f;
    float oceanSurfaceUnderwaterSssStrength = 0.0f;

    float oceanSurfaceSssRadiusMinimum = 0.0f;
    float oceanSurfaceSssRadiusMaximum = 0.0f;
    float oceanSurfaceSssIntensity = 0.0f;
    float oceanSurfaceSssFalloff = 0.0f;

    float oceanSurfaceSssSaturation = 0.0f;
    float oceanSurfaceFogStartDistance = 0.0f;
    float oceanSurfaceFogEndDistance = 0.0f;
    float oceanSurfaceFogExponent = 0.0f;

    float oceanSurfaceFogStrength = 0.0f;
    float oceanSurfaceCompositePlaneHeightOffset = 0.0f;
    float oceanSurfaceCompositeDistortionSpeed = 0.0f;
    float oceanSurfaceCompositeDistortionStrength = 0.0f;

    float oceanSurfaceCompositeDistortionTiling = 0.0f;
    float oceanSurfaceCompositeRefractionTintStrength = 0.0f;
    float oceanUnderwaterRayFogStrength = 0.0f;
    float oceanUnderwaterDarknessCurve = 0.0f;

    float oceanUnderwaterDistortionSpeed = 0.0f;
    float oceanUnderwaterDistortionStrength = 0.0f;
    float oceanUnderwaterDepthTintStrength = 0.0f;
    float oceanUnderwaterDepthTintOriginalWeight = 0.0f;

    float oceanUnderwaterGeometryWaterColorSquaredStrength = 0.0f;
    float oceanUnderwaterGeometryWaterColorStrength = 0.0f;
    float oceanUnderwaterGeometryTintStrength = 0.0f;
    float oceanUnderwaterOpenWaterTintStrength = 0.0f;

    float oceanUnderwaterOpenWaterBrightness = 0.0f;
    float emissiveStrength = 1.0f;
    float christmasLightRadius = 0.25f;
    float christmasLightStrength = 0.05f;
    float irradianceDampening = 0.0325f;
};

struct RenderItem {
    glm::mat4 modelMatrix = glm::mat4(1);
    glm::mat4 prevModelMatrix = glm::mat4(1);
    glm::mat4 inverseModelMatrix = glm::mat4(1);

    glm::vec4 aabbMin = glm::vec4(0);
    glm::vec4 aabbMax = glm::vec4(0);

    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;
    uint32_t baseVertex = 0;
    uint32_t baseIndex = 0;

    uint32_t baseVertexWeight = 0;
    uint32_t baseSkinningTransformIndex = 0;
    uint32_t objectIdLowerBit = 0;
    uint32_t objectIdUpperBit = 0;

    int32_t materialIndex = -1;
    int32_t woundMaskTextureIndex = -1;
    int32_t exclusiveViewportIndex = -1;
    int32_t ignoredViewportIndex = -1;

    uint32_t meshId = 0;
    uint32_t miscFlags = 0;
    uint32_t shadowFlags = 0;
    uint32_t vulkanFlags = 0;

    int32_t localMeshNodeIndex = 0;
    float emissiveR = 0.0f;
    float emissiveG = 0.0f;
    float emissiveB = 0.0f;

    uint32_t blendingMode = static_cast<uint32_t>(BlendingMode::DEFAULT);
    float tintColorR = 1.0f;
    float tintColorG = 1.0f;
    float tintColorB = 1.0f;

    uint32_t customId = 0;
    uint32_t openableId = 0;
    int32_t woundMaterialIndex = -1;
    uint32_t shadowMeshId = 0;

    float roughnessFactor = 1.0f;
    float metallicFactor = 1.0f;
    int padding1;
    int padding2;
};

struct GlassLightRange {
    uint32_t offset;
    uint32_t count;
};

struct SpriteSheetRenderItem {
    glm::mat4 modelMatrix = glm::mat4(1);
    glm::vec4 uvFrame = glm::vec4(0);
    glm::vec4 uvFrameNext = glm::vec4(0);
    glm::vec4 localOffset = glm::vec4(0);

    int textureIndex = -1;
    int isBillboard = 0;
    float mixFactor = 0.0f;
    int32_t exclusiveViewportIndex = -1;
};

struct GPULight {
    float posX;
    float posY;
    float posZ;
    float colorR;

    float colorG;
    float colorB;
    float strength;
    float radius;

    float iesVScale;
    float iesVBias;
    float iesHScale;
    float iesHBias;

    glm::vec3 forward;
    float iesMaxIntensity;

    glm::vec3 right;
    float iesExposure;

    glm::vec3 up;
    int padding0;

    int iesTextureIndex;
    int isDirtyForRaytracing = 0; // true or false
    int hiResShadowMapIndex;
    int lowResShadowMapIndex;

    glm::vec4 worldBoundsMin = glm::vec4(0.0f);
    glm::vec4 worldBoundsMax = glm::vec4(0.0f);
};

struct GPUSpotLight {
    glm::mat4 projectionView = glm::mat4(1.0f);
    glm::vec4 positionModifier = glm::vec4(0.0f);
    glm::vec4 direction = glm::vec4(0.0f);
    glm::vec4 worldBoundsMin = glm::vec4(0.0f);
    glm::vec4 worldBoundsMax = glm::vec4(0.0f);
    glm::ivec4 metadata = glm::ivec4(-1, -1, -1, 0);
};
static_assert(sizeof(GPUSpotLight) == 144);

struct GPUAABB {
    glm::vec4 boundsMin{};
    glm::vec4 boundsMax{};
};

struct GPUChristmasLight {
    glm::vec4 position;
    glm::vec4 color;
};

struct TileLights {
    uint32_t lightCount;
    uint32_t lightIndices[127];
};

struct TileSpotLights {
    uint32_t lightCount;
    uint32_t lightIndices[MAX_SPOT_LIGHTS];
};
static_assert(sizeof(TileSpotLights) == 32);

struct TileWorldBounds {
    glm::vec4 boundsMin; // w: count of non-background pixels
    glm::vec4 boundsMax; // w: unused
};

struct TileInstanceData {
    unsigned int count;
    unsigned int offset;
};

struct GpuParticle {
    glm::vec4 position;
    glm::vec4 velocity;

    float rotation;
    float rotationalVelocity;
    float lifeTime = 0.0f;
    uint32_t exists = 0;
};

// Skinning

struct SkinningMorphTarget {
    uint32_t positionDeltaOffset;
    uint32_t positionDeltaCount;
    uint32_t normalDeltaOffset;
    uint32_t normalDeltaCount;

    float currentWeight;
    float previousWeight;
    uint32_t padding0;
    uint32_t padding1;
};

static_assert(sizeof(SkinningMorphTarget) == 32);

struct SkinningMorphJob {
    uint32_t morphTargetOffset;
    uint32_t morphTargetCount;
    int32_t rigidBoneIndex;
    uint32_t padding;
};

static_assert(sizeof(SkinningMorphJob) == 16);

struct SkinningJob {
    uint32_t vertexCount;
    uint32_t indexCount;
    uint32_t sourceBaseVertex;
    uint32_t sourceBaseIndex;

    uint32_t sourceVertexWeightOffset;
    uint32_t skinnedBaseVertex;
    uint32_t skinningTransformOffset;
    uint32_t padding;
};

static_assert(sizeof(SkinningJob) == 32);

struct SkinningDispatchGroup {
    uint32_t jobIndex;
    uint32_t vertexOffset;
    uint32_t padding0;
    uint32_t padding1;
};

// Vulkan ray queries

struct RayQueryMesh {
    uint32_t baseVertex;
    uint32_t baseIndex;
    uint32_t vertexCount;
    uint32_t indexCount;
};

struct RayQueryMaterial {
    uint32_t blendingMode;
    int32_t materialIndex;
};

struct RayQueryMeshInstance {
    RayQueryMesh mesh;
    RayQueryMaterial material;
};

