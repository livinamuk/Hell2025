#pragma once

#include <cstdint>
#include <map>
#include <string>

struct BoneMask {
    uint32_t version = 1;
    std::string name;
    std::string skinnedModelName;
    std::map<std::string, float> weights;
};
