#include "Player.h"

#include "Hell/Logging.h"

namespace Unloved {

void Player::InitRagdoll() {
    SkinnedGameObject* characterModel = GetCharacterModelSkinnedGameObject();
    if (!characterModel) return;

    characterModel->SetOwnerObjectId(m_playerId);
    characterModel->SetPosition(GetFootPosition());
    characterModel->SetRotationX(0.0f);
    characterModel->SetRotationY(m_camera.GetEulerRotation().y + HELL_PI);
    characterModel->SetRotationZ(0.0f);

    PhysicsFilterData filterData;
    filterData.raycastGroup = RaycastGroup::RAYCAST_ENABLED;
    filterData.collisionGroup = CollisionGroup::RAGDOLL_PLAYER;
    filterData.collidesWith = CollisionGroup::ENVIROMENT_OBSTACLE;

    characterModel->CreateRagdoll("UnisexGuy", filterData);
}

} // namespace Unloved
