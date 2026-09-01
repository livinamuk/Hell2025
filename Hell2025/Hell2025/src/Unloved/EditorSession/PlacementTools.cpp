#include "PlacementTools.h"

#include "Hell/Common/Enum.h"
#include "Hell/Logging.h"

namespace Unloved::EditorSession {

    namespace {
        std::map<PlacementTool, PlacementToolInfo> g_placementTools;
    }

    void InitPlacementTools() {
        g_placementTools.clear();

        PlacementToolInfo& christmasLights = g_placementTools[PlacementTool::CHRISTMAS_LIGHTS];
        christmasLights.rayMode = PlacementRayMode::WORLD;
        christmasLights.insertMode = PlacementInsertMode::POINT_SEQUENCE;
        christmasLights.sequencePointDefaultValue = 0.3f;
        christmasLights.sequencePointValueStep = 0.02f;
        christmasLights.defaultEditorName = "Christmas Lights";

        PlacementToolInfo& ddgiVolume = g_placementTools[PlacementTool::DDGI_VOLUME];
        ddgiVolume.rayMode = PlacementRayMode::WORLD;
        ddgiVolume.insertMode = PlacementInsertMode::DIRECT;
        ddgiVolume.defaultEditorName = "DDGI Volume";

        PlacementToolInfo& dobermann = g_placementTools[PlacementTool::DOBERMANN];
        dobermann.rayMode = PlacementRayMode::WORLD;
        dobermann.insertMode = PlacementInsertMode::DIRECT;
        dobermann.defaultEditorName = "Dobermann";

        PlacementToolInfo& doorStandardA = g_placementTools[PlacementTool::DOOR_STANDARD_A];
        doorStandardA.rayMode = PlacementRayMode::WORLD;
        doorStandardA.insertMode = PlacementInsertMode::DIRECT;
        doorStandardA.defaultEditorName = "Door";

        PlacementToolInfo& doorStandardB = g_placementTools[PlacementTool::DOOR_STANDARD_B];
        doorStandardB.rayMode = PlacementRayMode::WORLD;
        doorStandardB.insertMode = PlacementInsertMode::DIRECT;
        doorStandardB.defaultEditorName = "Door";

        PlacementToolInfo& doorStainedGlass = g_placementTools[PlacementTool::DOOR_GLASS];
        doorStainedGlass.rayMode = PlacementRayMode::WORLD;
        doorStainedGlass.insertMode = PlacementInsertMode::DIRECT;
        doorStainedGlass.defaultEditorName = "Door";

        PlacementToolInfo& doorStainedGlass2 = g_placementTools[PlacementTool::DOOR_GLASS2];
        doorStainedGlass2.rayMode = PlacementRayMode::WORLD;
        doorStainedGlass2.insertMode = PlacementInsertMode::DIRECT;
        doorStainedGlass2.defaultEditorName = "Door";

        PlacementToolInfo& fenceFarm = g_placementTools[PlacementTool::FENCE_FARM];
        fenceFarm.rayMode = PlacementRayMode::HEIGHT_MAP;
        fenceFarm.insertMode = PlacementInsertMode::POINT_SEQUENCE;
        fenceFarm.defaultEditorName = "Fence";

        PlacementToolInfo& fireplaceOpen = g_placementTools[PlacementTool::FIREPLACE_OPEN];
        fireplaceOpen.rayMode = PlacementRayMode::WORLD;
        fireplaceOpen.insertMode = PlacementInsertMode::DIRECT;
        fireplaceOpen.defaultEditorName = "Fireplace";

        PlacementToolInfo& fireplaceWoodStove = g_placementTools[PlacementTool::FIREPLACE_WOOD_STOVE];
        fireplaceWoodStove.rayMode = PlacementRayMode::WORLD;
        fireplaceWoodStove.insertMode = PlacementInsertMode::DIRECT;
        fireplaceWoodStove.defaultEditorName = "Wood Stove";

        PlacementToolInfo& genericAnimatedRatKing = g_placementTools[PlacementTool::GENERIC_ANIMATED_RAT_KING];
        genericAnimatedRatKing.rayMode = PlacementRayMode::WORLD;
        genericAnimatedRatKing.insertMode = PlacementInsertMode::DIRECT;
        genericAnimatedRatKing.defaultEditorName = "Animated Rat King";

        PlacementToolInfo& genericBathroomBasin = g_placementTools[PlacementTool::GENERIC_BATHROOM_BASIN];
        genericBathroomBasin.rayMode = PlacementRayMode::WORLD;
        genericBathroomBasin.insertMode = PlacementInsertMode::DIRECT;
        genericBathroomBasin.defaultEditorName = "Basin";

        PlacementToolInfo& genericBathroomCabinet = g_placementTools[PlacementTool::GENERIC_BATHROOM_CABINET];
        genericBathroomCabinet.rayMode = PlacementRayMode::WORLD;
        genericBathroomCabinet.insertMode = PlacementInsertMode::DIRECT;
        genericBathroomCabinet.defaultEditorName = "Cabinet";

        PlacementToolInfo& genericBathroomTowelRack = g_placementTools[PlacementTool::GENERIC_BATHROOM_TOWEL_RACK];
        genericBathroomTowelRack.rayMode = PlacementRayMode::WORLD;
        genericBathroomTowelRack.insertMode = PlacementInsertMode::DIRECT;
        genericBathroomTowelRack.defaultEditorName = "Towel Rack";

        PlacementToolInfo& genericChairRE = g_placementTools[PlacementTool::GENERIC_CHAIR_RE];
        genericChairRE.rayMode = PlacementRayMode::WORLD;
        genericChairRE.insertMode = PlacementInsertMode::DIRECT;
        genericChairRE.defaultEditorName = "Chair RE";

        PlacementToolInfo& genericChairSpindleBack = g_placementTools[PlacementTool::GENERIC_CHAIR_SPINDLE_BACK];
        genericChairSpindleBack.rayMode = PlacementRayMode::WORLD;
        genericChairSpindleBack.insertMode = PlacementInsertMode::DIRECT;
        genericChairSpindleBack.defaultEditorName = "Chair Spindle Back";

        PlacementToolInfo& genericChristmasPresentLarge = g_placementTools[PlacementTool::GENERIC_CHRISTMAS_PRESENT_LARGE];
        genericChristmasPresentLarge.rayMode = PlacementRayMode::WORLD;
        genericChristmasPresentLarge.insertMode = PlacementInsertMode::DIRECT;
        genericChristmasPresentLarge.defaultEditorName = "Christmas Present Large";

        PlacementToolInfo& genericChristmasPresentSmall = g_placementTools[PlacementTool::GENERIC_CHRISTMAS_PRESENT_SMALL];
        genericChristmasPresentSmall.rayMode = PlacementRayMode::WORLD;
        genericChristmasPresentSmall.insertMode = PlacementInsertMode::DIRECT;
        genericChristmasPresentSmall.defaultEditorName = "Christmas Present Small";

        PlacementToolInfo& genericChristmasTree = g_placementTools[PlacementTool::GENERIC_CHRISTMAS_TREE];
        genericChristmasTree.rayMode = PlacementRayMode::WORLD;
        genericChristmasTree.insertMode = PlacementInsertMode::DIRECT;
        genericChristmasTree.defaultEditorName = "Christmas Tree";

        PlacementToolInfo& genericCouch = g_placementTools[PlacementTool::GENERIC_COUCH];
        genericCouch.rayMode = PlacementRayMode::WORLD;
        genericCouch.insertMode = PlacementInsertMode::DIRECT;
        genericCouch.defaultEditorName = "Couch";

        PlacementToolInfo& genericDeerHead = g_placementTools[PlacementTool::GENERIC_DEER_HEAD];
        genericDeerHead.rayMode = PlacementRayMode::WALL;
        genericDeerHead.insertMode = PlacementInsertMode::DIRECT;
        genericDeerHead.defaultEditorName = "Deer Head";

        PlacementToolInfo& genericDrawersLarge = g_placementTools[PlacementTool::GENERIC_DRAWERS_LARGE];
        genericDrawersLarge.rayMode = PlacementRayMode::WORLD;
        genericDrawersLarge.insertMode = PlacementInsertMode::DIRECT;
        genericDrawersLarge.defaultEditorName = "Drawers Large";

        PlacementToolInfo& genericDrawersSmall = g_placementTools[PlacementTool::GENERIC_DRAWERS_SMALL];
        genericDrawersSmall.rayMode = PlacementRayMode::WORLD;
        genericDrawersSmall.insertMode = PlacementInsertMode::DIRECT;
        genericDrawersSmall.defaultEditorName = "Drawers Small";

        PlacementToolInfo& genericMermaidRock = g_placementTools[PlacementTool::GENERIC_MERMAID_ROCK];
        genericMermaidRock.rayMode = PlacementRayMode::WORLD;
        genericMermaidRock.insertMode = PlacementInsertMode::DIRECT;
        genericMermaidRock.defaultEditorName = "Mermaid Visitor Rock";

        PlacementToolInfo& genericPlantBlackberries = g_placementTools[PlacementTool::GENERIC_PLANT_BLACKBERRIES];
        genericPlantBlackberries.rayMode = PlacementRayMode::WORLD;
        genericPlantBlackberries.insertMode = PlacementInsertMode::DIRECT;
        genericPlantBlackberries.defaultEditorName = "Blackberries";

        PlacementToolInfo& genericPlantTree = g_placementTools[PlacementTool::GENERIC_PLANT_TREE];
        genericPlantTree.rayMode = PlacementRayMode::WORLD;
        genericPlantTree.insertMode = PlacementInsertMode::DIRECT;
        genericPlantTree.defaultEditorName = "Tree";

        PlacementToolInfo& genericTestModel = g_placementTools[PlacementTool::GENERIC_TEST_MODEL];
        genericTestModel.rayMode = PlacementRayMode::WORLD;
        genericTestModel.insertMode = PlacementInsertMode::DIRECT;
        genericTestModel.defaultEditorName = "Test Model 1";

        PlacementToolInfo& genericTestModel2 = g_placementTools[PlacementTool::GENERIC_TEST_MODEL2];
        genericTestModel2.rayMode = PlacementRayMode::WORLD;
        genericTestModel2.insertMode = PlacementInsertMode::DIRECT;
        genericTestModel2.defaultEditorName = "Test Model 2";

        PlacementToolInfo& genericTestModel3 = g_placementTools[PlacementTool::GENERIC_TEST_MODEL3];
        genericTestModel3.rayMode = PlacementRayMode::WORLD;
        genericTestModel3.insertMode = PlacementInsertMode::DIRECT;
        genericTestModel3.defaultEditorName = "Test Model 3";

        PlacementToolInfo& genericTestModel4 = g_placementTools[PlacementTool::GENERIC_TEST_MODEL4];
        genericTestModel4.rayMode = PlacementRayMode::WORLD;
        genericTestModel4.insertMode = PlacementInsertMode::DIRECT;
        genericTestModel4.defaultEditorName = "Test Model 4";

        PlacementToolInfo& genericToilet = g_placementTools[PlacementTool::GENERIC_TOILET];
        genericToilet.rayMode = PlacementRayMode::WORLD;
        genericToilet.insertMode = PlacementInsertMode::DIRECT;
        genericToilet.defaultEditorName = "Toilet";

        PlacementToolInfo& houseLocation = g_placementTools[PlacementTool::HOUSE_LOCATION];
        houseLocation.rayMode = PlacementRayMode::PHYSICS;
        houseLocation.insertMode = PlacementInsertMode::DIRECT;
        houseLocation.defaultEditorName = "House Location";

        PlacementToolInfo& kangaroo = g_placementTools[PlacementTool::KANGAROO];
        kangaroo.rayMode = PlacementRayMode::WORLD;
        kangaroo.insertMode = PlacementInsertMode::DIRECT;
        kangaroo.defaultEditorName = "Kangaroo";

        PlacementToolInfo& jetty = g_placementTools[PlacementTool::JETTY];
        jetty.rayMode = PlacementRayMode::WORLD;
        jetty.insertMode = PlacementInsertMode::DIRECT;
        jetty.defaultEditorName = "Tasmanian Fisherman's Jetty";

        PlacementToolInfo& gutter = g_placementTools[PlacementTool::GUTTER];
        gutter.rayMode = PlacementRayMode::WORLD;
        gutter.insertMode = PlacementInsertMode::DIRECT;
        gutter.defaultEditorName = "Gutter";

        PlacementToolInfo& ridgeCapping = g_placementTools[PlacementTool::RIDGE_CAPPING];
        ridgeCapping.rayMode = PlacementRayMode::WORLD;
        ridgeCapping.insertMode = PlacementInsertMode::DIRECT;
        ridgeCapping.defaultEditorName = "Ridge Capping";

        PlacementToolInfo& downPipe = g_placementTools[PlacementTool::DOWN_PIPE];
        downPipe.rayMode = PlacementRayMode::WORLD;
        downPipe.insertMode = PlacementInsertMode::DIRECT;
        downPipe.defaultEditorName = "Down Pipe";

        PlacementToolInfo& roofingIron = g_placementTools[PlacementTool::ROOFING_IRON];
        roofingIron.rayMode = PlacementRayMode::WORLD;
        roofingIron.insertMode = PlacementInsertMode::DIRECT;
        roofingIron.defaultEditorName = "Roofing Iron";

        PlacementToolInfo& deckingBoards = g_placementTools[PlacementTool::DECKING_BOARDS];
        deckingBoards.rayMode = PlacementRayMode::WORLD;
        deckingBoards.insertMode = PlacementInsertMode::DIRECT;
        deckingBoards.defaultEditorName = "Decking Boards";

        PlacementToolInfo& deckingBearer = g_placementTools[PlacementTool::DECKING_BEARER];
        deckingBearer.rayMode = PlacementRayMode::WORLD;
        deckingBearer.insertMode = PlacementInsertMode::DIRECT;
        deckingBearer.defaultEditorName = "Decking Bearer";

        PlacementToolInfo& deckingPost = g_placementTools[PlacementTool::DECKING_POST];
        deckingPost.rayMode = PlacementRayMode::WORLD;
        deckingPost.insertMode = PlacementInsertMode::DIRECT;
        deckingPost.defaultEditorName = "Decking Post";

        PlacementToolInfo& ladder = g_placementTools[PlacementTool::LADDER];
        ladder.rayMode = PlacementRayMode::WORLD;
        ladder.insertMode = PlacementInsertMode::DIRECT;
        ladder.defaultEditorName = "Ladder";

        PlacementToolInfo& ladderDismount = g_placementTools[PlacementTool::LADDER_DISMOUNT];
        ladderDismount.rayMode = PlacementRayMode::WORLD;
        ladderDismount.insertMode = PlacementInsertMode::DIRECT;
        ladderDismount.defaultEditorName = "Ladder Dismount";

        PlacementToolInfo& lightHanging = g_placementTools[PlacementTool::LIGHT_HANGING];
        lightHanging.rayMode = PlacementRayMode::WORLD;
        lightHanging.insertMode = PlacementInsertMode::DIRECT;
        lightHanging.defaultEditorName = "Light";

        PlacementToolInfo& lightLampPost = g_placementTools[PlacementTool::LIGHT_LAMP_POST];
        lightLampPost.rayMode = PlacementRayMode::WORLD;
        lightLampPost.insertMode = PlacementInsertMode::DIRECT;
        lightLampPost.defaultEditorName = "Lamp Post";

        PlacementToolInfo& lightWallLamp = g_placementTools[PlacementTool::LIGHT_WALL_LAMP];
        lightWallLamp.rayMode = PlacementRayMode::WORLD;
        lightWallLamp.insertMode = PlacementInsertMode::DIRECT;
        lightWallLamp.defaultEditorName = "Wall Lamp";

        PlacementToolInfo& mermaid = g_placementTools[PlacementTool::MERMAID];
        mermaid.rayMode = PlacementRayMode::WORLD;
        mermaid.insertMode = PlacementInsertMode::DIRECT;
        mermaid.defaultEditorName = "Mermaid Shop Owner";

        PlacementToolInfo& piano = g_placementTools[PlacementTool::PIANO];
        piano.rayMode = PlacementRayMode::WORLD;
        piano.insertMode = PlacementInsertMode::DIRECT;
        piano.defaultEditorName = "Piano";

        PlacementToolInfo& pickup12GaugeBuckshot = g_placementTools[PlacementTool::PICKUP_12_GAUGE_BUCKSHOT];
        pickup12GaugeBuckshot.rayMode = PlacementRayMode::WORLD;
        pickup12GaugeBuckshot.insertMode = PlacementInsertMode::DIRECT;
        pickup12GaugeBuckshot.defaultEditorName = "Shotgun Shells Buckshot";

        PlacementToolInfo& pickupAKS74U = g_placementTools[PlacementTool::PICKUP_AKS74U];
        pickupAKS74U.rayMode = PlacementRayMode::WORLD;
        pickupAKS74U.insertMode = PlacementInsertMode::DIRECT;
        pickupAKS74U.defaultEditorName = "AKS74U";

        PlacementToolInfo& pickupBlackSkull = g_placementTools[PlacementTool::PICKUP_BLACK_SKULL];
        pickupBlackSkull.rayMode = PlacementRayMode::WORLD;
        pickupBlackSkull.insertMode = PlacementInsertMode::DIRECT;
        pickupBlackSkull.defaultEditorName = "Black Skull";

        PlacementToolInfo& pickupGlock = g_placementTools[PlacementTool::PICKUP_GLOCK];
        pickupGlock.rayMode = PlacementRayMode::WORLD;
        pickupGlock.insertMode = PlacementInsertMode::DIRECT;
        pickupGlock.defaultEditorName = "Glock";

        PlacementToolInfo& pickupGoldenGlock = g_placementTools[PlacementTool::PICKUP_GOLDEN_GLOCK];
        pickupGoldenGlock.rayMode = PlacementRayMode::WORLD;
        pickupGoldenGlock.insertMode = PlacementInsertMode::DIRECT;
        pickupGoldenGlock.defaultEditorName = "Golden Glock";

        PlacementToolInfo& pickupKnife = g_placementTools[PlacementTool::PICKUP_KNIFE];
        pickupKnife.rayMode = PlacementRayMode::WORLD;
        pickupKnife.insertMode = PlacementInsertMode::DIRECT;
        pickupKnife.defaultEditorName = "Knife";

        PlacementToolInfo& pickupP90 = g_placementTools[PlacementTool::PICKUP_P90];
        pickupP90.rayMode = PlacementRayMode::WORLD;
        pickupP90.insertMode = PlacementInsertMode::DIRECT;
        pickupP90.defaultEditorName = "P90";

        PlacementToolInfo& pickupPills = g_placementTools[PlacementTool::PICKUP_PILLS];
        pickupPills.rayMode = PlacementRayMode::WORLD;
        pickupPills.insertMode = PlacementInsertMode::DIRECT;
        pickupPills.defaultEditorName = "Relief Pills";

        PlacementToolInfo& pickupRemington870 = g_placementTools[PlacementTool::PICKUP_REMINGTON_870];
        pickupRemington870.rayMode = PlacementRayMode::WORLD;
        pickupRemington870.insertMode = PlacementInsertMode::DIRECT;
        pickupRemington870.defaultEditorName = "Remington 870";

        PlacementToolInfo& pickupSmallKey = g_placementTools[PlacementTool::PICKUP_SMALL_KEY];
        pickupSmallKey.rayMode = PlacementRayMode::WORLD;
        pickupSmallKey.insertMode = PlacementInsertMode::DIRECT;
        pickupSmallKey.defaultEditorName = "Small Key";

        PlacementToolInfo& pickupSmallKeySilver = g_placementTools[PlacementTool::PICKUP_SMALL_KEY_SILVER];
        pickupSmallKeySilver.rayMode = PlacementRayMode::WORLD;
        pickupSmallKeySilver.insertMode = PlacementInsertMode::DIRECT;
        pickupSmallKeySilver.defaultEditorName = "Small Key Silver";

        PlacementToolInfo& pickupSPAS = g_placementTools[PlacementTool::PICKUP_SPAS];
        pickupSPAS.rayMode = PlacementRayMode::WORLD;
        pickupSPAS.insertMode = PlacementInsertMode::DIRECT;
        pickupSPAS.defaultEditorName = "SPAS";

        PlacementToolInfo& pickupTokarev = g_placementTools[PlacementTool::PICKUP_TOKAREV];
        pickupTokarev.rayMode = PlacementRayMode::WORLD;
        pickupTokarev.insertMode = PlacementInsertMode::DIRECT;
        pickupTokarev.defaultEditorName = "Tokarev";

        PlacementToolInfo& pictureFrameBigLandscape = g_placementTools[PlacementTool::PICTURE_FRAME_BIG_LANDSCAPE];
        pictureFrameBigLandscape.rayMode = PlacementRayMode::WALL;
        pictureFrameBigLandscape.insertMode = PlacementInsertMode::DIRECT;
        pictureFrameBigLandscape.defaultEditorName = "Picture Frame";

        PlacementToolInfo& pictureFrameRegularLandscape = g_placementTools[PlacementTool::PICTURE_FRAME_REGULAR_LANDSCAPE];
        pictureFrameRegularLandscape.rayMode = PlacementRayMode::WALL;
        pictureFrameRegularLandscape.insertMode = PlacementInsertMode::DIRECT;
        pictureFrameRegularLandscape.defaultEditorName = "Picture Frame";

        PlacementToolInfo& pictureFrameRegularPortrait = g_placementTools[PlacementTool::PICTURE_FRAME_REGULAR_PORTRAIT];
        pictureFrameRegularPortrait.rayMode = PlacementRayMode::WALL;
        pictureFrameRegularPortrait.insertMode = PlacementInsertMode::DIRECT;
        pictureFrameRegularPortrait.defaultEditorName = "Picture Frame";

        PlacementToolInfo& pictureFrameTallThin = g_placementTools[PlacementTool::PICTURE_FRAME_TALL_THIN];
        pictureFrameTallThin.rayMode = PlacementRayMode::WALL;
        pictureFrameTallThin.insertMode = PlacementInsertMode::DIRECT;
        pictureFrameTallThin.defaultEditorName = "Picture Frame";

        PlacementToolInfo& playerCampaignSpawn = g_placementTools[PlacementTool::PLAYER_CAMPAIGN_SPAWN];
        playerCampaignSpawn.rayMode = PlacementRayMode::PHYSICS;
        playerCampaignSpawn.insertMode = PlacementInsertMode::DIRECT;
        playerCampaignSpawn.defaultEditorName = "Campaign Spawn";

        PlacementToolInfo& playerDeathmatchSpawn = g_placementTools[PlacementTool::PLAYER_DEATHMATCH_SPAWN];
        playerDeathmatchSpawn.rayMode = PlacementRayMode::PHYSICS;
        playerDeathmatchSpawn.insertMode = PlacementInsertMode::DIRECT;
        playerDeathmatchSpawn.defaultEditorName = "Deathmatch Spawn";

        PlacementToolInfo& powerPoles = g_placementTools[PlacementTool::POWER_POLES];
        powerPoles.rayMode = PlacementRayMode::HEIGHT_MAP;
        powerPoles.insertMode = PlacementInsertMode::POINT_SEQUENCE;
        powerPoles.defaultEditorName = "Power Pole Set";

        PlacementToolInfo& shark = g_placementTools[PlacementTool::SHARK];
        shark.rayMode = PlacementRayMode::WORLD;
        shark.insertMode = PlacementInsertMode::DIRECT;
        shark.defaultEditorName = "Shark";

        PlacementToolInfo& snake = g_placementTools[PlacementTool::SNAKE];
        snake.rayMode = PlacementRayMode::WORLD;
        snake.insertMode = PlacementInsertMode::DIRECT;
        snake.defaultEditorName = "Snake";

        PlacementToolInfo& staircase = g_placementTools[PlacementTool::STAIRCASE];
        staircase.rayMode = PlacementRayMode::WORLD;
        staircase.insertMode = PlacementInsertMode::DIRECT;
        staircase.defaultEditorName = "Staircase";

        PlacementToolInfo& wallInterior = g_placementTools[PlacementTool::WALL_INTERIOR];
        wallInterior.rayMode = PlacementRayMode::GROUND_PLANE;
        wallInterior.insertMode = PlacementInsertMode::POINT_SEQUENCE;
        wallInterior.sequencePointDefaultValue = 2.4f;
        wallInterior.sequencePointValueStep = -0.1f;
        wallInterior.defaultEditorName = "Wall";

        PlacementToolInfo& wallWeatherBoards = g_placementTools[PlacementTool::WALL_WEATHER_BOARDS];
        wallWeatherBoards.rayMode = PlacementRayMode::GROUND_PLANE;
        wallWeatherBoards.insertMode = PlacementInsertMode::POINT_SEQUENCE;
        wallWeatherBoards.sequencePointDefaultValue = 2.4f;
        wallWeatherBoards.sequencePointValueStep = -0.1f;
        wallWeatherBoards.defaultEditorName = "Weather Boards";

        PlacementToolInfo& window = g_placementTools[PlacementTool::WINDOW];
        window.rayMode = PlacementRayMode::WORLD;
        window.insertMode = PlacementInsertMode::DIRECT;
        window.defaultEditorName = "Window";

        PlacementToolInfo& worldPlaneCeiling = g_placementTools[PlacementTool::WORLD_PLANE_CEILING];
        worldPlaneCeiling.rayMode = PlacementRayMode::WORLD;
        worldPlaneCeiling.insertMode = PlacementInsertMode::DIRECT;
        worldPlaneCeiling.defaultEditorName = "Ceiling";

        PlacementToolInfo& worldPlaneFloor = g_placementTools[PlacementTool::WORLD_PLANE_FLOOR];
        worldPlaneFloor.rayMode = PlacementRayMode::WORLD;
        worldPlaneFloor.insertMode = PlacementInsertMode::DIRECT;
        worldPlaneFloor.defaultEditorName = "Floor";
    }

    const std::map<PlacementTool, PlacementToolInfo>& GetPlacementTools() {
        return g_placementTools;
    }

    const PlacementToolInfo* GetPlacementToolInfo(PlacementTool tool) {
        auto it = g_placementTools.find(tool);
        if (it == g_placementTools.end()) {
            Logging::Error() << "EditorSession::GetPlacementToolInfo() failed '" << Hell::Enum::ToString(tool) << "' not implemented\n";
            return nullptr;
        }
        return &it->second;
    }
}
