#include "EditorPlacement.h"

#include "EditorHierarchy.h"
#include "EditorObjectOptions.h"
#include "Unloved/EditorSession/Interaction/EditorSelection.h"
#include "Unloved/EditorSession/Core/EditorViewports.h"

#include "Hell/Audio.h"
#include "Hell/Common/Enum.h"
#include "Hell/Input.h"
#include "Hell/Logging.h"
#include "Hell/Math/Rotation.h"
#include "Hell/Physics/Physics.h"

#include "Unloved/Bible/Bible.h"
#include "Unloved/Common/Constants.h"
#include "Unloved/Debug/DebugDraw.h"
#include "Unloved/ObjectId.h"
#include "Unloved/Objects/Exterior/Fence.h"
#include "Unloved/Objects/Exterior/PowerPoleSet.h"
#include "Unloved/Objects/House/Wall.h"
#include "Unloved/Objects/House/PlanarQuadObject.h"
#include "Unloved/Objects/Props/Christmas/ChristmasLights.h"
#include "Unloved/Systems/WorldBVH/WorldBVH.h"
#include "Unloved/World/World.h"

#include <cmath>
#include <limits>
#include <vector>

namespace Unloved::EditorSession::Placement {
    namespace {
        constexpr float MAX_RAY_DISTANCE = 2000.0f;
        constexpr bool CULL_BACK_FACING = true;

        struct PlacementHit {
            bool hitFound = false;
            glm::vec3 position = glm::vec3(0.0f);
            glm::vec3 normal = glm::vec3(0.0f, 1.0f, 0.0f);
            uint64_t objectId = 0;
            float distanceToHit = std::numeric_limits<float>::max();
        };

        PlacementTool g_currentTool = PlacementTool::NONE;
        std::vector<SequencePoint> g_sequencePoints;
        uint64_t g_placementObjectId = 0;
        float g_sequencePointValue = 0.0f;

        void ResetState() {
            g_currentTool = PlacementTool::NONE;
            g_sequencePoints.clear();
            g_placementObjectId = 0;
            g_sequencePointValue = 0.0f;
        }

        PlacementHit GetWorldHit() {
            const int32_t viewportIndex = Viewports::GetHoveredViewportIndex();
            if (viewportIndex < 0) return {};

            const glm::vec3& rayOrigin = Viewports::GetMouseRayOrigin(static_cast<uint32_t>(viewportIndex));
            const glm::vec3& rayDirection = Viewports::GetMouseRayDirection(static_cast<uint32_t>(viewportIndex));
            const BvhRayResult bvhResult = WorldBVH::ClosestHit(rayOrigin, rayDirection, MAX_RAY_DISTANCE);
            const PhysXRayResult physXResult = Hell::Physics::CastPhysXRay(rayOrigin, rayDirection, MAX_RAY_DISTANCE, CULL_BACK_FACING);
            PlacementHit hit;

            if (bvhResult.hitFound) {
                hit.hitFound = true;
                hit.position = bvhResult.hitPosition;
                hit.normal = bvhResult.hitNormal;
                hit.objectId = bvhResult.objectId;
                hit.distanceToHit = bvhResult.distanceToHit;
            }
            if (physXResult.hitFound && physXResult.distanceToHit < hit.distanceToHit) {
                hit.hitFound = true;
                hit.position = physXResult.hitPosition;
                hit.normal = physXResult.hitNormal;
                hit.objectId = physXResult.userData.objectId;
                hit.distanceToHit = physXResult.distanceToHit;
            }
            return hit;
        }

        PlacementHit GetPhysicsHit() {
            const int32_t viewportIndex = Viewports::GetHoveredViewportIndex();
            if (viewportIndex < 0) return {};

            const glm::vec3& rayOrigin = Viewports::GetMouseRayOrigin(static_cast<uint32_t>(viewportIndex));
            const glm::vec3& rayDirection = Viewports::GetMouseRayDirection(static_cast<uint32_t>(viewportIndex));
            const PhysXRayResult result = Hell::Physics::CastPhysXRay(rayOrigin, rayDirection, MAX_RAY_DISTANCE, CULL_BACK_FACING);
            if (!result.hitFound) return {};

            PlacementHit hit;
            hit.hitFound = true;
            hit.position = result.hitPosition;
            hit.normal = result.hitNormal;
            hit.objectId = result.userData.objectId;
            hit.distanceToHit = result.distanceToHit;
            return hit;
        }

