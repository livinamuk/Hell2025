#include "World.h"

#include "Hell/Logging.h"
#include "Hell/Physics/PhysicsResourceManagement.h"
#include "Hell/Profiling/CPUProfiler.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/Time.h"

#include "Unloved/Characters/Enemies/Dobermann/Dobermann.h"
#include "Unloved/Characters/Enemies/Kangaroo/Kangaroo.h"
#include "Unloved/Characters/Enemies/Shark/Shark.h"
#include "Unloved/Characters/Enemies/Snake/Snake.h"
#include "Unloved/Characters/Mermaids/Mermaid/Mermaid.h"
#include "Unloved/Objects/Effects/Decal.h"
#include "Unloved/Objects/Exterior/Fence.h"
#include "Unloved/Objects/Exterior/Jetty.h"
#include "Unloved/Objects/Exterior/PowerPoleSet.h"
#include "Unloved/Objects/Exterior/Road.h"
#include "Unloved/Objects/Exterior/Wire.h"
#include "Unloved/Objects/House/Door.h"
#include "Unloved/Objects/House/Fireplace.h"
#include "Unloved/Objects/House/PlanarQuadObject.h"
#include "Unloved/Objects/House/PointPairObject.h"
#include "Unloved/Objects/House/TrimSet.h"
#include "Unloved/Objects/House/Wall.h"
#include "Unloved/Objects/House/Window.h"
#include "Unloved/Objects/House/WorldPlane.h"
#include "Unloved/Objects/Interior/Piano.h"
#include "Unloved/Objects/Interior/PictureFrame.h"
#include "Unloved/Objects/Lighting/Light.h"
#include "Unloved/Objects/Props/BulletCasing.h"
#include "Unloved/Objects/Props/Christmas/ChristmasLights.h"
#include "Unloved/Objects/Props/Christmas/ChristmasTree.h"
#include "Unloved/Objects/Props/GameObject.h"
#include "Unloved/Objects/Props/GenericObject.h"
#include "Unloved/Objects/Props/PickUp.h"
#include "Unloved/Objects/Renderables/SkinnedGameObject.h"
#include "Unloved/Objects/Spawns/HouseLocation.h"
#include "Unloved/Objects/Spawns/SpawnPoint.h"
#include "Unloved/Objects/Traversal/Ladder.h"
#include "Unloved/Objects/Traversal/LadderDismount.h"
#include "Unloved/Objects/Traversal/Staircase.h"
#include "Unloved/EditorSession/EditorSession.h"
#include "Unloved/Session/Session.h"

#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Render/Renderer.h"

namespace Unloved::World {

namespace {
    int32_t GetRagdollIgnoredViewportIndex(uint64_t ragdollId) {
        for (int viewportIndex = 0; viewportIndex < 4; viewportIndex++) {
            Unloved::Player* player = Unloved::Session::GetLocalPlayerByViewportIndex(viewportIndex);
            if (player && player->GetRagdollId() == ragdollId) return viewportIndex;
        }

        return -1;
    }

    bool RagdollHasSkinnedModel(uint64_t ragdollId) {
        for (SkinnedGameObject& skinnedGameObject : GetSkinnedGameObjects()) {
            if (skinnedGameObject.GetRagdollId() == ragdollId) {
                return skinnedGameObject.GetSkinnedModel() != nullptr;
            }
        }
        return false;
    }

