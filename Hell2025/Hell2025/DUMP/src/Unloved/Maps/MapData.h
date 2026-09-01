#pragma once

#include "Unloved/Common/Constants.h"
#include "Unloved/Common/CreateInfo.h"

#include <cstdint>
#include <string>
#include <vector>

namespace Unloved {

    struct TerrainControl {
        static constexpr uint32_t MATERIAL_MASK = 0x1F;
        static constexpr uint32_t BLEND_MASK = 0xFF;
        static constexpr uint32_t BASE_MATERIAL_SHIFT = 27;
        static constexpr uint32_t OVERLAY_MATERIAL_SHIFT = 22;
        static constexpr uint32_t BLEND_SHIFT = 14;
        static constexpr uint32_t AUTO_SHADER_MASK = 1;
        static constexpr uint32_t DEFAULT_VALUE = AUTO_SHADER_MASK;

        static constexpr uint32_t Encode(uint8_t baseMaterial, uint8_t overlayMaterial, uint8_t blend, bool useAutoShader) { return ((static_cast<uint32_t>(baseMaterial) & MATERIAL_MASK) << BASE_MATERIAL_SHIFT) | ((static_cast<uint32_t>(overlayMaterial) & MATERIAL_MASK) << OVERLAY_MATERIAL_SHIFT) | ((static_cast<uint32_t>(blend) & BLEND_MASK) << BLEND_SHIFT) | (useAutoShader ? AUTO_SHADER_MASK : 0); }
        static constexpr uint8_t GetBaseMaterial(uint32_t value) { return static_cast<uint8_t>((value >> BASE_MATERIAL_SHIFT) & MATERIAL_MASK); }
        static constexpr uint8_t GetOverlayMaterial(uint32_t value) { return static_cast<uint8_t>((value >> OVERLAY_MATERIAL_SHIFT) & MATERIAL_MASK); }
        static constexpr uint8_t GetBlend(uint32_t value) { return static_cast<uint8_t>((value >> BLEND_SHIFT) & BLEND_MASK); }
        static constexpr bool UsesAutoShader(uint32_t value) { return (value & AUTO_SHADER_MASK) != 0; }
    };

    struct MapData {
        static constexpr float DEFAULT_HEIGHT = 31.0f;

        void CreateNew(const std::string& filename, int chunkCountX, int chunkCountZ, float initialHeight);
        void ClearToHeight(float height);
        void ClearTerrainControl();
        bool Resize(int32_t chunkCountX, int32_t chunkCountZ);
        void SetFilename(const std::string& filename);
        void SetHeightMapData(int32_t chunkCountX, int32_t chunkCountZ, const std::vector<float>& data);
        void SetTerrainControlData(const std::vector<uint32_t>& data);
        void SetCreateInfoCollection(const CreateInfoCollection& createInfoCollection);
        void SetAdditionalMapData(const AdditionalMapData& additionalMapData);
        void AddPlayerDeathmatchSpawn(glm::vec3 position);
        void AddPlayerCampaignSpawn(glm::vec3 position);

        const glm::ivec2 GetHeightMapTextureSize();

        const std::string& GetFilename() const { return m_filename; }
        int32_t GetChunkCountX() const { return m_chunkCountX; }
        int32_t GetChunkCountZ() const { return m_chunkCountZ; }
        int32_t GetTextureWidth() const { return m_chunkCountX * HEIGHT_MAP_CHUNK_PIXEL_SIZE; }
        int32_t GetTextureHeight() const { return m_chunkCountZ * HEIGHT_MAP_CHUNK_PIXEL_SIZE; }
        AdditionalMapData& GetAdditionalMapData() { return m_additionalMapData; }
        const AdditionalMapData& GetAdditionalMapData() const { return m_additionalMapData; }
        CreateInfoCollection& GetCreateInfoCollection() { return m_createInfoCollection; }
        const CreateInfoCollection& GetCreateInfoCollection() const { return m_createInfoCollection; }
        std::vector<float>& GetHeightMapData() { return m_heightMapData; }
        const std::vector<float>& GetHeightMapData() const { return m_heightMapData; }
        std::vector<uint32_t>& GetTerrainControlData() { return m_terrainControlData; }
        const std::vector<uint32_t>& GetTerrainControlData() const { return m_terrainControlData; }

    private:
        std::string m_filename;
        int32_t m_chunkCountX = 8;
        int32_t m_chunkCountZ = 8;
        std::vector<float> m_heightMapData; // World-space metres, row-major z * width + x
        std::vector<uint32_t> m_terrainControlData; // Terrain3D-compatible packed control values, row-major z * width + x
        CreateInfoCollection m_createInfoCollection;
        AdditionalMapData m_additionalMapData;
    };

}
