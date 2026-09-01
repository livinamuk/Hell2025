#include "Map.h"

#include "Unloved/Systems/Map/MapManager.h"

namespace Unloved {

    uint32_t Map::GetChunkCountX() {
        MapData* mapData = MapManager::GetMapDataByIndex(m_mapIndex);
        if (!mapData) return 0;
        else return mapData->GetChunkCountX();
    }

    uint32_t Map::GetChunkCountZ() {
        MapData* mapData = MapManager::GetMapDataByIndex(m_mapIndex);
        if (!mapData) return 0;
        else return mapData->GetChunkCountZ();
    }

}
