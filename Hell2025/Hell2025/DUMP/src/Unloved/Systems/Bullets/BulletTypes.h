#pragma once

#include "glm/vec3.hpp"

#include <cstdint>

struct BulletCreateInfo {
    glm::vec3 origin = glm::vec3(0);
    glm::vec3 direction = glm::vec3(0);
    int32_t weaponIndex = 0;
    uint32_t damage = 0;
    uint64_t ownerObjectId = 0;
    uint64_t hitGroupId = 0;
    float rayLength = 1000.0f;
    float impactImpulse = 8.0f;
    bool createsDecals = true;
    bool createsFollowThroughBulletOnGlassHit = true;
    bool playsPiano = true;
    bool createsDecalTexturePaintedWounds = true;
};
