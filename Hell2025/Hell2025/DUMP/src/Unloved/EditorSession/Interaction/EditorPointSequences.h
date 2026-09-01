#pragma once

#include <cstdint>

#include <glm/glm.hpp>

namespace Unloved::EditorSession::PointSequences {

    enum class PointHandleType {
        ANCHOR,
        CONTROL,
    };

    bool UpdateInput(bool allowInput);
    void Draw();
    void CancelInteraction();
    void CommitInteraction();

    int32_t GetPointCount(uint64_t objectId);
    int32_t InsertPoint(uint64_t objectId, int32_t segmentStartIndex);
    bool RemovePoint(uint64_t objectId, int32_t pointIndex);
    bool GetPointPosition(uint64_t objectId, int32_t pointIndex, PointHandleType handleType, glm::vec3& position);
    bool SetPointPosition(uint64_t objectId, int32_t pointIndex, PointHandleType handleType, const glm::vec3& position);
}
