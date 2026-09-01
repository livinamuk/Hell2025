#pragma once

#include <cstdint>
#include <string>

namespace Unloved {

    struct MapCreateInfo {
        std::string mapName;
        uint32_t spawnOffsetChunkX = 0;
        uint32_t spawnOffsetChunkZ = 0;
    };

    struct Map {
        uint32_t m_mapIndex = 0;
        int32_t spawnOffsetChunkX = 0;
        int32_t spawnOffsetChunkZ = 0;

        uint32_t GetChunkCountX();
        uint32_t GetChunkCountZ();
    };

}
