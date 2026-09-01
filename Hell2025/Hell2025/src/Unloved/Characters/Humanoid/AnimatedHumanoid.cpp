#include "AnimatedHumanoid.h"

#include "Hell/Common/Enum.h"
#include "Hell/Logging.h"
#include "Hell/Physics/Ragdoll/Ragdoll.h"
#include "Unloved/Bible/Bible.h"
#include "Unloved/Debug/DebugDraw.h"
#include "Unloved/Objects/Renderables/SkinnedGameObject.h"
#include "Unloved/Systems/Animator/Animator.h"
#include "Unloved/World/World.h"

#include <algorithm>
#include <array>
#include <vector>

namespace Unloved {
namespace {
    constexpr std::array<Bible::AnimationSlot, 5> REQUIRED_LOCOMOTION_ANIMATION_SLOTS = {
        Bible::AnimationSlot::IDLE,
        Bible::AnimationSlot::WALK,
        Bible::AnimationSlot::IDLE_CROUCHING,
        Bible::AnimationSlot::WALK_CROUCHING,
        Bible::AnimationSlot::JUMP
    };
}

bool AnimatedHumanoid::LocomotionAnimationIndices::IsValid() const {
    return layer != UINT32_MAX && standingIdle != UINT32_MAX && standingMoving != UINT32_MAX && crouchingIdle != UINT32_MAX && crouchingMoving != UINT32_MAX;
}

bool AnimatedHumanoid::Init(uint64_t parentObjectId, Bible::SkinnedModelPreset bodySkinnedModelPreset, const glm::vec3& position, const glm::vec3& rotation, float scale) {
    CleanUp();

    if (!Bible::GetHumanoidInfo(bodySkinnedModelPreset)) return false;

    m_bodySkinnedModelPreset = bodySkinnedModelPreset;
    m_animatorInstanceId = Animator::CreateAnimatorInstance();
    m_bodySkinnedGameObjectId = World::CreateSkinnedGameObject();
    m_weaponSkinnedGameObjectId = World::CreateSkinnedGameObject();

    AnimatorInstance* animatorInstance = GetAnimatorInstance();
    SkinnedGameObject* bodySkinnedGameObject = GetBodySkinnedGameObject();
    SkinnedGameObject* weaponSkinnedGameObject = GetWeaponSkinnedGameObject();
    if (!animatorInstance || !bodySkinnedGameObject || !weaponSkinnedGameObject) {
        CleanUp();
        return false;
    }

    bodySkinnedGameObject->SetOwnerObjectId(parentObjectId);
    weaponSkinnedGameObject->SetOwnerObjectId(parentObjectId);
    SetPosition(position);
    SetRotation(rotation);
    SetScale(scale);

    Bible::ConfigureSkinnedModel(*bodySkinnedGameObject, bodySkinnedModelPreset);
    SkinnedModel* bodySkinnedModel = bodySkinnedGameObject->GetSkinnedModel();
    if (!bodySkinnedModel) {
        CleanUp();
        return false;
    }
    bodySkinnedGameObject->SetAnimatorInstanceId(m_animatorInstanceId);
    weaponSkinnedGameObject->SetAnimatorInstanceId(m_animatorInstanceId);
    bodySkinnedGameObject->SetAnimationModeToAnimated();
    weaponSkinnedGameObject->SetAnimationModeToAnimated();
    animatorInstance->RegisterSkinnedModels({ bodySkinnedModel->GetName() });

    return true;
}

void AnimatedHumanoid::DebugDraw() {
    if (m_debugDraw) {
        AnimatorInstance* animatorInstance = GetAnimatorInstance();
        SkinnedGameObject* bodySkinnedGameObject = GetBodySkinnedGameObject();
        if (animatorInstance && bodySkinnedGameObject) {
            const std::vector<SkeletonNode>& nodes = animatorInstance->GetSkeleton().GetNodes();
            const std::vector<glm::mat4>& globalPose = animatorInstance->GetGlobalPose();

            if (nodes.size() == globalPose.size()) {
                const glm::mat4 modelMatrix = bodySkinnedGameObject->GetModelMatrix();
                for (uint32_t i = 0; i < nodes.size(); i++) {
                    const glm::vec3 position = modelMatrix * globalPose[i] * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
                    DebugDraw::DrawPoint(position, YELLOW);

                    const int32_t parentIndex = nodes[i].parentIndex;
                    if (parentIndex == -1) continue;

                    const glm::vec3 parentPosition = modelMatrix * globalPose[parentIndex] * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
                    DebugDraw::DrawLine(position, parentPosition, WHITE);
                }
            }
        }
    }

    if (m_debugDrawEjectionPort) {
        SkinnedGameObject* weaponSkinnedGameObject = GetWeaponSkinnedGameObject();
        if (weaponSkinnedGameObject) DebugDraw::DrawPoint(weaponSkinnedGameObject->GetNodeWorldPosition("EjectionPort"), RED);
    }
}

void AnimatedHumanoid::CleanUp() {
    if (m_bodySkinnedGameObjectId != 0) World::RemoveObjectById(m_bodySkinnedGameObjectId);
    if (m_weaponSkinnedGameObjectId != 0) World::RemoveObjectById(m_weaponSkinnedGameObjectId);
    if (m_animatorInstanceId != 0) Animator::RemoveAnimatorInstance(m_animatorInstanceId);

    m_bodySkinnedModelPreset = Bible::SkinnedModelPreset::UNDEFINED;
    m_weapon = Bible::Weapon::UNDEFINED;
    m_animationProfile = Bible::AnimationProfile::UNDEFINED;
    m_bodySkinnedGameObjectId = 0;
    m_weaponSkinnedGameObjectId = 0;
    m_animatorInstanceId = 0;
    m_lowerBodyLocomotionAnimationIndices = {};
    m_chestLocomotionAnimationIndices = {};
    m_jumpAnimationLayerIndex = UINT32_MAX;
    m_upperBodyAnimationLayerIndex = UINT32_MAX;
    m_crouchBlend = 0.0f;
    m_movementBlend = 1.0f;
    m_jumpBlend = 0.0f;
    m_weaponAnimationSlot = Bible::AnimationSlot::UNDEFINED;
}

void AnimatedHumanoid::CreateRagdoll(const std::string& ragdollName) {
    SkinnedGameObject* bodySkinnedGameObject = GetBodySkinnedGameObject();
    if (bodySkinnedGameObject) bodySkinnedGameObject->CreateRagdoll(ragdollName);
}

void AnimatedHumanoid::SetPosition(const glm::vec3& position) {
    SkinnedGameObject* bodySkinnedGameObject = GetBodySkinnedGameObject();
    SkinnedGameObject* weaponSkinnedGameObject = GetWeaponSkinnedGameObject();
    if (bodySkinnedGameObject) bodySkinnedGameObject->SetPosition(position);
    if (weaponSkinnedGameObject) weaponSkinnedGameObject->SetPosition(position);
}

void AnimatedHumanoid::SetRotation(const glm::vec3& rotation) {
    SkinnedGameObject* bodySkinnedGameObject = GetBodySkinnedGameObject();
    SkinnedGameObject* weaponSkinnedGameObject = GetWeaponSkinnedGameObject();
    if (bodySkinnedGameObject) {
        bodySkinnedGameObject->SetRotationX(rotation.x);
        bodySkinnedGameObject->SetRotationY(rotation.y);
        bodySkinnedGameObject->SetRotationZ(rotation.z);
    }
    if (weaponSkinnedGameObject) {
        weaponSkinnedGameObject->SetRotationX(rotation.x);
        weaponSkinnedGameObject->SetRotationY(rotation.y);
        weaponSkinnedGameObject->SetRotationZ(rotation.z);
    }
}

void AnimatedHumanoid::SetScale(float scale) {
    SkinnedGameObject* bodySkinnedGameObject = GetBodySkinnedGameObject();
    SkinnedGameObject* weaponSkinnedGameObject = GetWeaponSkinnedGameObject();
    if (bodySkinnedGameObject) bodySkinnedGameObject->SetScale(scale);
    if (weaponSkinnedGameObject) weaponSkinnedGameObject->SetScale(scale);
}

void AnimatedHumanoid::SetCrouchBlend(float crouchBlend) {
    m_crouchBlend = std::clamp(crouchBlend, 0.0f, 1.0f);
    UpdateLocomotionAnimationWeights();
}

void AnimatedHumanoid::SetMovementBlend(float movementBlend) {
    m_movementBlend = std::clamp(movementBlend, 0.0f, 1.0f);
    UpdateLocomotionAnimationWeights();
}

void AnimatedHumanoid::SetJumpBlend(float jumpBlend) {
    m_jumpBlend = std::clamp(jumpBlend, 0.0f, 1.0f);
    UpdateJumpAnimationWeights();
}

void AnimatedHumanoid::SetWeapon(Bible::Weapon weapon) {
    if (weapon == m_weapon) return;
    if (weapon == Bible::Weapon::UNDEFINED) {
        Logging::Error() << "AnimatedHumanoid::SetWeapon(..) received Weapon::UNDEFINED\n";
        return;
    }

    WeaponInfo* weaponInfo = Bible::GetWeaponInfo(weapon);
    if (!weaponInfo) return;
    if (weaponInfo->characterSkinnedModelPreset == Bible::SkinnedModelPreset::UNDEFINED) {
        Logging::Error() << "AnimatedHumanoid::SetWeapon(..) failed because '" << Hell::Enum::ToString(weapon) << "' has no characterSkinnedModelPreset\n";
        return;
    }

    const Bible::AnimationProfile animationProfile = Bible::GetHumanoidAnimationProfile(m_bodySkinnedModelPreset, weapon);
    if (animationProfile == Bible::AnimationProfile::UNDEFINED) {
        Logging::Error() << "AnimatedHumanoid::SetWeapon(..) body and weapon combination has no animation profile\n";
        return;
    }

    for (Bible::AnimationSlot animationSlot : REQUIRED_LOCOMOTION_ANIMATION_SLOTS) {
        if (!Bible::HasAnimation(animationProfile, animationSlot)) {
            Logging::Error() << "AnimatedHumanoid::SetWeapon(..) animation profile '" << Hell::Enum::ToString(animationProfile) << "' is missing required slot '" << Hell::Enum::ToString(animationSlot) << "'\n";
            return;
        }
        if (m_animationProfile != Bible::AnimationProfile::UNDEFINED && Bible::GetAnimation(m_animationProfile, animationSlot) != Bible::GetAnimation(animationProfile, animationSlot)) {
            Logging::Error() << "AnimatedHumanoid::SetWeapon(..) animation profile '" << Hell::Enum::ToString(animationProfile) << "' has incompatible locomotion animation '" << Hell::Enum::ToString(animationSlot) << "'\n";
            return;
        }
    }

    const Bible::HumanoidInfo* humanoidInfo = Bible::GetHumanoidInfo(m_bodySkinnedModelPreset);
    AnimatorInstance* animatorInstance = GetAnimatorInstance();
    SkinnedGameObject* bodySkinnedGameObject = GetBodySkinnedGameObject();
    SkinnedGameObject* weaponSkinnedGameObject = GetWeaponSkinnedGameObject();
    SkinnedModel* bodySkinnedModel = bodySkinnedGameObject ? bodySkinnedGameObject->GetSkinnedModel() : nullptr;
    if (!humanoidInfo || !animatorInstance || !bodySkinnedModel || !weaponSkinnedGameObject) {
        Logging::Error() << "AnimatedHumanoid::SetWeapon(..) humanoid is not initialized\n";
        return;
    }

    const bool firstWeapon = m_weapon == Bible::Weapon::UNDEFINED;
    SkinnedModel* oldWeaponSkinnedModel = weaponSkinnedGameObject->GetSkinnedModel();
    if (!firstWeapon && !oldWeaponSkinnedModel) return;

    const std::string oldWeaponSkinnedModelName = oldWeaponSkinnedModel ? oldWeaponSkinnedModel->GetName() : UNDEFINED_STRING;
    Bible::ConfigureSkinnedModel(*weaponSkinnedGameObject, weaponInfo->characterSkinnedModelPreset);

    SkinnedModel* weaponSkinnedModel = weaponSkinnedGameObject->GetSkinnedModel();
    if (!weaponSkinnedModel) return;

    const std::string& bodySkinnedModelName = bodySkinnedModel->GetName();
    const std::string& weaponSkinnedModelName = weaponSkinnedModel->GetName();

    if (firstWeapon) {
        animatorInstance->RegisterSkinnedModels({ bodySkinnedModelName, weaponSkinnedModelName });
        if (!ConfigureAnimator(*humanoidInfo, animationProfile, weaponSkinnedModelName)) {
            Logging::Error() << "AnimatedHumanoid::SetWeapon(..) failed to configure the animator\n";
            return;
        }
    }
    else {
        animatorInstance->ReplaceSkinnedModel(oldWeaponSkinnedModelName, weaponSkinnedModelName);
    }

    m_weapon = weapon;
    m_animationProfile = animationProfile;
    m_weaponAnimationSlot = Bible::AnimationSlot::IDLE;
    UpdateLocomotionAnimationWeights();
    UpdateWeaponAnimation();
    animatorInstance->RestartAnimation();
}

void AnimatedHumanoid::SetWeaponAnimation(Bible::AnimationSlot animationSlot) {
    if (m_weaponAnimationSlot == animationSlot) return;
    if (animationSlot != Bible::AnimationSlot::UNDEFINED && !Bible::HasAnimation(m_animationProfile, animationSlot)) {
        Logging::Error() << "AnimatedHumanoid::SetWeaponAnimation(..) animation profile '" << Hell::Enum::ToString(m_animationProfile) << "' is missing slot '" << Hell::Enum::ToString(animationSlot) << "'\n";
        return;
    }
    m_weaponAnimationSlot = animationSlot;
    UpdateWeaponAnimation();
}

void AnimatedHumanoid::SetDebugDraw(bool debugDraw) {
    m_debugDraw = debugDraw;
}

void AnimatedHumanoid::SetDebugDrawEjectionPort(bool debugDrawEjectionPort) {
    m_debugDrawEjectionPort = debugDrawEjectionPort;
}

void AnimatedHumanoid::SetIgnoredViewportIndex(int32_t viewportIndex) {
    SkinnedGameObject* bodySkinnedGameObject = GetBodySkinnedGameObject();
    SkinnedGameObject* weaponSkinnedGameObject = GetWeaponSkinnedGameObject();
    if (bodySkinnedGameObject) bodySkinnedGameObject->SetIgnoredViewportIndex(viewportIndex);
    if (weaponSkinnedGameObject) weaponSkinnedGameObject->SetIgnoredViewportIndex(viewportIndex);
}

void AnimatedHumanoid::SetBodyAnimationModeToAnimated() {
    SkinnedGameObject* bodySkinnedGameObject = GetBodySkinnedGameObject();
    if (!bodySkinnedGameObject) return;

    if (Ragdoll* ragdoll = bodySkinnedGameObject->GetRagdoll()) ragdoll->DisableSimulation();
    bodySkinnedGameObject->SetAnimationModeToAnimated();
}

void AnimatedHumanoid::SetBodyAnimationModeToRagdoll() {
    SkinnedGameObject* bodySkinnedGameObject = GetBodySkinnedGameObject();
    if (bodySkinnedGameObject) bodySkinnedGameObject->SetAnimationModeToRagdoll();
}

void AnimatedHumanoid::EnableWeaponRendering() {
    SkinnedGameObject* weaponSkinnedGameObject = GetWeaponSkinnedGameObject();
    if (weaponSkinnedGameObject) weaponSkinnedGameObject->EnableRendering();
}

void AnimatedHumanoid::DisableWeaponRendering() {
    SkinnedGameObject* weaponSkinnedGameObject = GetWeaponSkinnedGameObject();
    if (weaponSkinnedGameObject) weaponSkinnedGameObject->DisableRendering();
}

void AnimatedHumanoid::PlayJumpAnimation() {
    AnimatorInstance* animatorInstance = GetAnimatorInstance();
    if (!animatorInstance || m_jumpAnimationLayerIndex == UINT32_MAX) return;
    if (!Bible::HasAnimation(m_animationProfile, Bible::AnimationSlot::JUMP)) {
        Logging::Error() << "AnimatedHumanoid::PlayJumpAnimation() animation profile '" << Hell::Enum::ToString(m_animationProfile) << "' is missing the jump animation\n";
        return;
    }

    animatorInstance->PlayAnimation(m_jumpAnimationLayerIndex, Bible::GetAnimation(m_animationProfile, Bible::AnimationSlot::JUMP), 1.0f);
}

bool AnimatedHumanoid::IsJumpAnimationComplete() {
    AnimatorInstance* animatorInstance = GetAnimatorInstance();
    if (!animatorInstance || m_jumpAnimationLayerIndex == UINT32_MAX) return true;
    return animatorInstance->IsAnimationComplete(m_jumpAnimationLayerIndex);
}

void AnimatedHumanoid::PlayCharacterWeaponAnimation(Bible::AnimationSlot animationSlot) {
    AnimatorInstance* animatorInstance = GetAnimatorInstance();
    if (!animatorInstance || m_upperBodyAnimationLayerIndex == UINT32_MAX) return;
    if (!Bible::HasAnimation(m_animationProfile, animationSlot)) {
        Logging::Error() << "AnimatedHumanoid::PlayCharacterWeaponAnimation(..) animation profile '" << Hell::Enum::ToString(m_animationProfile) << "' is missing slot '" << Hell::Enum::ToString(animationSlot) << "'\n";
        return;
    }

    animatorInstance->SetAnimationLayerWeight(m_upperBodyAnimationLayerIndex, 1.0f);
    animatorInstance->PlayAnimation(m_upperBodyAnimationLayerIndex, Bible::GetAnimation(m_animationProfile, animationSlot), 1.0f);
}

void AnimatedHumanoid::PlayAndLoopCharacterWeaponAnimation(Bible::AnimationSlot animationSlot) {
    AnimatorInstance* animatorInstance = GetAnimatorInstance();
    if (!animatorInstance || m_upperBodyAnimationLayerIndex == UINT32_MAX) return;
    if (!Bible::HasAnimation(m_animationProfile, animationSlot)) {
        Logging::Error() << "AnimatedHumanoid::PlayAndLoopCharacterWeaponAnimation(..) animation profile '" << Hell::Enum::ToString(m_animationProfile) << "' is missing slot '" << Hell::Enum::ToString(animationSlot) << "'\n";
        return;
    }

    animatorInstance->SetAnimationLayerWeight(m_upperBodyAnimationLayerIndex, 1.0f);
    animatorInstance->PlayAndLoopAnimation(m_upperBodyAnimationLayerIndex, Bible::GetAnimation(m_animationProfile, animationSlot), 1.0f);
}

uint64_t AnimatedHumanoid::GetRagdollId() {
    SkinnedGameObject* bodySkinnedGameObject = GetBodySkinnedGameObject();
    return bodySkinnedGameObject ? bodySkinnedGameObject->GetRagdollId() : 0;
}

Ragdoll* AnimatedHumanoid::GetRagdoll() {
    SkinnedGameObject* bodySkinnedGameObject = GetBodySkinnedGameObject();
    return bodySkinnedGameObject ? bodySkinnedGameObject->GetRagdoll() : nullptr;
}

AnimatorInstance* AnimatedHumanoid::GetAnimatorInstance() {
    return Animator::GetAnimatorInstanceByObjectId(m_animatorInstanceId);
}

SkinnedGameObject* AnimatedHumanoid::GetBodySkinnedGameObject() {
    return World::GetSkinnedGameObjectByObjectId(m_bodySkinnedGameObjectId);
}

SkinnedGameObject* AnimatedHumanoid::GetWeaponSkinnedGameObject() {
    return World::GetSkinnedGameObjectByObjectId(m_weaponSkinnedGameObjectId);
}

bool AnimatedHumanoid::ConfigureAnimator(const Bible::HumanoidInfo& humanoidInfo, Bible::AnimationProfile animationProfile, const std::string& weaponSkinnedModelName) {
    AnimatorInstance* animatorInstance = GetAnimatorInstance();
    if (!animatorInstance) return false;

    constexpr const char* weaponMotionSourceNodeName = "hand_R";
    animatorInstance->SetSkinnedModelMotionSource(weaponSkinnedModelName, weaponMotionSourceNodeName);

    const auto configureLocomotionLayer = [&](LocomotionAnimationIndices& indices, const std::vector<std::string>& boneMasks) {
        indices.layer = animatorInstance->CreateAnimationLayer();
        if (indices.layer == UINT32_MAX) return false;

        indices.standingIdle = animatorInstance->AddBlendAnimation(indices.layer, Bible::GetAnimation(animationProfile, Bible::AnimationSlot::IDLE), 1.0f);
        indices.standingMoving = animatorInstance->AddBlendAnimation(indices.layer, Bible::GetAnimation(animationProfile, Bible::AnimationSlot::WALK), 1.0f);
        indices.crouchingIdle = animatorInstance->AddBlendAnimation(indices.layer, Bible::GetAnimation(animationProfile, Bible::AnimationSlot::IDLE_CROUCHING), 1.0f);
        indices.crouchingMoving = animatorInstance->AddBlendAnimation(indices.layer, Bible::GetAnimation(animationProfile, Bible::AnimationSlot::WALK_CROUCHING), 1.0f);
        animatorInstance->SetAnimationLayerBoneMasks(indices.layer, boneMasks);
        return indices.IsValid();
    };

    if (!configureLocomotionLayer(m_lowerBodyLocomotionAnimationIndices, humanoidInfo.lowerBodyBoneMasks) ||
        !configureLocomotionLayer(m_chestLocomotionAnimationIndices, humanoidInfo.chestBoneMasks)) {
        Logging::Error() << "AnimatedHumanoid::ConfigureAnimator(..) failed to create the locomotion blends\n";
        return false;
    }

    m_jumpAnimationLayerIndex = animatorInstance->CreateAnimationLayer();
    if (m_jumpAnimationLayerIndex == UINT32_MAX) return false;
    animatorInstance->SetAnimationLayerBoneMasks(m_jumpAnimationLayerIndex, humanoidInfo.lowerBodyBoneMasks);
    animatorInstance->SetAnimationLayerWeight(m_jumpAnimationLayerIndex, 0.0f);

    m_upperBodyAnimationLayerIndex = animatorInstance->CreateAnimationLayer();
    if (m_upperBodyAnimationLayerIndex == UINT32_MAX) return false;

    animatorInstance->SetAnimationLayerSkinnedModelWeightSource(m_upperBodyAnimationLayerIndex, weaponSkinnedModelName, weaponMotionSourceNodeName);
    animatorInstance->SetAnimationLayerBoneMasks(m_upperBodyAnimationLayerIndex, humanoidInfo.upperBodyBoneMasks);
    animatorInstance->SetAnimationLayerGlobalRotationBlend(m_upperBodyAnimationLayerIndex, true);
    animatorInstance->SetMorphSourceLayer(m_upperBodyAnimationLayerIndex);
    UpdateLocomotionAnimationWeights();
    UpdateJumpAnimationWeights();
    return true;
}

void AnimatedHumanoid::UpdateLocomotionAnimationWeights() {
    AnimatorInstance* animatorInstance = GetAnimatorInstance();
    if (!animatorInstance) return;

    const float standingWeight = 1.0f - m_crouchBlend;
    const float crouchingWeight = m_crouchBlend;
    const auto updateLayer = [&](const LocomotionAnimationIndices& indices) {
        if (!indices.IsValid()) return;
        animatorInstance->SetAnimationWeight(indices.layer, indices.standingIdle, (1.0f - m_movementBlend) * standingWeight);
        animatorInstance->SetAnimationWeight(indices.layer, indices.standingMoving, m_movementBlend * standingWeight);
        animatorInstance->SetAnimationWeight(indices.layer, indices.crouchingIdle, (1.0f - m_movementBlend) * crouchingWeight);
        animatorInstance->SetAnimationWeight(indices.layer, indices.crouchingMoving, m_movementBlend * crouchingWeight);
    };

    updateLayer(m_lowerBodyLocomotionAnimationIndices);
    updateLayer(m_chestLocomotionAnimationIndices);
}

void AnimatedHumanoid::UpdateJumpAnimationWeights() {
    AnimatorInstance* animatorInstance = GetAnimatorInstance();
    if (!animatorInstance || !m_lowerBodyLocomotionAnimationIndices.IsValid() || !m_chestLocomotionAnimationIndices.IsValid() || m_jumpAnimationLayerIndex == UINT32_MAX) return;

    animatorInstance->SetAnimationLayerWeight(m_lowerBodyLocomotionAnimationIndices.layer, 1.0f - m_jumpBlend);
    animatorInstance->SetAnimationLayerWeight(m_chestLocomotionAnimationIndices.layer, 1.0f);
    animatorInstance->SetAnimationLayerWeight(m_jumpAnimationLayerIndex, m_jumpBlend);
}

void AnimatedHumanoid::UpdateWeaponAnimation() {
    AnimatorInstance* animatorInstance = GetAnimatorInstance();
    if (!animatorInstance || m_upperBodyAnimationLayerIndex == UINT32_MAX) return;

    const bool animationEnabled = m_weaponAnimationSlot != Bible::AnimationSlot::UNDEFINED;
    if (animationEnabled && !Bible::HasAnimation(m_animationProfile, m_weaponAnimationSlot)) return;
    animatorInstance->SetAnimationLayerWeight(m_upperBodyAnimationLayerIndex, animationEnabled ? 1.0f : 0.0f);
    if (animationEnabled) animatorInstance->PlayAndLoopAnimation(m_upperBodyAnimationLayerIndex, Bible::GetAnimation(m_animationProfile, m_weaponAnimationSlot), 1.0f);
}

}
