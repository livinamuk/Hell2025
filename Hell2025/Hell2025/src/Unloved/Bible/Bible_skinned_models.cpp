#include "Bible.h"

#include "Hell/Common/Enum.h"
#include "Hell/Common/String.h"
#include "Hell/Logging.h"

#include "Unloved/Objects/Renderables/SkinnedGameObject.h"

namespace Unloved::Bible {
    namespace {
        std::string GetIndexedMeshName(const std::string& prefix, int index) {
            return prefix + (index < 10 ? "0" : "") + std::to_string(index);
        }

        void ConfigureCharacterGlock(SkinnedGameObject& object, bool golden) {
            const std::string glockMaterial = golden ? "GlockGold" : "Glock";

            object.SetSkinnedModel("CharacterGlock");

            AnimatedMeshNodes& meshNodes = object.GetAnimatedMeshNodes();
            meshNodes.SetMeshMaterialByMeshName("GlockBarrel", glockMaterial);
            meshNodes.SetMeshMaterialByMeshName("GlockMagazine", glockMaterial);
            meshNodes.SetMeshMaterialByMeshName("GlockMagazine_02", glockMaterial);
            meshNodes.SetMeshMaterialByMeshName("GlockReceiver", glockMaterial);
            meshNodes.SetMeshMaterialByMeshName("GlockSlide", glockMaterial);
            meshNodes.SetMeshMaterialByMeshName("GlockSlideUnLock", glockMaterial);
            meshNodes.SetMeshMaterialByMeshName("GlockTrigger", glockMaterial);
            meshNodes.SetMeshMaterialByMeshName("LeupoldRedDot", "RedDot");
            meshNodes.SetMeshMaterialByMeshName("LeupoldRedDotGlass", "RedDotGlass");
            meshNodes.SetBlendingModeByMeshName("LeupoldRedDot", BlendingMode::DO_NOT_RENDER);
            meshNodes.SetBlendingModeByMeshName("LeupoldRedDotGlass", BlendingMode::DO_NOT_RENDER);
            meshNodes.SetBlendingModeByMeshName("Supressor", BlendingMode::DO_NOT_RENDER);
        }

        void ConfigureDobermann(SkinnedGameObject& object) {
            object.SetSkinnedModel("Dobermann");
        }

        void ConfigureKangaroo(SkinnedGameObject& object) {
            object.SetSkinnedModel("Kangaroo");
        }

