#pragma once

#include "Hell/Common/Constants.h"
#include "Hell/Common/Enums.h"
#include "Hell/Math/Transform.h"
#include "Hell/Math/VecXZ.h"
#include "Hell/Physics/PhysicsTypes.h"

#include "Unloved/Common/Constants.h"
#include "Unloved/Common/Enums.h"
#include "Unloved/Common/PlanarQuad.h"
#include "Unloved/Common/SequencePoint.h"
#include "Unloved/Common/Types.h"
#include "Unloved/Objects/ObjectEnums.h"
#include "Unloved/Render/RendererEnums.h"
#include "Unloved/Physics/PhysicsEnums.h"
#include "Unloved/Systems/Openables/Openable_types.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <array>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

// Physics

struct RigidDynamicCreateInfo {
    bool createObject = false;
    bool kinematic = true;
    float mass = 1.0f;
    Hell::Transform offsetTransform;
    PhysicsFilterData filterData;
    PhysicsShapeType shapeType = PhysicsShapeType::BOX;
    std::string convexMeshModelName = UNDEFINED_STRING;
    std::string editorName = UNDEFINED_STRING;
    std::string defaultEditorName = "Rigid Dynamic";
};

struct RigidStaticCreateInfo {
    std::string meshName = UNDEFINED_STRING;
    std::string editorName = UNDEFINED_STRING;
    std::string defaultEditorName = "Rigid Static";
};

// World Objects

struct EditableAxisSpan {
    float minimum = -1.0f;
    float maximum = 1.0f;
};

struct BulletCasingCreateInfo {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 force = glm::vec3(0.0f);
    float mass = 0.0f;
    uint32_t modelId = 0;
    uint32_t materialIndex = 0;
    std::string editorName = UNDEFINED_STRING;
    std::string defaultEditorName = "Bullet Casing";
};

struct ChristmasLightsCreateInfo {
    std::vector<Unloved::SequencePoint> sequencePoints;
    std::string editorName = UNDEFINED_STRING;
    std::string defaultEditorName = "Christmas Lights";

    glm::vec3 position = glm::vec3(0.0f);
    // Hanging lights
    float spacing = 0.05f;
    float wireRadius = 0.002f;

    // Tree spiral lights
    bool spiral = false;
    float spiralRadius = 1.0f;
    float spiarlHeight = 1.0f;
    glm::vec3 sprialTopCenter;
};

struct ChristmasTreeCreateInfo {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    std::string editorName = UNDEFINED_STRING;
    std::string defaultEditorName = "Christmas Tree";
};

struct DDGIVolumeCreateInfo {
    glm::vec3 origin = glm::vec3(0.0f);
    glm::vec3 extents = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    std::string editorName = UNDEFINED_STRING;
    std::string defaultEditorName = "DDGI Volume";
    float probeSpacing = 0.75f;
    float pointCloudSpacing = 0.4f;
    bool saveToFile = true;
};

struct DecalCreateInfo {
    uint32_t localMeshNodeIndex = 0;
    uint64_t parentObjectId = 0;
    glm::vec3 surfaceHitPosition = glm::vec3(0.0f);
    glm::vec3 surfaceHitNormal = glm::vec3(0.0f);
    std::string editorName = UNDEFINED_STRING;
    std::string defaultEditorName = "Decal";
};

struct DobermannCreateInfo {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    std::string editorName = UNDEFINED_STRING;
    std::string defaultEditorName = "Dobermann";
};

struct DoorChainCreateInfo {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    float scale = 1.0f;
    std::string editorName = UNDEFINED_STRING;
    std::string defaultEditorName = "Door Chain";
};

struct DoorCreateInfo {
    DoorType type = DoorType::STANDARD_A;
    DoorMaterialType materialTypeFront = DoorMaterialType::UNDEFINED;
    DoorMaterialType materialTypeBack = DoorMaterialType::UNDEFINED;
    DoorMaterialType materialTypeFrameFront = DoorMaterialType::UNDEFINED;
    DoorMaterialType materialTypeFrameBack = DoorMaterialType::UNDEFINED;
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    bool hasDeadLock = false;
    bool deadLockedAtInit = false;
    bool openAtStart = false;
    float maxOpenValue = 2.1f;
    std::string floorPlaneMaterialName = "FloorBoards";
    float floorPlaneTextureScale = 0.4f;
    float floorPlaneTextureOffsetU = 0.0f;
    float floorPlaneTextureOffsetV = 0.0f;
    bool floorPlaneRotateTexture90 = false;
    float floorPlaneRoughnessFactor = 1.0f;
    float floorPlaneMetallicFactor = 1.0f;
    std::string editorName = UNDEFINED_STRING;
    std::string defaultEditorName = "Door";
    bool hasSill = false;
};

