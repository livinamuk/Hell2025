#pragma once

#include "Hell/Physics/PhysicsTypes.h"

#pragma warning(push, 0)
#include <physx/PxPhysicsAPI.h>
#include <physx/PxQueryFiltering.h>
#pragma warning(pop)

#include <glm/vec3.hpp>
#include <vector>

using namespace physx;

namespace Hell::Physics {
    struct RaycastFilterCallback : PxQueryFilterCallback {
        PxQueryHitType::Enum preFilter(const PxFilterData& filterData, const PxShape* shape, const PxRigidActor* actor, PxHitFlags& /*queryFlags*/) override;
        PxQueryHitType::Enum postFilter(const PxFilterData& /*filterData*/, const PxQueryHit& /*hit*/, const PxShape* /*shape*/, const PxRigidActor* /*actor*/) override;

        std::vector<PxRigidActor*> m_ignoredActors;
        void AddIgnoredActor(PxRigidDynamic* pxRigidDynamic);
        void AddIgnoredActors(std::vector<PxRigidDynamic*> pxRigidDynamics);
    };

    struct RaycastHeightFieldFilterCallback : PxQueryFilterCallback {
        PxQueryHitType::Enum preFilter(const PxFilterData& /*filterData*/, const PxShape* shape, const PxRigidActor* /*actor*/, PxHitFlags& /*queryFlags*/) override;
        PxQueryHitType::Enum postFilter(const PxFilterData& /*filterData*/, const PxQueryHit& /*hit*/, const PxShape* /*shape*/, const PxRigidActor* /*actor*/) override;
    };

    struct RaycastStaticEnviromentFilterCallback : PxQueryFilterCallback {
        PxQueryHitType::Enum preFilter(const PxFilterData& /*filterData*/, const PxShape* shape, const PxRigidActor* /*actor*/, PxHitFlags& /*queryFlags*/) override;
        PxQueryHitType::Enum postFilter(const PxFilterData& /*filterData*/, const PxQueryHit& /*hit*/, const PxShape* /*shape*/, const PxRigidActor* /*actor*/) override;
    };

    PhysXRayResult CastPhysXRayStaticEnvironment(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, float rayLength);
    PhysXRayResult CastPhysXRayHeightMap(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, float rayLength);
    PhysXRayResult CastPhysXRay(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, float rayLength, bool cullBackFacing = false, std::vector<PxRigidActor*> ignoredActors = std::vector<PxRigidActor*>());
}
