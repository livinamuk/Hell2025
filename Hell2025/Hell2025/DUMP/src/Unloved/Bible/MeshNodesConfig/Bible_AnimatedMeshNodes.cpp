#include "../Bible.h"

#include "Hell/Common/Enum.h"

namespace Bible {
    using namespace Unloved;

    void ConfigureAnimatedMeshNodesRatKing(uint64_t id, AnimatedMeshNodes* meshNodes);
    void ConfigureAnimatedMeshNodesTrapKing(uint64_t id, AnimatedMeshNodes* meshNodes);
    void ConfigureAnimatedMeshNodesRemington870(uint64_t id, AnimatedMeshNodes* meshNodes);
    void ConfigureAnimatedMeshNodesSPAS(uint64_t id, AnimatedMeshNodes* meshNodes);

    void ConfigureAnimatedMeshNodes(uint64_t id, AnimatedMeshNodes* meshNodes, SkinnedModelPreset preset) {
        switch (preset) {
            case SkinnedModelPreset::RATKING: ConfigureAnimatedMeshNodesRatKing(id, meshNodes); break;
        }
    }

    void ConfigureAnimatedMeshNodes(uint64_t id, AnimatedMeshNodes* meshNodes, const std::string& presetName) {
        const auto preset = magic_enum::enum_cast<SkinnedModelPreset>(presetName);
        if (preset) ConfigureAnimatedMeshNodes(id, meshNodes, *preset);
    }

    std::vector<std::string> GetAnimatedMeshNodePresetNames() {
        return Hell::Enum::GetNames<SkinnedModelPreset>();
    }

