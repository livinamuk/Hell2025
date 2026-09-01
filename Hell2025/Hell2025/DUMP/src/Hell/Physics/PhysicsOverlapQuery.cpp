#include "Physics.h"

namespace Hell::Physics {
    PhysXOverlapReport OverlapTest(const PxGeometry& overlapShape, const PxTransform& shapePose, PxU32 collisionGroup) {
        PxScene* pxScene = Hell::Physics::GetPxScene();

        PxQueryFilterData overlapFilterData = PxQueryFilterData();
        overlapFilterData.data.word1 = collisionGroup;

        const PxU32 bufferSize = 256;
        PxOverlapHit hitBuffer[bufferSize];
        PxOverlapBuffer buf(hitBuffer, bufferSize);

        std::vector<PxActor*> hitActors;

        if (pxScene->overlap(overlapShape, shapePose, buf, overlapFilterData)) {
            for (int i = 0; i < buf.getNbTouches(); i++) {
                PxActor* hit = buf.getTouch(i).actor;
                bool found = false;
                for (const PxActor* foundHit : hitActors) {
                    if (foundHit == hit) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    hitActors.push_back(hit);
                }
            }
        }

        PhysXOverlapReport overlapReport;
        for (PxActor* hitActor : hitActors) {
            PhysicsUserData* userData = (PhysicsUserData*)hitActor->userData;
            if (userData) {
                if (userData->physicsType == PhysicsType::RIGID_DYNAMIC) {
                    PxRigidDynamic* rigid = (PxRigidDynamic*)hitActor;
                    PhysXOverlapResult& overlapResult = overlapReport.hits.emplace_back();
                    overlapResult.userData = *userData;
                    overlapResult.objectPosition.x = rigid->getGlobalPose().p.x;
                    overlapResult.objectPosition.y = rigid->getGlobalPose().p.y;
                    overlapResult.objectPosition.z = rigid->getGlobalPose().p.z;
                }
                if (userData->physicsType == PhysicsType::RIGID_STATIC ||
                    userData->physicsType == PhysicsType::HEIGHT_FIELD) {
                    PxRigidStatic* rigid = (PxRigidStatic*)hitActor;
                    PhysXOverlapResult& overlapResult = overlapReport.hits.emplace_back();
                    overlapResult.userData = *userData;
                    overlapResult.objectPosition.x = rigid->getGlobalPose().p.x;
                    overlapResult.objectPosition.y = rigid->getGlobalPose().p.y;
                    overlapResult.objectPosition.z = rigid->getGlobalPose().p.z;
                }
            }
        }
        return overlapReport;
    }
}