        void ConfigureRatKing(SkinnedGameObject& object) {
            object.SetSkinnedModel("RatKing");

            AnimatedMeshNodes& meshNodes = object.GetAnimatedMeshNodes();
            meshNodes.SetMeshMaterialByMeshName("CC_Base_Body", "RatKingHead");
            meshNodes.SetMeshMaterialByMeshName("CC_Base_Body2", "RatKingTorso");
            meshNodes.SetMeshMaterialByMeshName("CC_Base_Body3", "RatKingArms");
            meshNodes.SetMeshMaterialByMeshName("CC_Base_Body4", "RatKingLegs");
            meshNodes.SetMeshMaterialByMeshName("CC_Base_Body5", "RatKingNails");
            meshNodes.SetMeshMaterialByMeshName("CC_Base_Body6", "RatKingLashes2", BlendingMode::BLENDED);
            meshNodes.SetMeshMaterialByMeshName("CC_Base_Eye", "RatKingEye");
            meshNodes.SetBlendingModeByMeshName("CC_Base_Eye2", BlendingMode::DO_NOT_RENDER);
            meshNodes.SetMeshMaterialByMeshName("CC_Base_Eye3", "RatKingEye");
            meshNodes.SetBlendingModeByMeshName("CC_Base_Eye4", BlendingMode::DO_NOT_RENDER);
            meshNodes.SetBlendingModeByMeshName("CC_Base_TearLine", BlendingMode::DO_NOT_RENDER);
            meshNodes.SetBlendingModeByMeshName("CC_Base_TearLine2", BlendingMode::DO_NOT_RENDER);
            meshNodes.SetMeshMaterialByMeshName("CC_Base_Tongue", "TrapKingTongue");
            meshNodes.SetMeshMaterialByMeshName("CC_Base_Teeth", "TrapKingTeethUpper");
            meshNodes.SetMeshMaterialByMeshName("CC_Base_Teeth2", "TrapKingTeethLower");
            meshNodes.SetBlendingModeByMeshName("Brows_Bushy", BlendingMode::DO_NOT_RENDER);
            meshNodes.SetMeshMaterialByMeshName("Brows_Bushy2", "RatKingBrows", BlendingMode::BLENDED);
            meshNodes.SetMeshMaterialByMeshName("Brows_Bushy3", "RatKingBrows2", BlendingMode::BLENDED);
            meshNodes.SetBlendingModeByMeshName("CC_Base_EyeOcclusion", BlendingMode::DO_NOT_RENDER);
            meshNodes.SetBlendingModeByMeshName("CC_Base_EyeOcclusion2", BlendingMode::DO_NOT_RENDER);
            meshNodes.SetMeshMaterialByMeshName("Lash_Low_Short_Sparse", "RatKingLashes", BlendingMode::BLENDED);
            meshNodes.SetMeshMaterialByMeshName("Lash_Low_Sparse", "RatKingLashes", BlendingMode::BLENDED);
            meshNodes.SetMeshMaterialByMeshName("Lash_Up_Downward", "RatKingLashes", BlendingMode::BLENDED);
            meshNodes.SetMeshMaterialByMeshName("Lash_Up_Short_Sparse", "RatKingLashes", BlendingMode::BLENDED);
            meshNodes.SetMeshMaterialByMeshName("Side_swept_L", "RatKingHair", BlendingMode::HAIR);
            meshNodes.SetMeshMaterialByMeshName("Long_Hair_R", "RatKingHair", BlendingMode::HAIR);
            meshNodes.SetMeshMaterialByMeshName("Long_Hair_R2", "RatKingHair", BlendingMode::DO_NOT_RENDER);
            meshNodes.SetMeshMaterialByMeshName("Side_swept_Long_Hair_L", "RatKingScalp", BlendingMode::DO_NOT_RENDER);
            meshNodes.SetMeshMaterialByMeshName("Side_swept_Long_Hair_L2", "RatKingHair", BlendingMode::HAIR);
            meshNodes.SetMeshMaterialByMeshName("Side_swept_Long_Hair_R", "RatKingHair", BlendingMode::HAIR);
            meshNodes.SetMeshMaterialByMeshName("Hair_Tattoo", "RatKingHeadTattoo", BlendingMode::BLENDED);
            meshNodes.SetMeshMaterialByMeshName("Hair_Tattoo2", "RatKingScalp", BlendingMode::BLENDED);
            meshNodes.SetMeshMaterialByMeshName("Scalp_Male", "RatKingScalp", BlendingMode::DO_NOT_RENDER);
            meshNodes.SetMeshMaterialByMeshName("Side_swept_L2", "RatKingScalp", BlendingMode::DO_NOT_RENDER);
            meshNodes.SetMeshMaterialByMeshName("Boxers", "BoxersBlack");
            meshNodes.SetMeshMaterialByMeshName("Slim_Jeans", "Jeans");
        }

        void ConfigureSnake(SkinnedGameObject& object) {
            object.SetSkinnedModel("Snake");

            AnimatedMeshNodes& meshNodes = object.GetAnimatedMeshNodes();
            meshNodes.SetAllMeshMaterials("Snake");
        }

        void ConfigureShark(SkinnedGameObject& object) {
            object.SetSkinnedModel("Shark");
        }