struct FenceCreateInfo {
    std::vector<Unloved::SequencePoint> sequencePoints;
    std::string editorName = UNDEFINED_STRING;
    std::string defaultEditorName = "Fence";
    bool snapSequencePointsToTerrain = false;
};

struct FireplaceCreateInfo {
    FireplaceType type = FireplaceType::DEFAULT;
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    std::string editorName = UNDEFINED_STRING;
    std::string defaultEditorName = "Fireplace";
};

struct GameObjectCreateInfo {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
    std::string modelName = "";
    std::string editorName = UNDEFINED_STRING;
    std::string defaultEditorName = "Game Object";
};

struct GenericObjectCreateInfo {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
    std::string editorName = UNDEFINED_STRING;
    std::string defaultEditorName = "Generic Object";
    GenericObjectType type = GenericObjectType::UNDEFINED;
};

struct GenericAnimatedObjectCreateInfo {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    float scale = 1.0f;
    GenericAnimatedObjectType type = GenericAnimatedObjectType::UNDEFINED;
    std::string editorName = UNDEFINED_STRING;
    std::string defaultEditorName = "Generic Animated Object";
};

struct JettyCreateInfo {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(0.0f);
    uint32_t boardCount = 8;
    float poleSpacing = 1.0f;
    std::string editorName = UNDEFINED_STRING;
    std::string defaultEditorName = "Jetty";
};

struct WorldPlaneCreateInfo {
    glm::vec3 p0 = glm::vec3(0.0f);
    glm::vec3 p1 = glm::vec3(0.0f);
    glm::vec3 p2 = glm::vec3(0.0f);
    glm::vec3 p3 = glm::vec3(0.0f);
    std::string materialName = "";
    std::string editorName = UNDEFINED_STRING;
    std::string defaultEditorName = "World Plane";
    float textureScale = 1.0f;
    float textureOffsetU = 0.0f;
    float textureOffsetV = 0.0f;
    float textureRotation = 0.0f;
    bool rotateTexture90 = false;
    float roughnessFactor = 1.0f;
    float metallicFactor = 1.0f;
    uint64_t parentDoorId = 0;
    WorldPlaneType type = WorldPlaneType::UNDEFINED;
};

struct PlanarQuadObjectCreateInfo {
    Unloved::PlanarQuadCreateInfo planarQuad;
    PlanarQuadObjectType type = PlanarQuadObjectType::UNDEFINED;
    std::string editorName = UNDEFINED_STRING;
    std::array<float, 8> customFloats = {};
    std::array<int32_t, 4> customInts = {};
    std::array<bool, 8> customBools = {};
    std::array<glm::vec3, 4> customVec3s = {};
    std::array<std::string, 8> materialNames = {};
};

struct PointPairCreateInfo {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    float length = 1.0f;
    PointPairObjectType type = PointPairObjectType::UNDEFINED;
    std::string editorName = UNDEFINED_STRING;
    std::array<float, 8> customFloats = {};
    std::array<int32_t, 4> customInts = {};
    std::array<bool, 8> customBools = {};
    std::array<glm::vec3, 4> customVec3s = {};
    std::array<std::string, 8> materialNames = {};
};

struct KangarooCreateInfo {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    std::string editorName = UNDEFINED_STRING;
    std::string defaultEditorName = "Kangaroo";
};

struct LadderDismountCreateInfo {
    glm::vec3 position = glm::vec3(0.00f);
    std::string editorName = UNDEFINED_STRING;
    std::string defaultEditorName = "Ladder Dismount";
};

struct LadderCreateInfo {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    EditableAxisSpan editableAxisSpan;
    std::string editorName = UNDEFINED_STRING;
    std::string defaultEditorName = "Ladder";
    uint32_t stepCount = 1;
};

