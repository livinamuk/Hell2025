
#ifndef COMMON_TYPES_GLSL
#define COMMON_TYPES_GLSL

#define MAX_SPOT_LIGHTS 7

struct ViewportData {
    mat4 projectionReverseZ;
    mat4 inverseProjectionReverseZ;
    mat4 projectionViewReverseZ;
    mat4 prevProjectionViewReverseZ;
    mat4 inverseProjectionViewReverseZ;
    mat4 projection;
    mat4 inverseProjection;
    mat4 view;
    mat4 inverseView;
    mat4 projectionView;
    mat4 prevProjectionView;
    mat4 inverseProjectionView;
    mat4 skyboxProjectionView;
    mat4 flashlightProjectionView;
    mat4 previousProjectionView;

    mat4 jitteredProjectionViewReverseZ;
    mat4 inverseJitteredProjectionViewReverseZ;

    mat4 csmLightProjectionView[5];

    int xOffset;
    int yOffset;
    int width;
    int height;

    float posX;  // 0 t0 1 range
    float posY;  // 0 t0 1 range
    float sizeX; // 0 t0 1 range
    float sizeY; // 0 t0 1 range

    vec4 frustumPlane0;
    vec4 frustumPlane1;
    vec4 frustumPlane2;
    vec4 frustumPlane3;
    vec4 frustumPlane4;
    vec4 frustumPlane5;
    vec4 flashlightDir;
    vec4 flashlightPosition;

    float flashlightModifer;
    int isOrtho; //true or false
    float orthoSize;
    float fov;

    vec4 viewPos;
    vec4 cameraForward;
    vec4 cameraUp;
    vec4 cameraRight;

    vec4 colorTint;

    float colorContrast;
    int isInShop; //true or false
    float padding1;
    float intensityScalar;

    vec4 vignetteColor;
};

struct RendererData {
    vec4 moonLightDir;
    vec4 moonLightColorStrength;

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
    uint lightCount;
    bool enableIrradianceProbeSampling;
    bool enableIndirectSpecular;

    bool enableTAA;
    float indirectSpecularFactor;
    float indirectSpecularRoughnessDampening;
    uint directPointShadowMode;

    vec2 taaJitterPx; // WARNING SKETCHY
    float viewportSplitX;
    float viewportSplitY;

    vec4 flashlightColor;

    float flashlightRange;
    float flashlightFalloffExponent;
    float flashlightBrightness;
    float flashlightIESConeScale;

    float flashlightIESInnerAngle;
    float flashlightIESOuterAngle;
    float flashlightIESContrast;
    float flashlightIESVerticalScale;

    float flashlightIESVerticalBias;
    float flashlightIESHorizontalBias;
    int flashlightIESTextureIndex;
    uint flashlightIESEnabled;

    float flashlightCenterSpotRange;
    float flashlightCenterSpotFalloffExponent;
    float flashlightCenterSpotBrightness;
    float flashlightCenterSpotInnerAngle;

    float flashlightCenterSpotOuterAngle;
    uint flashlightCenterSpotEnabled;
    uint padding3;
    bool enableDDGIReflections;

    uint spotLightCount;
    uint activeViewportMask;
    uint padding5;
    uint padding6;

    vec4 oceanSurfaceAlbedo;
    vec4 oceanSurfaceFogColor;
    vec4 oceanSurfaceRippleVelocity;
    vec4 oceanUnderwaterTint;
    vec4 oceanUnderwaterRayFogColor;

    float oceanOriginY;
    int oceanDisplayMode;
    uint oceanSurfaceSpecularAntiAliasing;
    uint padding7;

    float oceanSurfaceNormalScale;
    float oceanSurfaceNormalConvergeStartDistance;
    float oceanSurfaceNormalConvergeEndDistance;
    float oceanSurfaceNormalConvergeMaxFactor;

    float oceanSurfaceNormalConvergeExponent;
    float oceanSurfaceNormalSoftening;
    float oceanSurfaceRippleTiling;
    float oceanSurfaceRippleStrength;

