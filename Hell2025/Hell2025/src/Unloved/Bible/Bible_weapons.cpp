#include "Unloved/Bible/Bible.h"

namespace Unloved::Bible {

    void InitWeaponInfo() {
        WeaponInfo& aks74u = CreateWeaponInfo("AKS74U");
        aks74u.weapon = Weapon::AKS74U;
        aks74u.viewSkinnedModelPreset = SkinnedModelPreset::VIEW_WEAPON_AKS74U;
        aks74u.viewWeaponAnimationProfile = AnimationProfile::VIEW_WEAPON_AKS74U;
        aks74u.type = WeaponType::AUTOMATIC;
        aks74u.damage = 25;
        aks74u.ammo = Ammo::AKS74U;
        aks74u.magSize = 30;
        aks74u.muzzleFlashBoneName = "Weapon";
        aks74u.casingEjectionBoneName = "SlideCatch";
        aks74u.animationCancelFrames.fire = 3.0f;
        aks74u.animationCancelFrames.reload = 80.0f;
        aks74u.animationCancelFrames.reloadFromEmpty = 95.0f;
        aks74u.animationCancelFrames.draw = 75.0f;
        aks74u.animationCancelFrames.adsFire = 3.0f;
        aks74u.audioFiles.fire.push_back("AKS74U_Fire0.wav");
        aks74u.audioFiles.fire.push_back("AKS74U_Fire1.wav");
        aks74u.audioFiles.fire.push_back("AKS74U_Fire2.wav");
        aks74u.audioFiles.fire.push_back("AKS74U_Fire3.wav");
        aks74u.audioFiles.reload = "AKS74U_Reload.wav";
        aks74u.audioFiles.reloadEmpty = "AKS74U_ReloadEmpty.wav";
        aks74u.audioFiles.drawFirst = "Tokarev_DrawFirst.wav";
        aks74u.animationSpeeds.fire = 1.825f;
        aks74u.animationSpeeds.adsFire = 1.825f;
        aks74u.animationSpeeds.draw = 1.0f;
        aks74u.animationSpeeds.adsIn = 3.5f;
        aks74u.animationSpeeds.adsOut = 3.5f;
        aks74u.muzzleFlashBoneName = "Muzzle";
        aks74u.casingEjectionBoneName = "EjectionPort";
        aks74u.muzzleFlashScale = 1.5f;
        aks74u.casingEjectionImpulse = 0.0175f;
        aks74u.reloadMagInFrameNumber = 23;
        aks74u.reloadEmptyMagInFrameNumber = 21;
        aks74u.hasADS = true;

        aks74u.animationCancelFrames.secondaryMelee = 20; // TODO move me into the Melee Attack Info
        aks74u.meleeAttacks = {
            {
                .animationSlot = AnimationSlot::MELEE,
                .startFrame = 9,
                .endFrame = 15,
                .damage = 34,
                .range = 1.5f,
                .width = 0.5f,
                .height = 0.2f
            }
        };

        WeaponInfo& glock = CreateWeaponInfo("Glock");
        glock.viewSkinnedModelPreset = SkinnedModelPreset::VIEW_WEAPON_GLOCK;
        glock.characterSkinnedModelPreset = SkinnedModelPreset::CHARACTER_GLOCK;
        glock.weapon = Weapon::GLOCK;
        glock.ammo = Ammo::GLOCK;
        glock.viewWeaponAnimationProfile = AnimationProfile::VIEW_WEAPON_GLOCK;
        glock.animationSpeeds.fire = 1.5f;
        glock.animationSpeeds.adsFire = 1.5f;
        glock.muzzleFlashBoneName = "Muzzle";
        glock.casingEjectionBoneName = "EjectionPort";
        glock.pistolSlideBoneName = "Slide";
        glock.pistolSlideOffset = 0.05;
        glock.reloadMagInFrameNumber = 20;
        glock.reloadEmptyMagInFrameNumber = 20;
        glock.audioFiles.fire.push_back("Glock_Fire0.wav");
        glock.audioFiles.fire.push_back("Glock_Fire1.wav");
        glock.audioFiles.fire.push_back("Glock_Fire2.wav");
        glock.audioFiles.fire.push_back("Glock_Fire3.wav");

        //glock.audioFiles.fire.clear();
		//glock.audioFiles.fire.push_back("Glock_FireSuppressed0.wav");
		//glock.audioFiles.fire.push_back("Glock_FireSuppressed1.wav");
		//glock.audioFiles.fire.push_back("Glock_FireSuppressed2.wav");

        glock.audioFiles.reload = "Glock_Reload.wav";
        glock.audioFiles.reloadEmpty = "Glock_ReloadEmpty.wav";
        glock.audioFiles.drawFirst = "Glock_DrawFirst.wav";
        glock.type = WeaponType::PISTOL;
        glock.animationCancelFrames.draw = 50;
        glock.animationCancelFrames.fire = 2;
        glock.animationCancelFrames.adsFire = 2;
        glock.animationCancelFrames.reload = 40;
        glock.animationCancelFrames.reloadFromEmpty = 40;
        glock.animationCancelFrames.draw = 75.0f;
        glock.damage = 15;
        glock.magSize = 15;
        glock.emptyReloadRequiresSlideOffset = true;
        glock.hasADS = true;
        glock.animationSpeeds.adsIn = 3.0f;
        glock.animationSpeeds.adsOut = 3.0f;
        glock.casingEjectionImpulse = 0.0175f;
        glock.silencerName = "GLOCK_SILENCER";
        glock.sightName = "GLOCK_RED_DOT";


        glock.animationCancelFrames.secondaryMelee = 24; // TODO move me into the Melee Attack Info
        glock.meleeAttacks = {
            {
                .animationSlot = AnimationSlot::MELEE,
                .startFrame = 11,
                .endFrame = 17,
                .damage = 34,
                .range = 1.5f,
                .width = 0.5f,
                .height = 0.2f
            }
        };

        WeaponInfo& goldeneGlock = CreateWeaponInfo("GoldenGlock");
        goldeneGlock.viewSkinnedModelPreset = SkinnedModelPreset::VIEW_WEAPON_GOLDEN_GLOCK;
        goldeneGlock.weapon = Weapon::GOLDEN_GLOCK;
        goldeneGlock.characterSkinnedModelPreset = SkinnedModelPreset::CHARACTER_GOLDEN_GLOCK;
        goldeneGlock.ammo = Ammo::GLOCK;
		goldeneGlock.viewWeaponAnimationProfile = AnimationProfile::VIEW_WEAPON_GLOCK;
        goldeneGlock.animationSpeeds.fire = 1.5f;
        goldeneGlock.animationSpeeds.adsFire = 1.5f;
        goldeneGlock.muzzleFlashBoneName = "Muzzle";
        goldeneGlock.casingEjectionBoneName = "EjectionPort";
        goldeneGlock.pistolSlideBoneName = "Slide";
        goldeneGlock.pistolSlideOffset = 0.05;
        goldeneGlock.reloadMagInFrameNumber = 20;
        goldeneGlock.reloadEmptyMagInFrameNumber = 20;
        goldeneGlock.audioFiles.fire.push_back("Glock_Fire0.wav");
        goldeneGlock.audioFiles.fire.push_back("Glock_Fire1.wav");
        goldeneGlock.audioFiles.fire.push_back("Glock_Fire2.wav");
        goldeneGlock.audioFiles.fire.push_back("Glock_Fire3.wav");
        goldeneGlock.audioFiles.reload = "Glock_Reload.wav";
        goldeneGlock.audioFiles.reloadEmpty = "Glock_ReloadEmpty.wav";
        goldeneGlock.audioFiles.drawFirst = "Glock_DrawFirst.wav";
        goldeneGlock.type = WeaponType::PISTOL;
        goldeneGlock.animationCancelFrames.draw = 50;
        goldeneGlock.animationCancelFrames.fire = 2;
        goldeneGlock.animationCancelFrames.adsFire = 2;
        goldeneGlock.animationCancelFrames.reload = 40;
        goldeneGlock.animationCancelFrames.reloadFromEmpty = 40;
        goldeneGlock.animationCancelFrames.draw = 75.0f;
        goldeneGlock.damage = 16;
        goldeneGlock.magSize = 80;
        goldeneGlock.emptyReloadRequiresSlideOffset = true;
        goldeneGlock.hasADS = true;
        goldeneGlock.animationSpeeds.adsIn = 3.0f;
        goldeneGlock.animationSpeeds.adsOut = 3.0f;
        goldeneGlock.casingEjectionImpulse = 0.0175f;
        goldeneGlock.silencerName = "GLOCK_SILENCER";
        goldeneGlock.sightName = "GLOCK_RED_DOT";
        goldeneGlock.pistolIsAuto = true;

        goldeneGlock.animationCancelFrames.secondaryMelee = 24; // TODO move me into the Melee Attack Info
        goldeneGlock.meleeAttacks = {
            {
                .animationSlot = AnimationSlot::MELEE,
                .startFrame = 11,
                .endFrame = 17,
                .damage = 34,
                .range = 1.5f,
                .width = 0.5f,
                .height = 0.2f
            }
        };

        WeaponInfo& knife = CreateWeaponInfo("Knife");
        knife.viewSkinnedModelPreset = SkinnedModelPreset::VIEW_WEAPON_KNIFE;
        knife.weapon = Weapon::KNIFE;
        knife.viewWeaponAnimationProfile = AnimationProfile::VIEW_WEAPON_KNIFE;
        knife.type = WeaponType::MELEE;
        knife.damage = 20;
        knife.animationCancelFrames.fire = 6;
        knife.meleeAttacks = {
            {
                .animationSlot = AnimationSlot::FIRE_1,
                .startFrame = 2,
                .endFrame = 6,
                .damage = 20,
                .range = 1.5f,
                .width = 0.5f,
                .height = 0.2f
            },
            {
                .animationSlot = AnimationSlot::FIRE_2,
                .startFrame = 2,
                .endFrame = 6,
                .damage = 20,
                .range = 1.5f,
                .width = 0.5f,
                .height = 0.2f
            },
            {
                .animationSlot = AnimationSlot::FIRE_3,
                .startFrame = 2,
                .endFrame = 6,
                .damage = 20,
                .range = 1.5f,
                .width = 0.5f,
                .height = 0.2f
            }
        };
        knife.audioFiles.fire.push_back("Knife.wav");

        WeaponInfo& tokarev = CreateWeaponInfo("Tokarev");
        tokarev.viewSkinnedModelPreset = SkinnedModelPreset::VIEW_WEAPON_TOKAREV;
        tokarev.weapon = Weapon::TOKAREV;
        tokarev.viewWeaponAnimationProfile = AnimationProfile::VIEW_WEAPON_TOKAREV;
        tokarev.animationSpeeds.fire = 1.5f;
        tokarev.audioFiles.fire.push_back("Tokarev_Fire0.wav");
        tokarev.audioFiles.fire.push_back("Tokarev_Fire1.wav");
        tokarev.audioFiles.fire.push_back("Tokarev_Fire2.wav");
        tokarev.audioFiles.fire.push_back("Tokarev_Fire3.wav");
        tokarev.audioFiles.reload = "Tokarev_Reload.wav";
        tokarev.audioFiles.reloadEmpty = "Tokarev_ReloadEmpty.wav";
        tokarev.audioFiles.drawFirst = "Tokarev_DrawFirst.wav";
        tokarev.type = WeaponType::PISTOL;
        tokarev.muzzleFlashBoneName = "Muzzle";
        tokarev.casingEjectionBoneName = "Ejection";
        tokarev.damage = 35;
        tokarev.magSize = 15;
        tokarev.ammo = Ammo::TOKAREV;
        tokarev.animationCancelFrames.draw = 50.0f;
        tokarev.animationCancelFrames.fire = 5;
        tokarev.animationCancelFrames.adsFire = 5;
        tokarev.animationCancelFrames.reload = 40.0f;
        tokarev.animationCancelFrames.reloadFromEmpty = 40.0f;
        tokarev.animationCancelFrames.draw = 75.0f;
        tokarev.pistolSlideBoneName = "Slide";
        //tokarev.pistolSlideOffset = 5;
        tokarev.pistolSlideOffset = 0.05;
        tokarev.reloadMagInFrameNumber = 20;
        tokarev.reloadEmptyMagInFrameNumber = 20;
        tokarev.emptyReloadRequiresSlideOffset = true;
        tokarev.hasADS = true;
        tokarev.casingEjectionImpulse = 0.0175f;

        tokarev.animationSpeeds.adsIn = 3.0f;
        tokarev.animationSpeeds.adsOut = 3.0f;

        tokarev.animationCancelFrames.secondaryMelee = 24; // TODO move me into the Melee Attack Info
        tokarev.meleeAttacks = {
            {
                .animationSlot = AnimationSlot::MELEE,
                .startFrame = 11,
                .endFrame = 17,
                .damage = 34,
                .range = 1.5f,
                .width = 0.5f,
                .height = 0.2f
            }
        };


        WeaponInfo& spas = CreateWeaponInfo("SPAS");
        spas.viewSkinnedModelPreset = SkinnedModelPreset::VIEW_WEAPON_SPAS;
        spas.weapon = Weapon::SPAS;
        spas.viewWeaponAnimationProfile = AnimationProfile::VIEW_WEAPON_SPAS;

        spas.audioFiles.fire.push_back("SPAS_Fire.wav");
        spas.audioFiles.shotgunPump = "SPAS_Pump.wav";

        spas.type = WeaponType::SHOTGUN;
        spas.damage = 15;
        spas.magSize = 8;
        spas.ammo = Ammo::SHOTGUN_SHELLS;
        spas.muzzleFlashBoneName = "Muzzle";
        spas.casingEjectionBoneName = "Shell_bone";
        spas.casingEjectionImpulse = 0.0175f;
        spas.hasAutoSwitch = true;

        spas.animationCancelFrames.fire = 21;
        spas.animationCancelFrames.dryFire = 5;
        spas.animationCancelFrames.fireAutoShotgun = 4;
        spas.animationCancelFrames.secondaryMelee = 28; // TODO move me into the Melee Attack Info
        spas.meleeAttacks = {
            {
                .animationSlot = AnimationSlot::MELEE,
                .startFrame = 8,
                .endFrame = 14,
                .damage = 34,
                .range = 1.5f,
                .width = 0.5f,
                .height = 0.2f
            }
        };

        WeaponInfo& remington870 = CreateWeaponInfo("Remington870");
        remington870.viewSkinnedModelPreset = SkinnedModelPreset::VIEW_WEAPON_REMINGTON_870;
        remington870.weapon = Weapon::REMINGTON_870;
        remington870.viewWeaponAnimationProfile = AnimationProfile::VIEW_WEAPON_REMINGTON870;
        remington870.animationSpeeds.melee = 1.25f;
        remington870.audioFiles.fire.push_back("Shotgun_Fire.wav");
        remington870.audioFiles.shotgunPump = "Remington870_Pump.wav";
        remington870.type = WeaponType::SHOTGUN;
        remington870.damage = 14;
        remington870.magSize = 8;
        remington870.ammo = Ammo::SHOTGUN_SHELLS;
        remington870.muzzleFlashBoneName = "Muzzle";
        remington870.casingEjectionBoneName = "Shell_bone";
        remington870.casingEjectionImpulse = 0.0175f;
        remington870.hasAutoSwitch = false;
        remington870.animationSpeeds.shotgunReloadStart = 1.0f;
        remington870.animationSpeeds.shotgunReloadEnd = 1.0f;
        remington870.animationSpeeds.shotgunReloadOneShell = 1.0f;
        remington870.animationSpeeds.shotgunReloadTwoShells = 1.0f;

        remington870.animationCancelFrames.fire = 21;
        remington870.animationCancelFrames.dryFire = 5;
        remington870.animationCancelFrames.secondaryMelee = 28; // TODO move me into the Melee Attack Info
        remington870.meleeAttacks = {
            {
                .animationSlot = AnimationSlot::MELEE,
                .startFrame = 8,
                .endFrame = 14,
                .damage = 34,
                .range = 1.5f,
                .width = 0.5f,
                .height = 0.2f
            }
        };

        WeaponInfo& p90 = CreateWeaponInfo("P90");
        p90.viewSkinnedModelPreset = SkinnedModelPreset::VIEW_WEAPON_P90;
        p90.weapon = Weapon::P90;
        p90.viewWeaponAnimationProfile = AnimationProfile::VIEW_WEAPON_P90;
        p90.type = WeaponType::AUTOMATIC;
        p90.damage = 20;
        p90.hasADS = true;
        p90.ammo = Ammo::P90;
        p90.magSize = 50;

        p90.casingEjectionBoneName = "Ejection_Port";
        p90.animationCancelFrames.fire = 15.0f;
        p90.animationCancelFrames.reload = 95.0f;
        p90.animationCancelFrames.reloadFromEmpty = 95.0f;
        p90.animationCancelFrames.draw = 75.0f;
        p90.animationCancelFrames.adsFire = 10.0;
        p90.audioFiles.fire.push_back("P90_Fire0.wav");
        p90.audioFiles.fire.push_back("P90_Fire1.wav");
        p90.audioFiles.fire.push_back("P90_Fire2.wav");
        p90.audioFiles.fire.push_back("P90_Fire3.wav");
        p90.audioFiles.reload = "P90_Reload.wav";
        p90.audioFiles.reloadEmpty = "P90_ReloadEmpty.wav";
        p90.audioFiles.drawFirst = "Tokarev_DrawFirst.wav";
        p90.casingEjectionBoneName = "Ejection_Port";  // FIGURE THIS OUT LATER
        p90.casingEjectionBoneName = "Weapon";
        p90.muzzleFlashScale = 1.5f;
        p90.casingEjectionImpulse = 0.0175f;
        p90.reloadMagInFrameNumber = 23;
        p90.reloadEmptyMagInFrameNumber = 21;

		p90.animationCancelFrames.fire = 3.0f;
		p90.animationCancelFrames.reload = 80.0f;
		p90.animationCancelFrames.reloadFromEmpty = 95.0f;
		p90.animationCancelFrames.draw = 75.0f;
		p90.animationCancelFrames.adsFire = 3.0f;
		p90.animationSpeeds.fire = 2.5f;
		p90.animationSpeeds.reload = 1.125f;
		p90.animationSpeeds.reloadempty = 1.125f;
		p90.animationSpeeds.adsFire = 2.5f;
		p90.animationSpeeds.draw = 1.225f;
		p90.animationSpeeds.adsIn = 3.0f;
		p90.animationSpeeds.adsOut = 3.0f;

		p90.muzzleFlashBoneName = "Muzzle";

        p90.animationCancelFrames.secondaryMelee = 20; // TODO move me into the Melee Attack Info
        p90.meleeAttacks = {
            {
                .animationSlot = AnimationSlot::MELEE,
                .startFrame = 9,
                .endFrame = 15,
                .damage = 34,
                .range = 1.5f,
                .width = 0.5f,
                .height = 0.2f
            }
        };
    }


    int32_t GetWeaponMagSize(const std::string& name) {
        if (WeaponInfo* weaponInfo = GetWeaponInfoByName(name)) {
            return weaponInfo->magSize;
        }
        return 0;
    }
}
