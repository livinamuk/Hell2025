#include "HouseManager.h"
#include <fstream>
#include "Hell/Logging.h"
#include "File/JSON.h"
#include "Unloved/World/World.h"

#include <vector>

namespace Unloved::HouseManager {

    std::vector<HouseData> g_houseData;

    void Init() {
        g_houseData.clear();
    }

    bool NewHouse(const std::string& filename) {
        for (HouseData& houseData : g_houseData) {
            if (houseData.GetFilename() == filename) return false;
        }

        HouseData& houseData = g_houseData.emplace_back();
        houseData.SetFilename(filename);
        return true;
    }

    void LoadHouseData(const std::string& filename) {
        for (HouseData& houseData : g_houseData) {
            if (houseData.GetFilename() == filename) return;
        }

        ReloadHouseData(filename);
    }

    bool ReloadHouseData(const std::string& filename) {
        const std::string path = "res/houses/" + filename + ".house";
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            Logging::Error() << "HouseManager::LoadHouseData(): failed to open '" << path;
            return false;
        }

        nlohmann::json json;
        if (!JSON::LoadJsonFromFile(json, path)) {
            Logging::Error() << "HouseManager::LoadHouseData() failed to open file: " << path;
            return false;
        }

        CreateInfoCollection createInfoCollection = JSON::CreateInfoCollectionFromJSONObject(json);

        for (size_t i = 0; i < createInfoCollection.genericObjects.size();) {
            if (createInfoCollection.genericObjects[i].type == GenericObjectType::UNDEFINED) {
                createInfoCollection.genericObjects.erase(createInfoCollection.genericObjects.begin() + i);
                Logging::Error() << "Found UNDEFINED GenericGameObject in " << filename << " and removed it";
            }
            else {
                ++i;
            }
        }

        HouseData* houseData = nullptr;
        for (HouseData& existingHouseData : g_houseData) {
            if (existingHouseData.GetFilename() == filename) houseData = &existingHouseData;
        }
        if (!houseData) houseData = &g_houseData.emplace_back();
        houseData->SetFilename(filename);
        houseData->SetCreateInfoCollection(createInfoCollection);
        return true;
    }
    
    void SaveHouse(const std::string& filename) {
        HouseData* houseData = GetHouseDataByName(filename);
        if (!houseData) {
            Logging::Error() << "HouseManager::SaveHouse(): failed because '" << filename << "' was not found.";
            return;
        }

        // Construct the JSON string
        CreateInfoCollection createInfoCollection = World::GetCreateInfoCollection();
        houseData->SetCreateInfoCollection(createInfoCollection);

        std::string createInfoJson = JSON::CreateInfoCollectionToJSON(createInfoCollection);

        // Create the file
        std::string outputPath = "res/houses/" + filename + ".house";
        std::ofstream file(outputPath, std::ios::binary | std::ios::trunc);
        if (!file.is_open()) {
            Logging::Error() << "Failed to open file for writing: " << outputPath << "\n";
            return;
        }

        // Quick n dirty dump of the string to file
        file.write(createInfoJson.data(), static_cast<std::streamsize>(createInfoJson.size()));

        Logging::Debug() 
            << "Saved: " << outputPath
            << "\n" << createInfoJson
            << "";
    }

    void UpdateCreateInfoCollectionFromWorld(const std::string& houseName) {
        HouseData* houseData = GetHouseDataByName(houseName);
        if (!houseData) {
            Logging::Error() << "HouseManager::UpdateCreateInfoCollectionFromWorld(): failed because '" << houseName << "' was not found.";
            return;
        }

        CreateInfoCollection createInfoCollection = World::GetCreateInfoCollection();
        houseData->SetCreateInfoCollection(createInfoCollection);
    }

    HouseData* GetHouseDataByName(const std::string& filename) {
        for (HouseData& houseData : g_houseData) {
            if (houseData.GetFilename() == filename) {
                return &houseData;
            }
        }
        Logging::Error() << "HouseManager::GetHouseDataByName() failed coz '" << filename << "' was not found";
        return nullptr;
    }
}
