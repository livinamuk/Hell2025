#include "BloodPoolState.h"

#include "Hell/Logging.h"
#include "Hell/Physics/Physics.h"
#include "Hell/Physics/PhysicsResourceManagement.h"

#include "Unloved/Systems/Blood/BloodSystem.h"

void BloodPoolState::Reset() {
    m_awaitingSpawn = true;
}

void BloodPoolState::Configure(uint64_t ragdollId, const std::string& boneName) {
    m_boneName = boneName;
    m_ragdollId = ragdollId;
}

void BloodPoolState::Update() {
    if (m_awaitingSpawn) {

        //Logging::Debug() << "BloodPoolState::Update()\n";

        Ragdoll* ragdoll = Hell::Physics::GetRagdollById(m_ragdollId);
        if (!ragdoll) {
            Logging::Error() << "BloodPoolState::Update() fucked up coz ragdoll Id '" << m_ragdollId << "' returned nullptr\n";
            return;
        }

        uint64_t physicsId = ragdoll->GetPhysicsIdByBoneName(m_boneName);
        PhysicsContactResult result = Hell::Physics::GetContactResult(physicsId, CollisionGroup::ENVIROMENT_OBSTACLE);

        if (result.hitFound) {

            //Logging::Debug() << m_boneName << " hit the ground\n";

 //           Unloved::BloodSystem::SpawnBloodPoolDecal(result.hitPosition, result.hitNormal);


        }
    }
    else {

        //Logging::Debug() << "BloodPoolState::Update() returned coz m_awaitingSpawn was false\n";
    }
}