        void ConfigureTrapKing(SkinnedGameObject& object) {
            object.SetSkinnedModel("TrapKing");

            AnimatedMeshNodes& meshNodes = object.GetAnimatedMeshNodes();
            meshNodes.SetMeshMaterialByMeshName("Body", "TrapKingBodyHead");
            meshNodes.SetMeshMaterialByMeshName("Body2", "TrapKingBodyTorso");
            meshNodes.SetMeshMaterialByMeshName("Body3", "TrapKingBodyArms");
            meshNodes.SetMeshMaterialByMeshName("Body4", "TrapKingBodyLegs");
            meshNodes.SetMeshMaterialByMeshName("Body5", "TrapKingNails");
            meshNodes.SetMeshMaterialByMeshName("Body6", "TrapKingEyeLashes", BlendingMode::BLENDED);
            meshNodes.SetMeshMaterialByMeshName("Tongue", "TrapKingTongue");
            meshNodes.SetMeshMaterialByMeshName("Teeth", "TrapKingTeethUpper");
            meshNodes.SetMeshMaterialByMeshName("Teeth2", "TrapKingTeethLower");
            meshNodes.SetMeshMaterialByMeshName("DreadsTop", "TrapKingHairScalp");
            meshNodes.SetMeshMaterialByMeshName("DreadsBottom", "TrapKingHairScalp");
            meshNodes.SetMeshMaterialByMeshName("DreadsFront", "TrapKingHairScalp");
            meshNodes.SetMeshMaterialByMeshName("DreadsShoulder", "TrapKingHairScalp");
            meshNodes.SetMeshMaterialByMeshName("DreadsKnot", "TrapKingHairScalp");
            meshNodes.SetMeshMaterialByMeshName("DreadsScalp", "TrapKingHairScalp", BlendingMode::BLENDED);
            meshNodes.SetBlendingModeByMeshName("EyeOcclusion", BlendingMode::DO_NOT_RENDER);
            meshNodes.SetBlendingModeByMeshName("EyeOcclusion2", BlendingMode::DO_NOT_RENDER);
            meshNodes.SetMeshMaterialByMeshName("Eye", "TrapKingEye");
            meshNodes.SetBlendingModeByMeshName("Eye2", BlendingMode::DO_NOT_RENDER);
            meshNodes.SetMeshMaterialByMeshName("Eye3", "TrapKingEye");
            meshNodes.SetBlendingModeByMeshName("Eye4", BlendingMode::DO_NOT_RENDER);
            meshNodes.SetBlendingModeByMeshName("Brow", BlendingMode::DO_NOT_RENDER);
            meshNodes.SetMeshMaterialByMeshName("Brow2", "TrapKingBrow", BlendingMode::BLENDED);
            meshNodes.SetBlendingModeByMeshName("TearLine", BlendingMode::DO_NOT_RENDER);
            meshNodes.SetBlendingModeByMeshName("TearLine2", BlendingMode::DO_NOT_RENDER);
            meshNodes.SetMeshMaterialByMeshName("Pants", "TrapKingPants");
            meshNodes.SetMeshMaterialByMeshName("Boxers", "TrapKingBoxes");
        }

        void ConfigureUnisexGuy(SkinnedGameObject& object) {
            object.SetSkinnedModel("UniSexGuyScaled");
        }

        void ConfigureViewWeaponAKS74U(SkinnedGameObject& object) {
            object.SetSkinnedModel("AKS74U");

            AnimatedMeshNodes& meshNodes = object.GetAnimatedMeshNodes();
            meshNodes.SetMeshMaterialByMeshName("ArmsMale", "ArmsMale");
            meshNodes.SetMeshMaterialByMeshName("ArmsFemale", "FemaleArms");
            meshNodes.SetMeshMaterialByMeshName("AKS74UBarrel", "AKS74U_4");
            meshNodes.SetMeshMaterialByMeshName("AKS74UBolt", "AKS74U_1");
            meshNodes.SetMeshMaterialByMeshName("AKS74UHandGuard", "AKS74U_0");
            meshNodes.SetMeshMaterialByMeshName("AKS74UMag", "AKS74U_3");
            meshNodes.SetMeshMaterialByMeshName("AKS74UPistolGrip", "AKS74U_2");
            meshNodes.SetMeshMaterialByMeshName("AKS74UReceiver", "AKS74U_1");
            meshNodes.SetMeshMaterialByMeshName("AKS74U_Lens", "Black");
            meshNodes.SetMeshMaterialByMeshName("AKS74U_RedDot", "Black");
            meshNodes.SetMeshMaterialByMeshName("AKS74U_ScopeBackCap", "Black");
            meshNodes.SetMeshMaterialByMeshName("AKS74U_ScopeFrontCap", "Black");
            meshNodes.SetMeshMaterialByMeshName("AKS74U_ScopeMain", "Black");
            meshNodes.SetMeshMaterialByMeshName("AKS74U_ScopeMain2", "Black");
            meshNodes.SetMeshMaterialByMeshName("AKS74U_ScopeSupport", "Black");
            meshNodes.SetBlendingModeByMeshName("ArmsFemale", BlendingMode::DO_NOT_RENDER);
            meshNodes.SetBlendingModeByMeshName("AKS74U_Lens", BlendingMode::DO_NOT_RENDER);
            meshNodes.SetBlendingModeByMeshName("AKS74U_RedDot", BlendingMode::DO_NOT_RENDER);
            meshNodes.SetBlendingModeByMeshName("AKS74U_ScopeBackCap", BlendingMode::DO_NOT_RENDER);
            meshNodes.SetBlendingModeByMeshName("AKS74U_ScopeFrontCap", BlendingMode::DO_NOT_RENDER);
            meshNodes.SetBlendingModeByMeshName("AKS74U_ScopeMain", BlendingMode::DO_NOT_RENDER);
            meshNodes.SetBlendingModeByMeshName("AKS74U_ScopeMain2", BlendingMode::DO_NOT_RENDER);
            meshNodes.SetBlendingModeByMeshName("AKS74U_ScopeSupport", BlendingMode::DO_NOT_RENDER);
        }