    void ConfigureAnimatedMeshNodesRatKing(uint64_t id, AnimatedMeshNodes* meshNodes) {
        meshNodes->SetSkinnedModel(id, "RatKing");

        // Body
        meshNodes->SetMeshMaterialByMeshName("CC_Base_Body", "RatKingHead");
        meshNodes->SetMeshMaterialByMeshName("CC_Base_Body2", "RatKingTorso");
        meshNodes->SetMeshMaterialByMeshName("CC_Base_Body3", "RatKingArms");
        meshNodes->SetMeshMaterialByMeshName("CC_Base_Body4", "RatKingLegs");
        meshNodes->SetMeshMaterialByMeshName("CC_Base_Body5", "RatKingNails");
        meshNodes->SetMeshMaterialByMeshName("CC_Base_Body6", "RatKingLashes2", BlendingMode::BLENDED);
        meshNodes->SetMeshMaterialByMeshName("CC_Base_Eye", "RatKingEye");
        meshNodes->SetBlendingModeByMeshName("CC_Base_Eye2", BlendingMode::DO_NOT_RENDER);
        meshNodes->SetMeshMaterialByMeshName("CC_Base_Eye3", "RatKingEye");
        meshNodes->SetBlendingModeByMeshName("CC_Base_Eye4", BlendingMode::DO_NOT_RENDER);
        meshNodes->SetBlendingModeByMeshName("CC_Base_TearLine", BlendingMode::DO_NOT_RENDER);
        meshNodes->SetBlendingModeByMeshName("CC_Base_TearLine2", BlendingMode::DO_NOT_RENDER);
        meshNodes->SetMeshMaterialByMeshName("CC_Base_Tongue", "TrapKingTongue");
        meshNodes->SetMeshMaterialByMeshName("CC_Base_Teeth", "TrapKingTeethUpper");
        meshNodes->SetMeshMaterialByMeshName("CC_Base_Teeth2", "TrapKingTeethLower");

        // Brows
        //meshNodes->SetMeshMaterialByMeshName("Brows_Bushy1", "???");
        meshNodes->SetBlendingModeByMeshName("Brows_Bushy", BlendingMode::DO_NOT_RENDER);
        meshNodes->SetMeshMaterialByMeshName("Brows_Bushy2", "RatKingBrows", BlendingMode::BLENDED);
        meshNodes->SetMeshMaterialByMeshName("Brows_Bushy3", "RatKingBrows2", BlendingMode::BLENDED);

        // Eyes
        meshNodes->SetBlendingModeByMeshName("CC_Base_EyeOcclusion", BlendingMode::DO_NOT_RENDER);
        meshNodes->SetBlendingModeByMeshName("CC_Base_EyeOcclusion2", BlendingMode::DO_NOT_RENDER);
        meshNodes->SetMeshMaterialByMeshName("Lash_Low_Short_Sparse", "RatKingLashes", BlendingMode::BLENDED);
        meshNodes->SetMeshMaterialByMeshName("Lash_Low_Sparse", "RatKingLashes", BlendingMode::BLENDED);
        meshNodes->SetMeshMaterialByMeshName("Lash_Up_Downward", "RatKingLashes", BlendingMode::BLENDED);
        meshNodes->SetMeshMaterialByMeshName("Lash_Up_Short_Sparse", "RatKingLashes", BlendingMode::BLENDED);

        // Hair
        meshNodes->SetMeshMaterialByMeshName("Side_swept_L", "RatKingHair", BlendingMode::HAIR);
        meshNodes->SetMeshMaterialByMeshName("Long_Hair_R", "RatKingHair", BlendingMode::HAIR);
        meshNodes->SetMeshMaterialByMeshName("Long_Hair_R2", "RatKingHair", BlendingMode::DO_NOT_RENDER); // some scalp
        meshNodes->SetMeshMaterialByMeshName("Side_swept_Long_Hair_L", "RatKingScalp", BlendingMode::DO_NOT_RENDER);
        meshNodes->SetMeshMaterialByMeshName("Side_swept_Long_Hair_L2", "RatKingHair", BlendingMode::HAIR);
        meshNodes->SetMeshMaterialByMeshName("Side_swept_Long_Hair_R", "RatKingHair", BlendingMode::HAIR);
        meshNodes->SetMeshMaterialByMeshName("Hair_Tattoo", "RatKingHeadTattoo", BlendingMode::BLENDED);
        meshNodes->SetMeshMaterialByMeshName("Hair_Tattoo2", "RatKingScalp", BlendingMode::BLENDED);
        meshNodes->SetMeshMaterialByMeshName("Scalp_Male", "RatKingScalp", BlendingMode::DO_NOT_RENDER);
        meshNodes->SetMeshMaterialByMeshName("Side_swept_L2", "RatKingScalp", BlendingMode::DO_NOT_RENDER);

        // Clothes
        meshNodes->SetMeshMaterialByMeshName("Boxers", "BoxersBlack");
        meshNodes->SetMeshMaterialByMeshName("Slim_Jeans", "Jeans");

        // Glock
        meshNodes->SetMeshMaterialByMeshName("GlockBarrel", "Glock");
        meshNodes->SetMeshMaterialByMeshName("GlockMagazine", "Glock");
        meshNodes->SetMeshMaterialByMeshName("GlockMagazine_02", "Glock");
        meshNodes->SetMeshMaterialByMeshName("GlockReceiver", "Glock");
        meshNodes->SetMeshMaterialByMeshName("GlockSlide", "Glock");
        meshNodes->SetMeshMaterialByMeshName("GlockSlideUnLock", "Glock");
        meshNodes->SetMeshMaterialByMeshName("GlockTrigger", "Glock");

        // Glock attachments
        meshNodes->SetMeshMaterialByMeshName("Supressor", "Glock");
        meshNodes->SetMeshMaterialByMeshName("LeupoldRedDot", "RedDot");
        meshNodes->SetMeshMaterialByMeshName("LeupoldRedDotGlass", "RedDotGlass", BlendingMode::GLASS);
    }

