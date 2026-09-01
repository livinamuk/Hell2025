#pragma once

#include <cstdint>

namespace Unloved::EditorSession::Hierarchy {

    extern bool g_expandHierachyOnLoad;

    void Init();
    void Refresh();
    void SetAllNodesExpanded(bool expanded);
    void RefreshRagdollMarkers();
    void RefreshObjectChildren(uint64_t objectId);
    void RemoveObject(uint64_t objectId);
    void Update(bool allowInput);
    void Render();

    bool WantsMouseCapture();
}
