#pragma once

#include "Unloved/EditorSession/EditorSessionTypes.h"

#include <cstdint>
#include <string>

namespace Unloved {
    struct Wall;
}

namespace Unloved::EditorSession::Inspector::Internal {

    void ApplyWeatherBoardMaterialDefaults(Unloved::Wall* wall, const std::string& materialName);
    void RenderPlanarQuadProperties(const EditorRect& rect, uint64_t objectId);
    void RenderPointPairProperties(const EditorRect& rect, uint64_t objectId);
}
