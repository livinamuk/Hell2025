#include "Bible.h"

#include "Hell/Common/Constants.h"

#include <map>
#include <string>

namespace Unloved::Bible {

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

    void InitEnemyAnimations();
    void InitRatKingAnimations();
    void InitViewWeaponsAnimations();

    void InitAnimationCatalog() {
        g_animationSets.clear();

        InitEnemyAnimations();
        InitRatKingAnimations();
        InitViewWeaponsAnimations();
    }

    void InitRatKingAnimations() {
        AnimationSet& glock = g_animationSets[AnimationProfile::RAT_KING_GLOCK];
        glock.Set(AnimationSlot::IDLE,           "RatKing_Idle_Standing");
        glock.Set(AnimationSlot::IDLE_CROUCHING, "RatKing_Idle_Crouching");
        glock.Set(AnimationSlot::WALK,           "RatKing_Walk_Standing");
        glock.Set(AnimationSlot::WALK_CROUCHING, "RatKing_Walk_Crouching");
        glock.Set(AnimationSlot::FIRE_1,         "RatKing_Glock_Fire1");
        glock.Set(AnimationSlot::FIRE_2,         "RatKing_Glock_Fire2");
        glock.Set(AnimationSlot::FIRE_3,         "RatKing_Glock_Fire3");
        glock.Set(AnimationSlot::RELOAD,         "RatKing_Glock_Reload");
        glock.Set(AnimationSlot::RELOAD_EMPTY,   "RatKing_Glock_ReloadEmpty");
        glock.Set(AnimationSlot::MELEE,          "RatKing_Glock_Melee");
        glock.Set(AnimationSlot::DRAW,           "RatKing_Glock_Draw");
        glock.Set(AnimationSlot::DRAW_FIRST,     "RatKing_Glock_DrawFirst");
        glock.Set(AnimationSlot::JUMP,           "RatKing_Jump");
    }

    void InitEnemyAnimations() {
        AnimationSet& dobermann = g_animationSets[AnimationProfile::DOBERMANN];
        dobermann.Set(AnimationSlot::DOBERMANN_LAY,            "Dobermann_Lay");
        dobermann.Set(AnimationSlot::DOBERMANN_LAY_TO_WALK,    "Dobermann_Lay_to_Walk");
        dobermann.Set(AnimationSlot::DOBERMANN_STRETCH_TO_LAY, "Dobermann_Stretch_to_Lay");
        dobermann.Set(AnimationSlot::DOBERMANN_WALK,           "Dobermann_Walk");

        AnimationSet& snake = g_animationSets[AnimationProfile::SNAKE];
        snake.Set(AnimationSlot::SNAKE_SLITHER, "Snake_Slither");

        AnimationSet& shark = g_animationSets[AnimationProfile::SHARK];
        shark.Set(AnimationSlot::SHARK_ATTACK_LEFT,  "Shark_Attack_Left_Quick");
        shark.Set(AnimationSlot::SHARK_ATTACK_RIGHT, "Shark_Attack_Right_Quick");
        shark.Set(AnimationSlot::SHARK_DEATH,        "Shark_Die");
        shark.Set(AnimationSlot::SHARK_SWIM,         "Shark_Swim");
    }