        void ConfigureViewWeaponGlock(SkinnedGameObject& object, bool golden) {
            const char* glockMaterial = golden ? "GlockGold" : "Glock";

            object.SetSkinnedModel("Glock");

            AnimatedMeshNodes& meshNodes = object.GetAnimatedMeshNodes();
            meshNodes.SetMeshMaterialByMeshName("GlockBarrel", glockMaterial);
            meshNodes.SetMeshMaterialByMeshName("GlockMagazine", glockMaterial);
            meshNodes.SetMeshMaterialByMeshName("GlockMagazine_02", glockMaterial);
            meshNodes.SetMeshMaterialByMeshName("GlockReceiver", glockMaterial);
            meshNodes.SetMeshMaterialByMeshName("GlockSlide", glockMaterial);
            meshNodes.SetMeshMaterialByMeshName("GlockSlideUnLock", glockMaterial);
            meshNodes.SetMeshMaterialByMeshName("GlockTrigger", glockMaterial);
            meshNodes.SetMeshMaterialByMeshName("LeupoldRedDot", golden ? "RedDotGold" : "RedDot");
            meshNodes.SetMeshMaterialByMeshName("LeupoldRedDotGlass", "RedDotGlass");
            meshNodes.SetMeshMaterialByMeshName("Supressor", golden ? "SuppressorGold" : "Suppressor");
            meshNodes.SetMeshMaterialByMeshName("ArmsMale", "ArmsMale");
            meshNodes.SetMeshMaterialByMeshName("ArmsFemale", "FemaleArms");
            meshNodes.SetBlendingModeByMeshName("ArmsFemale", BlendingMode::DO_NOT_RENDER);
            meshNodes.SetBlendingModeByMeshName("LeupoldRedDot", BlendingMode::DO_NOT_RENDER);
            meshNodes.SetBlendingModeByMeshName("LeupoldRedDotGlass", BlendingMode::DO_NOT_RENDER);
            meshNodes.SetBlendingModeByMeshName("Supressor", BlendingMode::DO_NOT_RENDER);
        }

        void ConfigureViewWeaponKnife(SkinnedGameObject& object) {
            object.SetSkinnedModel("Knife");

            AnimatedMeshNodes& meshNodes = object.GetAnimatedMeshNodes();
            meshNodes.SetMeshMaterialByMeshName("Knife", "Knife");
            meshNodes.SetMeshMaterialByMeshName("ArmsMale", "ArmsMale");
            meshNodes.SetMeshMaterialByMeshName("ArmsFemale", "FemaleArms");
            meshNodes.SetBlendingModeByMeshName("ArmsFemale", BlendingMode::DO_NOT_RENDER);
        }

