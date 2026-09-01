#pragma once

#include "Hell/Physics/PhysicsTypes.h"

#pragma warning(push, 0)
#include <physx/PxPhysicsAPI.h>
#pragma warning(pop)

using namespace physx;

namespace Hell::Physics {
    PhysXOverlapReport OverlapTest(const PxGeometry& overlapShape, const PxTransform& shapePose, PxU32 collisionGroup);
}