        PlacementHit GetTerrainHit() {
            const int32_t viewportIndex = Viewports::GetHoveredViewportIndex();
            if (viewportIndex < 0) return {};

            const glm::vec3& rayOrigin = Viewports::GetMouseRayOrigin(static_cast<uint32_t>(viewportIndex));
            const glm::vec3& rayDirection = Viewports::GetMouseRayDirection(static_cast<uint32_t>(viewportIndex));
            Hell::Physics::ActivateAllHeightFields();
            const PhysXRayResult heightMapResult = Hell::Physics::CastPhysXRayHeightMap(rayOrigin, rayDirection, MAX_RAY_DISTANCE);
            PlacementHit hit;

            if (heightMapResult.hitFound) {
                hit.hitFound = true;
                hit.position = heightMapResult.hitPosition;
                hit.normal = heightMapResult.hitNormal;
                hit.distanceToHit = heightMapResult.distanceToHit;
            }

            constexpr float GROUND_PLANE_Y = -0.01f;
            if (std::abs(rayDirection.y) < 0.000001f) return hit;

            const float rayDistance = (GROUND_PLANE_Y - rayOrigin.y) / rayDirection.y;
            if (rayDistance < 0.0f || rayDistance > MAX_RAY_DISTANCE) return hit;

            const glm::vec3 groundPosition = rayOrigin + rayDirection * rayDistance;
            const float groundDistance = glm::distance(rayOrigin, groundPosition);
            if (hit.hitFound && hit.distanceToHit <= groundDistance) return hit;

            hit.hitFound = true;
            hit.position = groundPosition;
            hit.normal = glm::vec3(0.0f, 1.0f, 0.0f);
            hit.distanceToHit = groundDistance;
            return hit;
        }

        PlacementHit GetGroundPlaneHit() {
            const int32_t viewportIndex = Viewports::GetHoveredViewportIndex();
            if (viewportIndex < 0) return {};

            const glm::vec3& rayOrigin = Viewports::GetMouseRayOrigin(static_cast<uint32_t>(viewportIndex));
            const glm::vec3& rayDirection = Viewports::GetMouseRayDirection(static_cast<uint32_t>(viewportIndex));
            if (std::abs(rayDirection.y) < 0.000001f) return {};

            const float rayDistance = -rayOrigin.y / rayDirection.y;
            if (rayDistance < 0.0f || rayDistance > MAX_RAY_DISTANCE) return {};

            PlacementHit hit;
            hit.hitFound = true;
            hit.position = rayOrigin + rayDirection * rayDistance;
            hit.normal = glm::vec3(0.0f, 1.0f, 0.0f);
            hit.distanceToHit = rayDistance;
            return hit;
        }

        uint64_t CreatePlanarQuadObject(const PlacementToolInfo& toolInfo, const PlacementHit& hit, PlanarQuadObjectType type) {
            PlanarQuadObjectCreateInfo createInfo;
            createInfo.planarQuad.position = hit.position;
            createInfo.planarQuad.points = { glm::vec3(-1.0f, 0.0f, -1.0f), glm::vec3(-1.0f, 0.0f, 1.0f), glm::vec3(1.0f, 0.0f, 1.0f), glm::vec3(1.0f, 0.0f, -1.0f) };

            if (type == PlanarQuadObjectType::DECKING_BOARDS) {
                createInfo.materialNames[0] = "NumGrid";
                createInfo.customFloats[0] = 0.6f;
            }
            createInfo.editorName = toolInfo.defaultEditorName;
            createInfo.type = type;
            return World::AddPlanarQuadObject(createInfo);
        }

        uint64_t CreatePointPairObject(const PlacementToolInfo& toolInfo, const PlacementHit& hit, PointPairObjectType type) {
            PointPairCreateInfo createInfo;
            createInfo.position = hit.position;
            createInfo.editorName = toolInfo.defaultEditorName;
            createInfo.type = type;

            if (type == PointPairObjectType::RIDGE_CAPPING) {
                createInfo.length = 1.0f;
                createInfo.materialNames[0] = "Brass";
            }
            if (type == PointPairObjectType::GUTTER) {
                createInfo.length = 1.0f;
                createInfo.materialNames[0] = "Brass";
            }
            if (type == PointPairObjectType::DOWN_PIPE) {
                createInfo.length = 1.0f;
                createInfo.rotation.x = glm::radians(-90.0f);
            }
            if (type == PointPairObjectType::DECKING_POST) {
                createInfo.length = 1.0f;
                createInfo.rotation.x = glm::radians(-90.0f);
            }
            if (type == PointPairObjectType::DECKING_BEARER) {
                createInfo.length = 1.0f;
            }

            return World::AddPointPairObject(createInfo);
        }

