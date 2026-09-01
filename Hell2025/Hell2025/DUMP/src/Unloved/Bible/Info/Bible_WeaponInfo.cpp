#include "Unloved/Bible/Bible.h"
#include "Hell/Logging.h"
#include <unordered_map>

namespace Bible {

    void InitWeaponInfo() {
        WeaponInfo& aks74u = CreateWeaponInfo("AKS74U");
        aks74u.itemInfoName = "AKS74U";
        aks74u.modelName = "AKS74U";
        aks74u.viewWeaponAnimationProfile = AnimationProfile::VIEW_WEAPON_AKS74U;
        aks74u.type = WeaponType::AUTOMATIC;
        aks74u.damage = 25;
        aks74u.meshMaterials["ArmsMale"] = "ArmsMale";
        aks74u.meshMaterials["ArmsFemale"] = "FemaleArms";
        aks74u.meshMaterials["AKS74UBarrel"] = "AKS74U_4";
        aks74u.meshMaterials["AKS74UBolt"] = "AKS74U_1";
        aks74u.meshMaterials["AKS74UHandGuard"] = "AKS74U_0";
        aks74u.meshMaterials["AKS74UMag"] = "AKS74U_3";
        aks74u.meshMaterials["AKS74UPistolGrip"] = "AKS74U_2";
        aks74u.meshMaterials["AKS74UReceiver"] = "AKS74U_1";
        aks74u.meshMaterials["AKS74U_Lens"] = "Black";
        aks74u.meshMaterials["AKS74U_RedDot"] = "Black";
        aks74u.meshMaterials["AKS74U_ScopeBackCap"] = "Black";
        aks74u.meshMaterials["AKS74U_ScopeFrontCap"] = "Black";
        aks74u.meshMaterials["AKS74U_ScopeMain"] = "Black";
        aks74u.meshMaterials["AKS74U_ScopeMain2"] = "Black";
        aks74u.meshMaterials["AKS74U_ScopeSupport"] = "Black";
        aks74u.hiddenMeshAtStart.push_back("ArmsFemale");
        aks74u.hiddenMeshAtStart.push_back("AKS74U_Lens");
        aks74u.hiddenMeshAtStart.push_back("AKS74U_RedDot");
        aks74u.hiddenMeshAtStart.push_back("AKS74U_ScopeBackCap");
        aks74u.hiddenMeshAtStart.push_back("AKS74U_ScopeFrontCap");
        aks74u.hiddenMeshAtStart.push_back("AKS74U_ScopeMain");
        aks74u.hiddenMeshAtStart.push_back("AKS74U_ScopeMain2");
        aks74u.hiddenMeshAtStart.push_back("AKS74U_ScopeSupport");
        aks74u.ammoInfoName = "AKS74U";
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


        WeaponInfo& glock = CreateWeaponInfo("Glock");
        glock.hiddenMeshAtStart.push_back("ArmsFemale");
        glock.hiddenMeshAtStart.push_back("LeupoldRedDot");
        glock.hiddenMeshAtStart.push_back("LeupoldRedDotGlass");
        glock.hiddenMeshAtStart.push_back("Supressor");
        glock.itemInfoName = "Glock";
        glock.ammoInfoName = "Glock";
        glock.modelName = "Glock";
        glock.viewWeaponAnimationProfile = AnimationProfile::VIEW_WEAPON_GLOCK;
        glock.meshMaterials["GlockBarrel"] = "Glock";
        glock.meshMaterials["GlockMagazine"] = "Glock";
        glock.meshMaterials["GlockMagazine_02"] = "Glock";
        glock.meshMaterials["GlockReceiver"] = "Glock";
        glock.meshMaterials["GlockSlide"] = "Glock";
        glock.meshMaterials["GlockSlideUnLock"] = "Glock";
        glock.meshMaterials["GlockTrigger"] = "Glock";
        glock.meshMaterials["LeupoldRedDot"] = "RedDot";           // Red Dot
        glock.meshMaterials["LeupoldRedDotGlass"] = "RedDotGlass"; // Red Dot
        glock.meshMaterials["Supressor"] = "Suppressor";        // Suppressor
        glock.meshMaterials["ArmsMale"] = "ArmsMale";                 // Arms
        glock.meshMaterials["ArmsFemale"] = "FemaleArms";          // Arms
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

        WeaponInfo& goldeneGlock = CreateWeaponInfo("GoldenGlock");
        goldeneGlock.hiddenMeshAtStart.push_back("ArmsFemale");
        goldeneGlock.itemInfoName = "GoldenGlock";
        goldeneGlock.ammoInfoName = "Glock";
		goldeneGlock.modelName = "Glock";
		goldeneGlock.viewWeaponAnimationProfile = AnimationProfile::VIEW_WEAPON_GLOCK;
		goldeneGlock.hiddenMeshAtStart.push_back("LeupoldRedDot");
		goldeneGlock.hiddenMeshAtStart.push_back("LeupoldRedDotGlass");
		goldeneGlock.hiddenMeshAtStart.push_back("Supressor");

        goldeneGlock.meshMaterials["GlockBarrel"] = "GlockGold";
        goldeneGlock.meshMaterials["GlockMagazine"] = "GlockGold";
        goldeneGlock.meshMaterials["GlockMagazine_02"] = "GlockGold";
        goldeneGlock.meshMaterials["GlockReceiver"] = "GlockGold";
        goldeneGlock.meshMaterials["GlockSlide"] = "GlockGold";
        goldeneGlock.meshMaterials["GlockSlideUnLock"] = "GlockGold";
        goldeneGlock.meshMaterials["GlockTrigger"] = "GlockGold";
        goldeneGlock.meshMaterials["LeupoldRedDot"] = "RedDotGold";           // Red Dot
        goldeneGlock.meshMaterials["LeupoldRedDotGlass"] = "RedDotGlass"; // Red Dot
        goldeneGlock.meshMaterials["Supressor"] = "SuppressorGold";        // Suppressor
        goldeneGlock.meshMaterials["ArmsMale"] = "ArmsMale";                 // Arms
        goldeneGlock.meshMaterials["ArmsFemale"] = "FemaleArms";          // Arms
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

        //WeaponInfo& glock = CreateWeaponInfo("Glock");
        //glock.itemInfoName = "Glock";
        //glock.modelName = "Glock";
        //glock.meshMaterials["Glock"] = "Glock";
        //glock.meshMaterials["ArmsMale"] = "ArmsMale";
        //glock.meshMaterials["ArmsFemale"] = "FemaleArms";
        //glock.meshMaterials["Glock_silencer"] = "Silencer";
        //glock.meshMaterials["RedDotSight"] = "RedDotSight";
        //glock.meshMaterials["RedDotSightGlass"] = "RedDotSight";
        //glock.hiddenMeshAtStart.push_back("ArmsFemale");
        //glock.hiddenMeshAtStart.push_back("Glock_silencer");
        //glock.animationSpeeds.fire = 1.5f;
        //glock.animationSpeeds.adsFire = 1.5f;
        //glock.audioFiles.fire.push_back("Glock_Fire0.wav");
        //glock.audioFiles.fire.push_back("Glock_Fire1.wav");
        //glock.audioFiles.fire.push_back("Glock_Fire2.wav");
        //glock.audioFiles.fire.push_back("Glock_Fire3.wav");
        //glock.audioFiles.reload = "Glock_Reload.wav";
        //glock.audioFiles.reloadEmpty = "Glock_ReloadEmpty.wav";
        //glock.audioFiles.drawFirst = "Glock_DrawFirst.wav";
        //glock.type = WeaponType::PISTOL;
        //glock.muzzleFlashBoneName = "Muzzle";
        //glock.casingEjectionBoneName = "EjectionPort";
        //glock.muzzleFlashOffset = glm::vec3(0, 0.002, 0.005f);
        //glock.casingEjectionOffset = glm::vec3(-0.098, -0.033, 0.238);
        //glock.damage = 15;
        //glock.magSize = 15;
        //glock.ammoInfoName = "Glock";
        //glock.animationCancelFrames.draw = 50.0f;
        //glock.animationCancelFrames.fire = 5;
        //glock.animationCancelFrames.adsFire = 6.0f;
        //glock.animationCancelFrames.reload = 40.0f;
        //glock.animationCancelFrames.reloadFromEmpty = 40.0f;
        //glock.animationCancelFrames.draw = 75.0f;
        //glock.pistolSlideBoneName = "Slide";
        //glock.pistolSlideOffset = 5;
        //glock.reloadMagInFrameNumber = 20;
        //glock.reloadEmptyMagInFrameNumber = 20;
        //glock.emptyReloadRequiresSlideOffset = true;
        //glock.hasADS = true;
        //glock.animationSpeeds.adsIn = 3.0f;
        //glock.animationSpeeds.adsOut = 3.0f;
        //glock.casingEjectionImpulse = 0.0175f;
        //glock.silencerName = "GLOCK_SILENCER";
        //glock.sightName = "GLOCK_RED_DOT";


        WeaponInfo& knife = CreateWeaponInfo("Knife");
        knife.itemInfoName = "Knife";
        knife.modelName = "Knife";
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
        knife.meshMaterials["Knife"] = "Knife";
        knife.meshMaterials["ArmsMale"] = "ArmsMale";
        knife.meshMaterials["ArmsFemale"] = "FemaleArms";
        //knife.hiddenMeshAtStart.push_back("ArmsMale");
        knife.hiddenMeshAtStart.push_back("ArmsFemale");

        //WeaponInfo& smith = CreateWeaponInfo("Smith & Wesson");
        //smith.type = WeaponType::PISTOL;
        //smith.damage = 50;
        //smith.modelName = "Smith";
        //smith.animationSpeeds.fire = 1.0f;
        //smith.meshMaterials["ArmsMale"] = "ArmsMale";
        //smith.meshMaterials["ArmsFemale"] = "FemaleArms";
        //smith.meshMaterials["Smith"] = "Smith";
        //smith.meshMaterials["LoadedBullet"] = "SmithBullet";
        //smith.meshMaterials["Bullet_0"] = "SmithBullet";
        //smith.meshMaterials["Bullet_1"] = "SmithBullet";
        //smith.meshMaterials["Bullet_2"] = "SmithBullet";
        //smith.meshMaterials["Bullet_3"] = "SmithBullet";
        //smith.meshMaterials["Bullet_4"] = "SmithBullet";
        //smith.meshMaterials["Bullet_5"] = "SmithBullet";
        //smith.hiddenMeshAtStart.push_back("ArmsFemale");
        //smith.audioFiles.fire.push_back("Smith_Fire0.wav");
        //smith.audioFiles.fire.push_back("Smith_Fire1.wav");
        //smith.audioFiles.fire.push_back("Smith_Fire2.wav");
        //smith.audioFiles.revolverCocks.push_back("Smith_Cock0.wav");
        //smith.audioFiles.revolverCocks.push_back("Smith_Cock1.wav");
        //smith.audioFiles.revolverCocks.push_back("Smith_Cock2.wav");
        //smith.muzzleFlashBoneName = "muzzle";
        //smith.type = WeaponType::PISTOL;
        //smith.damage = 500;
        //smith.magSize = 6;
        //smith.casingEjectionBoneName = "Ejection";
        //smith.casingEjectionOffset = glm::vec3(-0.066, -0.007, 0.249);
        //smith.ammoType = "Tokarev";
        //smith.animationCancelFrames.draw = 50.0f;
        //smith.animationCancelFrames.fire = 5.0f;
        //smith.animationCancelFrames.reload = 80.0f;
        //smith.animationCancelFrames.reloadFromEmpty = 80.0f;
        //smith.revolverCockFrameNumber = 18;
        //smith.relolverStyleReload = true;
        //smith.casingEjectionImpulse = 0.0175f;

        WeaponInfo& tokarev = CreateWeaponInfo("Tokarev");
        tokarev.itemInfoName = "Tokarev";
        tokarev.modelName = "Tokarev";
        tokarev.viewWeaponAnimationProfile = AnimationProfile::VIEW_WEAPON_TOKAREV;
        tokarev.meshMaterials["ArmsMale"] = "ArmsMale";
        tokarev.meshMaterials["ArmsFemale"] = "FemaleArms";
        tokarev.meshMaterials["TokarevBody"] = "Tokarev";
        tokarev.meshMaterials["Tokarev_Barrel"] = "Tokarev";
        tokarev.meshMaterials["Tokarev_Hammer"] = "Tokarev";
        tokarev.meshMaterials["Tokarev_Slide"] = "Tokarev";
        tokarev.meshMaterials["Tokarev_SlideCatch"] = "Tokarev";
        tokarev.meshMaterials["TokarevBody_Trigger"] = "Tokarev";
        tokarev.meshMaterials["TokarevBody"] = "Tokarev";
        tokarev.meshMaterials["TokarevMag_01"] = "TokarevMag";
        tokarev.meshMaterials["TokarevMag_02"] = "TokarevMag";
        tokarev.meshMaterials["TokarevGripPolymer"] = "TokarevGrip";
        tokarev.meshMaterials["TokarevGripWood"] = "TokarevGrip";

        tokarev.hiddenMeshAtStart.push_back("ArmsFemale");
        tokarev.hiddenMeshAtStart.push_back("TokarevGripWood");


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
        tokarev.ammoInfoName = "Tokarev";
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


        WeaponInfo& spas = CreateWeaponInfo("SPAS");
        spas.itemInfoName = "SPAS";
        spas.modelName = "SPAS";
        spas.viewWeaponAnimationProfile = AnimationProfile::VIEW_WEAPON_SPAS;

        spas.meshMaterials["ArmsMale"] = "ArmsMale";
        spas.meshMaterials["ArmsFemale"] = "FemaleArms";
        spas.meshMaterials["Shells"] = "Shell";
		spas.meshMaterials["Shells.002"] = "Shell";
		spas.meshMaterials["SPAS12_Beachshroud"] = "SPAS2_Moving";
		spas.meshMaterials["SPAS12_Bolt"] = "SPAS2_Moving";
		spas.meshMaterials["SPAS12_Main"] = "SPAS2_Main";
		spas.meshMaterials["SPAS12_Main_Moving_Low"] = "SPAS2_Moving";
		spas.meshMaterials["SPAS12_Main_StampedSG"] = "SPAS2_Stamped";
		spas.meshMaterials["SPAS12_Pumpslide"] = "SPAS2_Moving";
		spas.meshMaterials["SPAS12_Ring"] = "SPAS2_Moving";
		spas.meshMaterials["SPAS12_Stock_01"] = "SPAS2_Moving";
		spas.meshMaterials["SPAS12_Stock_02"] = "SPAS2_Moving";
		spas.meshMaterials["SPAS12_Stock_Holder"] = "SPAS2_Main";
		spas.meshMaterials["SPAS12_Strock_Release"] = "SPAS2_Moving";
		spas.meshMaterials["SPAS12_Trigger"] = "SPAS2_Moving";

        spas.audioFiles.fire.push_back("SPAS_Fire.wav");
        spas.audioFiles.shotgunPump = "SPAS_Pump.wav";

        spas.type = WeaponType::SHOTGUN;
        spas.damage = 15;
        spas.magSize = 8;
        spas.ammoInfoName = "12GaugeBuckShot";
        spas.hiddenMeshAtStart.push_back("ArmsFemale");
        spas.muzzleFlashBoneName = "Muzzle";
        spas.casingEjectionBoneName = "Shell_bone";
        spas.casingEjectionImpulse = 0.0175f;
        spas.hasAutoSwitch = true;

        spas.animationCancelFrames.fire = 21;
        spas.animationCancelFrames.dryFire = 5;
        spas.animationCancelFrames.fireAutoShotgun = 4;
        spas.animationCancelFrames.shotgunMelee = 28;
        spas.meleeAttacks = {
            {
                .animationSlot = AnimationSlot::MELEE,
                .startFrame = 8,
                .endFrame = 14,
                .damage = 10,
                .range = 1.5f,
                .width = 0.5f,
                .height = 0.2f
            }
        };

        WeaponInfo& remington870 = CreateWeaponInfo("Remington870");
        remington870.itemInfoName = "Remington870";
        remington870.modelName = "Remington870";
        remington870.viewWeaponAnimationProfile = AnimationProfile::VIEW_WEAPON_REMINGTON870;
        remington870.meshMaterials["Shotgun"] = "Shotgun";
        remington870.meshMaterials["ArmsMale"] = "ArmsMale";
        remington870.meshMaterials["ArmsFemale"] = "FemaleArms";
        remington870.animationSpeeds.melee = 1.25f;
        remington870.audioFiles.fire.push_back("Shotgun_Fire.wav");
        remington870.audioFiles.shotgunPump = "Remington870_Pump.wav";
        remington870.type = WeaponType::SHOTGUN;
        remington870.damage = 14;
        remington870.magSize = 8;
        remington870.ammoInfoName = "12GaugeBuckShot";
        remington870.hiddenMeshAtStart.push_back("ArmsFemale");
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
        remington870.animationCancelFrames.shotgunMelee = 28;
        remington870.meleeAttacks = {
            {
                .animationSlot = AnimationSlot::MELEE,
                .startFrame = 8,
                .endFrame = 14,
                .damage = 10,
                .range = 1.5f,
                .width = 0.5f,
                .height = 0.2f
            }
        };

        WeaponInfo& p90 = CreateWeaponInfo("P90");
        p90.modelName = "P90";
        p90.itemInfoName = "P90";
        p90.viewWeaponAnimationProfile = AnimationProfile::VIEW_WEAPON_P90;
        p90.type = WeaponType::AUTOMATIC;
        p90.damage = 20;
        p90.hasADS = true;


        p90.hiddenMeshAtStart.push_back("ArmsFemale");

		p90.hiddenMeshAtStart.push_back("P90_Magazine");
		p90.hiddenMeshAtStart.push_back("P90_Magazine2");

		p90.hiddenMeshAtStart.push_back("P90_SpringFollower2");


		p90.hiddenMeshAtStart.push_back("BulletB_01");
		p90.hiddenMeshAtStart.push_back("BulletB_02");
		p90.hiddenMeshAtStart.push_back("BulletB_03");
		p90.hiddenMeshAtStart.push_back("BulletB_04");
		p90.hiddenMeshAtStart.push_back("BulletB_05");
		p90.hiddenMeshAtStart.push_back("BulletB_06");
		p90.hiddenMeshAtStart.push_back("BulletB_07");
		p90.hiddenMeshAtStart.push_back("BulletB_08");
		p90.hiddenMeshAtStart.push_back("BulletB_09");
		p90.hiddenMeshAtStart.push_back("BulletB_10");
		p90.hiddenMeshAtStart.push_back("BulletB_11");
		p90.hiddenMeshAtStart.push_back("BulletB_12");
		p90.hiddenMeshAtStart.push_back("BulletB_13");
		p90.hiddenMeshAtStart.push_back("BulletB_14");
		p90.hiddenMeshAtStart.push_back("BulletB_15");
		p90.hiddenMeshAtStart.push_back("BulletB_16");
		p90.hiddenMeshAtStart.push_back("BulletB_17");
		p90.hiddenMeshAtStart.push_back("BulletB_18");
		p90.hiddenMeshAtStart.push_back("BulletB_19");
		p90.hiddenMeshAtStart.push_back("BulletB_20");
		p90.hiddenMeshAtStart.push_back("BulletB_21");
		p90.hiddenMeshAtStart.push_back("BulletB_22");
		p90.hiddenMeshAtStart.push_back("BulletB_23");
		p90.hiddenMeshAtStart.push_back("BulletB_24");
		p90.hiddenMeshAtStart.push_back("BulletB_25");
		p90.hiddenMeshAtStart.push_back("BulletB_26");
		p90.hiddenMeshAtStart.push_back("BulletB_27");
		p90.hiddenMeshAtStart.push_back("BulletB_28");
		p90.hiddenMeshAtStart.push_back("BulletB_29");
		p90.hiddenMeshAtStart.push_back("BulletB_30");
		p90.hiddenMeshAtStart.push_back("BulletB_31");
		p90.hiddenMeshAtStart.push_back("BulletB_32");
		p90.hiddenMeshAtStart.push_back("BulletB_33");
		p90.hiddenMeshAtStart.push_back("BulletB_34");
		p90.hiddenMeshAtStart.push_back("BulletB_35");
		p90.hiddenMeshAtStart.push_back("BulletB_36");
		p90.hiddenMeshAtStart.push_back("BulletB_37");
		p90.hiddenMeshAtStart.push_back("BulletB_38");
		p90.hiddenMeshAtStart.push_back("BulletB_39");
		p90.hiddenMeshAtStart.push_back("BulletB_40");
		p90.hiddenMeshAtStart.push_back("BulletB_41");
		p90.hiddenMeshAtStart.push_back("BulletB_42");
		p90.hiddenMeshAtStart.push_back("BulletB_43");
		p90.hiddenMeshAtStart.push_back("BulletB_44");
		p90.hiddenMeshAtStart.push_back("BulletB_45");
		p90.hiddenMeshAtStart.push_back("BulletB_46");
		p90.hiddenMeshAtStart.push_back("BulletB_47");
		p90.hiddenMeshAtStart.push_back("BulletB_48");
		p90.hiddenMeshAtStart.push_back("BulletB_49");
		p90.hiddenMeshAtStart.push_back("BulletB_50");
		p90.hiddenMeshAtStart.push_back("BulletB_51");

		p90.hiddenMeshAtStart.push_back("Bullet_01");
		p90.hiddenMeshAtStart.push_back("Bullet_02");
		p90.hiddenMeshAtStart.push_back("Bullet_03");
		p90.hiddenMeshAtStart.push_back("Bullet_04");
		p90.hiddenMeshAtStart.push_back("Bullet_05");
		p90.hiddenMeshAtStart.push_back("Bullet_06");
		p90.hiddenMeshAtStart.push_back("Bullet_07");
		p90.hiddenMeshAtStart.push_back("Bullet_08");
		p90.hiddenMeshAtStart.push_back("Bullet_09");
		p90.hiddenMeshAtStart.push_back("Bullet_10");
		p90.hiddenMeshAtStart.push_back("Bullet_11");
		p90.hiddenMeshAtStart.push_back("Bullet_12");
		p90.hiddenMeshAtStart.push_back("Bullet_13");
		p90.hiddenMeshAtStart.push_back("Bullet_14");
		p90.hiddenMeshAtStart.push_back("Bullet_15");
		p90.hiddenMeshAtStart.push_back("Bullet_16");
		p90.hiddenMeshAtStart.push_back("Bullet_17");
		p90.hiddenMeshAtStart.push_back("Bullet_18");
		p90.hiddenMeshAtStart.push_back("Bullet_19");
		p90.hiddenMeshAtStart.push_back("Bullet_20");
		p90.hiddenMeshAtStart.push_back("Bullet_21");
		p90.hiddenMeshAtStart.push_back("Bullet_22");
		p90.hiddenMeshAtStart.push_back("Bullet_23");
		p90.hiddenMeshAtStart.push_back("Bullet_24");
		p90.hiddenMeshAtStart.push_back("Bullet_25");
		p90.hiddenMeshAtStart.push_back("Bullet_26");
		p90.hiddenMeshAtStart.push_back("Bullet_27");
		p90.hiddenMeshAtStart.push_back("Bullet_28");
		p90.hiddenMeshAtStart.push_back("Bullet_29");
		p90.hiddenMeshAtStart.push_back("Bullet_30");
		p90.hiddenMeshAtStart.push_back("Bullet_31");
		p90.hiddenMeshAtStart.push_back("Bullet_32");
		p90.hiddenMeshAtStart.push_back("Bullet_33");
		p90.hiddenMeshAtStart.push_back("Bullet_34");
		p90.hiddenMeshAtStart.push_back("Bullet_35");
		p90.hiddenMeshAtStart.push_back("Bullet_36");
		p90.hiddenMeshAtStart.push_back("Bullet_37");
		p90.hiddenMeshAtStart.push_back("Bullet_38");
		p90.hiddenMeshAtStart.push_back("Bullet_39");
		p90.hiddenMeshAtStart.push_back("Bullet_40");
		p90.hiddenMeshAtStart.push_back("Bullet_41");
		p90.hiddenMeshAtStart.push_back("Bullet_42");
		p90.hiddenMeshAtStart.push_back("Bullet_43");
		p90.hiddenMeshAtStart.push_back("Bullet_44");
		p90.hiddenMeshAtStart.push_back("Bullet_45");
		p90.hiddenMeshAtStart.push_back("Bullet_46");
		p90.hiddenMeshAtStart.push_back("Bullet_47");
		p90.hiddenMeshAtStart.push_back("Bullet_48");
		p90.hiddenMeshAtStart.push_back("Bullet_49");
		p90.hiddenMeshAtStart.push_back("Bullet_50");
		p90.hiddenMeshAtStart.push_back("Bullet_51");

        //p90.hiddenMeshAtStart.push_back("P90_Magazine");
        //p90.hiddenMeshAtStart.push_back("P90_Magazine2");
        //p90.hiddenMeshAtStart.push_back("P90_MagazineSpring");
        //p90.hiddenMeshAtStart.push_back("P90_MagazineSpringFollower");

        p90.meshMaterials["ArmsMale"] = "ArmsMale";
        p90.meshMaterials["ArmsFemale"] = "FemaleArms";

        p90.meshMaterials["P90_Body"] = "P90_Main";
        p90.meshMaterials["P90_MagazineCatch"] = "P90_Main";
        p90.meshMaterials["P90_Trigger"] = "P90_Main";

        p90.meshMaterials["P90_ChargingHandle"] = "P90_FrontEnd";
        p90.meshMaterials["P90_ChargingHandle2"] = "P90_FrontEnd";
        p90.meshMaterials["P90_Compensator"] = "P90_FrontEnd";

        p90.meshMaterials["P90_TopRailStandard"] = "P90_Rails";

        p90.meshMaterials["P90_Velcro_Clip"] = "P90_Sling";

        p90.meshMaterials["P90_Magazine"] = "P90_Mag";
        p90.meshMaterials["P90_Magazine2"] = "P90_Mag";
        p90.meshMaterials["P90_MagazineSpring"] = "P90_Mag";
        p90.meshMaterials["P90_MagazineSpringFollower"] = "P90_Mag";

        p90.meshMaterials["Bullet_01"] = "P90_Mag";
        p90.meshMaterials["Bullet_02"] = "P90_Mag";
        p90.meshMaterials["Bullet_03"] = "P90_Mag";
        p90.meshMaterials["Bullet_04"] = "P90_Mag";
        p90.meshMaterials["Bullet_05"] = "P90_Mag";
        p90.meshMaterials["Bullet_06"] = "P90_Mag";
        p90.meshMaterials["Bullet_07"] = "P90_Mag";
        p90.meshMaterials["Bullet_08"] = "P90_Mag";
        p90.meshMaterials["Bullet_09"] = "P90_Mag";
        p90.meshMaterials["Bullet_10"] = "P90_Mag";
        p90.meshMaterials["Bullet_11"] = "P90_Mag";
        p90.meshMaterials["Bullet_12"] = "P90_Mag";
        p90.meshMaterials["Bullet_13"] = "P90_Mag";
        p90.meshMaterials["Bullet_14"] = "P90_Mag";
        p90.meshMaterials["Bullet_15"] = "P90_Mag";
        p90.meshMaterials["Bullet_16"] = "P90_Mag";
        p90.meshMaterials["Bullet_17"] = "P90_Mag";
        p90.meshMaterials["Bullet_18"] = "P90_Mag";
        p90.meshMaterials["Bullet_19"] = "P90_Mag";
        p90.meshMaterials["Bullet_20"] = "P90_Mag";
        p90.meshMaterials["Bullet_21"] = "P90_Mag";
        p90.meshMaterials["Bullet_22"] = "P90_Mag";
        p90.meshMaterials["Bullet_23"] = "P90_Mag";
        p90.meshMaterials["Bullet_24"] = "P90_Mag";
        p90.meshMaterials["Bullet_25"] = "P90_Mag";
        p90.meshMaterials["Bullet_26"] = "P90_Mag";
        p90.meshMaterials["Bullet_27"] = "P90_Mag";
        p90.meshMaterials["Bullet_28"] = "P90_Mag";
        p90.meshMaterials["Bullet_29"] = "P90_Mag";
        p90.meshMaterials["Bullet_30"] = "P90_Mag";
        p90.meshMaterials["Bullet_31"] = "P90_Mag";
        p90.meshMaterials["Bullet_32"] = "P90_Mag";
        p90.meshMaterials["Bullet_33"] = "P90_Mag";
        p90.meshMaterials["Bullet_34"] = "P90_Mag";
        p90.meshMaterials["Bullet_35"] = "P90_Mag";
        p90.meshMaterials["Bullet_36"] = "P90_Mag";
        p90.meshMaterials["Bullet_37"] = "P90_Mag";
        p90.meshMaterials["Bullet_38"] = "P90_Mag";
        p90.meshMaterials["Bullet_39"] = "P90_Mag";
        p90.meshMaterials["Bullet_40"] = "P90_Mag";
        p90.meshMaterials["Bullet_41"] = "P90_Mag";
        p90.meshMaterials["Bullet_42"] = "P90_Mag";
        p90.meshMaterials["Bullet_43"] = "P90_Mag";
        p90.meshMaterials["Bullet_44"] = "P90_Mag";
        p90.meshMaterials["Bullet_45"] = "P90_Mag";
        p90.meshMaterials["Bullet_46"] = "P90_Mag";
        p90.meshMaterials["Bullet_47"] = "P90_Mag";
        p90.meshMaterials["Bullet_48"] = "P90_Mag";
        p90.meshMaterials["Bullet_49"] = "P90_Mag";
        p90.meshMaterials["Bullet_50"] = "P90_Mag";
        p90.meshMaterials["Bullet_51"] = "P90_Mag";

        p90.ammoInfoName = "P90";
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
    }


    int32_t GetWeaponMagSize(const std::string& name) {
        if (WeaponInfo* weaponInfo = GetWeaponInfoByName(name)) {
            return weaponInfo->magSize;
        }
        return 0;
    }
}
