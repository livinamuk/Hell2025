#include "GenericAnimatedObject.h"

namespace Unloved {

GenericAnimatedObject::GenericAnimatedObject(uint64_t id, const GenericAnimatedObjectCreateInfo& createInfo, const SpawnOffset& spawnOffset) {
    m_objectId = id;
    m_createInfo = createInfo;
    m_createInfo.position += spawnOffset.translation;
    m_createInfo.rotation.y += spawnOffset.yRotation;
    CreateAnimatedHumanoid();
}

void GenericAnimatedObject::DebugDraw() {
    m_animatedHumanoid.DebugDraw();
}

void GenericAnimatedObject::CleanUp() {
    m_animatedHumanoid.CleanUp();
}

void GenericAnimatedObject::SetPosition(const glm::vec3& position) {
    m_createInfo.position = position;
    m_animatedHumanoid.SetPosition(position);
}

void GenericAnimatedObject::SetRotation(const glm::vec3& rotation) {
    m_createInfo.rotation = rotation;
    m_animatedHumanoid.SetRotation(rotation);
}

void GenericAnimatedObject::SetScale(float scale) {
    m_createInfo.scale = scale;
    m_animatedHumanoid.SetScale(scale);
}

void GenericAnimatedObject::SetType(GenericAnimatedObjectType type) {
    if (m_createInfo.type == type) return;

    m_createInfo.type = type;
    m_animatedHumanoid.CleanUp();
    CreateAnimatedHumanoid();
}

void GenericAnimatedObject::SetCrouchBlend(float crouchBlend) {
    m_animatedHumanoid.SetCrouchBlend(crouchBlend);
}

void GenericAnimatedObject::SetMovementBlend(float movementBlend) {
    m_animatedHumanoid.SetMovementBlend(movementBlend);
}

void GenericAnimatedObject::SetWeaponAnimation(Bible::AnimationSlot animationSlot) {
    m_animatedHumanoid.SetWeaponAnimation(animationSlot);
}

void GenericAnimatedObject::SetDebugDraw(bool debugDraw) {
    m_animatedHumanoid.SetDebugDraw(debugDraw);
}

void GenericAnimatedObject::SetDebugDrawEjectionPort(bool debugDrawEjectionPort) {
    m_animatedHumanoid.SetDebugDrawEjectionPort(debugDrawEjectionPort);
}

void GenericAnimatedObject::CreateAnimatedHumanoid() {
    if (m_createInfo.type != GenericAnimatedObjectType::RAT_KING) return;

    if (m_animatedHumanoid.Init(m_objectId, Bible::SkinnedModelPreset::RAT_KING, m_createInfo.position, m_createInfo.rotation, m_createInfo.scale)) {
        m_animatedHumanoid.CreateRagdoll("RatKing");
        m_animatedHumanoid.SetWeapon(Bible::Weapon::GLOCK);
    }
}

}