    void ConfigureAnimatedMeshNodesTrapKing(uint64_t id, AnimatedMeshNodes* meshNodes) {
        meshNodes->SetSkinnedModel(id, "TrapKing");

        // Body
        meshNodes->SetMeshMaterialByMeshName("Body", "TrapKingBodyHead");
        meshNodes->SetMeshMaterialByMeshName("Body2", "TrapKingBodyTorso");
        meshNodes->SetMeshMaterialByMeshName("Body3", "TrapKingBodyArms");
        meshNodes->SetMeshMaterialByMeshName("Body4", "TrapKingBodyLegs");
        meshNodes->SetMeshMaterialByMeshName("Body5", "TrapKingNails");
        meshNodes->SetMeshMaterialByMeshName("Body6", "TrapKingEyeLashes");
        meshNodes->SetBlendingModeByMeshName("Body6", BlendingMode::BLENDED);
        meshNodes->SetMeshMaterialByMeshName("Tongue", "TrapKingTongue");
        meshNodes->SetMeshMaterialByMeshName("Teeth", "TrapKingTeethUpper");
        meshNodes->SetMeshMaterialByMeshName("Teeth2", "TrapKingTeethLower");

        // Hair
        meshNodes->SetMeshMaterialByMeshName("DreadsTop", "TrapKingHairScalp");
        meshNodes->SetMeshMaterialByMeshName("DreadsBottom", "TrapKingHairScalp");
        meshNodes->SetMeshMaterialByMeshName("DreadsFront", "TrapKingHairScalp");
        meshNodes->SetMeshMaterialByMeshName("DreadsShoulder", "TrapKingHairScalp");
        meshNodes->SetMeshMaterialByMeshName("DreadsKnot", "TrapKingHairScalp");
        meshNodes->SetMeshMaterialByMeshName("DreadsScalp", "TrapKingHairScalp");
        meshNodes->SetBlendingModeByMeshName("DreadsScalp", BlendingMode::BLENDED);

        // Eyes
        meshNodes->SetBlendingModeByMeshName("EyeOcclusion", BlendingMode::DO_NOT_RENDER);
        meshNodes->SetBlendingModeByMeshName("EyeOcclusion2", BlendingMode::DO_NOT_RENDER);
        meshNodes->SetMeshMaterialByMeshName("Eye", "TrapKingEye");
        meshNodes->SetBlendingModeByMeshName("Eye2", BlendingMode::DO_NOT_RENDER);
        meshNodes->SetMeshMaterialByMeshName("Eye3", "TrapKingEye");
        meshNodes->SetBlendingModeByMeshName("Eye4", BlendingMode::DO_NOT_RENDER);
        meshNodes->SetBlendingModeByMeshName("Brow", BlendingMode::DO_NOT_RENDER);
        meshNodes->SetMeshMaterialByMeshName("Brow2", "TrapKingBrow");
        meshNodes->SetBlendingModeByMeshName("Brow2", BlendingMode::BLENDED);
        meshNodes->SetBlendingModeByMeshName("TearLine", BlendingMode::DO_NOT_RENDER);
        meshNodes->SetBlendingModeByMeshName("TearLine2", BlendingMode::DO_NOT_RENDER);

        // Clothes
        meshNodes->SetMeshMaterialByMeshName("Pants", "TrapKingPants");
        meshNodes->SetMeshMaterialByMeshName("Boxers", "TrapKingBoxes");
    }

    void ConfigureAnimatedMeshNodesRemington870(uint64_t id, AnimatedMeshNodes* meshNodes) {
        meshNodes->SetSkinnedModel(id, "Remington870");
        meshNodes->SetMeshMaterialByMeshName("ArmsMale", "ArmsMale");
        meshNodes->SetMeshMaterialByMeshName("ArmsFemale", "FemaleArms");
        meshNodes->SetMeshMaterialByMeshName("Shells", "Shell");
        meshNodes->SetMeshMaterialByMeshName("Shells.001", "Shell");
        meshNodes->SetMeshMaterialByMeshName("Shells.002", "Shell");
        meshNodes->SetMeshMaterialByMeshName("Shotgun", "Shotgun");
        meshNodes->PrintMeshNames();
    }

    void ConfigureAnimatedMeshNodesSPAS(uint64_t id, AnimatedMeshNodes* meshNodes) {
        meshNodes->SetSkinnedModel(id, "SPAS");
        meshNodes->SetMeshMaterialByMeshName("ArmsMale", "ArmsMale");
        meshNodes->SetMeshMaterialByMeshName("ArmsFemale", "FemaleArms");
        meshNodes->SetMeshMaterialByMeshName("Shells", "Shell");
        meshNodes->SetMeshMaterialByMeshName("Shells.002", "Shell");
        meshNodes->SetMeshMaterialByMeshName("SPAS12_Beachshroud", "SPAS2_Moving");
        meshNodes->SetMeshMaterialByMeshName("SPAS12_Bolt", "SPAS2_Moving");
        meshNodes->SetMeshMaterialByMeshName("SPAS12_Main", "SPAS2_Main");
        meshNodes->SetMeshMaterialByMeshName("SPAS12_Main_Moving_Low", "SPAS2_Moving");
        meshNodes->SetMeshMaterialByMeshName("SPAS12_Main_StampedSG", "SPAS2_Stamped");
        meshNodes->SetMeshMaterialByMeshName("SPAS12_Pumpslide", "SPAS2_Moving");
        meshNodes->SetMeshMaterialByMeshName("SPAS12_Ring", "SPAS2_Moving");
        meshNodes->SetMeshMaterialByMeshName("SPAS12_Stock_01", "SPAS2_Moving");
        meshNodes->SetMeshMaterialByMeshName("SPAS12_Stock_02", "SPAS2_Moving");
        meshNodes->SetMeshMaterialByMeshName("SPAS12_Stock_Holder", "SPAS2_Main");
        meshNodes->SetMeshMaterialByMeshName("SPAS12_Strock_Release", "SPAS2_Moving");
        meshNodes->SetMeshMaterialByMeshName("SPAS12_Trigger", "SPAS2_Moving");
    }
}


