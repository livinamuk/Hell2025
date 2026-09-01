#pragma once

#include <cstdint>


enum struct DecalType {
    GLASS,
    PLASTER,
    UNDEFINED
};

enum struct TrimType {
    NONE,
    TIMBER,
    PLASTER,
    UNDEFINED
};

enum struct TrimSetType {
    FLOOR,
    MIDDLE,
    CEILING,
    CEILING_FANCY,
    UNDEFINED
};


enum struct LightType {
    HANGING_LIGHT = 0,
    FIREPLACE_FIRE,
    WALL_LAMP,
    UNDEFINED
};

enum struct WorldPlaneType {
    FLOOR,
    CEILING,
    UNDEFINED
};

enum struct PlanarQuadObjectType {
    DECKING_BOARDS,
    ROOFING_IRON,
    UNDEFINED
};

enum struct PointPairObjectType {
    DECKING_BEARER,
    DECKING_POST,
    GUTTER,
    RIDGE_CAPPING,
    DOWN_PIPE,
    UNDEFINED
};

enum struct WallType {
    INTERIOR,
    WEATHER_BOARDS,
    UNDEFINED
};

enum struct TreeType {
    TREE_LARGE_0 = 0,
    TREE_LARGE_1,
    TREE_LARGE_2,
    BLACK_BERRIES,
    UNDEFINED
};


enum class PictureFrameType {
    BIG_LANDSCAPE,
    TALL_THIN,
    REGULAR_PORTRAIT,
    REGULAR_LANDSCAPE,
    UNDEFINED
};

enum class ChristmasPresentType : uint32_t {
    SMALL = 0,
    MEDIUM,
    LARGE,
    UNDEFINED
};

enum struct FireplaceType {
    WOOD_STOVE,
    DEFAULT,
    UNDEFINED
};

enum struct GenericObjectType {
    CHRISTMAS_TREE,
    CHRISTMAS_PRESENT_SMALL,
    CHRISTMAS_PRESENT_LARGE,

    DRAWERS_SMALL,
    DRAWERS_LARGE,
    TOILET,
    COUCH,
    BATHROOM_BASIN,
    BATHROOM_CABINET,
    BATHROOM_TOWEL_RACK,

    CHAIR_RE,
    CHAIR_SPINDLE_BACK,

    DEER_HEAD,

    MERMAID_ROCK,

    PLANT_BLACKBERRIES,
    PLANT_TREE,

    TEST_MODEL,
    TEST_MODEL2,
    TEST_MODEL3,
    TEST_MODEL4,
    UNDEFINED
};

enum struct GenericAnimatedObjectType {
    RAT_KING,
    UNDEFINED
};

enum struct SkinnedModelPreset {
    RATKING
};

enum struct DoorType {
    STANDARD_A,
    STANDARD_B,
    STAINED_GLASS,
    STAINED_GLASS2,
    UNDEFINED
};

enum struct DoorMaterialType {
    WHITE_PAINT,
    BACK_PAINT,
    RESIDENT_EVIL,
    UNDEFINED
};

enum struct ChainLinkType {
    DOOR_CHAIN,
    UNDEFINED
};
