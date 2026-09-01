#include "Player.h"

#include "Hell/Common/Random.h"
#include "Hell/Common/String.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/Audio.h"
#include "Hell/Logging.h"

#include "Legacy/World/LegacyWorld.h"
#include "Unloved/Render/Renderer.h"

#include "Unloved/Bible/Bible.h"
#include "Unloved/Config/PhysicsConfig.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Systems/Animator/Animator.h"
#include "Unloved/Systems/Bullets/BulletSystem.h"
#include "Unloved/World/World.h"

#include <algorithm>

namespace Audio = Hell::Audio;

namespace Unloved {

void Player::UpdateWeaponLogic(float deltaTime) {
    if (IsDead()) return;

    if (InventoryIsOpen()) {
        if (m_weaponAction == ADS_IN ||
            m_weaponAction == ADS_IDLE ||
            m_weaponAction == ADS_FIRE) {
            LeaveADS();
        }
    }
    if (InventoryIsClosed()) {
        if (PressedNextWeapon()) {
            NextWeapon();
        }
    }

    SkinnedGameObject* viewWeapon = GetViewWeaponSkinnedGameObject();
    WeaponInfo* weaponInfo = GetCurrentWeaponInfo();
    WeaponState* weaponState = GetCurrentWeaponState();

    if (!viewWeapon) return;
    if (!weaponInfo) return;

    switch (GetCurrentWeaponType()) {
        case WeaponType::MELEE:     UpdateMeleeLogic(deltaTime);        break;
        case WeaponType::PISTOL:    UpdateGunLogic(deltaTime);          break;
        case WeaponType::AUTOMATIC: UpdateGunLogic(deltaTime);          break;
        case WeaponType::SHOTGUN:   UpdateShotgunGunLogic(deltaTime);   break;
    }

    // Some flag used to prevent automatic fire on pistols
    if (weaponInfo->type == WeaponType::PISTOL) {
		if (!PressingFire()) {
			m_pistolAwaitingFireReleased = false;
		}
		if (PressingFire() && !weaponInfo->pistolIsAuto) {
			m_pistolAwaitingFireReleased = true;
		}
    }

    // Need to initiate draw animation?
    if (GetCurrentWeaponAction() == WeaponAction::DRAW_BEGIN) {
        Audio::PlayAudio("NextWeapon.wav", 0.5f);

        // Drawing a shotgun when it needs a pump
        if (GetCurrentWeaponType() == WeaponType::SHOTGUN && !IsShellInShotgunChamber() && weaponState->ammoInMag > 0) {
            PlayViewWeaponAnimation(AnimationSlot::SHOTGUN_DRAW_PUMP, weaponInfo->animationSpeeds.shotgunDrawPump);
            weaponState->shotgunAwaitingPumpAudio = true;
            weaponState->shotgunRequiresPump = true;
            m_weaponAction = DRAWING_WITH_SHOTGUN_PUMP;
        }
        // First draw
        else if (weaponState->awaitingDrawFirst && Bible::GetAnimation(weaponInfo->viewWeaponAnimationProfile, AnimationSlot::DRAW_FIRST) != UNDEFINED_STRING) {
            PlayViewWeaponAnimation(AnimationSlot::DRAW_FIRST, weaponInfo->animationSpeeds.drawFirst);
            weaponState->awaitingDrawFirst = false;
            m_weaponAction = DRAWING_FIRST;
            Audio::PlayAudio(weaponInfo->audioFiles.drawFirst, 1.0f);
        }
        // Regular draw
        else {
            PlayViewWeaponAnimation(AnimationSlot::DRAW, weaponInfo->animationSpeeds.draw);
            m_weaponAction = DRAWING;
        }
    }

    // Finished ADS in? Return to ADS idle
    if (GetWeaponAction() == WeaponAction::ADS_IN && IsViewWeaponAnimationComplete() ||
        GetWeaponAction() == WeaponAction::ADS_FIRE && IsViewWeaponAnimationComplete()) {
        m_weaponAction = WeaponAction::ADS_IDLE;
    }

    // Finished drawing weapon? Return to idle
    if (GetCurrentWeaponAction() == WeaponAction::DRAWING && IsViewWeaponAnimationComplete() ||
        GetCurrentWeaponAction() == WeaponAction::DRAWING_FIRST && IsViewWeaponAnimationComplete() ||
        GetCurrentWeaponAction() == WeaponAction::DRAWING_WITH_SHOTGUN_PUMP && IsViewWeaponAnimationComplete()) {
        m_weaponAction = WeaponAction::IDLE;
    }

    // In ADS idle?
    if (GetCurrentWeaponAction() == WeaponAction::ADS_IDLE) {
        if (IsMoving()) {
            PlayAndLoopViewWeaponAnimation(AnimationSlot::ADS_WALK, weaponInfo->animationSpeeds.adsWalk);
        }
        else {
            PlayAndLoopViewWeaponAnimation(AnimationSlot::ADS_IDLE, weaponInfo->animationSpeeds.adsIdle);
        }
    }

    // In idle? Then play idle or walk if moving
    if (GetCurrentWeaponAction() == WeaponAction::IDLE) {
        AnimationSlot animationSlot = IsMoving() ? AnimationSlot::WALK : AnimationSlot::IDLE;
        PlayAndLoopViewWeaponAnimation(animationSlot, 1.0f);
    }

    // Everything done? Go to idle
    if (IsViewWeaponAnimationComplete()) {
        m_weaponAction = WeaponAction::IDLE;
    }

    //viewWeapon->DisableDrawingForMeshByMeshName("Magazine_low");

    return;
}

void Player::GiveDefaultLoadout() {
    // Always give knife
    m_inventory.GiveWeapon("Knife");

    // Dev load out
    m_inventory.GiveWeapon("Glock");
    m_inventory.GiveWeapon("GoldenGlock");
    m_inventory.GiveWeapon("Tokarev");
    m_inventory.GiveWeapon("SPAS");
    m_inventory.GiveWeapon("P90");
    m_inventory.GiveWeapon("AKS74U");
    m_inventory.GiveWeapon("Remington870");

    m_inventory.GiveAmmo("Glock", 800);
    m_inventory.GiveAmmo("AKS74U", 20000);
    m_inventory.GiveAmmo("Tokarev", 400);
    m_inventory.GiveAmmo("P90", 420);

	// hack fill the shop
	m_shopInventory.GiveWeapon("GoldenGlock");
	m_shopInventory.AddInventoryItem("AKS74U");
	m_shopInventory.AddInventoryItem("SPAS");
	m_shopInventory.AddInventoryItem("Pills");
	m_shopInventory.AddInventoryItem("P90");
    return;

    m_inventory.GiveWeapon("GoldenGlock");
    m_inventory.GiveWeapon("Tokarev");
    //m_inventory.GiveWeapon("Remington870");
    //m_inventory.GiveWeapon("SPAS");
    //m_inventory.GiveWeapon("AKS74U");

    //m_inventory.GiveAmmo("12GaugeBuckShot", 80);
    m_inventory.GiveAmmo("Glock", 200);
    m_inventory.GiveAmmo("Tokarev", 200);
    //m_inventory.GiveAmmo("AKS74U", 200);

    //m_inventory.AddInventoryItem("BlackSkull");
    //m_inventory.AddInventoryItem("SmallKey");
    //m_inventory.AddInventoryItem("SmallKeySilver");
    //m_inventory.AddInventoryItem("Pills");
    //m_inventory.AddInventoryItem("ShotgunAmmo");

    //GiveSilencer("Glock");
    //GiveSight("GoldenGlock");

    //m_shopInventory.AddInventoryItem("SmallKey");
    //m_shopInventory.AddInventoryItem("SmallKeySilver");
    //m_shopInventory.AddInventoryItem("Pills");

}

void Player::NextWeapon() {
    std::vector<WeaponState>& weaponStates = m_inventory.GetWeaponStates();

    m_currentWeaponIndex++;
    if (m_currentWeaponIndex == weaponStates.size()) {
        m_currentWeaponIndex = 0;
    }
    while (!weaponStates[m_currentWeaponIndex].has) {
        m_currentWeaponIndex++;
        if (m_currentWeaponIndex == weaponStates.size()) {
            m_currentWeaponIndex = 0;
        }
    }
    SwitchWeapon(weaponStates[m_currentWeaponIndex].name, DRAW_BEGIN);

    // Handle me better
    if (weaponStates[m_currentWeaponIndex].name == "Glock") {
        std::vector<MeshNodeCreateInfo> meshNodeCreateInfoSet;
        m_redDot.Init(m_playerId, "RedDot", meshNodeCreateInfoSet);
        m_redDot.DisableMarkingStaticSceneBvhAsDirty();
        m_redDot.DisableCSMShadows();
        m_redDot.DisablePointLightShadows();

        m_supressor.Init(m_playerId, "Suppressor", meshNodeCreateInfoSet);
        m_supressor.DisableMarkingStaticSceneBvhAsDirty();
        m_supressor.DisableCSMShadows();
        m_supressor.DisablePointLightShadows();
    }
    else {
        m_redDot.CleanUp();
        m_supressor.CleanUp();
    }


    Bible::ConfigureP90MagazineMeshNodes(m_playerId, &m_p90MagMeshNodes);
}

const std::string& Player::GetViewWeaponModelName() {
    SkinnedGameObject* viewWeapon = GetViewWeaponSkinnedGameObject();
    if (viewWeapon && viewWeapon->GetSkinnedModel()) return viewWeapon->GetSkinnedModel()->GetName();

    static std::string invalid = UNDEFINED_STRING;
    return invalid;
}

void Player::SwitchWeapon(const std::string& name, WeaponAction weaponAction) {
    std::vector<WeaponState>& weaponStates = m_inventory.GetWeaponStates();

    AnimatorInstance* animatorInstance = Animator::GetAnimatorInstanceByObjectId(m_viewWeaponAnimatorInstanceId);
    WeaponInfo* weaponInfo = Bible::GetWeaponInfoByName(name);
    SkinnedGameObject* viewWeapon = GetViewWeaponSkinnedGameObject();

    if (!animatorInstance) return;
    if (!weaponInfo) return;
    if (!viewWeapon) return;

    for (int i = 0; i < weaponStates.size(); i++) {
        if (weaponStates[i].name == name) {
            m_currentWeaponIndex = i;
        }
    }

    const std::string newWeaponModelName = weaponInfo->modelName;
    const std::string oldWeaponModelName = GetViewWeaponModelName();

    // Register the first view weapon without destroying its animation layer
    if (oldWeaponModelName == UNDEFINED_STRING) {
        animatorInstance->RegisterSkinnedModels({ newWeaponModelName });
    }
    // Preserve the animation layer when switching view weapon models
    else if (oldWeaponModelName != newWeaponModelName) {
        animatorInstance->ReplaceSkinnedModel(oldWeaponModelName, newWeaponModelName);
    }

    viewWeapon->SetName(weaponInfo->itemInfoName);
    viewWeapon->SetSkinnedModel(weaponInfo->modelName);
    viewWeapon->EnableRendering();

    // Set materials
    for (auto& it : weaponInfo->meshMaterials) {
        viewWeapon->SetMeshMaterialByMeshName(it.first, it.second);
    }
    // Set materials by index
    for (auto& it : weaponInfo->meshMaterialsByIndex) {
        viewWeapon->SetMeshMaterialByMeshIndex(it.first, it.second);
    }
    // Hide mesh
    for (auto& meshName : weaponInfo->hiddenMeshAtStart) {
        viewWeapon->SetBlendingModeByMeshName(meshName, BlendingMode::DO_NOT_RENDER);
    }
    m_weaponAction = weaponAction;
}

WeaponType Player::GetCurrentWeaponType() {
    WeaponInfo* weaponInfo = GetCurrentWeaponInfo();
    if (weaponInfo) {
        return weaponInfo->type;
    }
    else {
        return WeaponType::UNDEFINED;
    }
}

WeaponAction Player::GetCurrentWeaponAction() {
    return m_weaponAction;
}

WeaponAction& Player::GetWeaponAction() {
    return m_weaponAction;
}

WeaponInfo* Player::GetCurrentWeaponInfo() {
    std::vector<WeaponState>& weaponStates = m_inventory.GetWeaponStates();
    return Bible::GetWeaponInfoByName(weaponStates[m_currentWeaponIndex].name);;
}

const std::string& Player::GetSelectedWeaponName() {
    static const std::string invalid = UNDEFINED_STRING;

    std::vector<WeaponState>& weaponStates = m_inventory.GetWeaponStates();
    if (m_currentWeaponIndex < 0 || m_currentWeaponIndex >= weaponStates.size()) return invalid;

    return weaponStates[m_currentWeaponIndex].name;
}

void Player::GiveWeapon(const std::string& name) {
    WeaponState* state = GetWeaponStateByName(name);
    WeaponInfo* weaponInfo = Bible::GetWeaponInfoByName(name);
    if (state && weaponInfo) {
        state->has = true;
        state->ammoInMag = weaponInfo->magSize;
    }
    else {
        std::cout << "FAILED TO GIVE PLAYER: " << name << "\n";
    }
}

void Player::GiveAmmo(const std::string& name, int amount) {
    AmmoState* state = GetAmmoStateByName(name);
    if (state) {
        state->ammoOnHand += amount;
    }
}

void Player::GiveSight(const std::string& weaponName) {
    WeaponInfo* weaponInfo = Bible::GetWeaponInfoByName(weaponName);
    WeaponState* state = GetWeaponStateByName(weaponName);
    if (state && weaponInfo) {
        state->hasSight = true;
    }
}

void Player::GiveSilencer(const std::string& weaponName) {
    WeaponInfo* weaponInfo = Bible::GetWeaponInfoByName(weaponName);
    WeaponState* state = GetWeaponStateByName(weaponName);
    if (state && weaponInfo) {
        state->hasSilencer = true;
    }
}

WeaponState* Player::GetWeaponStateByName(const std::string& name) {
    return m_inventory.GetWeaponStateByName(name);
}

AmmoState* Player::GetAmmoStateByName(const std::string& name) {
    return m_inventory.GetAmmoStateByName(name);
}

AmmoState* Player::GetCurrentAmmoState() {
    WeaponInfo* weaponInfo = GetCurrentWeaponInfo();
    if (!weaponInfo) return nullptr;

    return GetAmmoStateByName(weaponInfo->ammoInfoName);
}

AmmoInfo* Player::GetCurrentAmmoInfo() {
    WeaponInfo* weaponInfo = GetCurrentWeaponInfo();
    if (!weaponInfo) return nullptr;

    return Bible::GetAmmoInfoByName(weaponInfo->ammoInfoName);
}

WeaponState* Player::GetCurrentWeaponState() {
    WeaponInfo* weaponInfo = GetCurrentWeaponInfo();
    if (!weaponInfo) return nullptr;

    return GetWeaponStateByName(weaponInfo->itemInfoName);
}

int Player::GetCurrentWeaponMagAmmo() {
    WeaponInfo* weaponInfo = GetCurrentWeaponInfo();
    if (weaponInfo) {
        WeaponState* weaponState = GetWeaponStateByName(weaponInfo->itemInfoName);
        if (weaponState) {
            return weaponState->ammoInMag;
        }
    }
    return 0;
}

int Player::GetCurrentWeaponTotalAmmo() {
    AmmoState* ammoState = GetCurrentAmmoState();
    if (!ammoState) return 0;

    return ammoState->ammoOnHand;
}

void Player::SpawnMuzzleFlash(float speed, float scale) {
    m_muzzleFlash.SetSpeed(speed);
    m_muzzleFlash.SetScale(glm::vec3(scale));
    m_muzzleFlash.SetTime(0.0f);
    m_muzzleFlash.EnableRendering();
    m_muzzleFlash.SetRotation(glm::vec3(0.0f, 0.0f, Hell::Random::Float(0.0f, HELL_PI * 2)));
}

void Player::SpawnCasing() {
    SkinnedGameObject* viewWeapon = GetViewWeaponSkinnedGameObject();

    AmmoInfo* ammoInfo = GetCurrentAmmoInfo();
    WeaponInfo* weaponInfo = GetCurrentWeaponInfo();

    if (!ammoInfo) return;
    if (!weaponInfo) return;

    if (!Hell::String::Equals(ammoInfo->casingModelName, UNDEFINED_STRING)) {
        BulletCasingCreateInfo createInfo;
        createInfo.modelId = Hell::ResourceManager::GetModelIdByName(ammoInfo->casingModelName);
        createInfo.materialIndex = Hell::ResourceManager::GetMaterialIndexByName(ammoInfo->casingMaterialName);
        createInfo.position = viewWeapon->GetNodeWorldPosition(weaponInfo->casingEjectionBoneName);
        createInfo.rotation.y = m_camera.GetYaw() + (HELL_PI * 0.5f);
        createInfo.force = glm::normalize(GetCameraRight() + glm::vec3(0.0f, Hell::Random::Float(0.7f, 0.9f), 0.0f)) * glm::vec3(weaponInfo->casingEjectionImpulse);
    // createInfo.force = glm::normalize(GetCameraRight() + glm::vec3(0.0f, Hell::Random::Float(0.7f, 0.9f), 0.0f)) * glm::vec3(0.0175);
    // std::cout << "warning: you have hardcoded casing ejection impulse!\n";

        createInfo.position += GetCameraForward() * glm::vec3(0.15f);
        createInfo.position += GetCameraRight() * glm::vec3(0.05f);
        createInfo.position += GetCameraUp() * glm::vec3(-0.025f);

        //if (alternateAmmo) {
        //    createInfo.materialIndex = Hell::ResourceManager::GetMaterialIndexByName("ShellGreen");
        //}

        createInfo.mass = 0.008f;

        Unloved::World::AddBulletCasing(createInfo);


    }
    else {
        std::cout << "Player::SpawnCasing(AmmoInfo* ammoInfo) failed to spawn a casing coz invalid casing model name in weapon info\n";
    }
}

void Player::SpawnBullet(float variance) {
    // Spawn an underwater bullet if camera is underwater
    if (CameraIsUnderwater()) {
        SpawnUnderWaterBullet(variance);
        return;
    }

    // Otherwise spawn a regular bullet
    else {
        WeaponInfo* weaponInfo = GetCurrentWeaponInfo();

        glm::vec3 bulletDirection = GetCameraForward();
        bulletDirection.x += Hell::Random::Float(-(variance * 0.5f), variance * 0.5f);
        bulletDirection.y += Hell::Random::Float(-(variance * 0.5f), variance * 0.5f);
        bulletDirection.z += Hell::Random::Float(-(variance * 0.5f), variance * 0.5f);
        bulletDirection = glm::normalize(bulletDirection);

        BulletCreateInfo createInfo;
        const Config::Physics::Settings& physicsSettings = Config::Physics::GetSettings();
        createInfo.origin = GetCameraPosition();
        createInfo.direction = bulletDirection;
        createInfo.damage = weaponInfo->damage;
        createInfo.impactImpulse = weaponInfo->type == WeaponType::SHOTGUN
            ? physicsSettings.shotgunPelletImpactImpulse
            : physicsSettings.bulletImpactImpulse;
        createInfo.weaponIndex = Bible::GetWeaponIndexFromWeaponName(weaponInfo->itemInfoName);
        createInfo.ownerObjectId = m_playerId;

        Unloved::BulletSystem::AddBullet(createInfo);
    }
}

void Player::SpawnUnderWaterBullet(float variance) {
    WeaponInfo* weaponInfo = GetCurrentWeaponInfo();

    glm::vec3 vanishingPoint = GetCameraPosition() + (GetCameraForward() * 1000.0f);
    glm::vec3 origin = GetMuzzleFlashSpawnPosition() + GetCameraForward() * 0.05f;
    glm::vec3 bulletDirection = glm::normalize(vanishingPoint - GetCameraPosition());

    bulletDirection.x += Hell::Random::Float(-(variance * 0.5f), variance * 0.5f);
    bulletDirection.y += Hell::Random::Float(-(variance * 0.5f), variance * 0.5f);
    bulletDirection.z += Hell::Random::Float(-(variance * 0.5f), variance * 0.5f);
    bulletDirection = glm::normalize(bulletDirection);

    BulletCreateInfo createInfo;
    const Config::Physics::Settings& physicsSettings = Config::Physics::GetSettings();
    createInfo.origin = origin;
    createInfo.direction = bulletDirection;
    createInfo.damage = weaponInfo->damage;
    createInfo.impactImpulse = weaponInfo->type == WeaponType::SHOTGUN
        ? physicsSettings.shotgunPelletImpactImpulse
        : physicsSettings.bulletImpactImpulse;
    createInfo.weaponIndex = Bible::GetWeaponIndexFromWeaponName(weaponInfo->itemInfoName);
    createInfo.ownerObjectId = m_playerId;

    Unloved::BulletSystem::AddBulletTrail(createInfo);


    const std::vector<const char*> filenames = {
            "UnderwaterTrail0.wav",
            "UnderwaterTrail1.wav"
    };
    int random = rand() % filenames.size();
    Audio::PlayAudio(filenames[random], 5.0f);
}

void Player::UpdateWeaponSlide() {
    WeaponInfo* weaponInfo = GetCurrentWeaponInfo();
    WeaponState* weaponState = GetCurrentWeaponState();
    AnimatorInstance* animatorInstance = Animator::GetAnimatorInstanceByObjectId(m_viewWeaponAnimatorInstanceId);
    if (!weaponInfo) return;
    if (!weaponState) return;
    if (!animatorInstance) return;

    std::string& boneName = weaponInfo->pistolSlideBoneName;
    if (boneName == UNDEFINED_STRING) return;

    glm::vec3 translation = glm::vec3(0.0f);

    if (weaponState->requiresSlideOffset) {
        translation.z = -weaponInfo->pistolSlideOffset;
    }
    animatorInstance->SetNodeTranslationOffset(boneName, translation);
}

void Player::DropItems() {
    for (const Unloved::InventoryItem& item : m_inventory.GetItems()) {
        ItemType itemType = Bible::GetItemType(m_name);
        if (itemType == ItemType::HEAL) {
            DiscardItem(item.m_name);
        }
    }
}

void Player::DropWeapons() {
    for (WeaponState& weaponState : m_inventory.GetWeaponStates()) {
        // Skip the knife
        if (weaponState.name == "Knife")
            continue;

        if (weaponState.has) {

            WeaponInfo* weaponInfo = Bible::GetWeaponInfoByName(weaponState.name);
            if (!weaponInfo) {
                std::cout << "You tried to drop a weapon with an invalid name somehow...\n";
                continue;
            }

            if (weaponInfo->itemInfoName != "") {
                PickUpCreateInfo createInfo;
                createInfo.position = GetCameraPosition();
                createInfo.rotation.x = Hell::Random::Float(-HELL_PI, HELL_PI);
                createInfo.rotation.y = Hell::Random::Float(-HELL_PI, HELL_PI);
                createInfo.rotation.z = Hell::Random::Float(-HELL_PI, HELL_PI);
                createInfo.name = weaponInfo->itemInfoName;
                createInfo.saveToFile = false;
                createInfo.disablePhysicsAtSpawn = false;
                createInfo.respawn = false;
                createInfo.type = Bible::GetItemType(weaponInfo->itemInfoName);

                glm::vec3 force = glm::vec3(0.0f);
                force.x = Hell::Random::Float(-HELL_PI * 0.5f, HELL_PI * 0.5f);
                force.y = 1.0f;
                force.z = Hell::Random::Float(-HELL_PI * 0.5f, HELL_PI * 0.5f);
                force = glm::normalize(force);
                force *= 200.0f;

                uint64_t id = Unloved::World::AddPickUp(createInfo);
                if (PickUp* pickUp = Unloved::World::GetPickUpByObjectId(id)) {
                    pickUp->GetMeshNodes().AddForceToPhsyics(force);
                    //std::cout << "Tried to add force to " << weaponInfo->itemInfoName << "\n";
                }
            }
        }
    }
}

void Player::BeginMeleeAttack(AnimationSlot animationSlot) {
    m_meleeAttackState.active = false;

    WeaponInfo* weaponInfo = GetCurrentWeaponInfo();
    if (!weaponInfo) return;

    for (const MeleeAttackInfo& attackInfo : weaponInfo->meleeAttacks) {
        if (attackInfo.animationSlot != animationSlot) continue;
        if (attackInfo.endFrame < attackInfo.startFrame) return;

        m_meleeAttackState.active = true;
        m_meleeAttackState.attackInfo = attackInfo;
        m_meleeAttackState.weaponAction = m_weaponAction;
        m_meleeAttackState.weaponName = weaponInfo->itemInfoName;
        m_meleeAttackState.lastSampledFrame = static_cast<int32_t>(attackInfo.startFrame) - 1;
        m_meleeAttackState.hitGroupId = BulletSystem::CreateHitGroup();
        return;
    }
}

void Player::UpdateMeleeAttack() {
    if (!m_meleeAttackState.active) return;

    WeaponInfo* weaponInfo = GetCurrentWeaponInfo();
    if (!weaponInfo ||
        weaponInfo->itemInfoName != m_meleeAttackState.weaponName ||
        m_weaponAction != m_meleeAttackState.weaponAction) {
        m_meleeAttackState.active = false;
        return;
    }

    const MeleeAttackInfo& attackInfo = m_meleeAttackState.attackInfo;
    const uint32_t currentFrame = GetViewWeaponAnimationFrameNumber();
    const uint32_t finalFrameToSample = std::min(currentFrame, attackInfo.endFrame);
    const uint32_t nextFrameToSample = std::max(
        attackInfo.startFrame,
        static_cast<uint32_t>(m_meleeAttackState.lastSampledFrame + 1));

    for (uint32_t frame = nextFrameToSample; frame <= finalFrameToSample; ++frame) {
        SpawnMeleeHitSample(frame);
        m_meleeAttackState.lastSampledFrame = static_cast<int32_t>(frame);
    }

    if (currentFrame >= attackInfo.endFrame) {
        m_meleeAttackState.active = false;
    }
}

void Player::SpawnMeleeHitSample(uint32_t animationFrame) {
    constexpr int PROBE_COUNT = 5;

    const MeleeAttackInfo& attackInfo = m_meleeAttackState.attackInfo;
    const Config::Physics::Settings& physicsSettings = Config::Physics::GetSettings();
    const uint32_t frameCount = attackInfo.endFrame - attackInfo.startFrame + 1;
    const float sweepProgress = frameCount > 1
        ? static_cast<float>(animationFrame - attackInfo.startFrame) / static_cast<float>(frameCount - 1)
        : 0.5f;
    const float horizontalOffset = glm::mix(attackInfo.width * 0.5f, -attackInfo.width * 0.5f, sweepProgress);

    for (int probeIndex = 0; probeIndex < PROBE_COUNT; ++probeIndex) {
        const float verticalProgress = static_cast<float>(probeIndex) / static_cast<float>(PROBE_COUNT - 1);
        const float verticalOffset = glm::mix(-attackInfo.height * 0.5f, attackInfo.height * 0.5f, verticalProgress);

        BulletCreateInfo createInfo;
        createInfo.origin = GetCameraPosition();
        createInfo.origin += GetCameraRight() * horizontalOffset;
        createInfo.origin += GetCameraUp() * verticalOffset;
        createInfo.direction = GetCameraForward();
        createInfo.weaponIndex = -1;
        createInfo.damage = attackInfo.damage;
        createInfo.ownerObjectId = m_playerId;
        createInfo.hitGroupId = m_meleeAttackState.hitGroupId;
        createInfo.rayLength = attackInfo.range;
        createInfo.impactImpulse = physicsSettings.meleeImpactImpulse;
        createInfo.createsDecals = false;
        createInfo.createsFollowThroughBulletOnGlassHit = false;
        createInfo.playsPiano = false;
        createInfo.createsDecalTexturePaintedWounds = false;

        BulletSystem::AddBullet(createInfo);
    }
}

} // namespace Unloved
