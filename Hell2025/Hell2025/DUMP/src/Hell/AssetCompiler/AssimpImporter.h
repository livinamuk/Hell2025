#pragma once

#include "Hell/AssetFormats/AssetData.h"

#include <string>
#include <unordered_set>

namespace Hell::AssetCompiler {

    ModelData ImportModel(const std::string& path);
    ModelData ImportVatCarrierModel(const std::string& path);
    SkinnedModelData ImportSkinnedModel(const std::string& path, const std::unordered_set<std::string>& requestedMorphTargets = {});
}
