#pragma once

#include "Unloved/Render/RendererTypes.h"

#include <cstdint>
#include <vector>

enum class DirtyBoundsType : uint8_t {
    DYNAMIC,
    STATIC,
};

struct DirtyBounds {
    uint64_t objectId = 0;
    glm::vec3 boundsMin = glm::vec3(0.0f);
    glm::vec3 boundsMax = glm::vec3(0.0f);
    bool castShadows = true;
    DirtyBoundsType type = DirtyBoundsType::DYNAMIC;
};

namespace Unloved::DirtyTracker {
    void BeginFrame();
    void Update();

    void AddDirtyBounds(const DirtyBounds& dirtyBounds);

    const std::vector<GPUAABB>& GetDirtyDoorAABBs();

    const std::vector<uint64_t>& GetDirtyDoorIds();
    const std::vector<uint64_t>& GetStaticDirtyLightIds();
    const std::vector<uint64_t>& GetCompositeDirtyLightIds();
    uint8_t GetStaticDirtyLightFaceMask(uint64_t lightId);
    uint8_t GetCompositeDirtyLightFaceMask(uint64_t lightId);

}
