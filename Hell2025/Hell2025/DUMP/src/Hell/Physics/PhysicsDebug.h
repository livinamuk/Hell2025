#pragma once

#include "Hell/Physics/PhysicsTypes.h"

#include <string>
#include <vector>

namespace Hell::Physics {
    std::vector<PhysicsDebugLine> GetPhysicsDebugLines(DebugMode debugMode);
    std::string GetObjectCountsAsString();
    void PrintSceneInfo();
    void PrintSceneRigidInfo();
    void PrintSceneRagdollInfo();
    void DebugDrawRigidDynamicStateAABBs();
}