        uint64_t CreateDirectObject(PlacementTool tool, const PlacementToolInfo& toolInfo, const PlacementHit& hit) {
            GenericObjectType genericObjectType = GenericObjectType::UNDEFINED;
            const char* pickUpName = nullptr;

            switch (tool) {
                case PlacementTool::DDGI_VOLUME: {
                    DDGIVolumeCreateInfo createInfo;
                    createInfo.extents = glm::vec3(4.0f);
                    createInfo.origin = hit.position + hit.normal * createInfo.extents.y * 0.5f;
                    createInfo.defaultEditorName = toolInfo.defaultEditorName;
                    return World::AddDDGIVolume(createInfo);
                }
                case PlacementTool::DOOR_STANDARD_A: {
                    DoorCreateInfo createInfo;
                    createInfo.type = DoorType::STANDARD_A;
                    createInfo.materialTypeFront = DoorMaterialType::RESIDENT_EVIL;
                    createInfo.materialTypeBack = DoorMaterialType::RESIDENT_EVIL;
                    createInfo.materialTypeFrameFront = DoorMaterialType::RESIDENT_EVIL;
                    createInfo.materialTypeFrameBack = DoorMaterialType::RESIDENT_EVIL;
                    createInfo.position = hit.position;
                    createInfo.defaultEditorName = toolInfo.defaultEditorName;
                    return World::AddDoor(createInfo);
                }
                case PlacementTool::DOBERMANN: {
                    DobermannCreateInfo createInfo;
                    createInfo.position = hit.position;
                    createInfo.defaultEditorName = toolInfo.defaultEditorName;
                    return World::AddDobermann(createInfo);
                }
                case PlacementTool::GENERIC_ANIMATED_RAT_KING: {
                    GenericAnimatedObjectCreateInfo createInfo;
                    createInfo.position = hit.position;
                    createInfo.type = GenericAnimatedObjectType::RAT_KING;
                    createInfo.defaultEditorName = toolInfo.defaultEditorName;
                    return World::AddGenericAnimatedObject(createInfo);
                }
                case PlacementTool::KANGAROO: {
                    KangarooCreateInfo createInfo;
                    createInfo.position = hit.position;
                    createInfo.defaultEditorName = toolInfo.defaultEditorName;
                    return World::AddKangaroo(createInfo);
                }
                case PlacementTool::SHARK: {
                    SharkCreateInfo createInfo;
                    createInfo.position = hit.position;
                    createInfo.defaultEditorName = toolInfo.defaultEditorName;
                    return World::AddShark(createInfo);
                }
                case PlacementTool::HOUSE_LOCATION: {
                    const std::vector<std::string>& houseNames = ObjectOptions::GetHouseNames();
                    HouseLocationCreateInfo createInfo;
                    createInfo.position = hit.position;
                    createInfo.houseName = houseNames.empty() ? "" : houseNames.front();
                    createInfo.defaultEditorName = toolInfo.defaultEditorName;
                    return World::AddHouseLocation(createInfo);
                }
                case PlacementTool::JETTY: {
                    JettyCreateInfo createInfo;
                    createInfo.position = hit.position;
                    createInfo.scale = glm::vec3(1.0f);
                    createInfo.defaultEditorName = toolInfo.defaultEditorName;
                    return World::AddJetty(createInfo);
                }
                case PlacementTool::PLAYER_CAMPAIGN_SPAWN:
                case PlacementTool::PLAYER_DEATHMATCH_SPAWN: {
                    SpawnPointCreateInfo createInfo;
                    createInfo.position = hit.position;
                    createInfo.defaultEditorName = toolInfo.defaultEditorName;
                    return tool == PlacementTool::PLAYER_CAMPAIGN_SPAWN ? World::AddSpawnPointCampaign(createInfo) : World::AddSpawnPointDeathMatch(createInfo);
                }
                case PlacementTool::MERMAID: {
                    MermaidCreateInfo createInfo;
                    createInfo.position = hit.position;
                    createInfo.shopTeleportPosition = hit.position + glm::vec3(0.0f, 1.65f, 0.0f);
                    createInfo.defaultEditorName = toolInfo.defaultEditorName;
                    return World::AddMermaid(createInfo);
                }
                case PlacementTool::PIANO: {
                    PianoCreateInfo createInfo;
                    createInfo.position = hit.position;
                    createInfo.defaultEditorName = toolInfo.defaultEditorName;
                    return World::AddPiano(createInfo);
                }
                case PlacementTool::PICTURE_FRAME_BIG_LANDSCAPE:
                case PlacementTool::PICTURE_FRAME_REGULAR_LANDSCAPE:
                case PlacementTool::PICTURE_FRAME_REGULAR_PORTRAIT:
                case PlacementTool::PICTURE_FRAME_TALL_THIN: {
                    PictureFrameCreateInfo createInfo;
                    createInfo.position = hit.position;
                    createInfo.rotation = Hell::Math::EulerRotationFromNormal(hit.normal);
                    createInfo.defaultEditorName = toolInfo.defaultEditorName;
                    if (tool == PlacementTool::PICTURE_FRAME_BIG_LANDSCAPE) {
                        createInfo.type = PictureFrameType::BIG_LANDSCAPE;
                    }
                    if (tool == PlacementTool::PICTURE_FRAME_REGULAR_LANDSCAPE) {
                        createInfo.type = PictureFrameType::REGULAR_LANDSCAPE;
                    }
                    if (tool == PlacementTool::PICTURE_FRAME_REGULAR_PORTRAIT) {
                        createInfo.type = PictureFrameType::REGULAR_PORTRAIT;
                    }
                    if (tool == PlacementTool::PICTURE_FRAME_TALL_THIN) {
                        createInfo.type = PictureFrameType::TALL_THIN;
                    }
                    return World::AddPictureFrame(createInfo);
                }
                case PlacementTool::FIREPLACE_OPEN:
                case PlacementTool::FIREPLACE_WOOD_STOVE: {
                    FireplaceCreateInfo createInfo;
                    createInfo.type = tool == PlacementTool::FIREPLACE_WOOD_STOVE ? FireplaceType::WOOD_STOVE : FireplaceType::DEFAULT;
                    createInfo.position = hit.position;
                    createInfo.defaultEditorName = toolInfo.defaultEditorName;
                    return World::AddFireplace(createInfo);
                }
                case PlacementTool::LADDER: {
                    LadderCreateInfo createInfo;
                    createInfo.position = hit.position + glm::vec3(0.0f, 1.0f, 0.0f);
                    createInfo.defaultEditorName = toolInfo.defaultEditorName;
                    return World::AddLadder(createInfo);
                }
                case PlacementTool::LADDER_DISMOUNT: {
                    LadderDismountCreateInfo createInfo;
                    createInfo.position = hit.position;
                    createInfo.defaultEditorName = toolInfo.defaultEditorName;
                    return World::AddLadderDismount(createInfo);
                }
                case PlacementTool::LIGHT_HANGING: {
                    LightCreateInfo createInfo;
                    createInfo.position = hit.position;
                    createInfo.type = LightType::HANGING_LIGHT;
                    createInfo.radius = 3.0f;
                    createInfo.strength = 1.0f;
                    createInfo.defaultEditorName = toolInfo.defaultEditorName;
                    return World::AddLight(createInfo);
                }
                case PlacementTool::STAIRCASE: {
                    StaircaseCreateInfo createInfo;
                    createInfo.position = hit.position;
                    createInfo.defaultEditorName = toolInfo.defaultEditorName;
                    return World::AddStaircase(createInfo);
                }
                case PlacementTool::WINDOW: {
                    WindowCreateInfo createInfo;
                    createInfo.position = hit.position;
                    createInfo.defaultEditorName = toolInfo.defaultEditorName;
                    return World::AddWindow(createInfo);
                }
                case PlacementTool::DECKING_BOARDS: return CreatePlanarQuadObject(toolInfo, hit, PlanarQuadObjectType::DECKING_BOARDS);
                case PlacementTool::DECKING_BEARER: return CreatePointPairObject(toolInfo, hit, PointPairObjectType::DECKING_BEARER);
                case PlacementTool::DECKING_POST:   return CreatePointPairObject(toolInfo, hit, PointPairObjectType::DECKING_POST);
                case PlacementTool::DOWN_PIPE:      return CreatePointPairObject(toolInfo, hit, PointPairObjectType::DOWN_PIPE);
                case PlacementTool::GUTTER:         return CreatePointPairObject(toolInfo, hit, PointPairObjectType::GUTTER);
                case PlacementTool::RIDGE_CAPPING:  return CreatePointPairObject(toolInfo, hit, PointPairObjectType::RIDGE_CAPPING);
                case PlacementTool::ROOFING_IRON:   return CreatePlanarQuadObject(toolInfo, hit, PlanarQuadObjectType::ROOFING_IRON);
                case PlacementTool::WORLD_PLANE_CEILING:
                case PlacementTool::WORLD_PLANE_FLOOR: {
                    const bool ceiling = tool == PlacementTool::WORLD_PLANE_CEILING;
                    WorldPlaneCreateInfo createInfo;
                    if (ceiling) {
                        createInfo.p0 = hit.position + glm::vec3(1.0f, 2.4f, -1.0f);
                        createInfo.p1 = hit.position + glm::vec3(1.0f, 2.4f, 1.0f);
                        createInfo.p2 = hit.position + glm::vec3(-1.0f, 2.4f, 1.0f);
                        createInfo.p3 = hit.position + glm::vec3(-1.0f, 2.4f, -1.0f);
                    }
                    else {
                        createInfo.p0 = hit.position + glm::vec3(-1.0f, 0.0f, -1.0f);
                        createInfo.p1 = hit.position + glm::vec3(-1.0f, 0.0f, 1.0f);
                        createInfo.p2 = hit.position + glm::vec3(1.0f, 0.0f, 1.0f);
                        createInfo.p3 = hit.position + glm::vec3(1.0f, 0.0f, -1.0f);
                    }
                    createInfo.materialName = ceiling ? "Ceiling2" : "FloorBoards";
                    createInfo.type = ceiling ? WorldPlaneType::CEILING : WorldPlaneType::FLOOR;
                    createInfo.defaultEditorName = toolInfo.defaultEditorName;
                    return World::AddWorldPlane(createInfo);
                }
                case PlacementTool::GENERIC_BATHROOM_BASIN:          genericObjectType = GenericObjectType::BATHROOM_BASIN;          break;
                case PlacementTool::GENERIC_BATHROOM_CABINET:        genericObjectType = GenericObjectType::BATHROOM_CABINET;        break;
                case PlacementTool::GENERIC_CHAIR_RE:                genericObjectType = GenericObjectType::CHAIR_RE;                break;
                case PlacementTool::GENERIC_CHAIR_SPINDLE_BACK:      genericObjectType = GenericObjectType::CHAIR_SPINDLE_BACK;      break;
                case PlacementTool::GENERIC_CHRISTMAS_PRESENT_LARGE: genericObjectType = GenericObjectType::CHRISTMAS_PRESENT_LARGE; break;
                case PlacementTool::GENERIC_CHRISTMAS_PRESENT_SMALL: genericObjectType = GenericObjectType::CHRISTMAS_PRESENT_SMALL; break;
                case PlacementTool::GENERIC_CHRISTMAS_TREE:          genericObjectType = GenericObjectType::CHRISTMAS_TREE;          break;
                case PlacementTool::GENERIC_COUCH:                   genericObjectType = GenericObjectType::COUCH;                   break;
                case PlacementTool::GENERIC_DEER_HEAD:               genericObjectType = GenericObjectType::DEER_HEAD;               break;
                case PlacementTool::GENERIC_DRAWERS_LARGE:           genericObjectType = GenericObjectType::DRAWERS_LARGE;           break;
                case PlacementTool::GENERIC_DRAWERS_SMALL:           genericObjectType = GenericObjectType::DRAWERS_SMALL;           break;
                case PlacementTool::GENERIC_MERMAID_ROCK:            genericObjectType = GenericObjectType::MERMAID_ROCK;            break;
                case PlacementTool::GENERIC_PLANT_BLACKBERRIES:      genericObjectType = GenericObjectType::PLANT_BLACKBERRIES;      break;
                case PlacementTool::GENERIC_PLANT_TREE:              genericObjectType = GenericObjectType::PLANT_TREE;              break;
                case PlacementTool::GENERIC_TOILET:                  genericObjectType = GenericObjectType::TOILET;                  break;
                case PlacementTool::PICKUP_12_GAUGE_BUCKSHOT:        pickUpName = "12GaugeBuckShot"; break;
                case PlacementTool::PICKUP_AKS74U:                   pickUpName = "AKS74U";          break;
                case PlacementTool::PICKUP_BLACK_SKULL:              pickUpName = "BlackSkull";      break;
                case PlacementTool::PICKUP_GLOCK:                    pickUpName = "Glock";           break;
                case PlacementTool::PICKUP_GOLDEN_GLOCK:             pickUpName = "GoldenGlock";     break;
                case PlacementTool::PICKUP_KNIFE:                    pickUpName = "Knife";           break;
                case PlacementTool::PICKUP_P90:                      pickUpName = "P90";             break;
                case PlacementTool::PICKUP_PILLS:                    pickUpName = "Pills";           break;
                case PlacementTool::PICKUP_REMINGTON_870:            pickUpName = "Remington870";    break;
                case PlacementTool::PICKUP_SMALL_KEY:                pickUpName = "SmallKey";        break;
                case PlacementTool::PICKUP_SMALL_KEY_SILVER:         pickUpName = "SmallKeySilver";  break;
                case PlacementTool::PICKUP_SPAS:                     pickUpName = "SPAS";            break;
                case PlacementTool::PICKUP_TOKAREV:                  pickUpName = "Tokarev";         break;
                default: break;
            }

            if (pickUpName) {
                PickUpCreateInfo createInfo;
                createInfo.position = hit.position;
                createInfo.name = pickUpName;
                createInfo.respawn = true;
                createInfo.saveToFile = true;
                createInfo.type = Bible::GetItemType(pickUpName);
                createInfo.defaultEditorName = toolInfo.defaultEditorName;
                return World::AddPickUp(createInfo);
            }

            if (genericObjectType == GenericObjectType::UNDEFINED) {
                Logging::Error() << "EditorPlacement::CreateDirectObject() has no case for '" << Hell::Enum::ToString(tool) << "'\n";
                return 0;
            }

            GenericObjectCreateInfo createInfo;
            createInfo.position = hit.position;
            if (genericObjectType == GenericObjectType::DEER_HEAD) {
                createInfo.rotation = Hell::Math::EulerRotationFromNormal(hit.normal);
            }
            createInfo.type = genericObjectType;
            createInfo.defaultEditorName = toolInfo.defaultEditorName;
            return World::AddGenericObject(createInfo);
        }

