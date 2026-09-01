#include "Player.h"

#include "Hell/Audio.h"
#include "Hell/Time.h"
#include "Hell/Input.h"

#include "Legacy/World/LegacyWorld.h"

#include "Unloved/Session/Session.h"

namespace Audio = Hell::Audio;
namespace Input = Hell::Input;

namespace Unloved {

void Player::UpdateShotgunGunLogic(float deltaTime) {
    if (InventoryIsClosed()) {
        if (PressingFire() && CanFireShotgun()) {
            FireShotgun();
        }
        if (PressingFire() && CanDryFireShotgun()) {
            DryFireShotgun();
        }
        if (PressedReload() && CanReloadShotgun()) {
            ReloadShotgun();
        }
        if (PressedADS() && CanToggleShotgunAuto()) {
            ToggleAutoShotgun();
        }
        if (PressedMelee() && CanSecondaryMelee()) {
            SecondaryMelee();
        }
    }

    UpdateShotgunReloadLogic();
    UpdatePumpAudio();

    // Green shell hack
    static float delayCounter = 0.0f;
    if (delayCounter > 0.0f) {
        delayCounter -= Hell::Time::DeltaTime();
    }
    if (delayCounter <= 0.0f && (Input::MouseWheelDown() || Input::MouseWheelUp())) {
        WeaponState* weaponState = GetCurrentWeaponState();
        weaponState->shotgunSlug = !weaponState->shotgunSlug;
        Audio::PlayAudio("SPAS_AutoToggle_STOLEN_FROM_HALFLIFE.wav", 1.0f, GetWeaponAudioFrequency());
        delayCounter = 0.1f; // cooldown
    }

    WeaponState* weaponState = GetCurrentWeaponState();
    SkinnedGameObject* viewWeapon = GetViewWeaponSkinnedGameObject();
    if (weaponState->shotgunSlug) {
        viewWeapon->SetMeshMaterialByMeshName("Shells", "ShellGreen");
        viewWeapon->SetMeshMaterialByMeshName("Shells.001", "ShellGreen");
    }
    else {
        viewWeapon->SetMeshMaterialByMeshName("Shells", "Shell");
        viewWeapon->SetMeshMaterialByMeshName("Shells.001", "Shell");
    }
}

void Player::ToggleAutoShotgun() {
    WeaponState* weaponState = GetCurrentWeaponState();

    PlayViewWeaponAnimation(Bible::AnimationSlot::TOGGLE_AUTO, 1.0f);

    m_weaponAction = WeaponAction::TOGGLING_AUTO;
    weaponState->shotgunInAutoMode = !weaponState->shotgunInAutoMode;
    Audio::PlayAudio("SPAS_AutoToggle_STOLEN_FROM_HALFLIFE.wav", 1.0f, GetWeaponAudioFrequency());

    //if (weaponState->shotgunInAutoMode) {
    //    DisplayInfoText("AUTO SHOTGUN ENABLED");
    //}
    //else {
    //    DisplayInfoText("AUTO SHOTGUN DISABLED");
    //}
}

void Player::FireShotgun() {
    WeaponInfo* weaponInfo = GetCurrentWeaponInfo();
    WeaponState* weaponState = GetCurrentWeaponState();

    m_weaponAction = WeaponAction::FIRE;
    weaponState->ammoInMag--;
    weaponState->shotgunShellChambered = weaponState->shotgunInAutoMode && weaponState->ammoInMag > 0;

    // Needs to rack pump?
    if (!weaponState->shotgunInAutoMode && weaponState->ammoInMag > 0) {
        weaponState->shotgunAwaitingPumpAudio = true;
        PlayViewWeaponAnimation(Bible::AnimationSlot::FIRE_1, weaponInfo->animationSpeeds.fire);
    }
    // No pump rack
    else {
        weaponState->shotgunAwaitingPumpAudio = false;
        PlayViewWeaponAnimation(Bible::AnimationSlot::SHOTGUN_FIRE_NO_PUMP, 1.0f);
    }
    SpawnMuzzleFlash(55.0f, 0.3f);
    SpawnCasing();
    Audio::PlayAudio(weaponInfo->audioFiles.fire[0], 1.0f, GetWeaponAudioFrequency());

    for (int i = 0; i < 12; i++) {
        SpawnBullet(0.1f);
    }
}


void Player::SecondaryMelee() {
    WeaponInfo* weaponInfo = GetCurrentWeaponInfo();
    WeaponState* weaponState = GetCurrentWeaponState();

    if (!weaponInfo) return;
    if (!weaponState) return;

    // Silently perform any required pump as melee begins
    if (weaponState->ammoInMag > 0 && !weaponState->shotgunShellChambered) {
        weaponState->shotgunShellChambered = true;
        weaponState->shotgunAwaitingPumpAudio = false;
    }

    m_weaponAction = WeaponAction::SECONDARY_MELEE;

    PlayCharacterWeaponAnimation(Bible::AnimationSlot::MELEE);
    PlayViewWeaponAnimation(Bible::AnimationSlot::MELEE, weaponInfo->animationSpeeds.melee);

    BeginMeleeAttack(Bible::AnimationSlot::MELEE);

    Audio::PlayAudio("Shotgun_Melee_Miss.wav", 1.0f, GetWeaponAudioFrequency());
}

void Player::DryFireShotgun() {
    PlayViewWeaponAnimation(Bible::AnimationSlot::DRY_FIRE, 1.0f);

    m_weaponAction = WeaponAction::DRY_FIRE;
    Audio::PlayAudio("Dry_Fire_HalfLife.wav", 1.0f, GetWeaponAudioFrequency());
}

void Player::ReloadShotgun() {
    WeaponInfo* weaponInfo = GetCurrentWeaponInfo();
    WeaponState* weaponState = GetCurrentWeaponState();

    PlayViewWeaponAnimation(Bible::AnimationSlot::SHOTGUN_RELOAD_START, 1.0f);
    m_weaponAction = SHOTGUN_RELOAD_BEGIN;
    weaponState->shotgunAwaitingFirstShellReload = true;
    weaponState->shotgunAwaitingSecondShellReload = true;
}

void Player::UpdateShotgunReloadLogic() {
    WeaponAction weaponAction = GetCurrentWeaponAction();
    WeaponInfo* weaponInfo = GetCurrentWeaponInfo();
    WeaponState* weaponState = GetCurrentWeaponState();
    AmmoState* ammoState = GetCurrentAmmoState();

    // Begin loading anim has complete, time to reload!
    if (IsViewWeaponAnimationComplete() && weaponAction == WeaponAction::SHOTGUN_RELOAD_BEGIN ||
        IsViewWeaponAnimationComplete() && weaponAction == WeaponAction::SHOTGUN_RELOAD_SINGLE_SHELL ||
        IsViewWeaponAnimationComplete() && weaponAction == WeaponAction::SHOTGUN_RELOAD_DOUBLE_SHELL) {

        // Reload 1 shell ONLY if the shotty can only take 1, or you only have 1
        if (weaponState->ammoInMag == 7 && ammoState->ammoOnHand > 0 || ammoState->ammoOnHand == 1) {
            PlayViewWeaponAnimation(Bible::AnimationSlot::SHOTGUN_RELOAD_ONE_SHELL, weaponInfo->animationSpeeds.shotgunReloadOneShell);
            weaponState->shotgunAwaitingFirstShellReload = true;

            m_weaponAction = SHOTGUN_RELOAD_SINGLE_SHELL;
        }
        // Otherwise do the double reload
        else if (weaponState->ammoInMag <= 6 && ammoState->ammoOnHand >= 2) {
            PlayViewWeaponAnimation(Bible::AnimationSlot::SHOTGUN_RELOAD_TWO_SHELLS, weaponInfo->animationSpeeds.shotgunReloadTwoShells);
            weaponState->shotgunAwaitingFirstShellReload = true;
            weaponState->shotgunAwaitingSecondShellReload = true;

            m_weaponAction = SHOTGUN_RELOAD_DOUBLE_SHELL;
        }
        // Otherwise reload is done, time to flip the gun back to idle
        else {
            if (IsShellInShotgunChamber()) {
                m_weaponAction = SHOTGUN_RELOAD_END;
                weaponState->shotgunAwaitingPumpAudio = false;
                PlayViewWeaponAnimation(Bible::AnimationSlot::SHOTGUN_RELOAD_END, weaponInfo->animationSpeeds.shotgunReloadEnd);
            }
            else {
                m_weaponAction = SHOTGUN_RELOAD_END_WITH_PUMP;
                weaponState->shotgunAwaitingPumpAudio = true;
                PlayViewWeaponAnimation(Bible::AnimationSlot::SHOTGUN_RELOAD_END_PUMP, weaponInfo->animationSpeeds.shotgunReloadEndPump);
            }
        }
    }

    // Cancel reload with no shell in chamber ?
    if (PressedFire() || PressedADS()) {
        if (weaponAction == SHOTGUN_RELOAD_BEGIN ||
            weaponAction == SHOTGUN_RELOAD_SINGLE_SHELL ||
            weaponAction == SHOTGUN_RELOAD_DOUBLE_SHELL ||
            weaponAction == SHOTGUN_RELOAD_BEGIN ||
            weaponAction == SHOTGUN_RELOAD_BEGIN ||
            weaponAction == SHOTGUN_RELOAD_BEGIN) {
            weaponState->shotgunAwaitingPumpAudio = true;

            if (IsShellInShotgunChamber()) {
                PlayViewWeaponAnimation(Bible::AnimationSlot::SHOTGUN_RELOAD_END, weaponInfo->animationSpeeds.shotgunReloadEnd);
                m_weaponAction = SHOTGUN_RELOAD_END;
            }
            else {
                PlayViewWeaponAnimation(Bible::AnimationSlot::SHOTGUN_RELOAD_END_PUMP, weaponInfo->animationSpeeds.shotgunReloadEndPump);
                m_weaponAction = SHOTGUN_RELOAD_END_WITH_PUMP;
            }
        }
    }

    // Audio
    if (weaponAction == WeaponAction::SHOTGUN_RELOAD_SINGLE_SHELL) {
        if (weaponState->shotgunAwaitingFirstShellReload && IsViewWeaponAnimationPastFrameNumber(7)) {
            Audio::PlayAudio("Shotgun_Reload.wav", 1.0f, GetWeaponAudioFrequency());
            weaponState->shotgunAwaitingFirstShellReload = false;
            weaponState->ammoInMag++;
            ammoState->ammoOnHand--;
        }
    }

    if (weaponAction == WeaponAction::SHOTGUN_RELOAD_DOUBLE_SHELL) {
        if (weaponState->shotgunAwaitingFirstShellReload && IsViewWeaponAnimationPastFrameNumber(7)) {
            Audio::PlayAudio("Shotgun_Reload.wav", 1.0f, GetWeaponAudioFrequency());
            weaponState->shotgunAwaitingFirstShellReload = false;
            weaponState->ammoInMag++;
            ammoState->ammoOnHand--;
        }
        if (weaponState->shotgunAwaitingSecondShellReload && IsViewWeaponAnimationPastFrameNumber(19)) {
            Audio::PlayAudio("Shotgun_Reload.wav", 1.0f, GetWeaponAudioFrequency());
            weaponState->shotgunAwaitingSecondShellReload = false;
            weaponState->ammoInMag++;
            ammoState->ammoOnHand--;
        }
    }
}

void Player::UpdatePumpAudio() {
    WeaponInfo* weaponInfo = GetCurrentWeaponInfo();
    WeaponState* weaponState = GetCurrentWeaponState();

    int pumpFrame = 0;
    if (GetCurrentWeaponAction() == WeaponAction::FIRE) pumpFrame = 3;
    if (GetCurrentWeaponAction() == WeaponAction::SHOTGUN_RELOAD_END_WITH_PUMP) pumpFrame = 3;
    if (GetCurrentWeaponAction() == WeaponAction::DRAWING_WITH_SHOTGUN_PUMP) pumpFrame = 6;

    if (pumpFrame > 0 && IsViewWeaponAnimationPastFrameNumber(pumpFrame) && weaponState->shotgunAwaitingPumpAudio) {
        Audio::PlayAudio(weaponInfo->audioFiles.shotgunPump, 1.0f, GetWeaponAudioFrequency());
        weaponState->shotgunAwaitingPumpAudio = false;
        weaponState->shotgunShellChambered = true;
    }
}

bool Player::CanFireShotgun() {
    const WeaponAction weaponAction = GetCurrentWeaponAction();

    WeaponInfo* weaponInfo = GetCurrentWeaponInfo();
    WeaponState* weaponState = GetCurrentWeaponState();

    if (!weaponInfo) return false;
    if (!weaponState) return false;

    if (weaponState->ammoInMag > 0 && weaponState->shotgunShellChambered) {
        return (weaponAction == IDLE ||
                weaponAction == FIRE && !weaponState->shotgunInAutoMode && IsViewWeaponAnimationPastFrameNumber(weaponInfo->animationCancelFrames.fire) ||
                weaponAction == FIRE && weaponState->shotgunInAutoMode && IsViewWeaponAnimationPastFrameNumber(weaponInfo->animationCancelFrames.fireAutoShotgun));
    }

    return false;
}

bool Player::CanDryFireShotgun() {
    const WeaponAction weaponAction = GetCurrentWeaponAction();

    WeaponInfo* weaponInfo = GetCurrentWeaponInfo();
    WeaponState* weaponState = GetCurrentWeaponState();

    if (!weaponInfo) return false;
    if (!weaponState) return false;

    if (weaponState->ammoInMag == 0) {
        return (weaponAction == IDLE ||
                weaponAction == DRY_FIRE && IsViewWeaponAnimationPastFrameNumber(weaponInfo->animationCancelFrames.dryFire) ||
                weaponAction == FIRE && IsViewWeaponAnimationPastFrameNumber(weaponInfo->animationCancelFrames.fire));
    }

    return false;
}

bool Player::CanSecondaryMelee() {
    WeaponInfo* weaponInfo = GetCurrentWeaponInfo();
    if (!weaponInfo) return false;

    // Bail if this weapon has no melee attacks
    if (weaponInfo->meleeAttacks.empty()) return false;

    // Bail if you are already mid secondary melee
    if (GetCurrentWeaponAction() == SECONDARY_MELEE && !IsViewWeaponAnimationPastFrameNumber(weaponInfo->animationCancelFrames.secondaryMelee)) return false;

    return true;
}

bool Player::CanToggleShotgunAuto() {
    WeaponInfo* weaponInfo = GetCurrentWeaponInfo();
    WeaponState* weaponState = GetCurrentWeaponState();

    if (!weaponInfo) return false;
    if (!weaponState) return false;

    if (!weaponInfo->hasAutoSwitch) {
        return false;
    }

    WeaponAction weaponAction = GetCurrentWeaponAction();
    return (
        weaponAction == IDLE ||
        weaponAction == FIRE && !weaponState->shotgunInAutoMode && IsViewWeaponAnimationPastFrameNumber(weaponInfo->animationCancelFrames.fire) ||
        weaponAction == FIRE && weaponState->shotgunInAutoMode && IsViewWeaponAnimationPastFrameNumber(weaponInfo->animationCancelFrames.fireAutoShotgun) ||
        weaponAction == TOGGLING_AUTO && IsViewWeaponAnimationPastFrameNumber(2)
    );
}

bool Player::CanReloadShotgun() {
    WeaponInfo* weaponInfo = GetCurrentWeaponInfo();
    AmmoState* ammoState = GetCurrentAmmoState();
    WeaponState* weaponState = GetCurrentWeaponState();
    WeaponAction weaponAction = GetCurrentWeaponAction();

    // If shotty aint full, and you have enough ammo
    if (weaponState->ammoInMag < weaponInfo->magSize && ammoState->ammoOnHand > 0) {
        return (
            // And you are playing an acceptable animation to cancel
            weaponAction == IDLE ||
            weaponAction == FIRE && IsViewWeaponAnimationPastFrameNumber(22) ||
            weaponAction == DRY_FIRE && IsViewWeaponAnimationPastFrameNumber(5) ||
            weaponAction == SHOTGUN_RELOAD_END && IsViewWeaponAnimationPastFrameNumber(5) ||
            weaponAction == SHOTGUN_RELOAD_END_WITH_PUMP && IsViewWeaponAnimationPastFrameNumber(5)
        );
    }
    return false;
}


bool Player::IsShellInShotgunChamber() {
    WeaponState* weaponState = GetCurrentWeaponState();
    if (!weaponState) return false;

    if (weaponState && GetCurrentWeaponType() == WeaponType::SHOTGUN) {
        return weaponState->shotgunShellChambered;
    }
}

bool Player::ShotgunRequiresPump() {
    WeaponState* weaponState = GetCurrentWeaponState();
    if (!weaponState) return false;

    if (weaponState && GetCurrentWeaponType() == WeaponType::SHOTGUN) {
        return weaponState->shotgunRequiresPump;
    }
}

} // namespace Unloved
