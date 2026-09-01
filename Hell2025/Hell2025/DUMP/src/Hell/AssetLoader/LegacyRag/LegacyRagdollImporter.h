#pragma once

#include <string>
#include <vector>

struct RagdollAsset;

namespace Hell::AssetLoader {

    bool ImportLegacyRagdollAsset(
        const std::string& path,
        RagdollAsset& asset,
        std::vector<std::string>& warnings,
        std::string& error
    );
}
