#pragma once

#include "PlacementTools.h"

namespace Unloved::EditorSession::Placement {

    void Begin(PlacementTool tool);
    void Update(bool allowKeyboardInput, bool allowMouseInput);
    void Cancel();

    bool IsActive();
}