        void ConfigureViewWeaponP90(SkinnedGameObject& object) {
            object.SetSkinnedModel("P90");

            AnimatedMeshNodes& meshNodes = object.GetAnimatedMeshNodes();
            meshNodes.SetMeshMaterialByMeshName("ArmsMale", "ArmsMale");
            meshNodes.SetMeshMaterialByMeshName("ArmsFemale", "FemaleArms");
            meshNodes.SetMeshMaterialByMeshName("P90_Body", "P90_Main");
            meshNodes.SetMeshMaterialByMeshName("P90_MagazineCatch", "P90_Main");
            meshNodes.SetMeshMaterialByMeshName("P90_Trigger", "P90_Main");
            meshNodes.SetMeshMaterialByMeshName("P90_ChargingHandle", "P90_FrontEnd");
            meshNodes.SetMeshMaterialByMeshName("P90_ChargingHandle2", "P90_FrontEnd");
            meshNodes.SetMeshMaterialByMeshName("P90_Compensator", "P90_FrontEnd");
            meshNodes.SetMeshMaterialByMeshName("P90_TopRailStandard", "P90_Rails");
            meshNodes.SetMeshMaterialByMeshName("P90_Velcro_Clip", "P90_Sling");
            meshNodes.SetMeshMaterialByMeshName("P90_Magazine", "P90_Mag");
            meshNodes.SetMeshMaterialByMeshName("P90_Magazine2", "P90_Mag");
            meshNodes.SetMeshMaterialByMeshName("P90_MagazineSpring", "P90_Mag");
            meshNodes.SetMeshMaterialByMeshName("P90_MagazineSpringFollower", "P90_Mag");
            meshNodes.SetBlendingModeByMeshName("ArmsFemale", BlendingMode::DO_NOT_RENDER);
            meshNodes.SetBlendingModeByMeshName("P90_Magazine", BlendingMode::DO_NOT_RENDER);
            meshNodes.SetBlendingModeByMeshName("P90_Magazine2", BlendingMode::DO_NOT_RENDER);
            meshNodes.SetBlendingModeByMeshName("P90_SpringFollower2", BlendingMode::DO_NOT_RENDER);

            for (int i = 1; i <= 51; i++) {
                const std::string bulletName = GetIndexedMeshName("Bullet_", i);
                meshNodes.SetMeshMaterialByMeshName(bulletName, "P90_Mag");
                meshNodes.SetBlendingModeByMeshName(bulletName, BlendingMode::DO_NOT_RENDER);
                meshNodes.SetBlendingModeByMeshName(GetIndexedMeshName("BulletB_", i), BlendingMode::DO_NOT_RENDER);
            }
        }

        void ConfigureViewWeaponRemington870(SkinnedGameObject& object) {
            object.SetSkinnedModel("Remington870");

            AnimatedMeshNodes& meshNodes = object.GetAnimatedMeshNodes();
            meshNodes.SetMeshMaterialByMeshName("ArmsMale", "ArmsMale");
            meshNodes.SetMeshMaterialByMeshName("ArmsFemale", "FemaleArms");
            meshNodes.SetMeshMaterialByMeshName("Shells", "Shell");
            meshNodes.SetMeshMaterialByMeshName("Shells.001", "Shell");
            meshNodes.SetMeshMaterialByMeshName("Shells.002", "Shell");
            meshNodes.SetMeshMaterialByMeshName("Shotgun", "Shotgun");
            meshNodes.SetBlendingModeByMeshName("ArmsFemale", BlendingMode::DO_NOT_RENDER);
        }

        void ConfigureViewWeaponSPAS(SkinnedGameObject& object) {
            object.SetSkinnedModel("SPAS");

            AnimatedMeshNodes& meshNodes = object.GetAnimatedMeshNodes();
            meshNodes.SetMeshMaterialByMeshName("ArmsMale", "ArmsMale");
            meshNodes.SetMeshMaterialByMeshName("ArmsFemale", "FemaleArms");
            meshNodes.SetMeshMaterialByMeshName("Shells", "Shell");
            meshNodes.SetMeshMaterialByMeshName("Shells.002", "Shell");
            meshNodes.SetMeshMaterialByMeshName("SPAS12_Beachshroud", "SPAS2_Moving");
            meshNodes.SetMeshMaterialByMeshName("SPAS12_Bolt", "SPAS2_Moving");
            meshNodes.SetMeshMaterialByMeshName("SPAS12_Main", "SPAS2_Main");
            meshNodes.SetMeshMaterialByMeshName("SPAS12_Main_Moving_Low", "SPAS2_Moving");
            meshNodes.SetMeshMaterialByMeshName("SPAS12_Main_StampedSG", "SPAS2_Stamped");
            meshNodes.SetMeshMaterialByMeshName("SPAS12_Pumpslide", "SPAS2_Moving");
            meshNodes.SetMeshMaterialByMeshName("SPAS12_Ring", "SPAS2_Moving");
            meshNodes.SetMeshMaterialByMeshName("SPAS12_Stock_01", "SPAS2_Moving");
            meshNodes.SetMeshMaterialByMeshName("SPAS12_Stock_02", "SPAS2_Moving");
            meshNodes.SetMeshMaterialByMeshName("SPAS12_Stock_Holder", "SPAS2_Main");
            meshNodes.SetMeshMaterialByMeshName("SPAS12_Strock_Release", "SPAS2_Moving");
            meshNodes.SetMeshMaterialByMeshName("SPAS12_Trigger", "SPAS2_Moving");
            meshNodes.SetBlendingModeByMeshName("ArmsFemale", BlendingMode::DO_NOT_RENDER);
        }

