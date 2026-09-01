#include "Physics.h"

#include <vector>

namespace {
    PhysicsUserData* GetPhysicsUserData(PxActor* actor) {
        return actor ? static_cast<PhysicsUserData*>(actor->userData) : nullptr;
    }
}

void ContactReportCallback::onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs) {
    if (!pairHeader.actors[0] || !pairHeader.actors[1]) return;

    PhysicsUserData* userDataA = GetPhysicsUserData(pairHeader.actors[0]);
    PhysicsUserData* userDataB = GetPhysicsUserData(pairHeader.actors[1]);

    for (PxU32 pairIndex = 0; pairIndex < nbPairs; pairIndex++) {
        const PxContactPair& pair = pairs[pairIndex];
        if (pair.contactCount == 0) continue;

        std::vector<PxContactPairPoint> contactPoints(pair.contactCount);
        const PxU32 contactCount = pair.extractContacts(contactPoints.data(), pair.contactCount);
        if (contactCount == 0) continue;

        const PxContactPairPoint* strongestContact = &contactPoints[0];
        float strongestImpulse = strongestContact->impulse.magnitudeSquared();

        for (PxU32 contactIndex = 1; contactIndex < contactCount; contactIndex++) {
            const float impulse = contactPoints[contactIndex].impulse.magnitudeSquared();
            if (impulse > strongestImpulse) {
                strongestContact = &contactPoints[contactIndex];
                strongestImpulse = impulse;
            }
        }

        CollisionReport report;
        report.rigidA = pairHeader.actors[0];
        report.rigidB = pairHeader.actors[1];
        report.physicsIdA = userDataA ? userDataA->physicsId : 0;
        report.physicsIdB = userDataB ? userDataB->physicsId : 0;
        report.collisionGroupA = pair.shapes[0] ? pair.shapes[0]->getSimulationFilterData().word1 : static_cast<PxU32>(CollisionGroup::NO_COLLISION);
        report.collisionGroupB = pair.shapes[1] ? pair.shapes[1]->getSimulationFilterData().word1 : static_cast<PxU32>(CollisionGroup::NO_COLLISION);
        report.hitPosition = glm::vec3(strongestContact->position.x, strongestContact->position.y, strongestContact->position.z);
        report.hitNormal = glm::vec3(strongestContact->normal.x, strongestContact->normal.y, strongestContact->normal.z);
        Hell::Physics::AddCollisionReport(report);
    }
}

namespace Hell::Physics {
    PhysicsContactResult GetContactResult(uint64_t physicsId, CollisionGroup collisionGroup) {
        PhysicsContactResult result;
        if (physicsId == 0) return result;

        const PxU32 collisionGroupMask = static_cast<PxU32>(collisionGroup);

        for (const CollisionReport& report : GetCollisionReports()) {
            if (report.physicsIdA == physicsId && (report.collisionGroupB & collisionGroupMask)) {
                result.hitFound = true;
                result.hitPosition = report.hitPosition;
                result.hitNormal = report.hitNormal;
                return result;
            }

            if (report.physicsIdB == physicsId && (report.collisionGroupA & collisionGroupMask)) {
                result.hitFound = true;
                result.hitPosition = report.hitPosition;
                result.hitNormal = -report.hitNormal;
                return result;
            }
        }

        return result;
    }
}
