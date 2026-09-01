#pragma once

#include "Viewport.h"

#include <cstdint>
#include <vector>

namespace Unloved::ViewportManager {
    void Init();
    void Update();
    uint32_t GetActiveViewportMask();
    Unloved::Viewport* GetViewportByIndex(int32_t viewportIndex);
    std::vector<Unloved::Viewport>& GetViewports();
}
