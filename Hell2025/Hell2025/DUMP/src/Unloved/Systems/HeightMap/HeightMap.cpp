#include "HeightMap.h"

#include "Hell/Logging.h"
#include "Unloved/Common/Constants.h"
#include "Unloved/Maps/Map.h"
#include "Unloved/Maps/MapData.h"
#include "Unloved/Systems/Map/MapManager.h"
#include "Unloved/World/World.h"

#include <algorithm>
#include <cstddef>

namespace Unloved::HeightMap {

    namespace {
        std::vector<float> g_worldHeightData;
        std::vector<uint32_t> g_worldTerrainControlData;
        uint32_t g_worldTextureWidth = 0;
        uint32_t g_worldTextureHeight = 0;
    }

    void BuildWorldHeightData(uint32_t chunkCountX, uint32_t chunkCountZ) {
        g_worldTextureWidth = chunkCountX * HEIGHT_MAP_CHUNK_PIXEL_SIZE + 1;
        g_worldTextureHeight = chunkCountZ * HEIGHT_MAP_CHUNK_PIXEL_SIZE + 1;
        g_worldHeightData.assign(static_cast<size_t>(g_worldTextureWidth) * static_cast<size_t>(g_worldTextureHeight), 0.0f);
        g_worldTerrainControlData.assign(static_cast<size_t>(g_worldTextureWidth) * static_cast<size_t>(g_worldTextureHeight), TerrainControl::DEFAULT_VALUE);

        for (const Map& map : World::GetMaps()) {
            const MapData* mapData = MapManager::GetMapDataByIndex(map.m_mapIndex);
            if (!mapData) continue;

            const int32_t offsetX = map.spawnOffsetChunkX * HEIGHT_MAP_CHUNK_PIXEL_SIZE;
            const int32_t offsetZ = map.spawnOffsetChunkZ * HEIGHT_MAP_CHUNK_PIXEL_SIZE;
            const uint32_t sourceWidth = mapData->GetTextureWidth();
            const uint32_t sourceHeight = mapData->GetTextureHeight();
            const std::vector<float>& sourceData = mapData->GetHeightMapData();
            const std::vector<uint32_t>& sourceControlData = mapData->GetTerrainControlData();
            const size_t sourcePixelCount = static_cast<size_t>(sourceWidth) * static_cast<size_t>(sourceHeight);

            if (sourceData.size() < sourcePixelCount) {
                Logging::Error() << "HeightMap::BuildWorldHeightData() failed coz map '" << mapData->GetFilename() << "' has " << sourceData.size() << " height values but needs " << sourcePixelCount;
                continue;
            }
            if (sourceControlData.size() < sourcePixelCount) {
                Logging::Error() << "HeightMap::BuildWorldHeightData() failed coz map '" << mapData->GetFilename() << "' has " << sourceControlData.size() << " terrain control values but needs " << sourcePixelCount;
                continue;
            }
            if (offsetX < 0 || offsetZ < 0 || static_cast<uint32_t>(offsetX) + sourceWidth > g_worldTextureWidth || static_cast<uint32_t>(offsetZ) + sourceHeight > g_worldTextureHeight) {
                Logging::Error() << "HeightMap::BuildWorldHeightData() failed coz map '" << mapData->GetFilename() << "' is outside the world height map";
                continue;
            }

            for (uint32_t row = 0; row < sourceHeight; row++) {
                const size_t sourceOffset = static_cast<size_t>(row) * sourceWidth;
                const size_t destinationOffset = static_cast<size_t>(offsetZ + row) * g_worldTextureWidth + offsetX;
                std::copy_n(sourceData.data() + sourceOffset, sourceWidth, g_worldHeightData.data() + destinationOffset);
                std::copy_n(sourceControlData.data() + sourceOffset, sourceWidth, g_worldTerrainControlData.data() + destinationOffset);
            }
        }

        // Fill the extra world edge used by the chunk meshes
        if (g_worldTextureWidth > 1 && g_worldTextureHeight > 1) {
            for (uint32_t row = 0; row < g_worldTextureHeight - 1; row++) {
                const size_t rowOffset = static_cast<size_t>(row) * g_worldTextureWidth;
                g_worldHeightData[rowOffset + g_worldTextureWidth - 1] = g_worldHeightData[rowOffset + g_worldTextureWidth - 2];
                g_worldTerrainControlData[rowOffset + g_worldTextureWidth - 1] = g_worldTerrainControlData[rowOffset + g_worldTextureWidth - 2];
            }

            const size_t sourceRowOffset = static_cast<size_t>(g_worldTextureHeight - 2) * g_worldTextureWidth;
            const size_t destinationRowOffset = static_cast<size_t>(g_worldTextureHeight - 1) * g_worldTextureWidth;
            std::copy_n(g_worldHeightData.data() + sourceRowOffset, g_worldTextureWidth, g_worldHeightData.data() + destinationRowOffset);
            std::copy_n(g_worldTerrainControlData.data() + sourceRowOffset, g_worldTextureWidth, g_worldTerrainControlData.data() + destinationRowOffset);
        }
    }

    void Clear() {
        g_worldHeightData.clear();
        g_worldTerrainControlData.clear();
        g_worldTextureWidth = 0;
        g_worldTextureHeight = 0;
    }

    const std::vector<float>& GetWorldHeightData() {
        return g_worldHeightData;
    }

    const std::vector<uint32_t>& GetWorldTerrainControlData() {
        return g_worldTerrainControlData;
    }

    uint32_t GetWorldTextureWidth() {
        return g_worldTextureWidth;
    }

    uint32_t GetWorldTextureHeight() {
        return g_worldTextureHeight;
    }
}
