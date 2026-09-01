#include "World.h"

#include "Hell/Common/Constants.h"
#include "Hell/Logging.h"

#include "Unloved/Bible/Bible.h"
#include "Unloved/Characters/Enemies/Dobermann/Dobermann.h"
#include "Unloved/Characters/Enemies/Kangaroo/Kangaroo.h"
#include "Unloved/Characters/Enemies/Shark/Shark.h"
#include "Unloved/Characters/Mermaids/Mermaid/Mermaid.h"
#include "Unloved/EditorSession/ObjectNames.h"
#include "Unloved/ObjectId.h"
#include "Unloved/Objects/Exterior/Fence.h"
#include "Unloved/Objects/Exterior/Jetty.h"
#include "Unloved/Objects/Exterior/PowerPoleSet.h"
#include "Unloved/Objects/Exterior/Wire.h"
#include "Unloved/Objects/Effects/Decal.h"
#include "Unloved/Objects/House/Door.h"
#include "Unloved/Objects/House/Fireplace.h"
#include "Unloved/Objects/House/PlanarQuadObject.h"
#include "Unloved/Objects/House/PointPairObject.h"
#include "Unloved/Objects/House/WorldPlane.h"
#include "Unloved/Objects/House/TrimSet.h"
#include "Unloved/Objects/House/Wall.h"
#include "Unloved/Objects/House/Window.h"
#include "Unloved/Objects/Interior/Piano.h"
#include "Unloved/Objects/Interior/PictureFrame.h"
#include "Unloved/Objects/Lighting/Light.h"
#include "Unloved/Objects/Lighting/SpotLight.h"
#include "Unloved/Objects/Props/BulletCasing.h"
#include "Unloved/Objects/Props/Christmas/ChristmasLights.h"
#include "Unloved/Objects/Props/Christmas/ChristmasTree.h"
#include "Unloved/Objects/Props/GameObject.h"
#include "Unloved/Objects/Props/GenericAnimatedObject.h"
#include "Unloved/Objects/Props/GenericObject.h"
#include "Unloved/Objects/Props/PickUp.h"
#include "Unloved/Objects/Renderables/SkinnedGameObject.h"
#include "Unloved/Objects/Spawns/HouseLocation.h"
#include "Unloved/Objects/Spawns/SpawnPoint.h"
#include "Unloved/Objects/Traversal/Ladder.h"
#include "Unloved/Objects/Traversal/LadderDismount.h"
#include "Unloved/Objects/Traversal/Staircase.h"
#include "Unloved/Systems/DDGI/DDGIManager.h"
#include "Unloved/Systems/DDGI/DDGIVolume.h"
#include "Unloved/Systems/House/HouseBuilder.h"
#include "Unloved/Systems/NavMesh/NavMesh.h"
#include "Unloved/Systems/CoarseWorldBVH/CoarseWorldBVH.h"

namespace Unloved::World {

    Hell::SlotMap<SkinnedGameObject> g_skinnedGameObjects;
    Hell::SlotMap<BulletCasing> g_bulletCasings;
    Hell::SlotMap<ChristmasLightSet> g_christmasLightSets;
    Hell::SlotMap<ChristmasTree> g_christmasTrees;
    Hell::SlotMap<Decal> g_decals;
    Hell::SlotMap<Dobermann> g_dobermanns;
    Hell::SlotMap<Door> g_doors;
    Hell::SlotMap<Fence> g_fences;
    Hell::SlotMap<Fireplace> g_fireplaces;
    Hell::SlotMap<GameObject> g_gameObjects;
    Hell::SlotMap<GenericAnimatedObject> g_genericAnimatedObjects;
    Hell::SlotMap<GenericObject> g_genericObjects;
    Hell::SlotMap<HouseLocation> g_houseLocations;
    Hell::SlotMap<WorldPlane> g_worldPlanes;
    Hell::SlotMap<Kangaroo> g_kangaroos;
    Hell::SlotMap<Jetty> g_jetties;
    Hell::SlotMap<Ladder> g_ladders;
    Hell::SlotMap<LadderDismount> g_ladderDismounts;
    Hell::SlotMap<Light> g_lights;
    Hell::SlotMap<SpotLight> g_spotLights;
    Hell::SlotMap<Mermaid> g_mermaids;
    Hell::SlotMap<Piano> g_pianos;
    Hell::SlotMap<PickUp> g_pickUps;
    Hell::SlotMap<PictureFrame> g_pictureFrames;
    Hell::SlotMap<PowerPoleSet> g_powerPoleSets;
    Hell::SlotMap<PlanarQuadObject> g_planarQuadObjects;
    Hell::SlotMap<PointPairObject> g_pointPairObjects;
    Hell::SlotMap<Shark> g_sharks;
    Hell::SlotMap<SpawnPoint> g_spawnPointsCampaign;
    Hell::SlotMap<SpawnPoint> g_spawnPointsDeathMatch;
    Hell::SlotMap<Staircase> g_staircases;
    Hell::SlotMap<TrimSet> g_trimSets;
    Hell::SlotMap<Wall> g_walls;
    Hell::SlotMap<Wire> g_wires;
    Hell::SlotMap<Window> g_windows;

    Hell::SlotMap<SkinnedGameObject>& GetSkinnedGameObjects() { return g_skinnedGameObjects; }
    Hell::SlotMap<BulletCasing>& GetBulletCasings()             { return g_bulletCasings; }
    Hell::SlotMap<ChristmasLightSet>& GetChristmasLightSets()   { return g_christmasLightSets; }
    Hell::SlotMap<ChristmasTree>& GetChristmasTrees()           { return g_christmasTrees; }
    Hell::SlotMap<Decal>& GetDecals()                           { return g_decals; }
    Hell::SlotMap<DDGIVolume>& GetDDGIVolumes()                 { return DDGIManager::GetVolumes(); }
    Hell::SlotMap<Dobermann>& GetDobermanns()                   { return g_dobermanns; }
    Hell::SlotMap<Door>& GetDoors()                             { return g_doors; }
    Hell::SlotMap<Fence>& GetFences()                           { return g_fences; }
    Hell::SlotMap<Fireplace>& GetFireplaces()                   { return g_fireplaces; }
    Hell::SlotMap<GameObject>& GetGameObjects()                 { return g_gameObjects; }
    Hell::SlotMap<GenericAnimatedObject>& GetGenericAnimatedObjects() { return g_genericAnimatedObjects; }
    Hell::SlotMap<GenericObject>& GetGenericObjects()           { return g_genericObjects; }
    Hell::SlotMap<HouseLocation>& GetHouseLocations()           { return g_houseLocations; }
    Hell::SlotMap<WorldPlane>& GetWorldPlanes()                 { return g_worldPlanes; }
    Hell::SlotMap<Kangaroo>& GetKangaroos()                     { return g_kangaroos; }
    Hell::SlotMap<Jetty>& GetJetties()                          { return g_jetties; }
    Hell::SlotMap<Ladder>& GetLadders()                         { return g_ladders; }
    Hell::SlotMap<LadderDismount>& GetLadderDismounts()         { return g_ladderDismounts; }
    Hell::SlotMap<Light>& GetLights()                           { return g_lights; }
    Hell::SlotMap<SpotLight>& GetSpotLights()                   { return g_spotLights; }
    Hell::SlotMap<Mermaid>& GetMermaids()                       { return g_mermaids; }
    Hell::SlotMap<Piano>& GetPianos()                           { return g_pianos; }
    Hell::SlotMap<PickUp>& GetPickUps()                         { return g_pickUps; }
    Hell::SlotMap<PictureFrame>& GetPictureFrames()             { return g_pictureFrames; }
    Hell::SlotMap<PowerPoleSet>& GetPowerPoleSets()             { return g_powerPoleSets; }
    Hell::SlotMap<PlanarQuadObject>& GetPlanarQuadObjects()     { return g_planarQuadObjects; }
    Hell::SlotMap<PointPairObject>& GetPointPairObjects()       { return g_pointPairObjects; }
    Hell::SlotMap<Shark>& GetSharks()                           { return g_sharks; }
    Hell::SlotMap<SpawnPoint>& GetSpawnPointsCampaign()         { return g_spawnPointsCampaign; }
    Hell::SlotMap<SpawnPoint>& GetSpawnPointsDeathMatch()       { return g_spawnPointsDeathMatch; }
    Hell::SlotMap<Staircase>& GetStaircases()                   { return g_staircases; }
    Hell::SlotMap<TrimSet>& GetTrimSets()                       { return g_trimSets; }
    Hell::SlotMap<Wall>& GetWalls()                             { return g_walls; }
    Hell::SlotMap<Wire>& GetWires()                             { return g_wires; }
    Hell::SlotMap<Window>& GetWindows()                         { return g_windows; }

    // Skinned Game Objects

    uint64_t CreateSkinnedGameObject() {
        const uint64_t id = GetNextObjectId(ObjectType::SKINNED_GAME_OBJECT);
        g_skinnedGameObjects.emplace_with_id(id, id);
        return id;
    }

    SkinnedGameObject* GetSkinnedGameObjectByObjectId(uint64_t objectId) {
        return GetSkinnedGameObjects().get(objectId);
    }

    // Bullet Casings