    float oceanSurfaceRippleSecondLayerScale;
    float oceanSurfaceRoughness;
    float oceanSurfaceReflectance;
    float oceanSurfaceReflectionGamma;

    float oceanSurfaceDiffuseStrength;
    float oceanSurfaceSssHeightRange;
    float oceanSurfaceSssStrength;
    float oceanSurfaceUnderwaterSssStrength;

    float oceanSurfaceSssRadiusMinimum;
    float oceanSurfaceSssRadiusMaximum;
    float oceanSurfaceSssIntensity;
    float oceanSurfaceSssFalloff;

    float oceanSurfaceSssSaturation;
    float oceanSurfaceFogStartDistance;
    float oceanSurfaceFogEndDistance;
    float oceanSurfaceFogExponent;

    float oceanSurfaceFogStrength;
    float oceanSurfaceCompositePlaneHeightOffset;
    float oceanSurfaceCompositeDistortionSpeed;
    float oceanSurfaceCompositeDistortionStrength;

    float oceanSurfaceCompositeDistortionTiling;
    float oceanSurfaceCompositeRefractionTintStrength;
    float oceanUnderwaterRayFogStrength;
    float oceanUnderwaterDarknessCurve;

    float oceanUnderwaterDistortionSpeed;
    float oceanUnderwaterDistortionStrength;
    float oceanUnderwaterDepthTintStrength;
    float oceanUnderwaterDepthTintOriginalWeight;

    float oceanUnderwaterGeometryWaterColorSquaredStrength;
    float oceanUnderwaterGeometryWaterColorStrength;
    float oceanUnderwaterGeometryTintStrength;
    float oceanUnderwaterOpenWaterTintStrength;

    float oceanUnderwaterOpenWaterBrightness;
    float emissiveStrength;
    float christmasLightRadius;
    float christmasLightStrength;
    float irradianceDampening;
};

struct RenderItem {
    mat4 modelMatrix;
    mat4 prevModelMatrix;
    mat4 inverseModelMatrix;
    vec4 aabbMin;
    vec4 aabbMax;

    uint vertexCount;
    uint indexCount;
    uint baseVertex;
    uint baseIndex;

    uint baseVertexWeight;
    uint baseSkinningTransformIndex;
    uint objectIdLowerBit;
    uint objectIdUpperBit;

    int materialIndex;
    int woundMaskTextureIndex;
    int exclusiveViewportIndex;
    int ignoredViewportIndex;

    uint meshId;
    uint miscFlags;
    uint shadowFlags;
    uint vulkanFlags;

    int localMeshNodeIndex;
    float emissiveR;
    float emissiveG;
    float emissiveB;

    uint blendingMode;
    float tintColorR;
    float tintColorG;
    float tintColorB;

    uint customId;
    uint openableId;
    int woundMaterialIndex;
    uint shadowMeshId;

    float roughnessFactor;
    float metallicFactor;
    int padding1;
    int padding2;
};

struct SpriteSheetRenderItem {
    mat4 modelMatrix;
    vec4 uvFrame;
    vec4 uvFrameNext;
    vec4 localOffset;

    int textureIndex;
    int isBillboard;
    float mixFactor;
    int exclusiveViewportIndex;
};

struct Light {
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

    vec4 forward_iesMaxIntensity;
    vec4 right_iesExposure;
    vec4 up;

    int iesTextureIndex;
    int isDirtyForRaytracing; // true or false
    int hiResShadowMapIndex;
    int lowResShadowMapIndex;

    vec4 worldBoundsMin;
    vec4 worldBoundsMax;
};

struct SpotLight {
    mat4 projectionView;
    vec4 positionModifier;
    vec4 direction;
    vec4 worldBoundsMin;
    vec4 worldBoundsMax;
    ivec4 metadata;
};

struct TileLights {
    uint lightCount;
    uint lightIndices[127];
};

struct TileSpotLights {
    uint lightCount;
    uint lightIndices[MAX_SPOT_LIGHTS];
};

struct TileWorldBounds {
    vec4 boundsMin; // w: count of non-background pixels
    vec4 boundsMax; // w: unused
};