        uint64_t CreatePointSequenceObject(PlacementTool tool, const PlacementToolInfo& toolInfo) {
            switch (tool) {
                case PlacementTool::CHRISTMAS_LIGHTS: {
                    ChristmasLightsCreateInfo createInfo;
                    createInfo.sequencePoints = g_sequencePoints;
                    createInfo.defaultEditorName = toolInfo.defaultEditorName;
                    const uint64_t objectId = World::AddChristmasLights(createInfo);
                    WorldBVH::MarkStaticSceneBvhDirty();
                    return objectId;
                }
                case PlacementTool::FENCE_FARM: {
                    FenceCreateInfo createInfo;
                    createInfo.sequencePoints = g_sequencePoints;
                    createInfo.defaultEditorName = toolInfo.defaultEditorName;
                    const uint64_t objectId = World::AddFence(createInfo);
                    WorldBVH::MarkStaticSceneBvhDirty();
                    return objectId;
                }
                case PlacementTool::POWER_POLES: {
                    PowerPoleSetCreateInfo createInfo;
                    createInfo.sequencePoints = g_sequencePoints;
                    createInfo.defaultEditorName = toolInfo.defaultEditorName;
                    const uint64_t objectId = World::AddPowerPoleSet(createInfo);
                    WorldBVH::MarkStaticSceneBvhDirty();
                    return objectId;
                }
                case PlacementTool::WALL_INTERIOR:
                case PlacementTool::WALL_WEATHER_BOARDS: {
                    const bool weatherBoards = tool == PlacementTool::WALL_WEATHER_BOARDS;
                    WallCreateInfo createInfo;
                    createInfo.sequencePoints = g_sequencePoints;
                    createInfo.materialName = weatherBoards ? ObjectOptions::GetWeatherBoardMaterials().front() : ObjectOptions::GetInteriorMaterials().front();
                    createInfo.textureOffsetV = weatherBoards ? 0.0f : -1.4f;
                    createInfo.textureScale = weatherBoards ? 1.0f : 1.0f / 2.4f;
                    createInfo.ceilingTrimType = TrimType::TIMBER;
                    createInfo.floorTrimType = TrimType::TIMBER;
                    createInfo.wallType = weatherBoards ? WallType::WEATHER_BOARDS : WallType::INTERIOR;
                    createInfo.defaultEditorName = toolInfo.defaultEditorName;
                    const uint64_t objectId = World::AddWall(createInfo);
                    WorldBVH::MarkStaticSceneBvhDirty();
                    return objectId;
                }
                default: {
                    Logging::Error() << "EditorPlacement::CreatePointSequenceObject() has no case for '" << Hell::Enum::ToString(tool) << "'\n";
                    return 0;
                }
            }
        }