    uint64_t AddBulletCasing(BulletCasingCreateInfo createInfo) {
        EditorSession::AssignEditorName(createInfo, GetBulletCasings());
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::BULLET_CASING);
        GetBulletCasings().emplace_with_id(id, id, createInfo);
        return id;
    }

    BulletCasing* GetBulletCasingByObjectId(uint64_t objectId) {
        return GetBulletCasings().get(objectId);
    }

    // Christmas Lights

    uint64_t AddChristmasLights(ChristmasLightsCreateInfo createInfo, SpawnOffset spawnOffset) {
        EditorSession::AssignEditorName(createInfo, GetChristmasLightSets());
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::CHRISTMAS_LIGHTS);
        GetChristmasLightSets().emplace_with_id(id, id, createInfo, spawnOffset);
        return id;
    }

    ChristmasLightSet* GetChristmasLightsByObjectId(uint64_t objectId) {
        return GetChristmasLightSets().get(objectId);
    }

    // Christmas Trees

    uint64_t AddChristmasTree(ChristmasTreeCreateInfo createInfo, SpawnOffset spawnOffset) {
        EditorSession::AssignEditorName(createInfo, GetChristmasTrees());
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::TREE);
        GetChristmasTrees().emplace_with_id(id, id, createInfo, spawnOffset);
        return id;
    }

    ChristmasTree* GetChristmasTreeByObjectId(uint64_t objectId) {
        return GetChristmasTrees().get(objectId);
    }

    // Decals

    uint64_t AddDecal(DecalCreateInfo createInfo) {
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::DECAL);
        GetDecals().emplace_with_id(id, id, createInfo);
        return id;
    }

    Decal* GetDecalByObjectId(uint64_t objectId) {
        return GetDecals().get(objectId);
    }

    // DDGI Volumes

    uint64_t AddDDGIVolume(DDGIVolumeCreateInfo createInfo, SpawnOffset spawnOffset) {
        return DDGIManager::AddVolume(createInfo, spawnOffset);
    }

    DDGIVolume* GetDDGIVolumeByObjectId(uint64_t objectId) {
        return DDGIManager::GetVolumeByObjectId(objectId);
    }

    // Dobermanns

    uint64_t AddDobermann(DobermannCreateInfo createInfo, SpawnOffset spawnOffset) {
        EditorSession::AssignEditorName(createInfo, GetDobermanns());
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::DOBERMANN);
        GetDobermanns().emplace_with_id(id, id, createInfo, spawnOffset);
        return id;
    }

    Dobermann* GetDobermannByObjectId(uint64_t objectId) {
        return GetDobermanns().get(objectId);
    }

    // Doors

    uint64_t AddDoor(DoorCreateInfo createInfo, SpawnOffset spawnOffset) {
        EditorSession::AssignEditorName(createInfo, GetDoors());
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::DOOR);
        GetDoors().emplace_with_id(id, id, createInfo, spawnOffset);
        NavMeshManager::MarkDynamicDirty();
        HouseBuilder::MarkDirty();
        return id;
    }

    Door* GetDoorByObjectId(uint64_t objectId) {
        return GetDoors().get(objectId);
    }

    // Fences

    uint64_t AddFence(FenceCreateInfo createInfo, SpawnOffset spawnOffset) {
        EditorSession::AssignEditorName(createInfo, GetFences());
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::FENCE);
        GetFences().emplace_with_id(id, id, createInfo, spawnOffset);
        return id;
    }

    Fence* GetFenceByObjectId(uint64_t objectId) {
        return GetFences().get(objectId);
    }

    // Fireplaces

    uint64_t AddFireplace(FireplaceCreateInfo createInfo, SpawnOffset spawnOffset) {
        EditorSession::AssignEditorName(createInfo, GetFireplaces());
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::FIREPLACE);
        GetFireplaces().emplace_with_id(id, id, createInfo, spawnOffset);
        HouseBuilder::MarkDirty();
        return id;
    }

    Fireplace* GetFireplaceById(uint64_t objectId) {
        return GetFireplaces().get(objectId);
    }

    // Game Objects

    uint64_t AddGameObject(GameObjectCreateInfo createInfo, SpawnOffset spawnOffset) {
        EditorSession::AssignEditorName(createInfo, GetGameObjects());
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::GAME_OBJECT);
        GetGameObjects().emplace_with_id(id, id, createInfo, spawnOffset);
        return id;
    }

    GameObject* GetGameObjectByObjectId(uint64_t objectId) {
        return GetGameObjects().get(objectId);
    }

    GameObject* GetGameObjectByIndex(int32_t index) {
        if (index >= 0 && index < static_cast<int32_t>(GetGameObjects().size())) {
            return &GetGameObjects()[index];
        }
        return nullptr;
    }

    GameObject* GetGameObjectByName(const std::string& name) {
        for (GameObject& gameObject : GetGameObjects()) {
            if (gameObject.m_name == name) {
                return &gameObject;
            }
        }
        return nullptr;
    }

    // Generic Animated Objects

    uint64_t AddGenericAnimatedObject(GenericAnimatedObjectCreateInfo createInfo, SpawnOffset spawnOffset) {
        EditorSession::AssignEditorName(createInfo, GetGenericAnimatedObjects());
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::GENERIC_ANIMATED_OBJECT);
        GetGenericAnimatedObjects().emplace_with_id(id, id, createInfo, spawnOffset);
        return id;
    }

    GenericAnimatedObject* GetGenericAnimatedObjectById(uint64_t objectId) {
        return GetGenericAnimatedObjects().get(objectId);
    }

    // Generic Objects

    uint64_t AddGenericObject(GenericObjectCreateInfo createInfo, SpawnOffset spawnOffset) {
        EditorSession::AssignEditorName(createInfo, GetGenericObjects());
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::GENERIC_OBJECT);

        GetGenericObjects().emplace_with_id(id, id, createInfo, spawnOffset);
        NavMeshManager::MarkStaticDirty();
        return id;
    }

    GenericObject* GetGenericObjectById(uint64_t objectId) {
        return GetGenericObjects().get(objectId);
    }

    // House Locations

    uint64_t AddHouseLocation(HouseLocationCreateInfo createInfo, SpawnOffset spawnOffset) {
        EditorSession::AssignEditorName(createInfo, GetHouseLocations());
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::HOUSE_LOCATION);
        GetHouseLocations().emplace_with_id(id, id, createInfo, spawnOffset);
        return id;
    }

    HouseLocation* GetHouseLocationByObjectId(uint64_t objectId) {
        return GetHouseLocations().get(objectId);
    }

    // Jetties

    uint64_t AddJetty(JettyCreateInfo createInfo, SpawnOffset spawnOffset) {
        EditorSession::AssignEditorName(createInfo, g_jetties);
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::JETTY);
        g_jetties.emplace_with_id(id, id, createInfo, spawnOffset);
        return id;
    }

    // Kangaroos

    uint64_t AddKangaroo(KangarooCreateInfo createInfo, SpawnOffset spawnOffset) {
        EditorSession::AssignEditorName(createInfo, g_kangaroos);
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::KANGAROO);
        g_kangaroos.emplace_with_id(id, id, createInfo, spawnOffset);
        return id;
    }

    Kangaroo* GetKangarooByObjectId(uint64_t objectId) {
        return GetKangaroos().get(objectId);
    }

    // Ladders

    uint64_t AddLadder(LadderCreateInfo createInfo, SpawnOffset spawnOffset) {
        EditorSession::AssignEditorName(createInfo, GetLadders());
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::LADDER);
        GetLadders().emplace_with_id(id, id, createInfo, spawnOffset);
        return id;
    }

    Ladder* GetLadderByObjectId(uint64_t objectId) {
        return GetLadders().get(objectId);
    }

    // Ladder Dismounts

    uint64_t AddLadderDismount(LadderDismountCreateInfo createInfo, SpawnOffset spawnOffset) {
        EditorSession::AssignEditorName(createInfo, GetLadderDismounts());
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::LADDER_DISMOUNT);
        GetLadderDismounts().emplace_with_id(id, id, createInfo, spawnOffset);
        return id;
    }

    LadderDismount* GetLadderDismountByObjectId(uint64_t objectId) {
        return GetLadderDismounts().get(objectId);
    }

    // Lights

    uint64_t AddLight(LightCreateInfo createInfo, SpawnOffset spawnOffset) {
        EditorSession::AssignEditorName(createInfo, GetLights());
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::LIGHT);
        GetLights().emplace_with_id(id, id, createInfo, spawnOffset);
        return id;
    }

    Jetty* GetJettyById(uint64_t objectId) {
        return GetJetties().get(objectId);

    }
    Light* GetLightByObjectId(uint64_t objectId) {
        return GetLights().get(objectId);
    }


    Light* GetLightByIndex(int32_t index) {
        if (index >= 0 && index < static_cast<int32_t>(GetLights().size())) {
            return &GetLights()[index];
        }

        Logging::Warning() << "World::GetLightByIndex() failed: index " << index << " out of range of size " << GetLights().size();
        return nullptr;
    }

    uint32_t GetLightCount() {
        return static_cast<uint32_t>(GetLights().size());
    }

    std::vector<uint64_t> GetLightIds() {
        std::vector<uint64_t> ids;
        ids.reserve(GetLights().size());

        for (Light& light : GetLights()) {
            if (light.GetType() != LightType::FIREPLACE_FIRE) {
                ids.push_back(light.GetObjectId());
            }
        }

        return ids;
    }

    uint64_t AddPlanarQuadObject(PlanarQuadObjectCreateInfo createInfo, SpawnOffset spawnOffset) {
        const char* defaultEditorName = "Planar Quad Object";
        if (createInfo.type == PlanarQuadObjectType::DECKING_BOARDS) defaultEditorName = "Decking Boards";
        if (createInfo.type == PlanarQuadObjectType::ROOFING_IRON) defaultEditorName = "Roofing Iron";

        std::string desiredName = createInfo.editorName;
        if (desiredName.empty() || desiredName == UNDEFINED_STRING || desiredName == "Undefined") desiredName = defaultEditorName;

        int32_t suffix = 1;
        while (true) {
            createInfo.editorName = suffix == 1 ? desiredName : desiredName + " " + std::to_string(suffix);
            bool nameAvailable = true;
            for (const PlanarQuadObject& object : GetPlanarQuadObjects()) {
                if (object.GetEditorName() == createInfo.editorName) nameAvailable = false;
            }
            if (nameAvailable) break;
            suffix++;
        }

        const uint64_t id = Unloved::GetNextObjectId(ObjectType::PLANAR_QUAD_OBJECT);
        GetPlanarQuadObjects().emplace_with_id(id, id, createInfo, spawnOffset);
        return id;
    }

    PlanarQuadObject* GetPlanarQuadObjectByObjectId(uint64_t objectId) {
        return GetPlanarQuadObjects().get(objectId);
    }

    uint64_t AddPointPairObject(PointPairCreateInfo createInfo, SpawnOffset spawnOffset) {
        const char* defaultEditorName = "Point Pair Object";
        if (createInfo.type == PointPairObjectType::DECKING_BEARER) defaultEditorName = "Decking Bearer";
        if (createInfo.type == PointPairObjectType::DECKING_POST) defaultEditorName = "Decking Post";
        if (createInfo.type == PointPairObjectType::GUTTER) defaultEditorName = "Gutter";
        if (createInfo.type == PointPairObjectType::RIDGE_CAPPING) defaultEditorName = "Ridge Capping";
        if (createInfo.type == PointPairObjectType::DOWN_PIPE) defaultEditorName = "Down Pipe";

        std::string desiredName = createInfo.editorName;
        if (desiredName.empty() || desiredName == UNDEFINED_STRING || desiredName == "Undefined") desiredName = defaultEditorName;
        int32_t suffix = 1;
        while (true) {
            createInfo.editorName = suffix == 1 ? desiredName : desiredName + " " + std::to_string(suffix);
            bool nameAvailable = true;
            for (const PointPairObject& object : GetPointPairObjects()) {
                if (object.GetEditorName() == createInfo.editorName) nameAvailable = false;
            }
            if (nameAvailable) break;
            suffix++;
        }

        const uint64_t id = Unloved::GetNextObjectId(ObjectType::POINT_PAIR_OBJECT);
        GetPointPairObjects().emplace_with_id(id, id, createInfo, spawnOffset);
        return id;
    }

    PointPairObject* GetPointPairObjectByObjectId(uint64_t objectId) {
        return GetPointPairObjects().get(objectId);
    }

    uint64_t AddSpotLight(uint64_t ownerObjectId, int32_t ownerViewportIndex) {
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::SPOT_LIGHT);
        GetSpotLights().emplace_with_id(id, id, ownerObjectId, ownerViewportIndex);
        return id;
    }

    SpotLight* GetSpotLightByObjectId(uint64_t objectId) {
        return GetSpotLights().get(objectId);
    }

    // Mermaids

    uint64_t AddMermaid(MermaidCreateInfo createInfo, SpawnOffset spawnOffset) {
        EditorSession::AssignEditorName(createInfo, GetMermaids());
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::MERMAID);
        GetMermaids().emplace_with_id(id, id, createInfo, spawnOffset);
        return id;
    }

    Mermaid* GetMermaidByObjectId(uint64_t objectId) {
        return GetMermaids().get(objectId);
    }

    // Pianos

    uint64_t AddPiano(PianoCreateInfo createInfo, SpawnOffset spawnOffset) {
        if (createInfo.soundFontName == UNDEFINED_STRING) {
            createInfo.soundFontName = "YamahaGrandLiteV2";
        }

        EditorSession::AssignEditorName(createInfo, GetPianos());
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::PIANO);
        GetPianos().emplace_with_id(id, id, createInfo, spawnOffset);
        NavMeshManager::MarkStaticDirty();
        return id;
    }

    Piano* GetPianoByObjectId(uint64_t objectId) {
        return GetPianos().get(objectId);
    }

    Piano* GetPianoByMeshNodeObjectId(uint64_t objectId) {
        for (Piano& piano : GetPianos()) {
            MeshNodes& meshNodes = piano.GetMeshNodes();
            if (meshNodes.HasNodeWithObjectId(objectId)) {
                return &piano;
            }
        }
        return nullptr;
    }

    PianoKey* GetPianoKeyByObjectId(uint64_t objectId) {
        for (Piano& piano : GetPianos()) {
            if (piano.PianoKeyExists(objectId)) {
                return piano.GetPianoKey(objectId);
            }
        }
        return nullptr;
    }

    // Pick Ups

    uint64_t AddPickUp(PickUpCreateInfo createInfo, SpawnOffset spawnOffset) {
        if (!Bible::GetItemInfoByName(createInfo.name)) {
            Logging::Warning() << "World::AddPickUp(..) failed: '" << createInfo.name << "' ItemInfo not found in bible";
            return 0;
        }

        EditorSession::AssignEditorName(createInfo, GetPickUps());
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::PICK_UP);
        GetPickUps().emplace_with_id(id, id, createInfo, spawnOffset);
        return id;
    }

    PickUp* GetPickUpByObjectId(uint64_t objectId) {
        return GetPickUps().get(objectId);
    }

    // Picture Frames

    uint64_t AddPictureFrame(PictureFrameCreateInfo createInfo, SpawnOffset spawnOffset) {
        EditorSession::AssignEditorName(createInfo, GetPictureFrames());
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::PICTURE_FRAME);
        GetPictureFrames().emplace_with_id(id, id, createInfo, spawnOffset);
        return id;
    }

    PictureFrame* GetPictureFrameByObjectId(uint64_t objectId) {
        return GetPictureFrames().get(objectId);
    }

    // Power Pole Sets

    uint64_t AddPowerPoleSet(PowerPoleSetCreateInfo createInfo, SpawnOffset spawnOffset) {
        EditorSession::AssignEditorName(createInfo, GetPowerPoleSets());
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::POWER_POLE_SET);
        GetPowerPoleSets().emplace_with_id(id, id, createInfo, spawnOffset);
        return id;
    }

    PowerPoleSet* GetPowerPoleSetByObjectId(uint64_t objectId) {
        return GetPowerPoleSets().get(objectId);
    }

    // Sharks

    uint64_t AddShark(SharkCreateInfo createInfo, SpawnOffset spawnOffset) {
        EditorSession::AssignEditorName(createInfo, g_sharks);
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::SHARK);
        g_sharks.emplace_with_id(id, id, createInfo, spawnOffset);
        return id;
    }

    Shark* GetSharkByObjectId(uint64_t objectId) {
        return g_sharks.get(objectId);
    }

    // Spawn Points

    uint64_t AddSpawnPointCampaign(SpawnPointCreateInfo createInfo, SpawnOffset spawnOffset) {
        EditorSession::AssignEditorName(createInfo, g_spawnPointsCampaign);
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::SPAWN_POINT_CAMPAIGN);
        g_spawnPointsCampaign.emplace_with_id(id, id, createInfo, spawnOffset);
        return id;
    }

    uint64_t AddSpawnPointDeathMatch(SpawnPointCreateInfo createInfo, SpawnOffset spawnOffset) {
        EditorSession::AssignEditorName(createInfo, g_spawnPointsDeathMatch);
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::SPAWN_POINT_DEATHMATCH);
        g_spawnPointsDeathMatch.emplace_with_id(id, id, createInfo, spawnOffset);
        return id;
    }

    SpawnPoint* GetSpawnPointCampaignByObjectId(uint64_t objectId) {
        return g_spawnPointsCampaign.get(objectId);
    }

    SpawnPoint* GetSpawnPointDeathMatchByObjectId(uint64_t objectId) {
        return g_spawnPointsDeathMatch.get(objectId);
    }

    // Staircases

    uint64_t AddStaircase(StaircaseCreateInfo createInfo, SpawnOffset spawnOffset) {
        EditorSession::AssignEditorName(createInfo, GetStaircases());
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::STAIRCASE);
        GetStaircases().emplace_with_id(id, id, createInfo, spawnOffset);
        return id;
    }

    Staircase* GetStaircaseByObjectId(uint64_t objectId) {
        return GetStaircases().get(objectId);
    }

    // Trim Sets

    uint64_t AddTrimSet(TrimSetCreateInfo createInfo, SpawnOffset spawnOffset) {
        EditorSession::AssignEditorName(createInfo, GetTrimSets());
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::TRIM_SET);
        GetTrimSets().emplace_with_id(id, id, createInfo, spawnOffset);
        return id;
    }

    TrimSet* GetTrimSetByObjectId(uint64_t objectId) {
        return GetTrimSets().get(objectId);
    }

    // Walls

    uint64_t AddWall(WallCreateInfo createInfo, SpawnOffset spawnOffset) {
        if (createInfo.sequencePoints.empty()) {
            Logging::Warning() << "World::AddWall() failed: createInfo has zero points!";
            return 0;
        }

        EditorSession::AssignEditorName(createInfo, GetWalls());
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::WALL);

        GetWalls().emplace_with_id(id, id, createInfo, spawnOffset);
        HouseBuilder::MarkDirty();
        return id;
    }

    Wall* GetWallByObjectId(uint64_t objectId) {
        return GetWalls().get(objectId);
    }

    Wall* GetWallByWallSegmentObjectId(uint64_t objectId) {
        for (Wall& wall : GetWalls()) {
            for (WallSegment& wallSegment : wall.GetWallSegments()) {
                if (wallSegment.GetObjectId() == objectId) {
                    return &wall;
                }
            }
        }
        return nullptr;
    }

    // Wires

    uint64_t AddWire(WireCreateInfo createInfo) {
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::WIRE);
        GetWires().emplace_with_id(id, id, createInfo);
        return id;
    }

    Wire* GetWireByObjectId(uint64_t objectId) {
        return GetWires().get(objectId);
    }

    // Windows

    uint64_t AddWindow(WindowCreateInfo createInfo, SpawnOffset spawnOffset) {
        EditorSession::AssignEditorName(createInfo, GetWindows());
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::WINDOW);
        GetWindows().emplace_with_id(id, id, createInfo, spawnOffset);
        HouseBuilder::MarkDirty();
        return id;
    }

    Window* GetWindowByObjectId(uint64_t objectId) {
        return GetWindows().get(objectId);
    }

    // World Planes

    uint64_t AddWorldPlane(WorldPlaneCreateInfo createInfo, SpawnOffset spawnOffset) {
        EditorSession::AssignEditorName(createInfo, GetWorldPlanes());
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::WORLD_PLANE);

        GetWorldPlanes().emplace_with_id(id, id, createInfo, spawnOffset);
        HouseBuilder::MarkDirty();
        return id;
    }

    WorldPlane* GetWorldPlaneByObjectId(uint64_t objectId) {
        return GetWorldPlanes().get(objectId);
    }

    // Duplicate Object

    template<typename Container, typename AddFunction>
    uint64_t DuplicateObject_T(Container& objects, uint64_t objectId, AddFunction addObject) {
        auto* object = objects.get(objectId);
        if (!object) return 0;

        auto createInfo = object->GetCreateInfo();
        const size_t suffixStart = createInfo.editorName.find_last_of(' ');
        if (suffixStart != std::string::npos && suffixStart + 1 < createInfo.editorName.size()) {
            bool numberedSuffix = true;
            for (size_t i = suffixStart + 1; i < createInfo.editorName.size(); i++) numberedSuffix &= createInfo.editorName[i] >= '0' && createInfo.editorName[i] <= '9';
            const std::string baseName = createInfo.editorName.substr(0, suffixStart);
            if (numberedSuffix && !EditorSession::EditorNameAvailable(objects, baseName)) createInfo.editorName = baseName;
        }
        return addObject(createInfo, SpawnOffset());
    }

    uint64_t DuplicateObjectById(uint64_t objectId) {
        if (objectId == 0) return 0;

        switch (GetObjectIdType(objectId)) {
        case ObjectType::CHRISTMAS_LIGHTS:       return DuplicateObject_T(GetChristmasLightSets(), objectId, AddChristmasLights);
        case ObjectType::TREE:                   return DuplicateObject_T(GetChristmasTrees(), objectId, AddChristmasTree);
        case ObjectType::DDGI_VOLUME:            return DuplicateObject_T(GetDDGIVolumes(), objectId, AddDDGIVolume);
        case ObjectType::DOBERMANN:              return DuplicateObject_T(GetDobermanns(), objectId, AddDobermann);
        case ObjectType::DOOR:                   return DuplicateObject_T(GetDoors(), objectId, AddDoor);
        case ObjectType::FENCE:                  return DuplicateObject_T(GetFences(), objectId, AddFence);
        case ObjectType::FIREPLACE:              return DuplicateObject_T(GetFireplaces(), objectId, AddFireplace);
        case ObjectType::GAME_OBJECT:            return DuplicateObject_T(GetGameObjects(), objectId, AddGameObject);
        case ObjectType::GENERIC_ANIMATED_OBJECT:return DuplicateObject_T(GetGenericAnimatedObjects(), objectId, AddGenericAnimatedObject);
        case ObjectType::GENERIC_OBJECT:         return DuplicateObject_T(GetGenericObjects(), objectId, AddGenericObject);
        case ObjectType::HOUSE_LOCATION:         return DuplicateObject_T(GetHouseLocations(), objectId, AddHouseLocation);
        case ObjectType::WORLD_PLANE:            return DuplicateObject_T(GetWorldPlanes(), objectId, AddWorldPlane);
        case ObjectType::KANGAROO:               return DuplicateObject_T(GetKangaroos(), objectId, AddKangaroo);
        case ObjectType::JETTY:                  return DuplicateObject_T(GetJetties(), objectId, AddJetty);
        case ObjectType::LADDER:                 return DuplicateObject_T(GetLadders(), objectId, AddLadder);
        case ObjectType::LADDER_DISMOUNT:        return DuplicateObject_T(GetLadderDismounts(), objectId, AddLadderDismount);
        case ObjectType::LIGHT:                  return DuplicateObject_T(GetLights(), objectId, AddLight);
        case ObjectType::MERMAID:                return DuplicateObject_T(GetMermaids(), objectId, AddMermaid);
        case ObjectType::PIANO:                  return DuplicateObject_T(GetPianos(), objectId, AddPiano);
        case ObjectType::PICK_UP:                return DuplicateObject_T(GetPickUps(), objectId, AddPickUp);
        case ObjectType::PICTURE_FRAME:          return DuplicateObject_T(GetPictureFrames(), objectId, AddPictureFrame);
        case ObjectType::POWER_POLE_SET:         return DuplicateObject_T(GetPowerPoleSets(), objectId, AddPowerPoleSet);
        case ObjectType::PLANAR_QUAD_OBJECT:     return DuplicateObject_T(GetPlanarQuadObjects(), objectId, AddPlanarQuadObject);
        case ObjectType::POINT_PAIR_OBJECT:      return DuplicateObject_T(GetPointPairObjects(), objectId, AddPointPairObject);
        case ObjectType::SHARK:                  return DuplicateObject_T(GetSharks(), objectId, AddShark);
        case ObjectType::SPAWN_POINT_CAMPAIGN:   return DuplicateObject_T(GetSpawnPointsCampaign(), objectId, AddSpawnPointCampaign);
        case ObjectType::SPAWN_POINT_DEATHMATCH: return DuplicateObject_T(GetSpawnPointsDeathMatch(), objectId, AddSpawnPointDeathMatch);
        case ObjectType::STAIRCASE:              return DuplicateObject_T(GetStaircases(), objectId, AddStaircase);
        case ObjectType::TRIM_SET:               return DuplicateObject_T(GetTrimSets(), objectId, AddTrimSet);
        case ObjectType::WALL:                   return DuplicateObject_T(GetWalls(), objectId, AddWall);
        case ObjectType::WINDOW:                 return DuplicateObject_T(GetWindows(), objectId, AddWindow);
        default:                                 return 0;
        }
    }

    // Set Position

    template<typename Container>
    bool SetPosition_T(Container& objects, uint64_t objectId, const glm::vec3& position) {
        auto* object = objects.get(objectId);
        if (!object) return false;

        object->SetPosition(position);
        return true;
    }

    bool SetPositionById(uint64_t objectId, const glm::vec3& position) {
        if (objectId == 0) return false;

        bool updated = false;

        switch (GetObjectIdType(objectId)) {
        case ObjectType::SKINNED_GAME_OBJECT: updated = SetPosition_T(GetSkinnedGameObjects(), objectId, position); break;
        case ObjectType::CHRISTMAS_LIGHTS: updated = SetPosition_T(GetChristmasLightSets(), objectId, position); break;
        case ObjectType::DDGI_VOLUME:    updated = SetPosition_T(GetDDGIVolumes(), objectId, position); break;
        case ObjectType::DOBERMANN:      updated = SetPosition_T(GetDobermanns(), objectId, position); break;
        case ObjectType::DOOR:           updated = SetPosition_T(GetDoors(), objectId, position); break;
        case ObjectType::FENCE:          updated = SetPosition_T(GetFences(), objectId, position); break;
        case ObjectType::FIREPLACE:      updated = SetPosition_T(GetFireplaces(), objectId, position); break;
        case ObjectType::GAME_OBJECT:    updated = SetPosition_T(GetGameObjects(), objectId, position); break;
        case ObjectType::GENERIC_ANIMATED_OBJECT: updated = SetPosition_T(GetGenericAnimatedObjects(), objectId, position); break;
        case ObjectType::GENERIC_OBJECT: updated = SetPosition_T(GetGenericObjects(), objectId, position); break;
        case ObjectType::HOUSE_LOCATION: updated = SetPosition_T(GetHouseLocations(), objectId, position); break;
        case ObjectType::WORLD_PLANE:    updated = SetPosition_T(GetWorldPlanes(), objectId, position); break;
        case ObjectType::KANGAROO:       updated = SetPosition_T(GetKangaroos(), objectId, position); break;
        case ObjectType::JETTY:          updated = SetPosition_T(GetJetties(), objectId, position); break;
        case ObjectType::LADDER:         updated = SetPosition_T(GetLadders(), objectId, position); break;
        case ObjectType::LADDER_DISMOUNT:updated = SetPosition_T(GetLadderDismounts(), objectId, position); break;
        case ObjectType::LIGHT:          updated = SetPosition_T(GetLights(), objectId, position); break;
        case ObjectType::MERMAID:        updated = SetPosition_T(GetMermaids(), objectId, position); break;
        case ObjectType::PIANO:          updated = SetPosition_T(GetPianos(), objectId, position); break;
        case ObjectType::PICK_UP:        updated = SetPosition_T(GetPickUps(), objectId, position); break;
        case ObjectType::PICTURE_FRAME:  updated = SetPosition_T(GetPictureFrames(), objectId, position); break;
        case ObjectType::POWER_POLE_SET: updated = SetPosition_T(GetPowerPoleSets(), objectId, position); break;
        case ObjectType::PLANAR_QUAD_OBJECT: updated = SetPosition_T(GetPlanarQuadObjects(), objectId, position); break;
        case ObjectType::POINT_PAIR_OBJECT:  updated = SetPosition_T(GetPointPairObjects(), objectId, position); break;
        case ObjectType::SHARK:          updated = SetPosition_T(GetSharks(), objectId, position); break;
        case ObjectType::SPAWN_POINT_CAMPAIGN:   updated = SetPosition_T(GetSpawnPointsCampaign(), objectId, position); break;
        case ObjectType::SPAWN_POINT_DEATHMATCH: updated = SetPosition_T(GetSpawnPointsDeathMatch(), objectId, position); break;
        case ObjectType::STAIRCASE:      updated = SetPosition_T(GetStaircases(), objectId, position); break;
        case ObjectType::WALL:           updated = SetPosition_T(GetWalls(), objectId, position); break;
        case ObjectType::WINDOW:         updated = SetPosition_T(GetWindows(), objectId, position); break;
        default:
            Logging::Error() << "World::SetPositionById() failed: unsupported object type '" << Hell::Enum::ToString(GetObjectIdType(objectId)) << "'\n";
            return false;
        }

        return updated;
    }

    // Set Rotation

    template<typename Container>
    bool SetRotation_T(Container& objects, uint64_t objectId, const glm::vec3& rotation) {
        auto* object = objects.get(objectId);
        if (!object) return false;

        object->SetRotation(rotation);
        return true;
    }

    bool SetRotationById(uint64_t objectId, const glm::vec3& rotation) {
        if (objectId == 0) return false;

        switch (GetObjectIdType(objectId)) {
        case ObjectType::DDGI_VOLUME:    return SetRotation_T(GetDDGIVolumes(), objectId, rotation);
        case ObjectType::DOBERMANN:      return SetRotation_T(GetDobermanns(), objectId, rotation);
        case ObjectType::DOOR:           return SetRotation_T(GetDoors(), objectId, rotation);
        case ObjectType::FIREPLACE:      return SetRotation_T(GetFireplaces(), objectId, rotation);
        case ObjectType::GAME_OBJECT:    return SetRotation_T(GetGameObjects(), objectId, rotation);
        case ObjectType::GENERIC_ANIMATED_OBJECT: return SetRotation_T(GetGenericAnimatedObjects(), objectId, rotation);
        case ObjectType::GENERIC_OBJECT: return SetRotation_T(GetGenericObjects(), objectId, rotation);
        case ObjectType::HOUSE_LOCATION: return SetRotation_T(GetHouseLocations(), objectId, rotation);
        case ObjectType::KANGAROO:       return SetRotation_T(GetKangaroos(), objectId, rotation);
        case ObjectType::JETTY:          return SetRotation_T(GetJetties(), objectId, rotation);
        case ObjectType::LADDER:         return SetRotation_T(GetLadders(), objectId, rotation);
        case ObjectType::LIGHT:          return SetRotation_T(GetLights(), objectId, rotation);
        case ObjectType::MERMAID:        return SetRotation_T(GetMermaids(), objectId, rotation);
        case ObjectType::PIANO:          return SetRotation_T(GetPianos(), objectId, rotation);
        case ObjectType::PICK_UP:        return SetRotation_T(GetPickUps(), objectId, rotation);
        case ObjectType::PICTURE_FRAME:  return SetRotation_T(GetPictureFrames(), objectId, rotation);
        case ObjectType::PLANAR_QUAD_OBJECT: return SetRotation_T(GetPlanarQuadObjects(), objectId, rotation);
        case ObjectType::POINT_PAIR_OBJECT:  return SetRotation_T(GetPointPairObjects(), objectId, rotation);
        case ObjectType::SPAWN_POINT_CAMPAIGN:   return SetRotation_T(GetSpawnPointsCampaign(), objectId, rotation);
        case ObjectType::SPAWN_POINT_DEATHMATCH: return SetRotation_T(GetSpawnPointsDeathMatch(), objectId, rotation);
        case ObjectType::STAIRCASE:      return SetRotation_T(GetStaircases(), objectId, rotation);
        case ObjectType::WINDOW:         return SetRotation_T(GetWindows(), objectId, rotation);
        case ObjectType::WORLD_PLANE:    return SetRotation_T(GetWorldPlanes(), objectId, rotation);
        default:
            Logging::Error() << "World::SetRotationById() failed: unsupported object type '" << Hell::Enum::ToString(GetObjectIdType(objectId)) << "'\n";
            return false;
        }
    }

    // Get Position

    template<typename Container>
    const glm::vec3* GetPosition_T(Container& objects, uint64_t objectId) {
        auto* object = objects.get(objectId);
        if (!object) return nullptr;

        return &object->GetPosition();
    }

    template<typename Container>
    const glm::vec3* GetCreateInfoPosition_T(Container& objects, uint64_t objectId) {
        const auto* object = objects.get(objectId);
        if (!object) return nullptr;

        return &object->GetCreateInfo().position;
    }

    template<typename Container>
    const glm::vec3* GetWorldSpaceCenter_T(Container& objects, uint64_t objectId) {
        auto* object = objects.get(objectId);
        if (!object) return nullptr;

        return &object->GetWorldSpaceCenter();
    }

    const glm::vec3& GetPositionById(uint64_t objectId) {
        const static glm::vec3 invalid = glm::vec3(0.0f);
        if (objectId == 0) return invalid;

        const glm::vec3* position = nullptr;

        switch (GetObjectIdType(objectId)) {
        case ObjectType::SKINNED_GAME_OBJECT:   position = GetPosition_T(GetSkinnedGameObjects(), objectId); break;
        case ObjectType::BULLET_CASING:          position = GetCreateInfoPosition_T(GetBulletCasings(), objectId); break;
        case ObjectType::CHRISTMAS_LIGHTS:       position = GetPosition_T(GetChristmasLightSets(), objectId); break;
        case ObjectType::TREE:                   position = GetPosition_T(GetChristmasTrees(), objectId); break;
        case ObjectType::DDGI_VOLUME:            position = GetPosition_T(GetDDGIVolumes(), objectId); break;
        case ObjectType::DOBERMANN:              position = GetCreateInfoPosition_T(GetDobermanns(), objectId); break;
        case ObjectType::DOOR:                   position = GetPosition_T(GetDoors(), objectId); break;
        case ObjectType::FENCE:                  position = GetPosition_T(GetFences(), objectId); break;
        case ObjectType::FIREPLACE:              position = GetPosition_T(GetFireplaces(), objectId); break;
        case ObjectType::GAME_OBJECT:            position = GetPosition_T(GetGameObjects(), objectId); break;
        case ObjectType::GENERIC_ANIMATED_OBJECT:position = GetPosition_T(GetGenericAnimatedObjects(), objectId); break;
        case ObjectType::GENERIC_OBJECT:         position = GetPosition_T(GetGenericObjects(), objectId); break;
        case ObjectType::HOUSE_LOCATION:         position = GetPosition_T(GetHouseLocations(), objectId); break;
        case ObjectType::WORLD_PLANE:            position = GetWorldSpaceCenter_T(GetWorldPlanes(), objectId); break;
        case ObjectType::KANGAROO:               position = GetPosition_T(GetKangaroos(), objectId); break;
        case ObjectType::LADDER:                 position = GetPosition_T(GetLadders(), objectId); break;
        case ObjectType::LADDER_DISMOUNT:        position = GetPosition_T(GetLadderDismounts(), objectId); break;
        case ObjectType::JETTY:                  position = GetPosition_T(GetJetties(), objectId); break;
        case ObjectType::LIGHT:                  position = GetPosition_T(GetLights(), objectId); break;
        case ObjectType::MERMAID:                position = GetPosition_T(GetMermaids(), objectId); break;
        case ObjectType::PIANO:                  position = GetPosition_T(GetPianos(), objectId); break;
        case ObjectType::PICK_UP:                position = GetPosition_T(GetPickUps(), objectId); break;
        case ObjectType::PICTURE_FRAME:          position = GetPosition_T(GetPictureFrames(), objectId); break;
        case ObjectType::POWER_POLE_SET:         position = GetPosition_T(GetPowerPoleSets(), objectId); break;
        case ObjectType::PLANAR_QUAD_OBJECT:     position = GetPosition_T(GetPlanarQuadObjects(), objectId); break;
        case ObjectType::POINT_PAIR_OBJECT:      position = GetPosition_T(GetPointPairObjects(), objectId); break;
        case ObjectType::SHARK:                  position = GetCreateInfoPosition_T(GetSharks(), objectId); break;
        case ObjectType::SPAWN_POINT_CAMPAIGN:   position = GetPosition_T(GetSpawnPointsCampaign(), objectId); break;
        case ObjectType::SPAWN_POINT_DEATHMATCH: position = GetPosition_T(GetSpawnPointsDeathMatch(), objectId); break;
        case ObjectType::STAIRCASE:              position = GetPosition_T(GetStaircases(), objectId); break;
        case ObjectType::WALL:                   position = GetWorldSpaceCenter_T(GetWalls(), objectId); break;
        case ObjectType::WINDOW:                 position = GetPosition_T(GetWindows(), objectId); break;
        default:
            break;
        }

        if (position) return *position;

        Logging::Error() << "World::GetPositionById() failed: unsupported object type '" << Hell::Enum::ToString(GetObjectIdType(objectId)) << "'\n";
        return invalid;
    }

    // Get Rotation

    template<typename Container>
    const glm::vec3* GetRotation_T(Container& objects, uint64_t objectId) {
        auto* object = objects.get(objectId);
        if (!object) return nullptr;

        return &object->GetRotation();
    }

    template<typename Container>
    const glm::vec3* GetCreateInfoRotation_T(Container& objects, uint64_t objectId) {
        const auto* object = objects.get(objectId);
        if (!object) return nullptr;

        return &object->GetCreateInfo().rotation;
    }

    glm::vec3 GetRotationById(uint64_t objectId) {
        const static glm::vec3 invalid = glm::vec3(0.0f);
        if (objectId == 0) return invalid;

        const glm::vec3* rotation = nullptr;

        switch (GetObjectIdType(objectId)) {
        case ObjectType::BULLET_CASING:          rotation = GetCreateInfoRotation_T(GetBulletCasings(), objectId); break;
        case ObjectType::CHRISTMAS_LIGHTS:       rotation = GetRotation_T(GetChristmasLightSets(), objectId); break;
        case ObjectType::TREE:                   rotation = GetCreateInfoRotation_T(GetChristmasTrees(), objectId); break;
        case ObjectType::DDGI_VOLUME:            rotation = GetRotation_T(GetDDGIVolumes(), objectId); break;
        case ObjectType::DOBERMANN:              rotation = GetCreateInfoRotation_T(GetDobermanns(), objectId); break;
        case ObjectType::DOOR:                   rotation = GetRotation_T(GetDoors(), objectId); break;
        case ObjectType::FIREPLACE:              rotation = GetRotation_T(GetFireplaces(), objectId); break;
        case ObjectType::GAME_OBJECT:            rotation = GetRotation_T(GetGameObjects(), objectId); break;
        case ObjectType::GENERIC_ANIMATED_OBJECT:rotation = GetRotation_T(GetGenericAnimatedObjects(), objectId); break;
        case ObjectType::GENERIC_OBJECT:         rotation = GetRotation_T(GetGenericObjects(), objectId); break;
        case ObjectType::HOUSE_LOCATION:         if (HouseLocation* houseLocation = GetHouseLocationByObjectId(objectId)) return houseLocation->GetRotation(); break;
        case ObjectType::KANGAROO:               rotation = GetRotation_T(GetKangaroos(), objectId); break;
        case ObjectType::JETTY:                  rotation = GetRotation_T(GetJetties(), objectId); break;
        case ObjectType::LADDER:                 rotation = GetRotation_T(GetLadders(), objectId); break;
        case ObjectType::LADDER_DISMOUNT:        return invalid;
        case ObjectType::LIGHT:                  rotation = GetRotation_T(GetLights(), objectId); break;
        case ObjectType::MERMAID:                rotation = GetCreateInfoRotation_T(GetMermaids(), objectId); break;
        case ObjectType::PIANO:                  rotation = GetRotation_T(GetPianos(), objectId); break;
        case ObjectType::PICK_UP:                rotation = GetRotation_T(GetPickUps(), objectId); break;
        case ObjectType::PICTURE_FRAME:          rotation = GetRotation_T(GetPictureFrames(), objectId); break;
        case ObjectType::PLANAR_QUAD_OBJECT:     rotation = GetRotation_T(GetPlanarQuadObjects(), objectId); break;
        case ObjectType::POINT_PAIR_OBJECT:      rotation = GetRotation_T(GetPointPairObjects(), objectId); break;
        case ObjectType::SPAWN_POINT_CAMPAIGN:   if (SpawnPoint* spawnPoint = GetSpawnPointCampaignByObjectId(objectId)) return spawnPoint->GetRotation(); break;
        case ObjectType::SPAWN_POINT_DEATHMATCH: if (SpawnPoint* spawnPoint = GetSpawnPointDeathMatchByObjectId(objectId)) return spawnPoint->GetRotation(); break;
        case ObjectType::STAIRCASE:              rotation = GetRotation_T(GetStaircases(), objectId); break;
        case ObjectType::WINDOW:                 rotation = GetRotation_T(GetWindows(), objectId); break;
        case ObjectType::WORLD_PLANE:            rotation = GetRotation_T(GetWorldPlanes(), objectId); break;
        case ObjectType::SKINNED_GAME_OBJECT:
        case ObjectType::FENCE:
        case ObjectType::POWER_POLE_SET:
        case ObjectType::SHARK:
        case ObjectType::WALL:
            return invalid;
        default:
            break;
        }

        if (rotation) return *rotation;

        Logging::Error() << "World::GetRotationById() failed: unsupported object type '" << Hell::Enum::ToString(GetObjectIdType(objectId)) << "'\n";
        return invalid;
    }

    // Get Editor Name

    template<typename Container>
    const std::string* GetEditorName_T(Container& objects, uint64_t objectId) {
        const auto* object = objects.get(objectId);
        if (!object) return nullptr;

        return &object->GetEditorName();
    }

    template<typename Container>
    const std::string* GetCreateInfoEditorName_T(Container& objects, uint64_t objectId) {
        const auto* object = objects.get(objectId);
        if (!object) return nullptr;

        return &object->GetCreateInfo().editorName;
    }

    template<typename Container>
    bool SetEditorName_T(Container& objects, uint64_t objectId, const std::string& editorName) {
        const std::string* currentEditorName = GetEditorName_T(objects, objectId);
        if (!currentEditorName) return false;

        for (auto& object : objects) {
            if (object.GetObjectId() != objectId && object.GetEditorName() == editorName) return false;
        }

        *const_cast<std::string*>(currentEditorName) = editorName;
        return true;
    }

    template<typename Container>
    bool SetCreateInfoEditorName_T(Container& objects, uint64_t objectId, const std::string& editorName) {
        const std::string* currentEditorName = GetCreateInfoEditorName_T(objects, objectId);
        if (!currentEditorName) return false;

        for (auto& object : objects) {
            if (object.GetObjectId() != objectId && object.GetCreateInfo().editorName == editorName) return false;
        }

        *const_cast<std::string*>(currentEditorName) = editorName;
        return true;
    }

    const std::string& GetEditorNameById(uint64_t objectId) {
        const static std::string invalid = "";
        if (objectId == 0) return invalid;

        const std::string* editorName = nullptr;

        switch (GetObjectIdType(objectId)) {
        case ObjectType::SKINNED_GAME_OBJECT:   editorName = GetEditorName_T(GetSkinnedGameObjects(), objectId); break;
        case ObjectType::BULLET_CASING:          editorName = GetEditorName_T(GetBulletCasings(), objectId); break;
        case ObjectType::CHRISTMAS_LIGHTS:       editorName = GetCreateInfoEditorName_T(GetChristmasLightSets(), objectId); break;
        case ObjectType::TREE:                   editorName = GetEditorName_T(GetChristmasTrees(), objectId); break;
        case ObjectType::DECAL:                  editorName = GetEditorName_T(GetDecals(), objectId); break;
        case ObjectType::DDGI_VOLUME:            editorName = GetEditorName_T(GetDDGIVolumes(), objectId); break;
        case ObjectType::DOBERMANN:              editorName = GetEditorName_T(GetDobermanns(), objectId); break;
        case ObjectType::DOOR:                   editorName = GetEditorName_T(GetDoors(), objectId); break;
        case ObjectType::FENCE:                  editorName = GetCreateInfoEditorName_T(GetFences(), objectId); break;
        case ObjectType::FIREPLACE:              editorName = GetCreateInfoEditorName_T(GetFireplaces(), objectId); break;
        case ObjectType::GAME_OBJECT:            editorName = GetCreateInfoEditorName_T(GetGameObjects(), objectId); break;
        case ObjectType::GENERIC_ANIMATED_OBJECT:editorName = GetEditorName_T(GetGenericAnimatedObjects(), objectId); break;
        case ObjectType::GENERIC_OBJECT:         editorName = GetEditorName_T(GetGenericObjects(), objectId); break;
        case ObjectType::HOUSE_LOCATION:         editorName = GetCreateInfoEditorName_T(GetHouseLocations(), objectId); break;
        case ObjectType::WORLD_PLANE:            editorName = GetEditorName_T(GetWorldPlanes(), objectId); break;
        case ObjectType::KANGAROO:               editorName = GetEditorName_T(GetKangaroos(), objectId); break;
        case ObjectType::LADDER:                 editorName = GetCreateInfoEditorName_T(GetLadders(), objectId); break;
        case ObjectType::LADDER_DISMOUNT:        editorName = GetCreateInfoEditorName_T(GetLadderDismounts(), objectId); break;
        case ObjectType::LIGHT:                  editorName = GetCreateInfoEditorName_T(GetLights(), objectId); break;
        case ObjectType::MERMAID:                editorName = GetCreateInfoEditorName_T(GetMermaids(), objectId); break;
        case ObjectType::PIANO:                  editorName = GetEditorName_T(GetPianos(), objectId); break;
        case ObjectType::PICK_UP:                editorName = GetCreateInfoEditorName_T(GetPickUps(), objectId); break;
        case ObjectType::PICTURE_FRAME:          editorName = GetCreateInfoEditorName_T(GetPictureFrames(), objectId); break;
        case ObjectType::POWER_POLE_SET:         editorName = GetCreateInfoEditorName_T(GetPowerPoleSets(), objectId); break;
        case ObjectType::PLANAR_QUAD_OBJECT:     editorName = GetEditorName_T(GetPlanarQuadObjects(), objectId); break;
        case ObjectType::POINT_PAIR_OBJECT:      editorName = GetEditorName_T(GetPointPairObjects(), objectId); break;
        case ObjectType::SHARK:                  editorName = GetEditorName_T(GetSharks(), objectId); break;
        case ObjectType::SPAWN_POINT_CAMPAIGN:   editorName = GetCreateInfoEditorName_T(GetSpawnPointsCampaign(), objectId); break;
        case ObjectType::SPAWN_POINT_DEATHMATCH: editorName = GetCreateInfoEditorName_T(GetSpawnPointsDeathMatch(), objectId); break;
        case ObjectType::STAIRCASE:              editorName = GetCreateInfoEditorName_T(GetStaircases(), objectId); break;
        case ObjectType::TRIM_SET:               editorName = GetEditorName_T(GetTrimSets(), objectId); break;
        case ObjectType::WALL:                   editorName = GetEditorName_T(GetWalls(), objectId); break;
        case ObjectType::WIRE:                   return invalid;
        case ObjectType::WINDOW:                 editorName = GetCreateInfoEditorName_T(GetWindows(), objectId); break;
        default:
            break;
        }

        if (editorName) return *editorName;

        Logging::Error() << "World::GetEditorNameById() failed: unsupported object type '" << Hell::Enum::ToString(GetObjectIdType(objectId)) << "'\n";
        return invalid;
    }

    bool SetEditorNameById(uint64_t objectId, const std::string& editorName) {
        if (objectId == 0) return false;

        switch (GetObjectIdType(objectId)) {
        case ObjectType::SKINNED_GAME_OBJECT:   return SetEditorName_T(GetSkinnedGameObjects(), objectId, editorName);
        case ObjectType::BULLET_CASING:          return SetEditorName_T(GetBulletCasings(), objectId, editorName);
        case ObjectType::CHRISTMAS_LIGHTS:       return SetCreateInfoEditorName_T(GetChristmasLightSets(), objectId, editorName);
        case ObjectType::TREE:                   return SetEditorName_T(GetChristmasTrees(), objectId, editorName);
        case ObjectType::DECAL:                  return SetEditorName_T(GetDecals(), objectId, editorName);
        case ObjectType::DDGI_VOLUME:            return SetEditorName_T(GetDDGIVolumes(), objectId, editorName);
        case ObjectType::DOBERMANN:              return SetEditorName_T(GetDobermanns(), objectId, editorName);
        case ObjectType::DOOR:                   return SetEditorName_T(GetDoors(), objectId, editorName);
        case ObjectType::FENCE:                  return SetCreateInfoEditorName_T(GetFences(), objectId, editorName);
        case ObjectType::FIREPLACE:              return SetCreateInfoEditorName_T(GetFireplaces(), objectId, editorName);
        case ObjectType::GAME_OBJECT:            return SetCreateInfoEditorName_T(GetGameObjects(), objectId, editorName);
        case ObjectType::GENERIC_ANIMATED_OBJECT:return SetEditorName_T(GetGenericAnimatedObjects(), objectId, editorName);
        case ObjectType::GENERIC_OBJECT:         return SetEditorName_T(GetGenericObjects(), objectId, editorName);
        case ObjectType::HOUSE_LOCATION:         return SetCreateInfoEditorName_T(GetHouseLocations(), objectId, editorName);
        case ObjectType::WORLD_PLANE:            return SetEditorName_T(GetWorldPlanes(), objectId, editorName);
        case ObjectType::KANGAROO:               return SetEditorName_T(GetKangaroos(), objectId, editorName);
        case ObjectType::LADDER:                 return SetCreateInfoEditorName_T(GetLadders(), objectId, editorName);
        case ObjectType::LADDER_DISMOUNT:        return SetCreateInfoEditorName_T(GetLadderDismounts(), objectId, editorName);
        case ObjectType::LIGHT:                  return SetCreateInfoEditorName_T(GetLights(), objectId, editorName);
        case ObjectType::MERMAID:                return SetCreateInfoEditorName_T(GetMermaids(), objectId, editorName);
        case ObjectType::PIANO:                  return SetEditorName_T(GetPianos(), objectId, editorName);
        case ObjectType::PICK_UP:                return SetCreateInfoEditorName_T(GetPickUps(), objectId, editorName);
        case ObjectType::PICTURE_FRAME:          return SetCreateInfoEditorName_T(GetPictureFrames(), objectId, editorName);
        case ObjectType::POWER_POLE_SET:         return SetCreateInfoEditorName_T(GetPowerPoleSets(), objectId, editorName);
        case ObjectType::PLANAR_QUAD_OBJECT:     return SetEditorName_T(GetPlanarQuadObjects(), objectId, editorName);
        case ObjectType::POINT_PAIR_OBJECT:      return SetEditorName_T(GetPointPairObjects(), objectId, editorName);
        case ObjectType::SHARK:                  return SetEditorName_T(GetSharks(), objectId, editorName);
        case ObjectType::SPAWN_POINT_CAMPAIGN:   return SetCreateInfoEditorName_T(GetSpawnPointsCampaign(), objectId, editorName);
        case ObjectType::SPAWN_POINT_DEATHMATCH: return SetCreateInfoEditorName_T(GetSpawnPointsDeathMatch(), objectId, editorName);
        case ObjectType::STAIRCASE:              return SetCreateInfoEditorName_T(GetStaircases(), objectId, editorName);
        case ObjectType::TRIM_SET:               return SetEditorName_T(GetTrimSets(), objectId, editorName);
        case ObjectType::WALL:                   return SetEditorName_T(GetWalls(), objectId, editorName);
        case ObjectType::WINDOW:                 return SetCreateInfoEditorName_T(GetWindows(), objectId, editorName);
        default:
            Logging::Error() << "World::SetEditorNameById() failed: unsupported object type '" << Hell::Enum::ToString(GetObjectIdType(objectId)) << "'\n";
            return false;
        }
    }

    // Remove Object

    template<typename Container>
    bool RemoveFromSlotMap(Container& objects, uint64_t objectId) {
        if (!objects.contains(objectId)) return false;

        objects.get(objectId)->CleanUp();
        objects.erase(objectId);
        return true;
    }

    bool RemoveObjectById(uint64_t objectId) {
        if (objectId == 0) return false;

        const ObjectType objectType = GetObjectIdType(objectId);
        bool removed = false;

        switch (objectType) {
        case ObjectType::SKINNED_GAME_OBJECT:    removed = RemoveFromSlotMap(GetSkinnedGameObjects(), objectId); break;
        case ObjectType::BULLET_CASING:           removed = RemoveFromSlotMap(GetBulletCasings(), objectId); break;
        case ObjectType::CHRISTMAS_LIGHTS:        removed = RemoveFromSlotMap(GetChristmasLightSets(), objectId); break;
        case ObjectType::TREE:                    removed = RemoveFromSlotMap(GetChristmasTrees(), objectId); break;
        case ObjectType::DECAL:                   removed = RemoveFromSlotMap(GetDecals(), objectId); break;
        case ObjectType::DDGI_VOLUME:             removed = DDGIManager::RemoveVolume(objectId); break;
        case ObjectType::DOBERMANN:               removed = RemoveFromSlotMap(GetDobermanns(), objectId); break;
        case ObjectType::DOOR:                    removed = RemoveFromSlotMap(GetDoors(), objectId); break;
        case ObjectType::FENCE:                   removed = RemoveFromSlotMap(GetFences(), objectId); break;
        case ObjectType::FIREPLACE:               removed = RemoveFromSlotMap(GetFireplaces(), objectId); break;
        case ObjectType::GAME_OBJECT:             removed = RemoveFromSlotMap(GetGameObjects(), objectId); break;
        case ObjectType::GENERIC_ANIMATED_OBJECT: removed = RemoveFromSlotMap(GetGenericAnimatedObjects(), objectId); break;
        case ObjectType::GENERIC_OBJECT:          removed = RemoveFromSlotMap(GetGenericObjects(), objectId); break;
        case ObjectType::HOUSE_LOCATION:          removed = RemoveFromSlotMap(GetHouseLocations(), objectId); break;
        case ObjectType::WORLD_PLANE:             removed = RemoveFromSlotMap(GetWorldPlanes(), objectId); break;
        case ObjectType::KANGAROO:                removed = RemoveFromSlotMap(GetKangaroos(), objectId); break;
        case ObjectType::LADDER:                  removed = RemoveFromSlotMap(GetLadders(), objectId); break;
        case ObjectType::LADDER_DISMOUNT:         removed = RemoveFromSlotMap(GetLadderDismounts(), objectId); break;
        case ObjectType::LIGHT:                   removed = RemoveFromSlotMap(GetLights(), objectId); break;
        case ObjectType::SPOT_LIGHT:              removed = RemoveFromSlotMap(GetSpotLights(), objectId); break;
        case ObjectType::MERMAID:                 removed = RemoveFromSlotMap(GetMermaids(), objectId); break;
        case ObjectType::PIANO:                   removed = RemoveFromSlotMap(GetPianos(), objectId); break;
        case ObjectType::PICK_UP:                 removed = RemoveFromSlotMap(GetPickUps(), objectId); break;
        case ObjectType::PICTURE_FRAME:           removed = RemoveFromSlotMap(GetPictureFrames(), objectId); break;
        case ObjectType::POWER_POLE_SET:          removed = RemoveFromSlotMap(GetPowerPoleSets(), objectId); break;
        case ObjectType::PLANAR_QUAD_OBJECT:      removed = RemoveFromSlotMap(GetPlanarQuadObjects(), objectId); break;
        case ObjectType::POINT_PAIR_OBJECT:       removed = RemoveFromSlotMap(GetPointPairObjects(), objectId); break;
        case ObjectType::SHARK:                   removed = RemoveFromSlotMap(GetSharks(), objectId); break;
        case ObjectType::SPAWN_POINT_CAMPAIGN:    removed = RemoveFromSlotMap(GetSpawnPointsCampaign(), objectId); break;
        case ObjectType::SPAWN_POINT_DEATHMATCH:  removed = RemoveFromSlotMap(GetSpawnPointsDeathMatch(), objectId); break;
        case ObjectType::STAIRCASE:               removed = RemoveFromSlotMap(GetStaircases(), objectId); break;
        case ObjectType::TRIM_SET:                removed = RemoveFromSlotMap(GetTrimSets(), objectId); break;
        case ObjectType::WALL:                    removed = RemoveFromSlotMap(GetWalls(), objectId); break;
        case ObjectType::WIRE:                    removed = RemoveFromSlotMap(GetWires(), objectId); break;
        case ObjectType::WINDOW:                  removed = RemoveFromSlotMap(GetWindows(), objectId); break;
        default:
            Logging::Error() << "World::RemoveObjectById() failed: unsupported object type '" << Hell::Enum::ToString(objectType) << "'\n";
            return false;
        }

        if (removed &&
            (objectType == ObjectType::DOOR ||
             objectType == ObjectType::FIREPLACE ||
             objectType == ObjectType::WORLD_PLANE ||
             objectType == ObjectType::WALL ||
             objectType == ObjectType::WINDOW)) {
            HouseBuilder::MarkDirty();
        }

        if (removed && objectType == ObjectType::DOOR) {
            NavMeshManager::MarkDynamicDirty();
        }

        if (removed &&
            (objectType == ObjectType::FIREPLACE ||
             objectType == ObjectType::GENERIC_OBJECT ||
             objectType == ObjectType::WORLD_PLANE ||
             objectType == ObjectType::PIANO)) {
            NavMeshManager::MarkStaticDirty();
        }

        return removed;
    }

    // Mesh Node By Object ID And Local Node Index

    template<typename Container>
    MeshNode* GetMeshNodeByObjectIdAndLocalNodeIndexT(Container& objects, uint64_t objectId, int32_t meshNodeLocalIndex) {
        auto* object = objects.get(objectId);
        if (!object) return nullptr;

        return object->GetMeshNodes().GetMeshNodeByLocalIndex(meshNodeLocalIndex);
    }

    MeshNode* GetMeshNodeByObjectIdAndLocalNodeIndex(uint64_t objectId, int32_t meshNodeLocalIndex) {
        if (meshNodeLocalIndex < 0) return nullptr;

        switch (GetObjectIdType(objectId)) {
        case ObjectType::DOOR:           return GetMeshNodeByObjectIdAndLocalNodeIndexT(GetDoors(), objectId, meshNodeLocalIndex);
        case ObjectType::FIREPLACE:      return GetMeshNodeByObjectIdAndLocalNodeIndexT(GetFireplaces(), objectId, meshNodeLocalIndex);
        case ObjectType::GAME_OBJECT:    return GetMeshNodeByObjectIdAndLocalNodeIndexT(GetGameObjects(), objectId, meshNodeLocalIndex);
        case ObjectType::GENERIC_OBJECT: return GetMeshNodeByObjectIdAndLocalNodeIndexT(GetGenericObjects(), objectId, meshNodeLocalIndex);
        case ObjectType::LADDER:         return GetMeshNodeByObjectIdAndLocalNodeIndexT(GetLadders(), objectId, meshNodeLocalIndex);
        case ObjectType::LIGHT:          return GetMeshNodeByObjectIdAndLocalNodeIndexT(GetLights(), objectId, meshNodeLocalIndex);
        case ObjectType::MERMAID:        return GetMeshNodeByObjectIdAndLocalNodeIndexT(GetMermaids(), objectId, meshNodeLocalIndex);
        case ObjectType::PIANO:          return GetMeshNodeByObjectIdAndLocalNodeIndexT(GetPianos(), objectId, meshNodeLocalIndex);
        case ObjectType::PICK_UP:        return GetMeshNodeByObjectIdAndLocalNodeIndexT(GetPickUps(), objectId, meshNodeLocalIndex);
        case ObjectType::PICTURE_FRAME:  return GetMeshNodeByObjectIdAndLocalNodeIndexT(GetPictureFrames(), objectId, meshNodeLocalIndex);
        case ObjectType::WINDOW:         return GetMeshNodeByObjectIdAndLocalNodeIndexT(GetWindows(), objectId, meshNodeLocalIndex);
        default:
            Logging::Error() << "World::GetMeshNodeByObjectIdAndLocalNodeIndex() failed: unsupported object type '" << Hell::Enum::ToString(GetObjectIdType(objectId)) << "'\n";
            return nullptr;
        }
    }

    // Clean Up

    template<typename Container>
    void CleanUpSlotMap(Container& objects) {
        for (auto& object : objects) {
            object.CleanUp();
        }
        objects.clear();
    }

    void CleanUpCasings() {
        CleanUpSlotMap(g_bulletCasings);
    }

    void CleanUpDecals() {
        CleanUpSlotMap(g_decals);
    }

    void CleanUpAll() {
        CoarseWorldBVH::ClearScenes();
        DDGIManager::CleanUp();

        CleanUpSlotMap(g_bulletCasings);
        CleanUpSlotMap(g_christmasLightSets);
        CleanUpSlotMap(g_christmasTrees);
        CleanUpSlotMap(g_decals);
        CleanUpSlotMap(g_dobermanns);
        CleanUpSlotMap(g_doors);
        CleanUpSlotMap(g_fences);
        CleanUpSlotMap(g_fireplaces);
        CleanUpSlotMap(g_gameObjects);
        CleanUpSlotMap(g_genericAnimatedObjects);
        CleanUpSlotMap(g_genericObjects);
        CleanUpSlotMap(g_houseLocations);
        CleanUpSlotMap(g_worldPlanes);
        CleanUpSlotMap(g_jetties);
        CleanUpSlotMap(g_kangaroos);
        CleanUpSlotMap(g_ladders);
        CleanUpSlotMap(g_ladderDismounts);
        CleanUpSlotMap(g_lights);
        CleanUpSlotMap(g_spotLights);
        CleanUpSlotMap(g_mermaids);
        CleanUpSlotMap(g_pianos);
        CleanUpSlotMap(g_pickUps);
        CleanUpSlotMap(g_pictureFrames);
        CleanUpSlotMap(g_powerPoleSets);
        CleanUpSlotMap(g_planarQuadObjects);
        CleanUpSlotMap(g_pointPairObjects);
        CleanUpSlotMap(g_sharks);
        CleanUpSlotMap(g_spawnPointsCampaign);
        CleanUpSlotMap(g_spawnPointsDeathMatch);
        CleanUpSlotMap(g_staircases);
        CleanUpSlotMap(g_trimSets);
        CleanUpSlotMap(g_walls);
        CleanUpSlotMap(g_wires);
        CleanUpSlotMap(g_windows);
        NavMeshManager::MarkStaticDirty();
        NavMeshManager::MarkDynamicDirty();
    }

}
