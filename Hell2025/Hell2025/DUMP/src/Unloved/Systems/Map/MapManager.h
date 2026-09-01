#pragma once

#include "Unloved/Maps/MapData.h"

#include <string>
#include <vector>

namespace Unloved::MapManager {
    void Init();
    bool NewMap(const std::string& name, int chunkWidth, int chunkDepth, float initialHeight);
    void SaveMap(const std::string& mapName);
    void LoadMapData(const std::string& mapName);
    bool ReloadMapData(const std::string& mapName);
    void UpdateCreateInfoCollectionFromWorld(const std::string& mapName);

    MapData* GetTestMapData();
    MapData* GetMapDataByIndex(int32_t index);
    MapData* GetMapDataByName(const std::string& name);
    int32_t GetMapDataIndexByName(const std::string& name);
}
