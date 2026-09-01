#pragma once

#include <map>
#include <string>

namespace Unloved::EditorSession {

    enum class PlacementTool {
        NONE,

        CHRISTMAS_LIGHTS,
        DDGI_VOLUME,
        DOBERMANN,
        DOOR_STANDARD_A,
        DOOR_STANDARD_B,
        DOOR_GLASS,
        DOOR_GLASS2,
        FENCE_FARM,
        FIREPLACE_OPEN,
        FIREPLACE_WOOD_STOVE,
        GENERIC_ANIMATED_RAT_KING,
        GENERIC_BATHROOM_BASIN,
        GENERIC_BATHROOM_CABINET,
        GENERIC_BATHROOM_TOWEL_RACK,
        GENERIC_CHAIR_RE,
        GENERIC_CHAIR_SPINDLE_BACK,
        GENERIC_CHRISTMAS_PRESENT_LARGE,
        GENERIC_CHRISTMAS_PRESENT_SMALL,
        GENERIC_CHRISTMAS_TREE,
        GENERIC_COUCH,
        GENERIC_DEER_HEAD,
        GENERIC_DRAWERS_LARGE,
        GENERIC_DRAWERS_SMALL,
        GENERIC_MERMAID_ROCK,
        GENERIC_PLANT_BLACKBERRIES,
        GENERIC_PLANT_TREE,
        GENERIC_TEST_MODEL,
        GENERIC_TEST_MODEL2,
        GENERIC_TEST_MODEL3,
        GENERIC_TEST_MODEL4,
        GENERIC_TOILET,
        HOUSE_LOCATION,
        JETTY,
        KANGAROO,
        LADDER,
        LIGHT_HANGING,
        LIGHT_LAMP_POST,
        LIGHT_WALL_LAMP,
        MERMAID,
        PIANO,
        PICKUP_12_GAUGE_BUCKSHOT,
        PICKUP_AKS74U,
        PICKUP_BLACK_SKULL,
        PICKUP_GLOCK,
        PICKUP_GOLDEN_GLOCK,
        PICKUP_KNIFE,
        PICKUP_P90,
        PICKUP_PILLS,
        PICKUP_REMINGTON_870,
        PICKUP_SMALL_KEY,
        PICKUP_SMALL_KEY_SILVER,
        PICKUP_SPAS,
        PICKUP_TOKAREV,
        PICTURE_FRAME_BIG_LANDSCAPE,
        PICTURE_FRAME_REGULAR_LANDSCAPE,
        PICTURE_FRAME_REGULAR_PORTRAIT,
        PICTURE_FRAME_TALL_THIN,
        PLAYER_CAMPAIGN_SPAWN,
        PLAYER_DEATHMATCH_SPAWN,
        POWER_POLES,
        DECKING_BEARER,
        DECKING_BOARDS,
        DECKING_POST,
        DOWN_PIPE,
        GUTTER,
        RIDGE_CAPPING,
        ROOFING_IRON,
        SHARK,
        STAIRCASE,
        WALL_INTERIOR,
        WALL_WEATHER_BOARDS,
        WINDOW,
        WORLD_PLANE_CEILING,
        WORLD_PLANE_FLOOR,
        LADDER_DISMOUNT,
        UNDEFINED
    };

    enum class PlacementInsertMode {
        DIRECT,
        POINT_SEQUENCE,
        UNDEFINED
    };

    enum class PlacementRayMode {
        WORLD,
        PHYSICS,
        HEIGHT_MAP,
        GROUND_PLANE,
        WALL,
        UNDEFINED
    };

    struct PlacementToolInfo {
        PlacementInsertMode insertMode = PlacementInsertMode::UNDEFINED;
        PlacementRayMode rayMode = PlacementRayMode::UNDEFINED;
        float sequencePointDefaultValue = 0.0f;
        float sequencePointValueStep = 0.02f;
        std::string defaultEditorName;
    };

    void InitPlacementTools();
    const std::map<PlacementTool, PlacementToolInfo>& GetPlacementTools();
    const PlacementToolInfo* GetPlacementToolInfo(PlacementTool tool);
}