        void UpdatePointSequenceObject(PlacementTool tool) {
            switch (tool) {
                case PlacementTool::CHRISTMAS_LIGHTS: {
                    ChristmasLightSet* christmasLights = World::GetChristmasLightsByObjectId(g_placementObjectId);
                    if (christmasLights) {
                        christmasLights->UpdateSequencePoints(g_sequencePoints);
                    }
                    WorldBVH::MarkStaticSceneBvhDirty();
                    return;
                }
                case PlacementTool::FENCE_FARM: {
                    Fence* fence = World::GetFenceByObjectId(g_placementObjectId);
                    if (fence) {
                        fence->UpdateSequencePoints(g_sequencePoints);
                    }
                    WorldBVH::MarkStaticSceneBvhDirty();
                    return;
                }
                case PlacementTool::POWER_POLES: {
                    PowerPoleSet* powerPoleSet = World::GetPowerPoleSetByObjectId(g_placementObjectId);
                    if (powerPoleSet) {
                        powerPoleSet->UpdateSequencePoints(g_sequencePoints);
                    }
                    WorldBVH::MarkStaticSceneBvhDirty();
                    return;
                }
                case PlacementTool::WALL_INTERIOR:
                case PlacementTool::WALL_WEATHER_BOARDS: {
                    Wall* wall = World::GetWallByObjectId(g_placementObjectId);
                    if (wall) {
                        wall->UpdateSequencePoints(g_sequencePoints);
                    }
                    WorldBVH::MarkStaticSceneBvhDirty();
                    return;
                }
                default: {
                    Logging::Error() << "EditorPlacement::UpdatePointSequenceObject() has no case for '" << Hell::Enum::ToString(tool) << "'\n";
                    return;
                }
            }
        }