//struct TileBloodDecals {
//    uint decalCount;
//    uint decalOffset;
//};

struct TileInstanceData {
    uint count;
    uint offset;
};

struct BloodDecal {
    mat4 modelMatrix;
    mat4 inverseModelMatrix;
    int type;
    int textureIndex;
    float aspectScaleX;
    float aspectScaleY;
};

struct ChristmasLight {
    vec4 position;
    vec4 color;
};

struct MetaBall {
    vec4 posAndInvSigma2;
};

struct CloudPoint {
    vec4 position;
    vec4 normal;
    vec4 directLightingRGB_dirty;
    vec4 baseColor;
};

struct CloudPointTextureInfo {
    float u;
    float v;
    int baseColorIndex;
    int rmaIndex;
};

struct BvhNode {
    vec3 boundsMin;
    uint firstChildOrPrimitive;
    vec3 boundsMax;
    uint primitiveCount;
};

struct EntityInstance {
    mat4 worldTransform;
    mat4 inverseWorldTransform;

    int rootNodeIndex;
    int objectIdLowerBit;    // Unused on the GPU currently. CPU bvh stuff uses it.
    int objectIdUpperBit;    // Unused on the GPU currently. CPU bvh stuff uses it.
    int openableId;          // Unused on the GPU currently. CPU bvh stuff uses it.

    uint globalMeshIndex;    // Also unsued by the GPU
    uint customId;           // Also unsued by the GPU
    uint localMeshNodeIndex; // Also unsued by the GPU
    uint padding2;           // Also unsued by the GPU
};

struct Triangle {
    vec4 v0_and_e1x;     // p0.xyz, e1.x
    vec4 e1yz_and_e2xy;  // e1.yz,  e2.xy
    vec4 e2z_and_normal; // e2.z,   normal.xyz
};

struct DispatchIndirectArgs {
    uint num_groups_x;
    uint num_groups_y;
    uint num_groups_z;
};

const int INTERIOR_SIZE = 14;
//const int PROBE_NUM_IRRADIANCE_INTERIOR_TEXELS = 14;
const int PROBE_NUM_DISTANCE_INTERIOR_TEXELS = 14;

#ifndef PROBE_DISTANCE_OCTA_SIZE
    #define PROBE_DISTANCE_OCTA_SIZE 16
#endif

const int PROBE_DISTANCE_TEXEL_COUNT = PROBE_DISTANCE_OCTA_SIZE * PROBE_DISTANCE_OCTA_SIZE;
//#define RAYS_PER_PROBE 256

const int PROBE_MAX_DISTANCE_COOLDOWN = 20;
const int PROBE_MAX_IRRADIANCE_COOLDOWN = 35;
const float PROBE_MAX_RAY_DISTANCE = 1.5;
const float PROBE_NORMAL_BIAS = 0.075;
const float PROBE_VIEW_BIAS = 0.1;
struct ProbeState {
    vec3 relocationOffset;
    uint padding;

    bool isRelevant;
    bool isActive;
    uint distanceCooldown;
    uint irradianceCooldown;
};

struct DDGIVolume {
    vec3 origin;
    float probeSpacing;
    ivec3 probeCounts; // number of probes on each axis
    int totalProbes;
    vec3 worldBoundsMin;
    float padding0;
    vec3 worldBoundsMax;
    float padding1;
    uint probeOffset;
    uint padding2;
    uint padding3;
    uint padding4;
};

struct DDGIReflectionVolume {
    DDGIVolume volume;
    uint probeAtlasImageIndex;
    uint padding0;
    uint padding1;
    uint padding2;
};

struct AABB {
    vec4 boundsMin;
    vec4 boundsMax;
};

struct Particle {
    vec4 position;
    vec4 velocity;
    float rotation;
    float rotationalVelocity;
    float lifeTime;
    uint exists;
};

struct Material {
    int basecolor;
    int normal;
    int rma;
    int emissive;

    int opacity;
    int hairMaps;
    int disp;
    int padding2;

    float displacementOffset;
    float displacementScale;
    int padding3;
    int padding4;
};

#endif
