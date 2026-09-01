#pragma once

struct RagdollJointAsset;

namespace RagdollJoint {

    // Leaves authored data untouched and returns the equivalent PhysX-ready joint.
    RagdollJointAsset CreatePhysicsReadyCopy(const RagdollJointAsset& authoredJoint);
}