        void ConfigureViewWeaponTokarev(SkinnedGameObject& object) {
            object.SetSkinnedModel("Tokarev");

            AnimatedMeshNodes& meshNodes = object.GetAnimatedMeshNodes();
            meshNodes.SetMeshMaterialByMeshName("ArmsMale", "ArmsMale");
            meshNodes.SetMeshMaterialByMeshName("ArmsFemale", "FemaleArms");
            meshNodes.SetMeshMaterialByMeshName("TokarevBody", "Tokarev");
            meshNodes.SetMeshMaterialByMeshName("Tokarev_Barrel", "Tokarev");
            meshNodes.SetMeshMaterialByMeshName("Tokarev_Hammer", "Tokarev");
            meshNodes.SetMeshMaterialByMeshName("Tokarev_Slide", "Tokarev");
            meshNodes.SetMeshMaterialByMeshName("Tokarev_SlideCatch", "Tokarev");
            meshNodes.SetMeshMaterialByMeshName("TokarevBody_Trigger", "Tokarev");
            meshNodes.SetMeshMaterialByMeshName("TokarevMag_01", "TokarevMag");
            meshNodes.SetMeshMaterialByMeshName("TokarevMag_02", "TokarevMag");
            meshNodes.SetMeshMaterialByMeshName("TokarevGripPolymer", "TokarevGrip");
            meshNodes.SetMeshMaterialByMeshName("TokarevGripWood", "TokarevGrip");
            meshNodes.SetBlendingModeByMeshName("ArmsFemale", BlendingMode::DO_NOT_RENDER);
            meshNodes.SetBlendingModeByMeshName("TokarevGripWood", BlendingMode::DO_NOT_RENDER);
        }
    }

    void ConfigureSkinnedModel(SkinnedGameObject& object, SkinnedModelPreset preset) {
        switch (preset) {
            case SkinnedModelPreset::CHARACTER_GLOCK:           return ConfigureCharacterGlock(object, false);
            case SkinnedModelPreset::CHARACTER_GOLDEN_GLOCK:    return ConfigureCharacterGlock(object, true);
            case SkinnedModelPreset::DOBERMANN:                 return ConfigureDobermann(object);
            case SkinnedModelPreset::KANGAROO:                  return ConfigureKangaroo(object);
            case SkinnedModelPreset::RAT_KING:                  return ConfigureRatKing(object);
            case SkinnedModelPreset::SHARK:                     return ConfigureShark(object);
            case SkinnedModelPreset::SNAKE:                     return ConfigureSnake(object);
            case SkinnedModelPreset::TRAP_KING:                 return ConfigureTrapKing(object);
            case SkinnedModelPreset::UNISEX_GUY:                return ConfigureUnisexGuy(object);
            case SkinnedModelPreset::VIEW_WEAPON_AKS74U:        return ConfigureViewWeaponAKS74U(object);
            case SkinnedModelPreset::VIEW_WEAPON_GLOCK:         return ConfigureViewWeaponGlock(object, false);
            case SkinnedModelPreset::VIEW_WEAPON_GOLDEN_GLOCK:  return ConfigureViewWeaponGlock(object, true);
            case SkinnedModelPreset::VIEW_WEAPON_KNIFE:         return ConfigureViewWeaponKnife(object);
            case SkinnedModelPreset::VIEW_WEAPON_P90:           return ConfigureViewWeaponP90(object);
            case SkinnedModelPreset::VIEW_WEAPON_REMINGTON_870: return ConfigureViewWeaponRemington870(object);
            case SkinnedModelPreset::VIEW_WEAPON_SPAS:          return ConfigureViewWeaponSPAS(object);
            case SkinnedModelPreset::VIEW_WEAPON_TOKAREV:       return ConfigureViewWeaponTokarev(object);
            default: Logging::Error() << "Bible::ConfigureSkinnedModel(..) received unsupported preset '" << Hell::Enum::ToString(preset) << "'\n"; return;
        }
    }

    const std::vector<std::string>& GetSkinnedModelPresetNames() {
        static std::vector<std::string> names; 
    
        if (names.empty()) {
            names = Hell::Enum::GetNames<SkinnedModelPreset>();
            Hell::String::RemoveFromVector(names, "UNDEFINED");
        }

        return names;
    }
}
