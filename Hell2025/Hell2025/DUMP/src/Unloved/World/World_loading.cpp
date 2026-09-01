#include "World.h"

#include "Legacy/World/LegacyWorld.h"
#include "Hell/Common/Constants.h"
#include "Hell/Common/Random.h"
#include "Hell/File/File.h"
#include "Hell/Logging.h"

#include "Unloved/Common/Constants.h"
#include "Unloved/Maps/Map.h"
#include "Unloved/Maps/MapData.h"
#include "Unloved/Objects/House/HouseData.h"
#include "Unloved/Systems/House/HouseManager.h"
#include "Unloved/Systems/Map/MapManager.h"

#include <algorithm>

namespace {
    void MarkHouseAsLoaded(std::vector<std::string>& loadedHouseNames, const std::string& houseName) {
        if (std::find(loadedHouseNames.begin(), loadedHouseNames.end(), houseName) == loadedHouseNames.end()) loadedHouseNames.push_back(houseName);
    }

    std::string GetRandomHouseName(std::vector<std::string>& loadedHouseNames) {
        std::vector<std::string> houseNames;
        for (const FileInfo& houseFile : Hell::File::IterateDirectory("res/houses", { "house" })) {
            if (std::find(loadedHouseNames.begin(), loadedHouseNames.end(), houseFile.name) == loadedHouseNames.end()) houseNames.push_back(houseFile.name);
        }

        // Everything has been used so start another cycle
        if (houseNames.empty() && !loadedHouseNames.empty()) {
            loadedHouseNames.clear();
            for (const FileInfo& houseFile : Hell::File::IterateDirectory("res/houses", { "house" })) houseNames.push_back(houseFile.name);
        }

        if (houseNames.empty()) return "";

        const int32_t index = Hell::Random::Int(0, static_cast<int32_t>(houseNames.size()) - 1);
        return houseNames[index];
    }

    void LoadMapHousesFromLocations(const Unloved::MapData& mapData, SpawnOffset spawnOffset, bool randomHouses, std::vector<std::string>& loadedHouseNames) {
        for (const HouseLocationCreateInfo& houseLocation : mapData.GetAdditionalMapData().houseLocations) {
            if (houseLocation.randomHouse != randomHouses) continue;
            const std::string houseName = randomHouses ? GetRandomHouseName(loadedHouseNames) : houseLocation.houseName;

            if (houseName.empty() || houseName == UNDEFINED_STRING) {
                Logging::Error() << "World::LoadMap() failed because no house was selected";
                continue;
            }

            SpawnOffset houseSpawnOffset = spawnOffset;
            houseSpawnOffset.translation += houseLocation.position;
            houseSpawnOffset.yRotation += houseLocation.rotation;

            Unloved::World::LoadHouse(houseName, houseSpawnOffset);
            if (Unloved::HouseManager::GetHouseDataByName(houseName)) MarkHouseAsLoaded(loadedHouseNames, houseName);
        }
    }
}

namespace Unloved::World {
    void LoadMap(const std::string& mapName) {
        MapCreateInfo createInfo;
        createInfo.mapName = mapName;
        LoadMaps({ createInfo });
    }

    void LoadMaps(const std::vector<MapCreateInfo>& mapCreateInfoSet) {
        LegacyWorld::LoadMapsHeightMapData(mapCreateInfoSet);
        std::vector<std::string> loadedHouseNames;

        // Load map objects and fixed houses first
        for (const MapCreateInfo& mapCreateInfo : mapCreateInfoSet) {
            SpawnOffset spawnOffset;
            spawnOffset.translation.x = mapCreateInfo.spawnOffsetChunkX * HEIGHT_MAP_CHUNK_WORLD_SPACE_SIZE;
            spawnOffset.translation.z = mapCreateInfo.spawnOffsetChunkZ * HEIGHT_MAP_CHUNK_WORLD_SPACE_SIZE;

            MapData* mapData = MapManager::GetMapDataByName(mapCreateInfo.mapName);
            if (!mapData) {
                Logging::Error() << "World::LoadMaps() failed coz '" << mapCreateInfo.mapName << "' was not found";
                continue;
            }

            LoadMapObjects(*mapData, spawnOffset);
            LoadMapHousesFromLocations(*mapData, spawnOffset, false, loadedHouseNames);
        }

        // Random houses cannot take names used above
        for (const MapCreateInfo& mapCreateInfo : mapCreateInfoSet) {
            SpawnOffset spawnOffset;
            spawnOffset.translation.x = mapCreateInfo.spawnOffsetChunkX * HEIGHT_MAP_CHUNK_WORLD_SPACE_SIZE;
            spawnOffset.translation.z = mapCreateInfo.spawnOffsetChunkZ * HEIGHT_MAP_CHUNK_WORLD_SPACE_SIZE;

            MapData* mapData = MapManager::GetMapDataByName(mapCreateInfo.mapName);
            if (!mapData) continue;

            LoadMapHousesFromLocations(*mapData, spawnOffset, true, loadedHouseNames);
        }
    }

    void LoadMap(const MapData& mapData, SpawnOffset spawnOffset) {
        std::vector<std::string> loadedHouseNames;
        LoadMapObjects(mapData, spawnOffset);
        LoadMapHousesFromLocations(mapData, spawnOffset, false, loadedHouseNames);
        LoadMapHousesFromLocations(mapData, spawnOffset, true, loadedHouseNames);
    }

    void LoadMapObjects(const MapData& mapData, SpawnOffset spawnOffset) {
        AddCreateInfoCollection(mapData.GetCreateInfoCollection(), spawnOffset);
        for (const HouseLocationCreateInfo& createInfo : mapData.GetAdditionalMapData().houseLocations) AddHouseLocation(createInfo, spawnOffset);
    }

    void LoadSingleHouse(const std::string& houseName) {
        ResetWorld();
        LoadHouse(houseName, SpawnOffset());
    }

    void LoadHouse(const std::string& houseName, SpawnOffset spawnOffset) {
        HouseManager::LoadHouseData(houseName);
        HouseData* houseData = HouseManager::GetHouseDataByName(houseName);
        if (!houseData) {
            Logging::Error() << "World::LoadHouse() failed because " << houseName << " was not found";
            return;
        }

        LoadHouse(*houseData, spawnOffset);
    }

    void LoadHouse(const HouseData& houseData, SpawnOffset spawnOffset) {
        AddCreateInfoCollection(houseData.GetCreateInfoCollection(), spawnOffset);

        Logging::Debug() << "World::LoadHouse(): " << houseData.GetFilename() << " at " << spawnOffset.translation;
    }
}
