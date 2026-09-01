#include "Player.h"

#include "Hell/Audio.h"
#include "Hell/Logging.h"

#include "Unloved/Viewport/ViewportManager.h"
#include "Unloved/Config/Config.h"

namespace Audio = Hell::Audio;

namespace Unloved {

void Player::UpdateGunLogic(float deltaTime) {
    if (InventoryIsClosed()) {
        if (PressingADS() && CanEnterADS()) {
            EnterADS();
        }
        if (!PressingADS() && CanLeaveADS()) {
            LeaveADS();
        }
        if (PressingFire() && CanFireGun()) {
            FireGun();
        }
        if (PressedReload() && CanReloadGun()) {
            ReloadGun();
        }
        if (PressedMelee() && CanSecondaryMelee()) {
            SecondaryMelee();
        }
    }
    UpdateGunReloadLogic();
    UpdateSlideLogic();
    UpdateADSLogic(deltaTime);
}

void Player::FireGun() {
    WeaponInfo* weaponInfo = GetCurrentWeaponInfo();
    WeaponState* weaponState = GetCurrentWeaponState();

    int randomAnimationIndex = std::rand() % 3;
    Bible::AnimationSlot fireAnimationSlot = Bible::AnimationSlot::FIRE_1;
    Bible::AnimationSlot adsFireAnimationSlot = Bible::AnimationSlot::ADS_FIRE_1;
    if (randomAnimationIndex == 1) {
        fireAnimationSlot = Bible::AnimationSlot::FIRE_2;
        adsFireAnimationSlot = Bible::AnimationSlot::ADS_FIRE_2;
    }
    if (randomAnimationIndex == 2) {
        fireAnimationSlot = Bible::AnimationSlot::FIRE_3;
        adsFireAnimationSlot = Bible::AnimationSlot::ADS_FIRE_3;
    }

    if (weaponInfo->hasADS && IsInADS()) {
        PlayViewWeaponAnimation(adsFireAnimationSlot, weaponInfo->animationSpeeds.adsFire);
        m_weaponAction = WeaponAction::ADS_FIRE;
    }
    else {
        PlayViewWeaponAnimation(fireAnimationSlot, weaponInfo->animationSpeeds.fire);
        m_weaponAction = WeaponAction::FIRE;
    }

    switch (Hell::Random::Int(0, 2)) {
        case 0: PlayCharacterWeaponAnimation(Bible::AnimationSlot::FIRE_1); break;
        case 1: PlayCharacterWeaponAnimation(Bible::AnimationSlot::FIRE_2); break;
        case 2: PlayCharacterWeaponAnimation(Bible::AnimationSlot::FIRE_3); break;
        default: break;
    }

    SpawnMuzzleFlash(55.0f, 0.2f);
    SpawnCasing();
    SpawnBullet(0.05f);

    weaponState->ammoInMag--;

    int rand = std::rand() % weaponInfo->audioFiles.fire.size();
    if (weaponState->hasSilencer) {
        Audio::PlayAudio("Silenced.wav", 1.0f, GetWeaponAudioFrequency());
    }
    else {
        Audio::PlayAudio(weaponInfo->audioFiles.fire[rand], 1.0f, GetWeaponAudioFrequency());
    }
}

void Player::ReloadGun() {
    WeaponState* weaponState = GetCurrentWeaponState();
    WeaponInfo* weaponInfo = GetCurrentWeaponInfo();

    if (GetCurrentWeaponMagAmmo() == 0) {
        Audio::PlayAudio(weaponInfo->audioFiles.reloadEmpty, 0.7f);
        PlayViewWeaponAnimation(Bible::AnimationSlot::RELOAD_EMPTY, weaponInfo->animationSpeeds.reloadempty);
        m_weaponAction = RELOAD_FROM_EMPTY;

        // Third person
        PlayCharacterWeaponAnimation(Bible::AnimationSlot::RELOAD_EMPTY);
    }
    else {
        Audio::PlayAudio(weaponInfo->audioFiles.reload, 0.8f);
        PlayViewWeaponAnimation(Bible::AnimationSlot::RELOAD, weaponInfo->animationSpeeds.reload);
        m_weaponAction = RELOAD;

        // Third person
        PlayCharacterWeaponAnimation(Bible::AnimationSlot::RELOAD);
    }
    weaponState->awaitingMagReload = true;
}

void Player::UpdateGunReloadLogic() {
    WeaponState* weaponState = GetCurrentWeaponState();
    WeaponInfo* weaponInfo = GetCurrentWeaponInfo();
    AmmoState* ammoState = GetCurrentAmmoState();

    if (m_weaponAction != RELOAD && m_weaponAction != RELOAD_FROM_EMPTY) {
        return;
    }

    int frameNumber = 0;
    switch (m_weaponAction) {
        case RELOAD:            frameNumber = weaponInfo->reloadMagInFrameNumber;       break;
        case RELOAD_FROM_EMPTY: frameNumber = weaponInfo->reloadEmptyMagInFrameNumber;  break;
        default: return;
    }

    if (weaponState->awaitingMagReload && IsViewWeaponAnimationPastFrameNumber(frameNumber)) {
        int ammoToGive = std::min(weaponInfo->magSize - weaponState->ammoInMag, ammoState->ammoOnHand);
        weaponState->ammoInMag += ammoToGive;
        ammoState->ammoOnHand -= ammoToGive;
        weaponState->awaitingMagReload = false;
    }
}

void Player::UpdateSlideLogic() {
    WeaponState* weaponState = GetCurrentWeaponState();
    WeaponInfo* weaponInfo = GetCurrentWeaponInfo();
    AmmoState* ammoState = GetCurrentAmmoState();

    if (!weaponState) {
        Logging::Error() << "weaponState was nullptr, failed calling GetCurrentWeaponState()";
    }
    if (!weaponInfo) {
        Logging::Error() << "weaponState was nullptr, failed calling GetCurrentWeaponInfo()";
    }
    if (!ammoState) {
        Logging::Error() << "weaponState was nullptr, failed calling GetCurrentAmmoState()";
    }

    if (weaponInfo->emptyReloadRequiresSlideOffset && m_weaponAction != RELOAD_FROM_EMPTY && weaponState->ammoInMag == 0) {
        weaponState->requiresSlideOffset = true;
    }
    else {
        weaponState->requiresSlideOffset = false;
    }
}

void Player::UpdateADSLogic(float deltaTime) {
    // Zoom
    float zoomSpeed = 0.075f;
    if (m_weaponAction == ADS_IN || m_weaponAction == ADS_IDLE || m_weaponAction == ADS_FIRE) {
        m_cameraZoom -= zoomSpeed;
    }
    else {
        m_cameraZoom += zoomSpeed;
    }
    m_cameraZoom = std::max(0.575f, m_cameraZoom);
    m_cameraZoom = std::min(1.0f, m_cameraZoom);

    Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(m_viewportIndex);
    viewport->SetPerspective(m_cameraZoom, Config::GetNearPlane(), Config::GetFarPlane());
}

void Player::EnterADS() {
    WeaponInfo* weaponInfo = GetCurrentWeaponInfo();
    PlayViewWeaponAnimation(Bible::AnimationSlot::ADS_IN, weaponInfo->animationSpeeds.adsIn);
    m_weaponAction = ADS_IN;
}

void Player::LeaveADS() {
    WeaponInfo* weaponInfo = GetCurrentWeaponInfo();
    PlayViewWeaponAnimation(Bible::AnimationSlot::ADS_OUT, weaponInfo->animationSpeeds.adsOut);
    m_weaponAction = ADS_OUT;
}

bool Player::CanReloadGun() {
    AmmoState* ammoState = GetCurrentAmmoState();
    WeaponInfo* weaponInfo = GetCurrentWeaponInfo();
    WeaponState* weaponState = GetCurrentWeaponState();

    if (!ammoState) return false;
    if (!weaponInfo) return false;
    if (!weaponState) return false;

    return (weaponState->ammoInMag < weaponInfo->magSize && ammoState->ammoOnHand > 0);
}

bool Player::CanFireGun() {
    WeaponState* weaponState = GetCurrentWeaponState();
    WeaponInfo* weaponInfo = GetCurrentWeaponInfo();

    // Prevent automatic fire on pistols
    if (weaponInfo->type == WeaponType::PISTOL && m_pistolAwaitingFireReleased) {
        return false;
    }

    WeaponAction weaponAction = GetCurrentWeaponAction();
    if (weaponState->ammoInMag > 0) {
        return (
            weaponAction == IDLE              ||
            weaponAction == ADS_IDLE          ||
            weaponAction == ADS_FIRE          && IsViewWeaponAnimationPastFrameNumber(weaponInfo->animationCancelFrames.adsFire) ||
            weaponAction == DRAWING           && IsViewWeaponAnimationPastFrameNumber(weaponInfo->animationCancelFrames.draw) ||
            weaponAction == FIRE              && IsViewWeaponAnimationPastFrameNumber(weaponInfo->animationCancelFrames.fire) ||
            weaponAction == RELOAD            && IsViewWeaponAnimationPastFrameNumber(weaponInfo->animationCancelFrames.reload) ||
            weaponAction == RELOAD_FROM_EMPTY && IsViewWeaponAnimationPastFrameNumber(weaponInfo->animationCancelFrames.reloadFromEmpty));
    }
    else {
        //std::cout << "Cannot fire gun\n";
        return false;
    }
}

bool Player::CanEnterADS() {
    WeaponState* weaponState = GetCurrentWeaponState();
    WeaponInfo* weaponInfo = GetCurrentWeaponInfo();

    if (!weaponInfo->hasADS) return false;

    return (
        (m_weaponAction != RELOAD && m_weaponAction != RELOAD_FROM_EMPTY && !IsInADS()) ||

        (m_weaponAction == RELOAD && IsViewWeaponAnimationPastFrameNumber(weaponInfo->animationCancelFrames.reload)) ||

        (m_weaponAction == RELOAD_FROM_EMPTY && IsViewWeaponAnimationPastFrameNumber(weaponInfo->animationCancelFrames.reloadFromEmpty)));
}

bool Player::CanLeaveADS() {
    return (m_weaponAction == ADS_IN || m_weaponAction == ADS_IDLE || m_weaponAction == ADS_FIRE);
}

bool Player::IsInADS() {
    return (m_weaponAction == ADS_IN ||
            m_weaponAction == ADS_OUT ||
            m_weaponAction == ADS_IDLE ||
            m_weaponAction == ADS_FIRE);
}

glm::mat4 Player::GetViewWeaponBoneWorldMatrix(const std::string& boneName) {
    SkinnedGameObject* viewWeapon = GetViewWeaponSkinnedGameObject();
    if (!viewWeapon) {
        return glm::mat4(1.0f);
    }

    glm::mat4 worldMatrix = viewWeapon->GetNodeWorldMatrix(boneName);

    return worldMatrix;
}

} // namespace Unloved
