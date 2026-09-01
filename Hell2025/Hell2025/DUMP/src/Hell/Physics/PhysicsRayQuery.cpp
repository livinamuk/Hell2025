#include "Physics.h"

#include <glm/geometric.hpp>
#include <utility>

namespace Hell::Physics {
    PxQueryHitType::Enum RaycastFilterCallback::preFilter(const PxFilterData& filterData, const PxShape* shape, const PxRigidActor* actor, PxHitFlags& /*queryFlags*/) {
        const PxFilterData sf = shape->getQueryFilterData();

        for (const PxRigidActor* pxRigidActor : m_ignoredActors) {
            if (actor == pxRigidActor) {
                return PxQueryHitType::eNONE;
            }
        }

        if ((sf.word0 & filterData.word0) == 0) {
            return PxQueryHitType::eNONE;
        }

        return PxQueryHitType::eBLOCK;
    }

    PxQueryHitType::Enum RaycastFilterCallback::postFilter(const PxFilterData& /*filterData*/, const PxQueryHit& /*hit*/, const PxShape* /*shape*/, const PxRigidActor* /*actor*/) {
        return PxQueryHitType::eBLOCK;
    }

    void RaycastFilterCallback::AddIgnoredActor(PxRigidDynamic* pxRigidDynamic) {
        m_ignoredActors.push_back(pxRigidDynamic);
    }

    void RaycastFilterCallback::AddIgnoredActors(std::vector<PxRigidDynamic*> pxRigidDynamics) {
        m_ignoredActors.reserve(m_ignoredActors.size() + pxRigidDynamics.size());
        for (PxRigidDynamic* pxRigidDynamic : pxRigidDynamics) {
            if (pxRigidDynamic) {
                m_ignoredActors.push_back(pxRigidDynamic);
            }
        }
    }

    PxQueryHitType::Enum RaycastHeightFieldFilterCallback::preFilter(const PxFilterData& /*filterData*/, const PxShape* shape, const PxRigidActor* /*actor*/, PxHitFlags& /*queryFlags*/) {
        const PxGeometryHolder geomHolder = shape->getGeometry();
        if (geomHolder.getType() != PxGeometryType::eHEIGHTFIELD) {
            return PxQueryHitType::eNONE;
        }
        return PxQueryHitType::eBLOCK;
    }

    PxQueryHitType::Enum RaycastHeightFieldFilterCallback::postFilter(const PxFilterData& /*filterData*/, const PxQueryHit& /*hit*/, const PxShape* /*shape*/, const PxRigidActor* /*actor*/) {
        return PxQueryHitType::eBLOCK;
    }

    PxQueryHitType::Enum RaycastStaticEnviromentFilterCallback::preFilter(const PxFilterData& /*filterData*/, const PxShape* shape, const PxRigidActor* /*actor*/, PxHitFlags& /*queryFlags*/) {
        const PxGeometryHolder geomHolder = shape->getGeometry();
        if (geomHolder.getType() != PxGeometryType::eHEIGHTFIELD && geomHolder.getType() != PxGeometryType::eTRIANGLEMESH) {
            return PxQueryHitType::eNONE;
        }
        return PxQueryHitType::eBLOCK;
    }

    PxQueryHitType::Enum RaycastStaticEnviromentFilterCallback::postFilter(const PxFilterData& /*filterData*/, const PxQueryHit& /*hit*/, const PxShape* /*shape*/, const PxRigidActor* /*actor*/) {
        return PxQueryHitType::eBLOCK;
    }

    PhysXRayResult CastPhysXRayStaticEnvironment(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, float rayLength) {
        PxScene* scene = Hell::Physics::GetPxScene();
        PxVec3 origin = PxVec3(rayOrigin.x, rayOrigin.y, rayOrigin.z);
        PxVec3 unitDir = PxVec3(rayDirection.x, rayDirection.y, rayDirection.z);
        PxReal maxDistance = rayLength;
        PxRaycastBuffer hit;
        PxHitFlags outputFlags = PxHitFlag::ePOSITION | PxHitFlag::eNORMAL | PxHitFlag::eMESH_BOTH_SIDES;

        PxQueryFilterData filterData = PxQueryFilterData();
        filterData.flags = PxQueryFlag::eSTATIC | PxQueryFlag::eDYNAMIC | PxQueryFlag::ePREFILTER;

        PhysXRayResult result;
        result.hitObjectName = "NO_USERDATA";
        result.hitPosition = glm::vec3(0, 0, 0);
        result.hitNormal = glm::vec3(0, 0, 0);
        result.rayDirection = rayDirection;
        result.userData = PhysicsUserData();
        result.distanceToHit = rayLength;

        RaycastStaticEnviromentFilterCallback callback;
        result.hitFound = scene->raycast(origin, unitDir, maxDistance, hit, outputFlags, filterData, &callback);

        if (result.hitFound) {
            result.hitPosition = glm::vec3(hit.block.position.x, hit.block.position.y, hit.block.position.z);
            result.hitNormal = glm::vec3(hit.block.normal.x, hit.block.normal.y, hit.block.normal.z);
            result.hitFound = true;
            PhysicsUserData* userData = (PhysicsUserData*)hit.block.actor->userData;
            if (userData) {
                result.userData = *userData;
                result.hitObjectName = "HAS_USERDATA";
            }
        }

        return result;
    }

