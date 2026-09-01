#include "../Bible.h"

#include "Hell/Common/Constants.h"

#include <map>
#include <string>

namespace Bible {
    using namespace Unloved;

    namespace {
        struct AnimationSet {
            void Set(AnimationSlot slot, const std::string& animationName) { m_animationNames[slot] = animationName; }
            const std::string& Get(AnimationSlot slot) const { return m_animationNames.at(slot); }
            bool Has(AnimationSlot slot) const { return m_animationNames.contains(slot); }

        private:
            std::map<AnimationSlot, std::string> m_animationNames;
        };

        std::map<AnimationProfile, AnimationSet> g_animationSets;
    }

    void InitAnimationCatalog() {
        g_animationSets.clear();

        AnimationSet& viewWeaponAks74u = g_animationSets[AnimationProfile::VIEW_WEAPON_AKS74U];
        viewWeaponAks74u.Set(AnimationSlot::IDLE,       "AKS74U_Idle");
        viewWeaponAks74u.Set(AnimationSlot::WALK,       "AKS74U_Walk");
        viewWeaponAks74u.Set(AnimationSlot::DRAW,       "AKS74U_Draw");
        viewWeaponAks74u.Set(AnimationSlot::DRAW_FIRST, "AKS74U_DrawFirst");
        viewWeaponAks74u.Set(AnimationSlot::FIRE_1,     "AKS74U_Fire0");
        viewWeaponAks74u.Set(AnimationSlot::FIRE_2,     "AKS74U_Fire1");
        viewWeaponAks74u.Set(AnimationSlot::FIRE_3,     "AKS74U_Fire2");
        viewWeaponAks74u.Set(AnimationSlot::RELOAD,     "AKS74U_Reload");
        viewWeaponAks74u.Set(AnimationSlot::RELOAD_EMPTY, "AKS74U_ReloadEmpty");
        viewWeaponAks74u.Set(AnimationSlot::ADS_IN,     "AKS74U_ADS_In");
        viewWeaponAks74u.Set(AnimationSlot::ADS_OUT,    "AKS74U_ADS_Out");
        viewWeaponAks74u.Set(AnimationSlot::ADS_IDLE,   "AKS74U_ADS_Idle");
        viewWeaponAks74u.Set(AnimationSlot::ADS_WALK,   "AKS74U_ADS_Walk");
        viewWeaponAks74u.Set(AnimationSlot::ADS_FIRE_1, "AKS74U_ADS_Fire0");
        viewWeaponAks74u.Set(AnimationSlot::ADS_FIRE_2, "AKS74U_ADS_Fire1");
        viewWeaponAks74u.Set(AnimationSlot::ADS_FIRE_3, "AKS74U_ADS_Fire2");

        AnimationSet& dobermann = g_animationSets[AnimationProfile::DOBERMANN];
        dobermann.Set(AnimationSlot::DOBERMANN_LAY,            "Dobermann_Lay");
        dobermann.Set(AnimationSlot::DOBERMANN_LAY_TO_WALK,    "Dobermann_Lay_to_Walk");
        dobermann.Set(AnimationSlot::DOBERMANN_STRETCH_TO_LAY, "Dobermann_Stretch_to_Lay");
        dobermann.Set(AnimationSlot::DOBERMANN_WALK,           "Dobermann_Walk");

        AnimationSet& viewWeaponGlock = g_animationSets[AnimationProfile::VIEW_WEAPON_GLOCK];
        viewWeaponGlock.Set(AnimationSlot::IDLE,       "Glock_Idle");
        viewWeaponGlock.Set(AnimationSlot::WALK,       "Glock_Walk");
        viewWeaponGlock.Set(AnimationSlot::DRAW,       "Glock_Draw");
        viewWeaponGlock.Set(AnimationSlot::DRAW_FIRST, "Glock_DrawFirst");
        viewWeaponGlock.Set(AnimationSlot::FIRE_1,     "Glock_Fire1");
        viewWeaponGlock.Set(AnimationSlot::FIRE_2,     "Glock_Fire2");
        viewWeaponGlock.Set(AnimationSlot::FIRE_3,     "Glock_Fire3");
        viewWeaponGlock.Set(AnimationSlot::RELOAD,     "Glock_Reload");
        viewWeaponGlock.Set(AnimationSlot::RELOAD_EMPTY, "Glock_ReloadEmpty");
        viewWeaponGlock.Set(AnimationSlot::ADS_IN,     "Glock_ADS_In");
        viewWeaponGlock.Set(AnimationSlot::ADS_OUT,    "Glock_ADS_Out");
        viewWeaponGlock.Set(AnimationSlot::ADS_IDLE,   "Glock_ADS_Idle");
        viewWeaponGlock.Set(AnimationSlot::ADS_WALK,   "Glock_ADS_Walk");
        viewWeaponGlock.Set(AnimationSlot::ADS_FIRE_1, "Glock_ADS_Fire1");
        viewWeaponGlock.Set(AnimationSlot::ADS_FIRE_2, "Glock_ADS_Fire2");
        viewWeaponGlock.Set(AnimationSlot::ADS_FIRE_3, "Glock_ADS_Fire3");

        AnimationSet& viewWeaponKnife = g_animationSets[AnimationProfile::VIEW_WEAPON_KNIFE];
        viewWeaponKnife.Set(AnimationSlot::IDLE,   "Knife_Idle");
        viewWeaponKnife.Set(AnimationSlot::WALK,   "Knife_Walk");
        viewWeaponKnife.Set(AnimationSlot::DRAW,   "Knife_Draw");
        viewWeaponKnife.Set(AnimationSlot::FIRE_1, "Knife_Swing0");
        viewWeaponKnife.Set(AnimationSlot::FIRE_2, "Knife_Swing1");
        viewWeaponKnife.Set(AnimationSlot::FIRE_3, "Knife_Swing2");

        AnimationSet& viewWeaponP90 = g_animationSets[AnimationProfile::VIEW_WEAPON_P90];
        viewWeaponP90.Set(AnimationSlot::IDLE,         "P90_Idle");
        viewWeaponP90.Set(AnimationSlot::WALK,         "P90_Walk");
        viewWeaponP90.Set(AnimationSlot::DRAW,         "P90_Draw");
        viewWeaponP90.Set(AnimationSlot::DRAW_FIRST,   "P90_DrawFirst");
        viewWeaponP90.Set(AnimationSlot::FIRE_1,       "P90_Fire1");
        viewWeaponP90.Set(AnimationSlot::FIRE_2,       "P90_Fire2");
        viewWeaponP90.Set(AnimationSlot::FIRE_3,       "P90_Fire3");
        viewWeaponP90.Set(AnimationSlot::RELOAD,       "P90_Reload");
        viewWeaponP90.Set(AnimationSlot::RELOAD_EMPTY, "P90_ReloadEmpty");
        viewWeaponP90.Set(AnimationSlot::ADS_IN,       "P90_ADS_In");
        viewWeaponP90.Set(AnimationSlot::ADS_OUT,      "P90_ADS_Out");
        viewWeaponP90.Set(AnimationSlot::ADS_IDLE,     "P90_ADS_Idle");
        viewWeaponP90.Set(AnimationSlot::ADS_WALK,     "P90_ADS_Walk");
        viewWeaponP90.Set(AnimationSlot::ADS_FIRE_1,   "P90_ADS_Fire1");
        viewWeaponP90.Set(AnimationSlot::ADS_FIRE_2,   "P90_ADS_Fire2");
        viewWeaponP90.Set(AnimationSlot::ADS_FIRE_3,   "P90_ADS_Fire3");

        AnimationSet& ratKingGlock = g_animationSets[AnimationProfile::RAT_KING_GLOCK];
        ratKingGlock.Set(AnimationSlot::IDLE,           "RatKing_Idle_Standing");
        ratKingGlock.Set(AnimationSlot::IDLE_CROUCHING, "RatKing_Idle_Crouching");
        ratKingGlock.Set(AnimationSlot::WALK,           "RatKing_Walk_Standing");
        ratKingGlock.Set(AnimationSlot::WALK_CROUCHING, "RatKing_Walk_Crouching");
        ratKingGlock.Set(AnimationSlot::FIRE_1,         "RatKing_Glock_Fire1");
        ratKingGlock.Set(AnimationSlot::FIRE_2,         "RatKing_Glock_Fire2");
        ratKingGlock.Set(AnimationSlot::FIRE_3,         "RatKing_Glock_Fire3");
        ratKingGlock.Set(AnimationSlot::RELOAD,         "RatKing_Glock_Reload");
        ratKingGlock.Set(AnimationSlot::RELOAD_EMPTY,   "RatKing_Glock_ReloadEmpty");

        AnimationSet& ratKingSpas = g_animationSets[AnimationProfile::RAT_KING_SPAS];
        // TODO

        AnimationSet& viewWeaponRemington870 = g_animationSets[AnimationProfile::VIEW_WEAPON_REMINGTON870];
        viewWeaponRemington870.Set(AnimationSlot::IDLE, "Remington870_Idle");
        viewWeaponRemington870.Set(AnimationSlot::WALK, "Remington870_Walk");
        viewWeaponRemington870.Set(AnimationSlot::DRAW, "Remington870_Draw");
        viewWeaponRemington870.Set(AnimationSlot::FIRE_1, "Remington870_Fire");
        viewWeaponRemington870.Set(AnimationSlot::MELEE, "Remington870_Melee");
        viewWeaponRemington870.Set(AnimationSlot::DRY_FIRE, "SPAS_DryFire");
        viewWeaponRemington870.Set(AnimationSlot::SHOTGUN_DRAW_PUMP, "Remington870_DrawPump");
        viewWeaponRemington870.Set(AnimationSlot::SHOTGUN_FIRE_NO_PUMP, "Remington870_FireNoPump");
        viewWeaponRemington870.Set(AnimationSlot::SHOTGUN_RELOAD_START, "Remington870_Reload_Start");
        viewWeaponRemington870.Set(AnimationSlot::SHOTGUN_RELOAD_END, "Remington870_Reload_End");
        viewWeaponRemington870.Set(AnimationSlot::SHOTGUN_RELOAD_END_PUMP, "Remington870_Reload_Pump_End");
        viewWeaponRemington870.Set(AnimationSlot::SHOTGUN_RELOAD_ONE_SHELL, "Remington870_Reload_Shells1");
        viewWeaponRemington870.Set(AnimationSlot::SHOTGUN_RELOAD_TWO_SHELLS, "Remington870_Reload_Shells2");

        AnimationSet& shark = g_animationSets[AnimationProfile::SHARK];
        shark.Set(AnimationSlot::SHARK_ATTACK_LEFT,  "Shark_Attack_Left_Quick");
        shark.Set(AnimationSlot::SHARK_ATTACK_RIGHT, "Shark_Attack_Right_Quick");
        shark.Set(AnimationSlot::SHARK_DEATH,        "Shark_Die");
        shark.Set(AnimationSlot::SHARK_SWIM,         "Shark_Swim");

        AnimationSet& viewWeaponSpas = g_animationSets[AnimationProfile::VIEW_WEAPON_SPAS];
        viewWeaponSpas.Set(AnimationSlot::IDLE, "SPAS_Idle");
        viewWeaponSpas.Set(AnimationSlot::WALK, "SPAS_Walk");
        viewWeaponSpas.Set(AnimationSlot::DRAW, "SPAS_Draw");
        viewWeaponSpas.Set(AnimationSlot::FIRE_1, "SPAS_Fire");
        viewWeaponSpas.Set(AnimationSlot::MELEE, "SPAS_Melee");
        viewWeaponSpas.Set(AnimationSlot::DRY_FIRE, "SPAS_DryFire");
        viewWeaponSpas.Set(AnimationSlot::TOGGLE_AUTO, "SPAS_ToggleAuto");
        viewWeaponSpas.Set(AnimationSlot::SHOTGUN_DRAW_PUMP, "SPAS_DrawPump");
        viewWeaponSpas.Set(AnimationSlot::SHOTGUN_FIRE_NO_PUMP, "SPAS_FireNoPump");
        viewWeaponSpas.Set(AnimationSlot::SHOTGUN_RELOAD_START, "SPAS_ReloadStart");
        viewWeaponSpas.Set(AnimationSlot::SHOTGUN_RELOAD_END, "SPAS_ReloadEnd");
        viewWeaponSpas.Set(AnimationSlot::SHOTGUN_RELOAD_END_PUMP, "SPAS_ReloadEndPump");
        viewWeaponSpas.Set(AnimationSlot::SHOTGUN_RELOAD_ONE_SHELL, "SPAS_Reload1Shell");
        viewWeaponSpas.Set(AnimationSlot::SHOTGUN_RELOAD_TWO_SHELLS, "SPAS_Reload2Shells");
        AnimationSet& viewWeaponTokarev = g_animationSets[AnimationProfile::VIEW_WEAPON_TOKAREV];
        viewWeaponTokarev.Set(AnimationSlot::IDLE,       "Tokarev_Idle");
        viewWeaponTokarev.Set(AnimationSlot::WALK,       "Tokarev_Walk");
        viewWeaponTokarev.Set(AnimationSlot::DRAW,       "Tokarev_Draw");
        viewWeaponTokarev.Set(AnimationSlot::DRAW_FIRST, "Tokarev_DrawFirst");
        viewWeaponTokarev.Set(AnimationSlot::FIRE_1,     "Tokarev_Fire1");
        viewWeaponTokarev.Set(AnimationSlot::FIRE_2,     "Tokarev_Fire2");
        viewWeaponTokarev.Set(AnimationSlot::FIRE_3,     "Tokarev_Fire3");
        viewWeaponTokarev.Set(AnimationSlot::RELOAD,     "Tokarev_Reload");
        viewWeaponTokarev.Set(AnimationSlot::RELOAD_EMPTY, "Tokarev_ReloadEmpty");
        viewWeaponTokarev.Set(AnimationSlot::ADS_IN,     "Tokarev_ADS_In");
        viewWeaponTokarev.Set(AnimationSlot::ADS_OUT,    "Tokarev_ADS_Out");
        viewWeaponTokarev.Set(AnimationSlot::ADS_IDLE,   "Tokarev_ADS_Idle");
        viewWeaponTokarev.Set(AnimationSlot::ADS_WALK,   "Tokarev_ADS_Walk");
        viewWeaponTokarev.Set(AnimationSlot::ADS_FIRE_1, "Tokarev_ADS_Fire1");
        viewWeaponTokarev.Set(AnimationSlot::ADS_FIRE_2, "Tokarev_ADS_Fire2");
        viewWeaponTokarev.Set(AnimationSlot::ADS_FIRE_3, "Tokarev_ADS_Fire3");
    }

    const std::string& GetAnimation(AnimationProfile animationProfile, AnimationSlot animationSlot) {
        static std::string invalid = UNDEFINED_STRING;

        auto animationSetIt = g_animationSets.find(animationProfile);
        if (animationSetIt == g_animationSets.end()) {
            return invalid;
        }

        const AnimationSet& animationSet = animationSetIt->second;
        if (animationSet.Has(animationSlot)) {
            return animationSet.Get(animationSlot);
        }

        return invalid;
    }

}
