#pragma once

#include "Unloved/Bible/Bible_enums.h"

#include <glm/vec3.hpp>

#include <cstdint>
#include <string>

struct Ragdoll;

namespace Unloved::Bible {
    struct HumanoidInfo;
}

namespace Unloved {

struct AnimatorInstance;
struct SkinnedGameObject;

struct AnimatedHumanoid {
    AnimatedHumanoid() = default;
    AnimatedHumanoid(const AnimatedHumanoid&) = delete;
    AnimatedHumanoid& operator=(const AnimatedHumanoid&) = delete;
    AnimatedHumanoid(AnimatedHumanoid&&) noexcept = default;
    AnimatedHumanoid& operator=(AnimatedHumanoid&&) noexcept = default;
    ~AnimatedHumanoid() = default;

    bool Init(uint64_t parentObjectId, Bible::SkinnedModelPreset bodySkinnedModelPreset, const glm::vec3& position, const glm::vec3& rotation, float scale = 1.0f);
    void DebugDraw();
    void CleanUp();
    void CreateRagdoll(const std::string& ragdollName);

    void SetPosition(const glm::vec3& position);
    void SetRotation(const glm::vec3& rotation);
    void SetScale(float scale);
    void SetCrouchBlend(float crouchBlend);
    void SetMovementBlend(float movementBlend);
    void SetJumpBlend(float jumpBlend);
    void SetWeapon(Bible::Weapon weapon);
    void SetWeaponAnimation(Bible::AnimationSlot animationSlot);
    void SetDebugDraw(bool debugDraw);
    void SetDebugDrawEjectionPort(bool debugDrawEjectionPort);
    void SetIgnoredViewportIndex(int32_t viewportIndex);

    void SetBodyAnimationModeToAnimated();
    void SetBodyAnimationModeToRagdoll();
    void EnableWeaponRendering();
    void DisableWeaponRendering();
    void PlayJumpAnimation();
    void PlayCharacterWeaponAnimation(Bible::AnimationSlot animationSlot);
    void PlayAndLoopCharacterWeaponAnimation(Bible::AnimationSlot animationSlot);

    bool IsJumpAnimationComplete();
    uint64_t GetRagdollId();
    Ragdoll* GetRagdoll();
    AnimatorInstance* GetAnimatorInstance();
    SkinnedGameObject* GetBodySkinnedGameObject();
    SkinnedGameObject* GetWeaponSkinnedGameObject();

    uint64_t GetBodySkinnedGameObjectId() const { return m_bodySkinnedGameObjectId; }
    float GetCrouchBlend() const { return m_crouchBlend; }
    float GetMovementBlend() const { return m_movementBlend; }
    float GetJumpBlend() const { return m_jumpBlend; }
    Bible::AnimationSlot GetWeaponAnimationSlot() const { return m_weaponAnimationSlot; }
    bool GetDebugDraw() const { return m_debugDraw; }
    bool GetDebugDrawEjectionPort() const { return m_debugDrawEjectionPort; }

private:
    struct LocomotionAnimationIndices {
        uint32_t layer = UINT32_MAX;
        uint32_t standingIdle = UINT32_MAX;
        uint32_t standingMoving = UINT32_MAX;
        uint32_t crouchingIdle = UINT32_MAX;
        uint32_t crouchingMoving = UINT32_MAX;

        bool IsValid() const;
    };

    bool ConfigureAnimator(const Bible::HumanoidInfo& humanoidInfo, Bible::AnimationProfile animationProfile, const std::string& weaponSkinnedModelName);
    void UpdateLocomotionAnimationWeights();
    void UpdateJumpAnimationWeights();
    void UpdateWeaponAnimation();

    Bible::SkinnedModelPreset m_bodySkinnedModelPreset = Bible::SkinnedModelPreset::UNDEFINED;
    Bible::Weapon m_weapon = Bible::Weapon::UNDEFINED;
    Bible::AnimationProfile m_animationProfile = Bible::AnimationProfile::UNDEFINED;
    uint64_t m_bodySkinnedGameObjectId = 0;
    uint64_t m_weaponSkinnedGameObjectId = 0;
    uint64_t m_animatorInstanceId = 0;
    LocomotionAnimationIndices m_lowerBodyLocomotionAnimationIndices;
    LocomotionAnimationIndices m_chestLocomotionAnimationIndices;
    uint32_t m_jumpAnimationLayerIndex = UINT32_MAX;
    uint32_t m_upperBodyAnimationLayerIndex = UINT32_MAX;
    float m_crouchBlend = 0.0f;
    float m_movementBlend = 0.0f;
    float m_jumpBlend = 0.0f;
    Bible::AnimationSlot m_weaponAnimationSlot = Bible::AnimationSlot::UNDEFINED;
    bool m_debugDraw = false;
    bool m_debugDrawEjectionPort = false;
};

}
