#pragma once

enum class ShadingMode {
    SHADED,
    WIREFRAME,
    WIREFRAME_OVERLAY,
    SHADING_MODE_COUNT
};

enum struct IESProfileType {
    NONE = 0,
    LAMP_0,
    LAMP_1,
    LAMP_2,
    LAMP_3,
    LAMP_4,
    LAMP_5,
    LAMP_6,
    LAMP_7,
    LAMP_8,
    LAMP_9,
    LAMP_10,
    LAMP_11,
};

enum struct ItemType {
    HEAL,
    WEAPON,
    KEY,
    AMMO,
    USELESS,
    UNDEFINED
};


enum InputType {
    KEYBOARD_AND_MOUSE,
    CONTROLLER
};



enum struct CollisionShapeType {
    BOX,
    CAPSULE,
    CONVEX_MESH,
    UNDEFINED
};



enum WeaponAction {
    IDLE = 0,
    FIRE,
    DRY_FIRE,
    RELOAD,
    RELOAD_FROM_EMPTY,
    DRAW_BEGIN,
    DRAWING,
    DRAWING_FIRST,
    DRAWING_WITH_SHOTGUN_PUMP,
    SPAWNING,
    SHOTGUN_RELOAD_BEGIN,
    SHOTGUN_RELOAD_SINGLE_SHELL,
    SHOTGUN_RELOAD_DOUBLE_SHELL,
    SHOTGUN_RELOAD_END,
    SHOTGUN_RELOAD_END_WITH_PUMP,
    SECONDARY_MELEE,
    ADS_IN,
    ADS_OUT,
    ADS_IDLE,
    ADS_FIRE,
    MELEE,
    TOGGLING_AUTO,
    UNDEFINED
};

enum class ShellEjectionState {
    IDLE, AWAITING_SHELL_EJECTION
};



//enum struct PickUpTypeOld {
//    SHOTGUN_AMMO_BUCKSHOT,
//    SHOTGUN_AMMO_SLUG,
//    GLOCK,
//    GOLDEN_GLOCK,
//    AKS74U,
//    SPAS,
//    REMINGTON_870,
//    TOKAREV,
//    UNDEFINED
//};

enum struct OpeningState {
    CLOSED,
    CLOSING,
    OPEN,
    OPENING,
    UNDEFINED
};


enum struct InventoryState {
    CLOSED,
    MAIN_SCREEN,
    EXAMINE_ITEM,
    MOVING_ITEM,
    ROTATING_ITEM,
    SHOP,
    UNDEFINED
};
