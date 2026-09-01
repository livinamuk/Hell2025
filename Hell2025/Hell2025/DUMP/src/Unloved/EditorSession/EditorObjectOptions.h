#pragma once

#include "EditorSessionTypes.h"

#include <cstdint>
#include <string>
#include <vector>

namespace Unloved::EditorSession::ObjectOptions {
    struct WeatherBoardMaterialSettings {
        std::string materialName;
        uint32_t boardCount = 16;
        uint32_t startIndex = 0;
        uint32_t endIndex = 15;
        float textureOffsetU = 0.0f;
        float textureOffsetV = 0.0f;
    };

    const std::vector<std::string>& GetInteriorMaterials();
    const std::vector<std::string>& GetHouseNames();
    const std::vector<std::string>& GetWeatherBoardMaterials();
    const std::vector<WeatherBoardMaterialSettings>& GetWeatherBoardMaterialSettings();
    const WeatherBoardMaterialSettings* GetWeatherBoardMaterialSettings(const std::string& materialName);
    EditorObjectMode GetEditorMode(uint64_t objectId);
}
