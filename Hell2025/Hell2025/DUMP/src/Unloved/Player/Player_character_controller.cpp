#include "Player.h"

#include "Hell/Physics/Physics.h"

namespace Unloved {

void Player::CreateCharacterController(const glm::vec3& position) {

    float capsuleHeight = PLAYER_CAPSULE_HEIGHT;
    capsuleHeight = m_viewHeightStanding - PLAYER_CAPSULE_RADIUS - PLAYER_CAPSULE_RADIUS;

    float capsuleRadius = PLAYER_CAPSULE_RADIUS;

    PhysicsFilterData physicsFilterData;
    physicsFilterData.raycastGroup = RaycastGroup::RAYCAST_ENABLED;
    physicsFilterData.collisionGroup = CollisionGroup::CHARACTER_CONTROLLER;
    physicsFilterData.collidesWith = CollisionGroup(ENVIROMENT_OBSTACLE | CHARACTER_CONTROLLER);

    m_characterControllerId = Hell::Physics::CreateCharacterController(m_playerId, position, capsuleHeight, capsuleRadius, physicsFilterData);
}

void Player::SetFootPosition(glm::vec3 position) {
    CharacterController* characterControler = Hell::Physics::GetCharacterControllerById(m_characterControllerId);
    if (characterControler) {
        PxController* pxControler = characterControler->GetPxController();
        PxExtendedVec3 pxVec3 = PxExtendedVec3(position.x, position.y, position.z);
        pxControler->setFootPosition(pxVec3);
    }
}

PxShape* Player::GetCharacterControllerShape() const {

    CharacterController* characterControler = Hell::Physics::GetCharacterControllerById(m_characterControllerId);
    if (characterControler) {
        PxController* pxControler = characterControler->GetPxController();
        if (!pxControler) {
            return nullptr;
        }

        PxShape* shape = nullptr;
        return pxControler->getActor()->getShapes(&shape, 1) == 1 ? shape : nullptr;
    } 
    else {
        return nullptr;
    }
}

PxRigidDynamic* Player::GetCharacterControllerActor() const {
    CharacterController* characterControler = Hell::Physics::GetCharacterControllerById(m_characterControllerId);
    if (characterControler) {
        PxController* pxControler = characterControler->GetPxController();
        return pxControler ? pxControler->getActor() : nullptr;
    }
    else {
        return nullptr;
    }
}

} // namespace Unloved
