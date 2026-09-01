#include "MapData.h"

#include "Hell/Logging.h"

#include <algorithm>
#include <utility>

namespace Unloved {

    void MapData::CreateNew(const std::string& filename, int chunkCountX, int chunkCountZ, float initialHeight) {
        m_filename = filename;
        m_chunkCountX = chunkCountX;
        m_chunkCountZ = chunkCountZ;

        ClearToHeight(initialHeight);
        Logging::Debug() << "Created map: '" << filename << "' with height map size " << GetTextureWidth() << "x" << GetTextureHeight();
    }

    void MapData::ClearToHeight(float height) {
        const size_t pixelCount = static_cast<size_t>(GetTextureWidth()) * static_cast<size_t>(GetTextureHeight());
        m_heightMapData.assign(pixelCount, height);
        if (m_terrainControlData.size() != pixelCount) ClearTerrainControl();
    }

    void MapData::ClearTerrainControl() {
        m_terrainControlData.assign(static_cast<size_t>(GetTextureWidth()) * static_cast<size_t>(GetTextureHeight()), TerrainControl::DEFAULT_VALUE);
    }

    bool MapData::Resize(int32_t chunkCountX, int32_t chunkCountZ) {
        if (chunkCountX < 1 || chunkCountZ < 1) return false;

        const int32_t oldWidth = GetTextureWidth();
        const int32_t oldHeight = GetTextureHeight();
        const int32_t newWidth = chunkCountX * HEIGHT_MAP_CHUNK_PIXEL_SIZE;
        const int32_t newHeight = chunkCountZ * HEIGHT_MAP_CHUNK_PIXEL_SIZE;
        const size_t oldPixelCount = static_cast<size_t>(oldWidth) * oldHeight;
        if (m_heightMapData.size() != oldPixelCount || m_terrainControlData.size() != oldPixelCount) return false;

        std::vector<float> resizedData(static_cast<size_t>(newWidth) * newHeight, DEFAULT_HEIGHT);
        std::vector<uint32_t> resizedControlData(static_cast<size_t>(newWidth) * newHeight, TerrainControl::DEFAULT_VALUE);
        const int32_t copyWidth = std::min(oldWidth, newWidth);
        const int32_t copyHeight = std::min(oldHeight, newHeight);
        for (int32_t row = 0; row < copyHeight; row++) std::copy_n(m_heightMapData.data() + static_cast<size_t>(row) * oldWidth, copyWidth, resizedData.data() + static_cast<size_t>(row) * newWidth);
        for (int32_t row = 0; row < copyHeight; row++) std::copy_n(m_terrainControlData.data() + static_cast<size_t>(row) * oldWidth, copyWidth, resizedControlData.data() + static_cast<size_t>(row) * newWidth);

        m_chunkCountX = chunkCountX;
        m_chunkCountZ = chunkCountZ;
        m_heightMapData = std::move(resizedData);
        m_terrainControlData = std::move(resizedControlData);
        return true;
    }

    void MapData::SetFilename(const std::string& filename) {
        m_filename = filename;
    }

    void MapData::SetHeightMapData(int32_t chunkCountX, int32_t chunkCountZ, const std::vector<float>& data) {
        m_chunkCountX = chunkCountX;
        m_chunkCountZ = chunkCountZ;
        m_heightMapData = data;
        if (m_terrainControlData.size() != data.size()) ClearTerrainControl();
    }

    void MapData::SetTerrainControlData(const std::vector<uint32_t>& data) {
        m_terrainControlData = data;
    }

    void MapData::SetCreateInfoCollection(const CreateInfoCollection& createInfoCollection) {
        m_createInfoCollection = createInfoCollection;
    }

    void MapData::SetAdditionalMapData(const AdditionalMapData& additionalMapData) {
        m_additionalMapData = additionalMapData;
    }

    const glm::ivec2 MapData::GetHeightMapTextureSize() {
        return glm::ivec2(GetTextureWidth(), GetTextureHeight());
    }


    void MapData::AddPlayerCampaignSpawn(glm::vec3 position) {
        SpawnPointCreateInfo& spawnPoint = m_createInfoCollection.spawnPointsCampaign.emplace_back();
        spawnPoint.position = position;
        spawnPoint.rotation = glm::vec2(0.0f);
    }

    void MapData::AddPlayerDeathmatchSpawn(glm::vec3 position) {
        SpawnPointCreateInfo& spawnPoint = m_createInfoCollection.spawnPointsDeathMatch.emplace_back();
        spawnPoint.position = position;
        spawnPoint.rotation = glm::vec2(0.0f);
    }

}