        void FinishPointObject() {
            const uint64_t objectId = g_placementObjectId;
            ResetState();
            Hierarchy::Refresh();
            Selection::SelectObject(objectId);
        }

        void UpdatePointSequence(const PlacementToolInfo& toolInfo, const PlacementHit& hit, bool allowKeyboardInput) {
            // Adjust point value
            if (allowKeyboardInput && Hell::Input::KeyDown(HELL_KEY_LEFT_ALT)) {
                if (Hell::Input::MouseWheelUp()) {
                    g_sequencePointValue -= toolInfo.sequencePointValueStep;
                }
                if (Hell::Input::MouseWheelDown()) {
                    g_sequencePointValue += toolInfo.sequencePointValueStep;
                }
            }

            // Snap the live point
            PlacementHit sequenceHit = hit;
            if (allowKeyboardInput && sequenceHit.hitFound && (Hell::Input::KeyDown(HELL_KEY_LEFT_CONTROL_GLFW) || Hell::Input::KeyDown(HELL_KEY_RIGHT_CONTROL))) {
                sequenceHit.position = glm::round(sequenceHit.position * 10.0f) / 10.0f;
            }
            const bool placingWall = g_currentTool == PlacementTool::WALL_INTERIOR || g_currentTool == PlacementTool::WALL_WEATHER_BOARDS;
            const bool closesWall = placingWall && g_placementObjectId != 0 && g_sequencePoints.size() > 2 && sequenceHit.hitFound && glm::distance(sequenceHit.position, g_sequencePoints.front().position) < 0.25f;
            if (closesWall) {
                sequenceHit.position = g_sequencePoints.front().position;
            }

            // Update the live point
            if (g_placementObjectId != 0 && sequenceHit.hitFound) {
                g_sequencePoints.back().position = sequenceHit.position;
                g_sequencePoints.back().normal = closesWall ? g_sequencePoints.front().normal : hit.normal;
                g_sequencePoints.back().customFloat = closesWall ? g_sequencePoints.front().customFloat : g_sequencePointValue;
                UpdatePointSequenceObject(g_currentTool);
            }

            if (!Hell::Input::LeftMousePressed() || !sequenceHit.hitFound) return;

            // Finish a closed wall
            if (closesWall) {
                FinishPointObject();
                return;
            }

            // Commit the current point
            if (g_placementObjectId == 0) {
                SequencePoint& sequencePoint = g_sequencePoints.emplace_back();
                sequencePoint.position = sequenceHit.position;
                sequencePoint.normal = hit.normal;
                sequencePoint.customFloat = g_sequencePointValue;
            }
            else {
                g_sequencePoints.back().position = sequenceHit.position;
                g_sequencePoints.back().normal = hit.normal;
                g_sequencePoints.back().customFloat = g_sequencePointValue;
            }

            const bool flatPlacement = toolInfo.rayMode == PlacementRayMode::HEIGHT_MAP || toolInfo.rayMode == PlacementRayMode::GROUND_PLANE;
            const glm::vec3 livePointOffset = flatPlacement ? glm::vec3(0.1f, 0.0f, 0.0f) : hit.normal * 0.1f;
            SequencePoint& liveSequencePoint = g_sequencePoints.emplace_back();
            liveSequencePoint.position = sequenceHit.position + livePointOffset;
            liveSequencePoint.normal = hit.normal;
            liveSequencePoint.customFloat = g_sequencePointValue;

            if (g_placementObjectId == 0) {
                g_placementObjectId = CreatePointSequenceObject(g_currentTool, toolInfo);
            }
            else {
                UpdatePointSequenceObject(g_currentTool);
            }

            if (g_placementObjectId == 0) {
                ResetState();
            }
        }
    }

