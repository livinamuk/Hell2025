#include "RagdollJoint.h"

#include "Hell/Math/Matrix.h"
#include "RagdollAsset.h"

#include <glm/gtc/quaternion.hpp>
#include <glm/mat3x3.hpp>
#include <glm/matrix.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace {
    struct AngularBasisCandidate {
        std::array<RagdollAxisLimit, 3> limits;
        glm::quat frameAdjustment = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        uint32_t layoutPenalty = std::numeric_limits<uint32_t>::max();
        float rotationCost = std::numeric_limits<float>::max();
    };

    bool IsMovable(const RagdollAxisLimit& limit) {
        return limit.motion != RagdollAxisMotion::LOCKED;
    }

    uint32_t GetLayoutPenalty(const std::array<RagdollAxisLimit, 3>& limits) {
        uint32_t movableAxisCount = 0;
        for (const RagdollAxisLimit& limit : limits) {
            movableAxisCount += IsMovable(limit) ? 1U : 0U;
        }

        // PhysX's canonical angular layouts alternate between twist and the
        // swing plane as degrees of freedom are added: none, twist, swings,
        // then all three. Axis permutations let us select that layout without
        // changing the authored world-space directions.
        const bool canonicalTwistIsMovable = (movableAxisCount & 1U) != 0;
        return IsMovable(limits[0]) == canonicalTwistIsMovable ? 0U : 1U;
    }

    AngularBasisCandidate FindClosestPhysicsBasis(const std::array<RagdollAxisLimit, 3>& authoredLimits) {
        constexpr std::array<glm::vec3, 3> AXES = {
            glm::vec3(1.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, 1.0f)
        };
        constexpr float COST_EPSILON = 1e-6f;

        AngularBasisCandidate bestCandidate;
        std::array<size_t, 3> sourceAxes = { 0, 1, 2 };
        do {
            for (uint32_t signMask = 0; signMask < 8; signMask++) {
                glm::mat3 basis(1.0f);
                AngularBasisCandidate candidate;

                for (size_t destinationAxis = 0; destinationAxis < sourceAxes.size(); destinationAxis++) {
                    const size_t sourceAxis = sourceAxes[destinationAxis];
                    const float direction = (signMask & (1U << destinationAxis)) == 0 ? 1.0f : -1.0f;
                    basis[destinationAxis] = AXES[sourceAxis] * direction;
                    candidate.limits[destinationAxis] = authoredLimits[sourceAxis];
                }

                // Reflections cannot be represented by a quaternion. Axis
                // signs do not affect the current symmetric angular limits.
                if (glm::determinant(basis) < 0.0f) continue;

                candidate.frameAdjustment = glm::normalize(glm::quat_cast(basis));
                candidate.layoutPenalty = GetLayoutPenalty(candidate.limits);
                candidate.rotationCost = 1.0f - std::abs(candidate.frameAdjustment.w);

                const bool hasBetterLayout = candidate.layoutPenalty < bestCandidate.layoutPenalty;
                const bool hasSmallerRotation =
                    candidate.layoutPenalty == bestCandidate.layoutPenalty &&
                    candidate.rotationCost + COST_EPSILON < bestCandidate.rotationCost;
                if (hasBetterLayout || hasSmallerRotation) {
                    bestCandidate = candidate;
                }
            }
        } while (std::next_permutation(sourceAxes.begin(), sourceAxes.end()));

        return bestCandidate;
    }

    void RotateFrame(glm::mat4& frame, const glm::quat& adjustment) {
        frame = Hell::Math::RemoveScaleAndShear(frame);
        const glm::quat rotation = glm::normalize(Hell::Math::ExtractRotation(frame) * adjustment);
        Hell::Math::SetRotationPreserveTranslation(frame, rotation);
    }
}

namespace RagdollJoint {

    RagdollJointAsset CreatePhysicsReadyCopy(const RagdollJointAsset& authoredJoint) {
        RagdollJointAsset joint = authoredJoint;
        if (!joint.limitEnabled) return joint;

        const AngularBasisCandidate physicsBasis = FindClosestPhysicsBasis(joint.angularLimits);
        joint.angularLimits = physicsBasis.limits;
        if (physicsBasis.rotationCost > 1e-6f) {
            RotateFrame(joint.parentFrame, physicsBasis.frameAdjustment);
            RotateFrame(joint.childFrame, physicsBasis.frameAdjustment);
        }
        return joint;
    }
}
