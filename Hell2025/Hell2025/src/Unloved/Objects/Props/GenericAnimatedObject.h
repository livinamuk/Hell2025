#pragma once

#include "Unloved/Characters/Humanoid/AnimatedHumanoid.h"
#include "Unloved/Common/CreateInfo.h"

namespace Unloved {

struct GenericAnimatedObject {
    GenericAnimatedObject() = default;
    GenericAnimatedObject(uint64_t id, const GenericAnimatedObjectCreateInfo& createInfo, const SpawnOffset& spawnOffset);
    GenericAnimatedObject(const GenericAnimatedObject&) = delete;
    GenericAnimatedObject& operator=(const GenericAnimatedObject&) = delete;
    GenericAnimatedObject(GenericAnimatedObject&&) noexcept = default;
    GenericAnimatedObject& operator=(GenericAnimatedObject&&) noexcept = default;
    ~GenericAnimatedObject() = default;

    void DebugDraw();
    void CleanUp();

    void SetPosition(const glm::vec3& position);
    void SetRotation(const glm::vec3& rotation);
    void SetScale(float scale);
    void SetType(GenericAnimatedObjectType type);
    void SetCrouchBlend(float crouchBlend);
    void SetMovementBlend(float movementBlend);
    void SetWeaponAnimation(Bible::AnimationSlot animationSlot);
    void SetDebugDraw(bool debugDraw);
    void SetDebugDrawEjectionPort(bool debugDrawEjectionPort);

    uint64_t GetObjectId() const { return m_objectId; }
    float GetCrouchBlend() const { return m_animatedHumanoid.GetCrouchBlend(); }
    float GetMovementBlend() const { return m_animatedHumanoid.GetMovementBlend(); }
    Bible::AnimationSlot GetWeaponAnimationSlot() const { return m_animatedHumanoid.GetWeaponAnimationSlot(); }
    bool GetDebugDraw() const { return m_animatedHumanoid.GetDebugDraw(); }
    bool GetDebugDrawEjectionPort() const { return m_animatedHumanoid.GetDebugDrawEjectionPort(); }
    const glm::vec3& GetPosition() const { return m_createInfo.position; }
    const glm::vec3& GetRotation() const { return m_createInfo.rotation; }
    float GetScale() const { return m_createInfo.scale; }
    GenericAnimatedObjectType GetType() const { return m_createInfo.type; }
    const std::string& GetEditorName() const { return m_createInfo.editorName; }
    const GenericAnimatedObjectCreateInfo& GetCreateInfo() const { return m_createInfo; }

private:
    void CreateAnimatedHumanoid();

    GenericAnimatedObjectCreateInfo m_createInfo;
    uint64_t m_objectId = 0;
    AnimatedHumanoid m_animatedHumanoid;
};

}
