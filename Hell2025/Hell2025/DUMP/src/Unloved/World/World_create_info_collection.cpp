#include "World.h"

#include "Unloved/Characters/Enemies/Dobermann/Dobermann.h"
#include "Unloved/Characters/Enemies/Kangaroo/Kangaroo.h"
#include "Unloved/Characters/Enemies/Shark/Shark.h"
#include "Unloved/Characters/Mermaids/Mermaid/Mermaid.h"
#include "Unloved/Objects/Exterior/Fence.h"
#include "Unloved/Objects/Exterior/Jetty.h"
#include "Unloved/Objects/Exterior/PowerPoleSet.h"
#include "Unloved/Objects/House/Door.h"
#include "Unloved/Objects/House/Fireplace.h"
#include "Unloved/Objects/House/PlanarQuadObject.h"
#include "Unloved/Objects/House/PointPairObject.h"
#include "Unloved/Objects/House/WorldPlane.h"
#include "Unloved/Objects/House/Wall.h"
#include "Unloved/Objects/House/Window.h"
#include "Unloved/Objects/Interior/Piano.h"
#include "Unloved/Objects/Interior/PictureFrame.h"
#include "Unloved/Objects/Lighting/Light.h"
#include "Unloved/Objects/Props/Christmas/ChristmasLights.h"
#include "Unloved/Objects/Props/GenericAnimatedObject.h"
#include "Unloved/Objects/Props/GenericObject.h"
#include "Unloved/Objects/Props/PickUp.h"
#include "Unloved/Objects/Spawns/HouseLocation.h"
#include "Unloved/Objects/Spawns/SpawnPoint.h"
#include "Unloved/Objects/Traversal/Ladder.h"
#include "Unloved/Objects/Traversal/LadderDismount.h"
#include "Unloved/Objects/Traversal/Staircase.h"
#include "Unloved/Systems/DDGI/DDGIVolume.h"

namespace Unloved::World {

    // Adds all objects within a CreateInfoCollection to the world
    void AddCreateInfoCollection(const CreateInfoCollection& createInfoCollection, SpawnOffset spawnOffset) {
        for (const ChristmasLightsCreateInfo& createInfo : createInfoCollection.christmasLights)  AddChristmasLights(createInfo, spawnOffset);
        for (const DDGIVolumeCreateInfo& createInfo : createInfoCollection.ddgiVolumes)           AddDDGIVolume(createInfo, spawnOffset);
        for (const DobermannCreateInfo& createInfo : createInfoCollection.dobermanns)             AddDobermann(createInfo, spawnOffset);
        for (const DoorCreateInfo& createInfo : createInfoCollection.doors)                       AddDoor(createInfo, spawnOffset);
        for (const FenceCreateInfo& createInfo : createInfoCollection.fences)                     AddFence(createInfo, spawnOffset);
        for (const FireplaceCreateInfo& createInfo : createInfoCollection.fireplaces)             AddFireplace(createInfo, spawnOffset);
        for (const GenericAnimatedObjectCreateInfo& createInfo : createInfoCollection.genericAnimatedObjects) AddGenericAnimatedObject(createInfo, spawnOffset);
        for (const GenericObjectCreateInfo& createInfo : createInfoCollection.genericObjects)     AddGenericObject(createInfo, spawnOffset);
        for (const LightCreateInfo& createInfo : createInfoCollection.lights)                     AddLight(createInfo, spawnOffset);
        for (const JettyCreateInfo& createInfo : createInfoCollection.jetties)                    AddJetty(createInfo, spawnOffset);
        for (const KangarooCreateInfo& createInfo : createInfoCollection.kangaroos)               AddKangaroo(createInfo, spawnOffset);
        for (const MermaidCreateInfo& createInfo : createInfoCollection.mermaids)                 AddMermaid(createInfo, spawnOffset);
        for (const LadderCreateInfo& createInfo : createInfoCollection.ladders)                   AddLadder(createInfo, spawnOffset);
        for (const LadderDismountCreateInfo& createInfo : createInfoCollection.ladderDismounts)   AddLadderDismount(createInfo, spawnOffset);
        for (const PianoCreateInfo& createInfo : createInfoCollection.pianos)                     AddPiano(createInfo, spawnOffset);
        for (const PickUpCreateInfo& createInfo : createInfoCollection.pickUps)                   AddPickUp(createInfo, spawnOffset);
        for (const PictureFrameCreateInfo& createInfo : createInfoCollection.pictureFrames)       AddPictureFrame(createInfo, spawnOffset);
        for (const PowerPoleSetCreateInfo& createInfo : createInfoCollection.powerPoleSets)       AddPowerPoleSet(createInfo, spawnOffset);
        for (const PlanarQuadObjectCreateInfo& createInfo : createInfoCollection.planarQuadObjects) AddPlanarQuadObject(createInfo, spawnOffset);
        for (const PointPairCreateInfo& createInfo : createInfoCollection.pointPairObjects)         AddPointPairObject(createInfo, spawnOffset);
        for (const SharkCreateInfo& createInfo : createInfoCollection.sharks)                     AddShark(createInfo, spawnOffset);
        for (const SpawnPointCreateInfo& createInfo : createInfoCollection.spawnPointsCampaign)   AddSpawnPointCampaign(createInfo, spawnOffset);
        for (const SpawnPointCreateInfo& createInfo : createInfoCollection.spawnPointsDeathMatch) AddSpawnPointDeathMatch(createInfo, spawnOffset);
        for (const WorldPlaneCreateInfo& createInfo : createInfoCollection.worldPlanes)           AddWorldPlane(createInfo, spawnOffset);
        for (const StaircaseCreateInfo& createInfo : createInfoCollection.staircases)             AddStaircase(createInfo, spawnOffset);
        for (const WallCreateInfo& createInfo : createInfoCollection.walls)                       AddWall(createInfo, spawnOffset);
        for (const WindowCreateInfo& createInfo : createInfoCollection.windows)                   AddWindow(createInfo, spawnOffset);
    }