    void InitViewWeaponsAnimations() {
        AnimationSet& aks74u = g_animationSets[AnimationProfile::VIEW_WEAPON_AKS74U];
        aks74u.Set(AnimationSlot::IDLE,         "AKS74U_Idle");
        aks74u.Set(AnimationSlot::WALK,         "AKS74U_Walk");
        aks74u.Set(AnimationSlot::DRAW,         "AKS74U_Draw");
        aks74u.Set(AnimationSlot::DRAW_FIRST,   "AKS74U_DrawFirst");
        aks74u.Set(AnimationSlot::FIRE_1,       "AKS74U_Fire0");
        aks74u.Set(AnimationSlot::FIRE_2,       "AKS74U_Fire1");
        aks74u.Set(AnimationSlot::FIRE_3,       "AKS74U_Fire2");
        aks74u.Set(AnimationSlot::RELOAD,       "AKS74U_Reload");
        aks74u.Set(AnimationSlot::RELOAD_EMPTY, "AKS74U_ReloadEmpty");
        aks74u.Set(AnimationSlot::ADS_IN,       "AKS74U_ADS_In");
        aks74u.Set(AnimationSlot::ADS_OUT,      "AKS74U_ADS_Out");
        aks74u.Set(AnimationSlot::ADS_IDLE,     "AKS74U_ADS_Idle");
        aks74u.Set(AnimationSlot::ADS_WALK,     "AKS74U_ADS_Walk");
        aks74u.Set(AnimationSlot::ADS_FIRE_1,   "AKS74U_ADS_Fire0");
        aks74u.Set(AnimationSlot::ADS_FIRE_2,   "AKS74U_ADS_Fire1");
        aks74u.Set(AnimationSlot::ADS_FIRE_3,   "AKS74U_ADS_Fire2");
        aks74u.Set(AnimationSlot::MELEE,        "AKS74U_Melee");

        AnimationSet& glock = g_animationSets[AnimationProfile::VIEW_WEAPON_GLOCK];
        glock.Set(AnimationSlot::IDLE,         "Glock_Idle");
        glock.Set(AnimationSlot::WALK,         "Glock_Walk");
        glock.Set(AnimationSlot::DRAW,         "Glock_Draw");
        glock.Set(AnimationSlot::DRAW_FIRST,   "Glock_DrawFirst");
        glock.Set(AnimationSlot::FIRE_1,       "Glock_Fire1");
        glock.Set(AnimationSlot::FIRE_2,       "Glock_Fire2");
        glock.Set(AnimationSlot::FIRE_3,       "Glock_Fire3");
        glock.Set(AnimationSlot::RELOAD,       "Glock_Reload");
        glock.Set(AnimationSlot::RELOAD_EMPTY, "Glock_ReloadEmpty");
        glock.Set(AnimationSlot::ADS_IN,       "Glock_ADS_In");
        glock.Set(AnimationSlot::ADS_OUT,      "Glock_ADS_Out");
        glock.Set(AnimationSlot::ADS_IDLE,     "Glock_ADS_Idle");
        glock.Set(AnimationSlot::ADS_WALK,     "Glock_ADS_Walk");
        glock.Set(AnimationSlot::ADS_FIRE_1,   "Glock_ADS_Fire1");
        glock.Set(AnimationSlot::ADS_FIRE_2,   "Glock_ADS_Fire2");
        glock.Set(AnimationSlot::ADS_FIRE_3,   "Glock_ADS_Fire3");
        glock.Set(AnimationSlot::MELEE,        "Glock_Melee");
 
        AnimationSet& knife = g_animationSets[AnimationProfile::VIEW_WEAPON_KNIFE];
        knife.Set(AnimationSlot::IDLE,   "Knife_Idle");
        knife.Set(AnimationSlot::WALK,   "Knife_Walk");
        knife.Set(AnimationSlot::DRAW,   "Knife_Draw");
        knife.Set(AnimationSlot::FIRE_1, "Knife_Swing0");
        knife.Set(AnimationSlot::FIRE_2, "Knife_Swing1");
        knife.Set(AnimationSlot::FIRE_3, "Knife_Swing2");

        AnimationSet& p90 = g_animationSets[AnimationProfile::VIEW_WEAPON_P90];
        p90.Set(AnimationSlot::IDLE,         "P90_Idle");
        p90.Set(AnimationSlot::WALK,         "P90_Walk");
        p90.Set(AnimationSlot::DRAW,         "P90_Draw");
        p90.Set(AnimationSlot::DRAW_FIRST,   "P90_DrawFirst");
        p90.Set(AnimationSlot::FIRE_1,       "P90_Fire1");
        p90.Set(AnimationSlot::FIRE_2,       "P90_Fire2");
        p90.Set(AnimationSlot::FIRE_3,       "P90_Fire3");
        p90.Set(AnimationSlot::RELOAD,       "P90_Reload");
        p90.Set(AnimationSlot::RELOAD_EMPTY, "P90_ReloadEmpty");
        p90.Set(AnimationSlot::ADS_IN,       "P90_ADS_In");
        p90.Set(AnimationSlot::ADS_OUT,      "P90_ADS_Out");
        p90.Set(AnimationSlot::ADS_IDLE,     "P90_ADS_Idle");
        p90.Set(AnimationSlot::ADS_WALK,     "P90_ADS_Walk");
        p90.Set(AnimationSlot::ADS_FIRE_1,   "P90_ADS_Fire1");
        p90.Set(AnimationSlot::ADS_FIRE_2,   "P90_ADS_Fire2");
        p90.Set(AnimationSlot::ADS_FIRE_3,   "P90_ADS_Fire3");
        p90.Set(AnimationSlot::MELEE,        "P90_Melee");

        AnimationSet& remington870 = g_animationSets[AnimationProfile::VIEW_WEAPON_REMINGTON870];
        remington870.Set(AnimationSlot::IDLE,                      "Remington870_Idle");
        remington870.Set(AnimationSlot::WALK,                      "Remington870_Walk");
        remington870.Set(AnimationSlot::DRAW,                      "Remington870_Draw");
        remington870.Set(AnimationSlot::FIRE_1,                    "Remington870_Fire");
        remington870.Set(AnimationSlot::MELEE,                     "Remington870_Melee");
        remington870.Set(AnimationSlot::DRY_FIRE,                  "SPAS_DryFire");
        remington870.Set(AnimationSlot::SHOTGUN_DRAW_PUMP,         "Remington870_DrawPump");
        remington870.Set(AnimationSlot::SHOTGUN_FIRE_NO_PUMP,      "Remington870_FireNoPump");
        remington870.Set(AnimationSlot::SHOTGUN_RELOAD_START,      "Remington870_Reload_Start");
        remington870.Set(AnimationSlot::SHOTGUN_RELOAD_END,        "Remington870_Reload_End");
        remington870.Set(AnimationSlot::SHOTGUN_RELOAD_END_PUMP,   "Remington870_Reload_Pump_End");
        remington870.Set(AnimationSlot::SHOTGUN_RELOAD_ONE_SHELL,  "Remington870_Reload_Shells1");
        remington870.Set(AnimationSlot::SHOTGUN_RELOAD_TWO_SHELLS, "Remington870_Reload_Shells2");

        AnimationSet& spas = g_animationSets[AnimationProfile::VIEW_WEAPON_SPAS];
        spas.Set(AnimationSlot::IDLE,                      "SPAS_Idle");
        spas.Set(AnimationSlot::WALK,                      "SPAS_Walk");
        spas.Set(AnimationSlot::DRAW,                      "SPAS_Draw");
        spas.Set(AnimationSlot::FIRE_1,                    "SPAS_Fire");
        spas.Set(AnimationSlot::MELEE,                     "SPAS_Melee");
        spas.Set(AnimationSlot::DRY_FIRE,                  "SPAS_DryFire");
        spas.Set(AnimationSlot::TOGGLE_AUTO,               "SPAS_ToggleAuto");
        spas.Set(AnimationSlot::SHOTGUN_DRAW_PUMP,         "SPAS_DrawPump");
        spas.Set(AnimationSlot::SHOTGUN_FIRE_NO_PUMP,      "SPAS_FireNoPump");
        spas.Set(AnimationSlot::SHOTGUN_RELOAD_START,      "SPAS_ReloadStart");
        spas.Set(AnimationSlot::SHOTGUN_RELOAD_END,        "SPAS_ReloadEnd");
        spas.Set(AnimationSlot::SHOTGUN_RELOAD_END_PUMP,   "SPAS_ReloadEndPump");
        spas.Set(AnimationSlot::SHOTGUN_RELOAD_ONE_SHELL,  "SPAS_Reload1Shell");
        spas.Set(AnimationSlot::SHOTGUN_RELOAD_TWO_SHELLS, "SPAS_Reload2Shells");

        AnimationSet& tokarev = g_animationSets[AnimationProfile::VIEW_WEAPON_TOKAREV];
        tokarev.Set(AnimationSlot::IDLE,         "Tokarev_Idle");
        tokarev.Set(AnimationSlot::WALK,         "Tokarev_Walk");
        tokarev.Set(AnimationSlot::DRAW,         "Tokarev_Draw");
        tokarev.Set(AnimationSlot::DRAW_FIRST,   "Tokarev_DrawFirst");
        tokarev.Set(AnimationSlot::FIRE_1,       "Tokarev_Fire1");
        tokarev.Set(AnimationSlot::FIRE_2,       "Tokarev_Fire2");
        tokarev.Set(AnimationSlot::FIRE_3,       "Tokarev_Fire3");
        tokarev.Set(AnimationSlot::RELOAD,       "Tokarev_Reload");
        tokarev.Set(AnimationSlot::RELOAD_EMPTY, "Tokarev_ReloadEmpty");
        tokarev.Set(AnimationSlot::ADS_IN,       "Tokarev_ADS_In");
        tokarev.Set(AnimationSlot::ADS_OUT,      "Tokarev_ADS_Out");
        tokarev.Set(AnimationSlot::ADS_IDLE,     "Tokarev_ADS_Idle");
        tokarev.Set(AnimationSlot::ADS_WALK,     "Tokarev_ADS_Walk");
        tokarev.Set(AnimationSlot::ADS_FIRE_1,   "Tokarev_ADS_Fire1");
        tokarev.Set(AnimationSlot::ADS_FIRE_2,   "Tokarev_ADS_Fire2");
        tokarev.Set(AnimationSlot::ADS_FIRE_3,   "Tokarev_ADS_Fire3");
        tokarev.Set(AnimationSlot::MELEE,        "Tokarev_Melee");
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

    bool HasAnimation(AnimationProfile animationProfile, AnimationSlot animationSlot) {
        auto animationSetIt = g_animationSets.find(animationProfile);
        return animationSetIt != g_animationSets.end() && animationSetIt->second.Has(animationSlot);
    }

}
