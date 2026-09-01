#pragma once

#include <cstdint>

namespace Unloved::EditorSession::Visibility {

    bool Hide(uint64_t objectId);
    bool UnhideAll();
    void Clear();
    bool ShouldHide(uint64_t objectId);
}