    void Begin(PlacementTool tool) {
        if (tool == PlacementTool::NONE) return;

        const PlacementToolInfo* toolInfo = GetPlacementToolInfo(tool);
        const bool supportedRayMode = toolInfo && (toolInfo->rayMode == PlacementRayMode::WORLD || toolInfo->rayMode == PlacementRayMode::PHYSICS || toolInfo->rayMode == PlacementRayMode::HEIGHT_MAP || toolInfo->rayMode == PlacementRayMode::GROUND_PLANE || toolInfo->rayMode == PlacementRayMode::WALL);
        if (!supportedRayMode || (toolInfo->insertMode != PlacementInsertMode::DIRECT && toolInfo->insertMode != PlacementInsertMode::POINT_SEQUENCE)) {
            Logging::Error() << "EditorPlacement::Begin() only supports direct or point sequence world, physics, height map, ground plane and wall tools\n";
            Cancel();
            return;
        }

        Cancel();
        Selection::ClearSelection();
        g_currentTool = tool;
        g_sequencePointValue = toolInfo->sequencePointDefaultValue;
    }

    void Update(bool allowKeyboardInput, bool allowMouseInput) {
        if (!IsActive()) return;

        const PlacementToolInfo* toolInfo = GetPlacementToolInfo(g_currentTool);
        if (!toolInfo) {
            Cancel();
            return;
        }

        if (allowKeyboardInput && Hell::Input::KeyPressed(HELL_KEY_ESCAPE)) {
            Hell::Audio::PlayAudio(AUDIO_SELECT, 1.0f);
            Cancel();
            return;
        }
        if (allowMouseInput && Hell::Input::RightMousePressed()) {
            if (toolInfo->insertMode == PlacementInsertMode::POINT_SEQUENCE && g_placementObjectId != 0) {
                FinishPointObject();
            }
            else {
                Cancel();
            }
            return;
        }
        if (!allowMouseInput) return;

        // Find the placement surface
        PlacementHit hit;
        if (toolInfo->rayMode == PlacementRayMode::PHYSICS) {
            hit = GetPhysicsHit();
        }
        else if (toolInfo->rayMode == PlacementRayMode::HEIGHT_MAP) {
            hit = GetTerrainHit();
        }
        else if (toolInfo->rayMode == PlacementRayMode::GROUND_PLANE) {
            hit = GetGroundPlaneHit();
        }
        else if (toolInfo->rayMode == PlacementRayMode::WALL) {
            hit = GetWorldHit();
            if (GetObjectIdType(hit.objectId) != ObjectType::WALL_SEGMENT) {
                hit = {};
            }
        }
        else {
            hit = GetWorldHit();
        }

        // Update the sequence preview
        if (toolInfo->insertMode == PlacementInsertMode::POINT_SEQUENCE) {
            UpdatePointSequence(*toolInfo, hit, allowKeyboardInput);
            return;
        }
        if (!Hell::Input::LeftMousePressed() || !hit.hitFound) return;

        // Place the direct object
        const uint64_t objectId = CreateDirectObject(g_currentTool, *toolInfo, hit);
        if (objectId == 0) return;

        ResetState();
        Hierarchy::Refresh();
        Selection::SelectObject(objectId);
    }

    void Cancel() {
        if (g_placementObjectId != 0 && World::RemoveObjectById(g_placementObjectId)) {
            WorldBVH::MarkStaticSceneBvhDirty();
        }
        ResetState();
    }

    bool IsActive() {
        return g_currentTool != PlacementTool::NONE;
    }
}
