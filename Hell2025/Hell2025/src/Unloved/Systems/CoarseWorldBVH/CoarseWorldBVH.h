#pragma once

#include "Hell/BVH/Types.h"

#include <glm/glm.hpp>
#include <cstdint>

namespace Unloved::CoarseWorldBVH {
    void Init();
    void ClearScenes();
    void CleanUp();
    void Rebuild();
    void Update();

    uint64_t GetDoorProxyBvhId();

    bool AnyHitWithoutDoors(glm::vec3 pointA, glm::vec3 pointB);
    bool AnyHitWithDoors(glm::vec3 pointA, glm::vec3 pointB);

    BvhRayResult ClosestHitWithoutDoors(glm::vec3 rayOrigin, glm::vec3 rayDir, float maxRayDistance);
    BvhRayResult ClosestHitWithDoors(glm::vec3 rayOrigin, glm::vec3 rayDir, float maxRayDistance);
}
