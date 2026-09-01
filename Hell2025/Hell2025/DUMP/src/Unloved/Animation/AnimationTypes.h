#pragma once

namespace Unloved {

    enum class AnimationProfile {
        DOBERMANN,
        RAT_KING_GLOCK,
        RAT_KING_SPAS,
        SHARK,
        VIEW_WEAPON_AKS74U,
        VIEW_WEAPON_GLOCK,
        VIEW_WEAPON_KNIFE,
        VIEW_WEAPON_P90,
        VIEW_WEAPON_REMINGTON870,
        VIEW_WEAPON_SPAS,
        VIEW_WEAPON_TOKAREV,
        UNDEFINED
    };

    enum class AnimationSlot {
        IDLE,
        WALK,
        IDLE_CROUCHING,
        WALK_CROUCHING,
        DRAW,
        DRAW_FIRST,
        FIRE_1,
        FIRE_2,
        FIRE_3,
        RELOAD,
        RELOAD_EMPTY,
        ADS_IN,
        ADS_OUT,
        ADS_IDLE,
        ADS_WALK,
        ADS_FIRE_1,
        ADS_FIRE_2,
        ADS_FIRE_3,
        MELEE,
        DRY_FIRE,
        TOGGLE_AUTO,
        SHOTGUN_DRAW_PUMP,
        SHOTGUN_FIRE_NO_PUMP,
        SHOTGUN_RELOAD_START,
        SHOTGUN_RELOAD_END,
        SHOTGUN_RELOAD_END_PUMP,
        SHOTGUN_RELOAD_ONE_SHELL,
        SHOTGUN_RELOAD_TWO_SHELLS,
        DOBERMANN_LAY,
        DOBERMANN_LAY_TO_WALK,
        DOBERMANN_STRETCH_TO_LAY,
        DOBERMANN_WALK,

        SHARK_SWIM,
        SHARK_ATTACK_LEFT,
        SHARK_ATTACK_RIGHT,
        SHARK_DEATH
    };

}
