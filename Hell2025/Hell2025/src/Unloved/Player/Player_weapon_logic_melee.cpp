#include "Player.h"

#include "Hell/Audio.h"

namespace Audio = Hell::Audio;

namespace Unloved {

void Player::UpdateMeleeLogic(float deltaTime) {
    if (InventoryIsClosed()) {
        if (PressingFire() && CanFireMelee()) {
            FireMelee();
        }
    }
}

void Player::FireMelee() {
    WeaponInfo* weaponInfo = GetCurrentWeaponInfo();

    if (!weaponInfo || weaponInfo->meleeAttacks.empty()) return;

    if (weaponInfo->audioFiles.fire.size()) {
        int rand = std::rand() % weaponInfo->audioFiles.fire.size();
        Audio::PlayAudio(weaponInfo->audioFiles.fire[rand], 1.0f);
    }

    const int randomAnimationIndex = std::rand() % weaponInfo->meleeAttacks.size();
    const Bible::AnimationSlot animationSlot = weaponInfo->meleeAttacks[randomAnimationIndex].animationSlot;
    PlayViewWeaponAnimation(animationSlot, weaponInfo->animationSpeeds.fire);

    m_weaponAction = WeaponAction::FIRE;
    BeginMeleeAttack(animationSlot);
}

bool Player::CanFireMelee() {
    WeaponInfo* weaponInfo = GetCurrentWeaponInfo();
    if (!weaponInfo) return false;

    WeaponAction weaponAction = GetCurrentWeaponAction();
    return (
        weaponAction == IDLE ||
        weaponAction == FIRE && IsViewWeaponAnimationPastFrameNumber(weaponInfo->animationCancelFrames.fire)
    );
}

} // namespace Unloved