    void SubmitRagdollPhysicsShapes() {
        const bool debugDrawRagdolls = Unloved::Renderer::GetCurrentRendererSettings().debugDrawRagdolls;

        Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("PhysicsShapeGeometry");
        if (meshBuffer.GetMeshCount() == 0) return;

        for (auto& [ragdollId, ragdoll] : Hell::Physics::GetRagdolls()) {
            if (!debugDrawRagdolls && RagdollHasSkinnedModel(ragdollId)) continue;

            const int32_t ignoredViewportIndex = GetRagdollIgnoredViewportIndex(ragdollId);

            for (uint32_t rigidIndex = 0; rigidIndex < ragdoll.m_pxRigidDynamics.size(); rigidIndex++) {
                const uint32_t meshId = ragdoll.GetMarkerMeshIdByRigidIndex(rigidIndex);
                if (meshId == 0) continue;

                Mesh* mesh = meshBuffer.GetMeshById(meshId);
                if (!mesh) continue;

                const glm::mat4 modelMatrix = ragdoll.GetModelMatrixByRigidIndex(rigidIndex);
                const glm::vec3 markerColor = ragdoll.GetMarkerColorByRigidIndex(rigidIndex);

                RenderItem renderItem;
                renderItem.modelMatrix = modelMatrix;
                renderItem.prevModelMatrix = modelMatrix;
                renderItem.inverseModelMatrix = glm::inverse(modelMatrix);
                renderItem.vertexCount = mesh->vertexCount;
                renderItem.indexCount = mesh->indexCount;
                renderItem.baseVertex = mesh->baseVertex;
                renderItem.baseIndex = mesh->baseIndex;
                renderItem.meshId = meshId;
                renderItem.tintColorR = markerColor.r;
                renderItem.tintColorG = markerColor.g;
                renderItem.tintColorB = markerColor.b;
                renderItem.ignoredViewportIndex = ignoredViewportIndex;

                RenderDataManager::SubmitRenderItemPhysicsShape(renderItem);
            }
        }
    }
}

// TODO: This whole file is pretty fucked. Clean it up.

void SubmitRenderItems() {
    ProfilerCPUZoneFunction();

    SubmitRagdollPhysicsShapes();

    for (int i = 0; i < Unloved::Session::GetLocalPlayerCount(); i++) {
        Unloved::Player* player = Unloved::Session::GetLocalPlayerByViewportIndex(i);
        if (!player) continue;

        player->SubmitP90MagsRenderItems();
    }

    // Main mesh buffer
    for (Door& object : Unloved::World::GetDoors())                   RenderDataManager::SubmitMeshNodes(object.GetMeshNodes());
    for (Fireplace& object : Unloved::World::GetFireplaces())         RenderDataManager::SubmitMeshNodes(object.GetMeshNodes());
    for (GameObject& object : Unloved::World::GetGameObjects())       RenderDataManager::SubmitMeshNodes(object.GetMeshNodes());
    for (GenericObject& object : Unloved::World::GetGenericObjects()) RenderDataManager::SubmitMeshNodes(object.GetMeshNodes());
    for (Mermaid& object : Unloved::World::GetMermaids())             RenderDataManager::SubmitMeshNodes(object.GetMeshNodes());
    for (Piano& object : Unloved::World::GetPianos())                 RenderDataManager::SubmitMeshNodes(object.GetMeshNodes());
    for (PictureFrame& object : Unloved::World::GetPictureFrames())   RenderDataManager::SubmitMeshNodes(object.GetMeshNodes());
    for (Window& object : Unloved::World::GetWindows())               RenderDataManager::SubmitMeshNodes(object.GetMeshNodes());

    // PickUps only if they are not despawned
    for (PickUp& pickUp : Unloved::World::GetPickUps()) {
        if (!pickUp.IsDespawned()) {
            RenderDataManager::SubmitMeshNodes(pickUp.GetMeshNodes());
        }
    }

    // Clean me up
    for (ChristmasTree& object : Unloved::World::GetChristmasTrees())          RenderDataManager::SubmitRenderItems(object.GetRenderItems());
    for (ChristmasLightSet& object : Unloved::World::GetChristmasLightSets())  RenderDataManager::SubmitRenderItems(object.GetRenderItems());
    for (Fence& object : Unloved::World::GetFences())                          RenderDataManager::SubmitRenderItems(object.GetRenderItems());
    for (Jetty& object : Unloved::World::GetJetties())                         RenderDataManager::SubmitRenderItems(object.GetRenderItems());
    for (Ladder& object : Unloved::World::GetLadders())                        RenderDataManager::SubmitRenderItems(object.GetRenderItems());
    for (Light& object : Unloved::World::GetLights())                          RenderDataManager::SubmitRenderItems(object.GetRenderItems());
    for (Staircase& object : Unloved::World::GetStaircases())                  RenderDataManager::SubmitRenderItems(object.GetRenderItems());
    for (TrimSet& object : Unloved::World::GetTrimSets())                      RenderDataManager::SubmitRenderItems(object.GetRenderItems());
    for (PowerPoleSet& object : Unloved::World::GetPowerPoleSets())            RenderDataManager::SubmitRenderItems(object.GetRenderItems());
    for (PlanarQuadObject& object : Unloved::World::GetPlanarQuadObjects())    object.SubmitRenderItems();
    for (PointPairObject& object : Unloved::World::GetPointPairObjects())      object.SubmitRenderItems();
    for (Wall& object : Unloved::World::GetWalls())                            RenderDataManager::SubmitRenderItems(object.GetWeatherBoardstopRenderItems());
    for (Door& object : Unloved::World::GetDoors())                            RenderDataManager::SubmitRenderItems(object.GetAdditionalStaticRenderItems());

    if (Unloved::EditorSession::IsActive()) {
        for (LadderDismount& object : Unloved::World::GetLadderDismounts())    RenderDataManager::SubmitRenderItem(object.GetRenderItem());
        for (HouseLocation& object : Unloved::World::GetHouseLocations())      RenderDataManager::SubmitRenderItems(object.GetRenderItems());
        for (SpawnPoint& object : Unloved::World::GetSpawnPointsCampaign())    RenderDataManager::SubmitRenderItems(object.GetRenderItems());
        for (SpawnPoint& object : Unloved::World::GetSpawnPointsDeathMatch())  RenderDataManager::SubmitRenderItems(object.GetRenderItems());
    }

    for (Wall& wall : Unloved::World::GetWalls()) {
        wall.SubmitRenderItems();
    }

    for (Wire& wire : Unloved::World::GetWires()) {
        wire.SubmitRenderItem();
    }

    for (WorldPlane& worldPlane : Unloved::World::GetWorldPlanes()) {
        worldPlane.SubmitRenderItem();
    }


    for (BulletCasing& bulletCasing : Unloved::World::GetBulletCasings()) {
        bulletCasing.SubmitRenderItem();
    }

    // Animated mesh nodes
    const bool debugDrawRagdolls = Unloved::Renderer::GetCurrentRendererSettings().debugDrawRagdolls;
    for (SkinnedGameObject& skinnedGameObject : Unloved::World::GetSkinnedGameObjects()) {
        if (debugDrawRagdolls && skinnedGameObject.GetRagdoll()) continue;

        skinnedGameObject.UpdateRenderItems();
        RenderDataManager::SubmitAnimatedMeshNodes(skinnedGameObject.GetAnimatedMeshNodes());
    }

    // Update UI after all else
    for (int i = 0; i < Unloved::Session::GetLocalPlayerCount(); i++) {
        Unloved::Player* player = Unloved::Session::GetLocalPlayerByViewportIndex(i);
        if (!player) continue;

        player->UpdateUI(Hell::Time::DeltaTime());
    }
}

}
