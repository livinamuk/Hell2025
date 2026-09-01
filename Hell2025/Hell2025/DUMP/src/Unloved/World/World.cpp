#include "World.h"

#include "Legacy/World/LegacyWorld.h"
#include "Hell/Common/Constants.h"

#include "Unloved/Characters/Enemies/Kangaroo/Kangaroo.h"
#include "Unloved/Maps/Map.h"
#include "Unloved/Maps/MapData.h"
#include "Unloved/Render/Renderer.h"
#include "Unloved/Systems/House/HouseBuilder.h"
#include "Unloved/Systems/Map/MapManager.h"

namespace {
    uint64_t g_generation = 0;
}

namespace Unloved::World {

    void NewRun(const std::string& mapName) {
        MapManager::LoadMapData(mapName);
        MapData* mapData = MapManager::GetMapDataByName(mapName);
        if (!mapData) return;

        ResetWorld();

        for (Kangaroo& kangaroo : GetKangaroos()) {
            kangaroo.Respawn();
        }

        MapCreateInfo mapCreateInfo;
        mapCreateInfo.mapName = mapName;
        LoadMaps({ mapCreateInfo });
    }

    void BeginFrame() {
        LegacyWorld::BeginFrame();
    }

    void Update() {
        UpdateEnvironment();
    }

    void EndFrame() {
        LegacyWorld::EndFrame();
    }

    void CleanUp() {
        CleanUpAll();
    }

    void ResetWorld() {
        Renderer::WaitIdle();
        LegacyWorld::ResetWorld();
        HouseBuilder::ResetPictureFrameImageList();
        g_generation++;
    }

    void ClearAllObjects() {
        LegacyWorld::ClearAllObjects();
        g_generation++;
    }

    uint64_t GetGeneration() {
        return g_generation;
    }
}
