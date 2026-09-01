#pragma once

#include "Hell/Physics/PhysicsTypes.h"

namespace Hell::Physics {
    PhysicsContactResult GetContactResult(uint64_t physicsId, CollisionGroup collisionGroup);
}
