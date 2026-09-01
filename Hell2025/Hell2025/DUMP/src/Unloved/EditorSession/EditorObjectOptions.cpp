#include "EditorObjectOptions.h"

#include "Hell/File/File.h"

#include "Unloved/ObjectId.h"

#include <algorithm>

namespace Unloved::EditorSession::ObjectOptions {

    const std::vector<std::string>& GetInteriorMaterials() {
        static const std::vector<std::string> materials = {
            "Ceiling2",
            "WallPaper",
            "PlasterRed",
            "BathroomWall",
            "FloorBoards",
            "BathroomFloor",
            "WeatherBoards_Bare"
        };
        return materials;
    }

    const std::vector<std::string>& GetHouseNames() {
        static std::vector<std::string> houseNames;
        if (!houseNames.empty()) return houseNames;

        for (const FileInfo& fileInfo : Hell::File::IterateDirectory("res/houses", { "house" })) {
            houseNames.push_back(fileInfo.name);
        }

        std::sort(houseNames.begin(), houseNames.end());
        return houseNames;
    }

    const std::vector<std::string>& GetWeatherBoardMaterials() {
        static std::vector<std::string> materialNames;

        if (materialNames.empty()) {
            const std::vector<WeatherBoardMaterialSettings>& allSettings = GetWeatherBoardMaterialSettings();
            materialNames.reserve(allSettings.size());

            for (const WeatherBoardMaterialSettings& materialSettings : allSettings) {
                materialNames.push_back(materialSettings.materialName);
            }
        }

        return materialNames;
    }

    const std::vector<WeatherBoardMaterialSettings>& GetWeatherBoardMaterialSettings() {
        static const std::vector<WeatherBoardMaterialSettings> settings = {
            // Name                     Board count  Start board  End board  Offset U  Offset V
            { "WeatherBoards0",         16,          0,           15,        0.0f,     0.0f },
            { "WeatherBoards1",         16,          0,           15,        0.0f,     0.0f },
            { "WeatherBoards_Greenish", 26,          0,           25,        0.0f,     0.025f },
            { "WeatherBoards_Bare",     26,          0,           25,        0.0f,     0.025f }
        };
        return settings;
    }

    const WeatherBoardMaterialSettings* GetWeatherBoardMaterialSettings(const std::string& materialName) {
        for (const WeatherBoardMaterialSettings& materialSettings : GetWeatherBoardMaterialSettings()) {
            if (materialSettings.materialName == materialName) return &materialSettings;
        }

        return nullptr;
    }

    EditorObjectMode GetEditorMode(uint64_t objectId) {
        const ObjectType objectType = GetObjectIdType(objectId);
        switch (objectType) {
            case ObjectType::DDGI_VOLUME:        return EditorObjectMode::VERTEX;
            case ObjectType::CHRISTMAS_LIGHTS:
            case ObjectType::FENCE:
            case ObjectType::POWER_POLE_SET:
            case ObjectType::WALL:
            case ObjectType::WORLD_PLANE:
            case ObjectType::PLANAR_QUAD_OBJECT:
            case ObjectType::LADDER:
            case ObjectType::POINT_PAIR_OBJECT:  return EditorObjectMode::VERTEX_AND_OBJECT;
            default:                             return EditorObjectMode::OBJECT;
        }
    }
}
