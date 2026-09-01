#pragma once

#include "Unloved/EditorSession/EditorSessionTypes.h"

#include <cstdint>

namespace Unloved::EditorSession::PointSequences {

    enum class PointHandleType;
}

namespace Unloved::EditorSession::Selection {

    void Reset();
    void Update(bool allowInput);
    void SelectWorkspace();
    void SelectObject(uint64_t objectId);
    void SelectPoint(uint64_t objectId, int32_t pointIndex);
    void SelectPoint(uint64_t objectId, int32_t pointIndex, PointSequences::PointHandleType handleType);
    void SelectWallSegment(uint64_t objectId, int32_t segmentIndex);
    void SetMode(EditorSelectionMode mode);
    bool AddPoint();
    bool DeleteSelected();
    void ClearSelection();

    uint64_t GetHoveredObjectId();
    uint64_t GetSelectedObjectId();
    EditorSelectionMode GetMode();
    int32_t GetSelectedPointIndex();
    PointSequences::PointHandleType GetSelectedPointHandleType();
    int32_t GetSelectedWallSegmentIndex();
    bool HasSelectedPoint();
    bool HasSelectedWallSegment();
    bool HasWorkspaceSelection();
    bool HasObjectSelection();
    bool HasSelection();
    bool ShouldOutlineObject(uint64_t objectId);
}
