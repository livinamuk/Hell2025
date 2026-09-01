#pragma once

#include "Hell/AssetFormats/AssetData.h"

#include <string>

namespace Hell::AssetCompiler {

    bool ExportObj(const std::string& path, const MeshData& mesh);
    bool ExportObj(const std::string& path, const SkinnedMeshData& mesh);
}
