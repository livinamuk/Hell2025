#include "Player.h"

#include "Unloved/Bible/Bible.h"
#include "Unloved/Objects/Renderables/SkinnedGameObject.h"
#include "Unloved/Systems/Animator/Animator.h"

namespace Unloved {

    void Player::PlayViewWeaponAnimation(AnimationSlot animationSlot, float speed) {
        WeaponInfo* weaponInfo = GetCurrentWeaponInfo();
        SkinnedGameObject* viewWeapon = GetViewWeaponSkinnedGameObject();
        AnimatorInstance* animatorInstance = Animator::GetAnimatorInstanceByObjectId(m_viewWeaponAnimatorInstanceId);

        if (!weaponInfo) return;
        if (!viewWeapon) return;
        if (!animatorInstance) return;

        viewWeapon->SetAnimationModeToAnimated();
        animatorInstance->PlayAnimation(m_viewWeaponAnimationLayerIndex, Bible::GetAnimation(weaponInfo->viewWeaponAnimationProfile, animationSlot), speed);
        animatorInstance->Update(0.0f);
    }

    void Player::PlayAndLoopViewWeaponAnimation(AnimationSlot animationSlot, float speed) {
        WeaponInfo* weaponInfo = GetCurrentWeaponInfo();
        SkinnedGameObject* viewWeapon = GetViewWeaponSkinnedGameObject();
        AnimatorInstance* animatorInstance = Animator::GetAnimatorInstanceByObjectId(m_viewWeaponAnimatorInstanceId);

        if (!weaponInfo) return;
        if (!viewWeapon) return;
        if (!animatorInstance) return;

        viewWeapon->SetAnimationModeToAnimated();
        animatorInstance->PlayAndLoopAnimation(m_viewWeaponAnimationLayerIndex, Bible::GetAnimation(weaponInfo->viewWeaponAnimationProfile, animationSlot), speed);
    }

    bool Player::IsViewWeaponAnimationComplete() {
        AnimatorInstance* animatorInstance = Animator::GetAnimatorInstanceByObjectId(m_viewWeaponAnimatorInstanceId);
        if (!animatorInstance) return true;

        return animatorInstance->IsAnimationComplete(m_viewWeaponAnimationLayerIndex);
    }

    uint32_t Player::GetViewWeaponAnimationFrameNumber() {
        AnimatorInstance* animatorInstance = Animator::GetAnimatorInstanceByObjectId(m_viewWeaponAnimatorInstanceId);
        if (!animatorInstance) return 0;

        return animatorInstance->GetAnimationFrameNumber(m_viewWeaponAnimationLayerIndex);
    }

    bool Player::IsViewWeaponAnimationPastFrameNumber(uint32_t frameNumber) {
        return GetViewWeaponAnimationFrameNumber() > frameNumber;
    }

    void Player::PlayAndLoopCharacterAnimation(const std::string& animationName, float speed) {
        SkinnedGameObject* characterModel = GetCharacterModelSkinnedGameObject();
        AnimatorInstance* animatorInstance = GetCharacterModelAnimatorInstance();
        if (!characterModel) return;
        if (!animatorInstance) return;

        characterModel->SetAnimationModeToAnimated();
        animatorInstance->PlayAndLoopAnimation(m_characterModelAnimationLayerIndex, animationName, speed);
    }

}