struct LightCreateInfo {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 forward = glm::vec3(0.0f);
    glm::vec3 color = glm::vec3(1, 0.7799999713897705, 0.5289999842643738);
    float radius = 6.0f;
    float strength = 1.0f;
    bool saveToFile = true;
    float iesExposure = 1.0f;
    float twist = 0.0f;
    IESProfileType iesProfileType = IESProfileType::NONE;
    LightType type = LightType::HANGING_LIGHT;
    std::string editorName = UNDEFINED_STRING;
    std::string defaultEditorName = "Light";
};

struct MapDataCreateInfo {
    std::string name;
    uint32_t width = 4;
    uint32_t depth = 4;
    Hell::ivecXZ spawnCoords = Hell::ivecXZ(0, 0);
    std::string m_sectorNames[MAX_MAP_WIDTH][MAX_MAP_DEPTH];
    std::string editorName = UNDEFINED_STRING;
    std::string defaultEditorName = "Map";
};

struct MermaidCreateInfo {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 shopTeleportPosition = glm::vec3(0.0f, 1.65f, 0.0f);
    glm::vec3 shopTeleportEuler = glm::vec3(-0.08f, -1.65f, 0.0f);
    std::string editorName = UNDEFINED_STRING;
    std::string defaultEditorName = "Mermaid";
};

struct MeshNodeCreateInfo {
    std::string meshName;
    std::string shadowModelName = UNDEFINED_STRING;
    std::string shadowMeshName = UNDEFINED_STRING;
    std::string materialName = UNDEFINED_STRING;
    std::string baseColorOverrideTextureName = UNDEFINED_STRING;
    BlendingMode blendingMode = BlendingMode::DEFAULT;
    Unloved::OpenableCreateInfo openable;
    RigidDynamicCreateInfo rigidDynamic;
    RigidDynamicCreateInfo rigidDynamicConvexHull; // needs implementing
    RigidStaticCreateInfo rigidStatic;
    int32_t customId;
    DecalType decalType = DecalType::PLASTER;
    bool forceDynamic = false;
	bool castShadows = true;
    bool excludeFromVulkanTLAS = false;
    bool addtoNavMesh = false;
    glm::vec3 emissiveColor = glm::vec3(0.0f);
    glm::vec3 tintColor = glm::vec3(1.0f);
    glm::vec3 scale = glm::vec3(1.0f);
    std::string editorName = UNDEFINED_STRING;
    std::string defaultEditorName = "Mesh Node";
};

struct PianoCreateInfo {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    std::string soundFontName = UNDEFINED_STRING;
    std::string editorName = UNDEFINED_STRING;
    std::string defaultEditorName = "Piano";
};

struct PickUpCreateInfo {
    std::string name = UNDEFINED_STRING;
    std::string editorName = UNDEFINED_STRING;
    std::string defaultEditorName = "Pick Up";
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    bool saveToFile = false;
    bool respawn = false;
    bool disablePhysicsAtSpawn = true;
    ItemType type = ItemType::UNDEFINED;
    std::string parentObjectName = UNDEFINED_STRING;
    std::string parentMeshName = UNDEFINED_STRING;

    // For when this pick up is placed inside objects
    //uint64_t parentObjectId = 0;
    //uint32_t parentLocalMeshNodeIndex = 0;
    //uint64_t openableId = 0;
};

struct PictureFrameCreateInfo {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
    PictureFrameType type = PictureFrameType::BIG_LANDSCAPE;
    bool useRandom = true;
    std::string materialName = "Picture_SHNakedLady";
    std::string editorName = UNDEFINED_STRING;
    std::string defaultEditorName = "Picture Frame";
};

struct PowerPoleSetCreateInfo {
    std::vector<Unloved::SequencePoint> sequencePoints;
    std::string editorName = UNDEFINED_STRING;
    std::string defaultEditorName = "Power Pole Set";
};

struct SharkCreateInfo {
    glm::vec3 position = glm::vec3(0.0f);
    std::string editorName = UNDEFINED_STRING;
    std::string defaultEditorName = "Shark";
};

struct SpawnPointCreateInfo {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec2 rotation = glm::vec2(0.0f);
    std::string editorName = UNDEFINED_STRING;
    std::string defaultEditorName = "Spawn Point";
};