    template<typename Object, typename Container>
    void AddObject(Object& object, Container& container) {
            container.push_back(object.GetCreateInfo());
    }

    template<typename Object, typename Container>
    void AddObjectIfMarkedForSaving(Object& object, Container& container) {
        if (object.GetCreateInfo().saveToFile) {
            container.push_back(object.GetCreateInfo());
        }
    }

    template<typename Container>
    void AddWorldPlaneIfNotDoorChild(WorldPlane& object, Container& container) {
        if (object.GetParentDoorId() == 0) {
            container.push_back(object.GetCreateInfo());
        }
    }

    // Creates a CreateInfoCollection from all objects in the world
    CreateInfoCollection GetCreateInfoCollection() {
        CreateInfoCollection createInfoCollection;

        for (ChristmasLightSet& object : GetChristmasLightSets()) AddObject(object, createInfoCollection.christmasLights);
        for (Dobermann& object : GetDobermanns())                 AddObject(object, createInfoCollection.dobermanns);
        for (Door& object : GetDoors())                           AddObject(object, createInfoCollection.doors);
        for (Fence& object : GetFences())                         AddObject(object, createInfoCollection.fences);
        for (Fireplace& object : GetFireplaces())                 AddObject(object, createInfoCollection.fireplaces);
        for (GenericAnimatedObject& object : GetGenericAnimatedObjects()) AddObject(object, createInfoCollection.genericAnimatedObjects);
        for (GenericObject& object : GetGenericObjects())         AddObject(object, createInfoCollection.genericObjects);
        for (Ladder& object : GetLadders())                       AddObject(object, createInfoCollection.ladders);
        for (LadderDismount& object : GetLadderDismounts())       AddObject(object, createInfoCollection.ladderDismounts);
        for (Jetty& object : GetJetties())                        AddObject(object, createInfoCollection.jetties);
        for (Kangaroo& object : GetKangaroos())                   AddObject(object, createInfoCollection.kangaroos);
        for (Mermaid& object : GetMermaids())                     AddObject(object, createInfoCollection.mermaids);
        for (Piano& object : GetPianos())                         AddObject(object, createInfoCollection.pianos);
        for (PictureFrame& object : GetPictureFrames())           AddObject(object, createInfoCollection.pictureFrames);
        for (PowerPoleSet& object : GetPowerPoleSets())           AddObject(object, createInfoCollection.powerPoleSets);
        for (PlanarQuadObject& object : GetPlanarQuadObjects()) createInfoCollection.planarQuadObjects.push_back(object.GetCreateInfo());
        for (PointPairObject& object : GetPointPairObjects()) createInfoCollection.pointPairObjects.push_back(object.GetCreateInfo());
        for (Shark& object : GetSharks())                         AddObject(object, createInfoCollection.sharks);
        for (SpawnPoint& object : GetSpawnPointsCampaign())       AddObject(object, createInfoCollection.spawnPointsCampaign);
        for (SpawnPoint& object : GetSpawnPointsDeathMatch())     AddObject(object, createInfoCollection.spawnPointsDeathMatch);
        for (Staircase& object : GetStaircases())                 AddObject(object, createInfoCollection.staircases);
        for (Wall& object : GetWalls())                           AddObject(object, createInfoCollection.walls);
        for (Window& object : GetWindows())                       AddObject(object, createInfoCollection.windows);

        // Conditionals
        for (DDGIVolume& object : GetDDGIVolumes())               AddObjectIfMarkedForSaving(object, createInfoCollection.ddgiVolumes);
        for (Light& object : GetLights())                         AddObjectIfMarkedForSaving(object, createInfoCollection.lights);
        for (PickUp& object : GetPickUps())                       AddObjectIfMarkedForSaving(object, createInfoCollection.pickUps);
        for (WorldPlane& object : GetWorldPlanes())               AddWorldPlaneIfNotDoorChild(object, createInfoCollection.worldPlanes);

        return createInfoCollection;
    }

    std::vector<HouseLocationCreateInfo> GetHouseLocationCreateInfos() {
        std::vector<HouseLocationCreateInfo> createInfos;
        createInfos.reserve(GetHouseLocations().size());
        for (HouseLocation& houseLocation : GetHouseLocations()) createInfos.push_back(houseLocation.GetCreateInfo());
        return createInfos;
    }
}
