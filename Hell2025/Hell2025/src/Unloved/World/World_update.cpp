#include "World.h"

#include "Hell/Profiling/CPUProfiler.h"
#include "Hell/Time.h"

#include "Unloved/Characters/Enemies/Dobermann/Dobermann.h"
#include "Unloved/Characters/Enemies/Kangaroo/Kangaroo.h"
#include "Unloved/Characters/Enemies/Shark/Shark.h"
#include "Unloved/Characters/Mermaids/Mermaid/Mermaid.h"
#include "Unloved/EditorSession/EditorSession.h"
#include "Unloved/Objects/Effects/Decal.h"
#include "Unloved/Objects/Exterior/Fence.h"
#include "Unloved/Objects/Exterior/Jetty.h"
#include "Unloved/Objects/Exterior/PowerPoleSet.h"
#include "Unloved/Objects/Exterior/Road.h"
#include "Unloved/Objects/House/Door.h"
#include "Unloved/Objects/House/Fireplace.h"
#include "Unloved/Objects/House/TrimSet.h"
#include "Unloved/Objects/House/Window.h"
#include "Unloved/Objects/Interior/Piano.h"
#include "Unloved/Objects/Interior/PictureFrame.h"
#include "Unloved/Objects/Lighting/Light.h"
#include "Unloved/Objects/Props/BulletCasing.h"
#include "Unloved/Objects/Props/Christmas/ChristmasLights.h"
#include "Unloved/Objects/Props/Christmas/ChristmasTree.h"
#include "Unloved/Objects/Props/GameObject.h"
#include "Unloved/Objects/Props/GenericAnimatedObject.h"
#include "Unloved/Objects/Props/GenericObject.h"
#include "Unloved/Objects/Props/PickUp.h"
#include "Unloved/Objects/Renderables/SkinnedGameObject.h"
#include "Unloved/Objects/Traversal/Ladder.h"
#include "Unloved/Objects/Traversal/Staircase.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Systems/DDGI/DDGIManager.h"
#include "Unloved/Systems/P90Mag/P90MagManager.h"
#include "Unloved/Systems/WorldBVH/WorldBVH.h"
#include "Unloved/Systems/CoarseWorldBVH/CoarseWorldBVH.h"

#include "Legacy/World/LegacyWorld.h"

namespace Unloved::World {
    namespace {
        void UpdateSkinnedGameObjects() {
            ProfilerCPUZone("Skinned game objects");

            Hell::SlotMap<SkinnedGameObject>& skinnedGameObjects = GetSkinnedGameObjects();

            for (SkinnedGameObject& object : skinnedGameObjects) {
                object.FinalizeAnimation();
            }
        }

        void UpdatePropsAndTraversal(float deltaTime) {
            for (GameObject& object : GetGameObjects())                       object.Update(deltaTime);
            for (GenericObject& object : GetGenericObjects())                 object.Update(deltaTime);
            for (Ladder& object : GetLadders())                               object.Update(deltaTime);
            for (Mermaid& object : GetMermaids())                             object.Update(deltaTime);
            for (Piano& object : GetPianos())                                 object.Update(deltaTime);
            for (PickUp& object : GetPickUps())                               object.Update(deltaTime);
            for (PictureFrame& object : GetPictureFrames())                   object.Update();
            for (PowerPoleSet& object : GetPowerPoleSets())                   object.Update();
            for (Road& object : LegacyWorld::GetRoads())                      object.Update();
            for (Staircase& object : GetStaircases())                         object.Update(deltaTime);
            for (TrimSet& object : GetTrimSets())                             object.Update();
            for (Window& object : GetWindows())                               object.Update(deltaTime);
        }

        void UpdateEnemyObjects(float deltaTime) {
            for (Dobermann& object : GetDobermanns()) object.Update(deltaTime);
            for (Kangaroo& object : GetKangaroos())   object.Update(deltaTime);
            for (Shark& object : GetSharks())         object.Update(deltaTime);
        }

        void UpdateLightingAndDecals(float deltaTime) {
            // These must run in this order otherwise various dirty flags are stale
            DDGIManager::Update();
            for (Light& object : GetLights()) object.Update(deltaTime);
            for (Decal& object : GetDecals()) object.Update();

            P90MagManager::SubmitRenderItems();
        }
    }

    void UpdateBvhs() {
        Unloved::WorldBVH::UpdateBvhs();
        Unloved::CoarseWorldBVH::Update();
    }

    void UpdateEnemyMovement() {
        const float deltaTime = Hell::Time::DeltaTime();

        for (Dobermann& object : GetDobermanns()) object.UpdateMovement(deltaTime);
        for (Kangaroo& object : GetKangaroos())   object.UpdateMovement(deltaTime);
        for (Shark& object : GetSharks())         object.UpdateMovement(deltaTime);
    }

    void UpdateObjects() {
        ProfilerCPUZoneFunction();

        const float deltaTime = Hell::Time::DeltaTime();

        UpdateSkinnedGameObjects();
        for (BulletCasing& object : GetBulletCasings())           object.Update(deltaTime);
        for (ChristmasLightSet& object : GetChristmasLightSets()) object.Update(deltaTime);
        for (ChristmasTree& object : GetChristmasTrees())         object.Update(deltaTime);
        for (Door& object : GetDoors())                           object.Update(deltaTime);
        for (Fence& object : GetFences())                         object.Update();
        for (Fireplace& object : GetFireplaces())                 object.Update(deltaTime);
        for (Jetty& object : GetJetties())                        object.Update();
        UpdatePropsAndTraversal(deltaTime);
        UpdateEnemyObjects(deltaTime);
        UpdateLightingAndDecals(deltaTime);
    }

    void UpdatePlayers() {
        ProfilerCPUZoneFunction();

        const float deltaTime = Hell::Time::DeltaTime();
        const bool disableControl = EditorSession::IsActive();

        for (uint64_t playerId : Session::GetLocalPlayerIds()) {
            Unloved::Player* player = Session::GetPlayerById(playerId);
            if (!player) continue;

            if (disableControl) {
                player->DisableControl();
            }
            else {
                player->EnableControl();
            }
        }

        for (uint64_t playerId : Session::GetLocalPlayerIds()) {
            Unloved::Player* player = Session::GetPlayerById(playerId);
            if (!player) continue;

            player->Update(deltaTime);
        }
    }
}