struct HouseLocationCreateInfo {
    glm::vec3 position = glm::vec3(0.0f);
    float rotation = 0.0f;
    bool randomHouse = false;
    std::string houseName = UNDEFINED_STRING;
    std::string editorName = UNDEFINED_STRING;
    std::string defaultEditorName = "House Location";
};

struct SpriteSheetObjectCreateInfo {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
    glm::vec2 uvOffset = glm::vec2(0.0f);
    bool loop = false;
    bool billboard = true;
    bool renderingEnabled = true;
    float animationSpeed = 1.0f;
    std::string textureName = "";
    std::string editorName = UNDEFINED_STRING;
    std::string defaultEditorName = "Sprite Sheet Object";
};

struct StaircaseCreateInfo {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
    std::string editorName = UNDEFINED_STRING;
    std::string defaultEditorName = "Staircase";
    uint32_t stepCount = 1;
};

struct TreeCreateInfo {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
    TreeType type = TreeType::TREE_LARGE_0;
    std::string editorName = UNDEFINED_STRING;
    std::string defaultEditorName = "Tree";
};

struct TrimSetCreateInfo {
	TrimSetType type = TrimSetType::CEILING;
	std::vector<glm::vec3> points;
	float trimScale = 1.0f;
    std::string editorName = UNDEFINED_STRING;
    std::string defaultEditorName = "Trim Set";
};

struct WallCreateInfo {
    std::vector<Unloved::SequencePoint> sequencePoints;
    std::string materialName = "";
    std::string weatherBoardStopMaterialName = "WeatherBoards0";
    std::string editorName = UNDEFINED_STRING;
    std::string defaultEditorName = "Wall";
    float textureScale = 1.0f;
    float textureOffsetU = 0.0f;
    float textureOffsetV = 0.0f;
    float textureRotation = 0.0f;
    float roughnessFactor = 1.0f;
    float metallicFactor = 1.0f;
    uint32_t weatherBoardTextureBoardCount = 16;
    uint32_t weatherBoardStartIndex = 0;
    uint32_t weatherBoardEndIndex = 15;
    float middleTrimHeight = 2.4f;
    bool useReversePointOrder = false;
    TrimType ceilingTrimType = TrimType::NONE;
    TrimType floorTrimType = TrimType::NONE;
    WallType wallType = WallType::INTERIOR;
};

struct WindowCreateInfo {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    std::string editorName = UNDEFINED_STRING;
    std::string defaultEditorName = "Window";
};

// Create Info Collection

struct CreateInfoCollection {
    std::vector<ChristmasLightsCreateInfo> christmasLights;
    std::vector<DDGIVolumeCreateInfo> ddgiVolumes;
    std::vector<DobermannCreateInfo> dobermanns;
    std::vector<DoorCreateInfo> doors;
    std::vector<FenceCreateInfo> fences;
    std::vector<FireplaceCreateInfo> fireplaces;
    std::vector<GenericAnimatedObjectCreateInfo> genericAnimatedObjects;
    std::vector<GenericObjectCreateInfo> genericObjects;
    std::vector<JettyCreateInfo> jetties;
    std::vector<KangarooCreateInfo> kangaroos;
    std::vector<PlanarQuadObjectCreateInfo> planarQuadObjects;
    std::vector<PointPairCreateInfo> pointPairObjects;
    std::vector<WorldPlaneCreateInfo> worldPlanes;
    std::vector<LadderCreateInfo> ladders;
    std::vector<LadderDismountCreateInfo> ladderDismounts;
    std::vector<LightCreateInfo> lights;
    std::vector<MermaidCreateInfo> mermaids;
    std::vector<PianoCreateInfo> pianos;
    std::vector<PickUpCreateInfo> pickUps;
    std::vector<PictureFrameCreateInfo> pictureFrames;
    std::vector<PowerPoleSetCreateInfo> powerPoleSets;
    std::vector<SharkCreateInfo> sharks;
    std::vector<SpawnPointCreateInfo> spawnPointsCampaign;
    std::vector<SpawnPointCreateInfo> spawnPointsDeathMatch;
    std::vector<StaircaseCreateInfo> staircases;
    std::vector<TreeCreateInfo> trees;
    std::vector<WallCreateInfo> walls;
    std::vector<WindowCreateInfo> windows;
};

struct AdditionalMapData {
    std::vector<HouseLocationCreateInfo> houseLocations;
};
