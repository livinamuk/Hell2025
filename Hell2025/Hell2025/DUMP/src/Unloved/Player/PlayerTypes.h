#pragma once

#include "Hell/Input/keycodes.h"

struct PlayerControls {
    unsigned int WALK_FORWARD = HELL_KEY_W;
    unsigned int WALK_BACKWARD = HELL_KEY_S;
    unsigned int WALK_LEFT = HELL_KEY_A;
    unsigned int WALK_RIGHT = HELL_KEY_D;
    unsigned int INTERACT = HELL_KEY_E;
    unsigned int RELOAD = HELL_KEY_R;
    unsigned int FIRE = HELL_MOUSE_LEFT;
    unsigned int ADS = HELL_MOUSE_RIGHT;
    unsigned int JUMP = HELL_KEY_SPACE;
    unsigned int CROUCH = HELL_KEY_WIN_CONTROL;
    unsigned int NEXT_WEAPON = HELL_KEY_Q;
    unsigned int ESCAPE = HELL_KEY_WIN_ESCAPE;
    unsigned int DEBUG_FULLSCREEN = HELL_KEY_G;
    unsigned int DEBUG_ONE = HELL_KEY_1;
    unsigned int DEBUG_TWO = HELL_KEY_2;
    unsigned int DEBUG_THREE = HELL_KEY_3;
    unsigned int DEBUG_FOUR = HELL_KEY_4;
    unsigned int MELEE = HELL_KEY_V;
    unsigned int FLASHLIGHT = HELL_KEY_F;
    unsigned int MISC_WEAPON_FUNCTION = HELL_KEY_T;
    unsigned int RUN = HELL_KEY_LEFT_SHIFT;
    unsigned int TOGGLE_INVENTORY = HELL_KEY_WIN_TAB;
};

struct WaterState {
    float heightBeneathWater = 0;
    float heightAboveWater = 0;

    bool cameraUnderWater = false;
    bool feetUnderWater = false;
    bool wading = false;
    bool swimming = false;

    bool cameraUnderWaterPrevious = false;
    bool feetUnderWaterPrevious = false;
    bool wadingPrevious = true;
    bool swimmingPrevious = true;
};