    PhysXRayResult CastPhysXRayHeightMap(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, float rayLength) {
        PxScene* scene = Hell::Physics::GetPxScene();
        PxVec3 origin = PxVec3(rayOrigin.x, rayOrigin.y, rayOrigin.z);
        PxVec3 unitDir = PxVec3(rayDirection.x, rayDirection.y, rayDirection.z);
        PxReal maxDistance = rayLength;
        PxRaycastBuffer hit;
        PxHitFlags outputFlags = PxHitFlag::ePOSITION | PxHitFlag::eNORMAL | PxHitFlag::eMESH_BOTH_SIDES;

        PxQueryFilterData filterData = PxQueryFilterData();
        filterData.flags = PxQueryFlag::eSTATIC | PxQueryFlag::eDYNAMIC | PxQueryFlag::ePREFILTER;

        PhysXRayResult result;
        result.hitObjectName = "NO_USERDATA";
        result.hitPosition = glm::vec3(0, 0, 0);
        result.hitNormal = glm::vec3(0, 0, 0);
        result.rayDirection = rayDirection;
        result.userData = PhysicsUserData();

        RaycastHeightFieldFilterCallback callback;
        result.hitFound = scene->raycast(origin, unitDir, maxDistance, hit, outputFlags, filterData, &callback);

        if (result.hitFound) {
            result.hitPosition = glm::vec3(hit.block.position.x, hit.block.position.y, hit.block.position.z);
            result.hitNormal = glm::vec3(hit.block.normal.x, hit.block.normal.y, hit.block.normal.z);
            result.hitFound = true;
            result.distanceToHit = glm::distance(rayOrigin, result.hitPosition);
            PhysicsUserData* userData = (PhysicsUserData*)hit.block.actor->userData;
            if (userData) {
                result.userData = *userData;
                result.hitObjectName = "HAS_USERDATA";
            }
        }

        return result;
    }

    PhysXRayResult CastPhysXRay(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, float rayLength, bool cullBackFacing, std::vector<PxRigidActor*> ignoredActors) {
        PxScene* scene = Hell::Physics::GetPxScene();
        PxVec3 origin = PxVec3(rayOrigin.x, rayOrigin.y, rayOrigin.z);
        PxVec3 unitDir = PxVec3(rayDirection.x, rayDirection.y, rayDirection.z);
        PxReal maxDistance = rayLength;
        PxRaycastBuffer hit;
        PxHitFlags outputFlags = PxHitFlag::ePOSITION | PxHitFlag::eNORMAL;

        if (!cullBackFacing) {
            outputFlags |= PxHitFlag::eMESH_BOTH_SIDES;
        }

        PxQueryFilterData filterData = PxQueryFilterData();
        filterData.data.word0 = 0xFFFFFFFF;
        filterData.data.word1 = 0xFFFFFFFF;
        filterData.data.word2 = 0;
        filterData.flags = PxQueryFlag::eSTATIC | PxQueryFlag::eDYNAMIC | PxQueryFlag::ePREFILTER;

        PhysXRayResult result;
        result.hitObjectName = "NO_USERDATA";
        result.hitPosition = glm::vec3(0, 0, 0);
        result.hitNormal = glm::vec3(0, 0, 0);
        result.rayDirection = rayDirection;
        result.userData = PhysicsUserData();

        RaycastFilterCallback callback;
        callback.m_ignoredActors = std::move(ignoredActors);

        result.hitFound = scene->raycast(origin, unitDir, maxDistance, hit, outputFlags, filterData, &callback);

        if (result.hitFound) {
            result.hitPosition = glm::vec3(hit.block.position.x, hit.block.position.y, hit.block.position.z);
            result.hitNormal = glm::vec3(hit.block.normal.x, hit.block.normal.y, hit.block.normal.z);
            result.distanceToHit = glm::distance(rayOrigin, result.hitPosition);
            result.hitFound = true;
            PhysicsUserData* userData = (PhysicsUserData*)hit.block.actor->userData;
            if (userData) {
                result.userData = *userData;
                result.hitObjectName = "HAS_USERDATA";
            }
        }
        return result;
    }
}
