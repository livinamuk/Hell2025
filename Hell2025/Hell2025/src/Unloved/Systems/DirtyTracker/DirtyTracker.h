#pragma once

#include "Unloved/Render/RendererTypes.h"

#include <vector>

struct DirtyBounds {
    uint64_t objectId = 0;
    glm::vec3 boundsMin = glm::vec3(0.0f);
    glm::vec3 boundsMax = glm::vec3(0.0f);
    bool castShadows = true;
};

namespace Unloved::DirtyTracker {
    void BeginFrame();
    void Update();

    //void AddDirtyBounds(const DirtyBounds& dirtyBounds);

    void AddStaticDirtyBounds(const DirtyBounds& dirtyBounds);
    void AddDynamicDirtyBounds(const DirtyBounds& dirtyBounds);

    const std::vector<GPUAABB>& GetDirtyDoorAABBs();

    const std::vector<uint64_t>& GetDirtyDoorIds();

    const std::vector<uint64_t>& GetStaticDirtyLightIds();
    const std::vector<uint64_t>& GetDynamicDirtyLightIds();

}
