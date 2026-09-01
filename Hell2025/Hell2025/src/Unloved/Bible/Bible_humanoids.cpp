#include "Bible.h"

#include "Hell/Logging.h"

#include <map>

namespace Unloved::Bible {
    std::map<SkinnedModelPreset, HumanoidInfo> g_humanoidInfos;
    
    void InitHumanoidCatalog() {
        g_humanoidInfos.clear();

        HumanoidInfo& ratKing = g_humanoidInfos[SkinnedModelPreset::RAT_KING];
        ratKing.lowerBodyBoneMasks = { "RatKing_LowerBody" };
        ratKing.chestBoneMasks = { "RatKing_Chest" };
        ratKing.upperBodyBoneMasks = { "RatKing_ArmsHead" };
    }

    AnimationProfile GetHumanoidAnimationProfile(SkinnedModelPreset bodyPreset, Weapon weapon) {
        if (bodyPreset == SkinnedModelPreset::RAT_KING && weapon == Weapon::AKS74U)        return AnimationProfile::RAT_KING_AKS74U;
        if (bodyPreset == SkinnedModelPreset::RAT_KING && weapon == Weapon::GLOCK)         return AnimationProfile::RAT_KING_GLOCK;
        if (bodyPreset == SkinnedModelPreset::RAT_KING && weapon == Weapon::GOLDEN_GLOCK)  return AnimationProfile::RAT_KING_GLOCK;
        if (bodyPreset == SkinnedModelPreset::RAT_KING && weapon == Weapon::KNIFE)         return AnimationProfile::RAT_KING_KNIFE;
        if (bodyPreset == SkinnedModelPreset::RAT_KING && weapon == Weapon::REMINGTON_870) return AnimationProfile::RAT_KING_REMINGTON870;
        if (bodyPreset == SkinnedModelPreset::RAT_KING && weapon == Weapon::P90)           return AnimationProfile::RAT_KING_P90;
        if (bodyPreset == SkinnedModelPreset::RAT_KING && weapon == Weapon::SPAS)          return AnimationProfile::RAT_KING_SPAS;
        if (bodyPreset == SkinnedModelPreset::RAT_KING && weapon == Weapon::TOKAREV)       return AnimationProfile::RAT_KING_TOKAREV;
        return AnimationProfile::UNDEFINED;
    }

    const HumanoidInfo* GetHumanoidInfo(SkinnedModelPreset bodyPreset) {
        auto it = g_humanoidInfos.find(bodyPreset);
        if (it != g_humanoidInfos.end()) return &it->second;

        Logging::Error() << "Bible::GetHumanoidInfo(..) received an unsupported body preset\n";
        return nullptr;
    }
}
