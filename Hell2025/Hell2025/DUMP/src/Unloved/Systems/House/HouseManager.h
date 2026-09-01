#pragma once
//#include "Unloved/Common/CreateInfo.h"
#include "Unloved/Objects/House/HouseData.h"

#include <string>

namespace Unloved::HouseManager {

    void Init();
    bool NewHouse(const std::string& filename);
    void LoadHouseData(const std::string& filename);
    bool ReloadHouseData(const std::string& filename);
    void SaveHouse(const std::string& filename);
    void UpdateCreateInfoCollectionFromWorld(const std::string& houseName);

    HouseData* GetHouseDataByName(const std::string& filename);
}
