#pragma once

#include "Unloved/Common/SequencePoint.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <vector>

namespace Unloved {

struct WireCreateInfo {
    std::vector<SequencePoint> sequencePoints;
    float spacing = 0.25f;
    float radius = 0.1f;
    uint64_t parentObjectId = 0;
};

struct Wire {
    Wire() = default;
    Wire(uint64_t id, const WireCreateInfo& createInfo);

    void Init(const WireCreateInfo& createInfo);
    void Update();
    void SubmitRenderItem();
    void CleanUp();

    uint32_t GetMeshId() const                              { return m_meshId; }
    uint64_t GetObjectId() const                            { return m_objectId; }
    uint64_t GetParentObjectId() const                      { return m_createInfo.parentObjectId; }
    const std::vector<glm::vec3>& GetSegmentPoints() const  { return m_segmentPoints; }

private:
    uint64_t m_objectId = 0;
    WireCreateInfo m_createInfo;
    std::vector<glm::vec3> m_segmentPoints;
    uint32_t m_meshId = 0;
};
}
