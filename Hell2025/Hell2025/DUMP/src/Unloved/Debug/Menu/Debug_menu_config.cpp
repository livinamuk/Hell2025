#include "Debug_menu.h"

#include "Unloved/Config/Config.h"
#include "Unloved/Config/FlashlightConfig.h"
#include "Unloved/Debug/Debug.h"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <string>
#include <vector>

namespace Debug::Menu::OceanMenu {
    PageId RegisterMenu(PageId parentPage);
}

namespace Debug::Menu::PhysicsMenu {
    PageId RegisterMenu(PageId parentPage);
}

namespace Debug::Menu::FogMenu {
    PageId RegisterMenu(PageId parentPage);
}

namespace Debug::Menu::ConfigMenu {

    enum struct RootSetting : uint32_t {
        CHRISTMAS,
        FLASHLIGHT,
        FOG,
        GRASS,
        MOONLIGHT,
        OCEAN,
        PHYSICS,
    };

    enum struct ChristmasSetting : uint32_t {
        LIGHT_RADIUS,
        LIGHT_STRENGTH,
        SAVE_TO_DISK,
        LOAD_FROM_DISK,
    };

    enum struct MoonlightSetting : uint32_t {
        COLOR_R,
        COLOR_G,
        COLOR_B,
        STRENGTH,
        SAVE_TO_DISK,
        LOAD_FROM_DISK,
    };

    enum struct GrassSetting : uint32_t {
        SEGMENT_COUNT,
        CURVE_AMOUNT,
        SPACING,
        MIN_CULL_DISTANCE,
        MAX_CULL_DISTANCE,
        CULL_EXPONENT,
        BLADE_HEIGHT,
        BLADE_WIDTH,
        COLOR_1_R,
        COLOR_1_G,
        COLOR_1_B,
        COLOR_1_DARKNESS_FACTOR,
        COLOR_2_R,
        COLOR_2_G,
        COLOR_2_B,
        COLOR_2_DARKNESS_FACTOR,
        NOISE_SQUARE_MULTIPLIER,
        NOISE_MIX_MULTIPLIER,
        ROUGHNESS,
        SUB_SURFACE_FACTOR,
        NORMAL_UP_BLEND,
        NORMAL_BLEND_START_DISTANCE,
        NORMAL_BLEND_END_DISTANCE,
        DIFFUSE_WRAP,
        TRANSMISSION_POWER,
        SPECULAR_STRENGTH,
        SAVE_TO_DISK,
        LOAD_FROM_DISK,
    };

    enum struct FlashlightSetting : uint32_t {
        IES_PROFILE,
        IES_ENABLED,
        RANGE,
        FALLOFF_EXPONENT,
        BRIGHTNESS,
        COLOR_R,
        COLOR_G,
        COLOR_B,
        IES_CONE_SCALE,
        IES_INNER_ANGLE,
        IES_OUTER_ANGLE,
        IES_CONTRAST,
        CENTER_SPOT_ENABLED,
        CENTER_SPOT_RANGE,
        CENTER_SPOT_FALLOFF_EXPONENT,
        CENTER_SPOT_BRIGHTNESS,
        CENTER_SPOT_INNER_ANGLE,
        CENTER_SPOT_OUTER_ANGLE,
        SAVE_TO_DISK,
        LOAD_FROM_DISK,
    };

    PageId g_homePage = ROOT_PAGE_ID;
    PageId g_christmasPage = ROOT_PAGE_ID;
    PageId g_flashlightPage = ROOT_PAGE_ID;
    PageId g_fogPage = ROOT_PAGE_ID;
    PageId g_grassPage = ROOT_PAGE_ID;
    PageId g_moonlightPage = ROOT_PAGE_ID;
    PageId g_oceanPage = ROOT_PAGE_ID;
    PageId g_physicsPage = ROOT_PAGE_ID;

    void BuildMainMenu();
    void BuildChristmasMenu();
    void BuildFlashlightMenu();
    void BuildGrassMenu();
    void BuildMoonlightMenu();
    void ApplyChristmasEdit(uint32_t id, const Value& value);
    void ApplyFlashlightEdit(uint32_t id, const Value& value);
    void ApplyGrassEdit(uint32_t id, const Value& value);
    void ApplyMoonlightEdit(uint32_t id, const Value& value);

    std::vector<std::string> GetIESProfileNames() {
        std::vector<std::string> names = { "ThreeJS_0", "ThreeJS_1", "ThreeJS_2", "ThreeJS_3" };
        const std::string& currentName = Config::Flashlight::GetSettings().iesProfile;
        if (std::find(names.begin(), names.end(), currentName) == names.end()) names.push_back(currentName);
        return names;
    }

    int32_t GetIESProfileIndex(const std::vector<std::string>& names, const std::string& profileName) {
        const auto it = std::find(names.begin(), names.end(), profileName);
        return it == names.end() ? 0 : static_cast<int32_t>(std::distance(names.begin(), it));
    }

    void RegisterMenu() {
        g_homePage = RegisterRootPage("Config", "CONFIG", BuildMainMenu, nullptr);
        g_christmasPage = RegisterPage("CHRISTMAS", g_homePage, BuildChristmasMenu, ApplyChristmasEdit);
        g_flashlightPage = RegisterPage("FLASHLIGHT", g_homePage, BuildFlashlightMenu, ApplyFlashlightEdit);
        g_fogPage = FogMenu::RegisterMenu(g_homePage);
        g_grassPage = RegisterPage("GRASS", g_homePage, BuildGrassMenu, ApplyGrassEdit);
        g_moonlightPage = RegisterPage("MOONLIGHT", g_homePage, BuildMoonlightMenu, ApplyMoonlightEdit);
        g_oceanPage = OceanMenu::RegisterMenu(g_homePage);
        g_physicsPage = PhysicsMenu::RegisterMenu(g_homePage);
    }

    Registrar g_registrar(RegisterMenu);

    void BuildMainMenu() {
        AddSubMenu(static_cast<uint32_t>(RootSetting::CHRISTMAS), "Christmas", g_christmasPage);
        AddSubMenu(static_cast<uint32_t>(RootSetting::FLASHLIGHT), "Flashlight", g_flashlightPage);
        AddSubMenu(static_cast<uint32_t>(RootSetting::FOG), "Fog", g_fogPage);
        AddSubMenu(static_cast<uint32_t>(RootSetting::GRASS), "Grass", g_grassPage);
        AddSubMenu(static_cast<uint32_t>(RootSetting::MOONLIGHT), "Moonlight", g_moonlightPage);
        AddSubMenu(static_cast<uint32_t>(RootSetting::OCEAN), "Ocean", g_oceanPage);
        AddSubMenu(static_cast<uint32_t>(RootSetting::PHYSICS), "Physics", g_physicsPage);
    }

    void BuildChristmasMenu() {
        const Config::Christmas::Settings& settings = Config::Christmas::GetSettings();

        AddFloat(static_cast<uint32_t>(ChristmasSetting::LIGHT_RADIUS), "Christmas Light Radius", settings.lightRadius, 0.0f, 5.0f, 0.05f, 2);
        AddFloat(static_cast<uint32_t>(ChristmasSetting::LIGHT_STRENGTH), "Christmas Light Strength", settings.lightStrength, 0.0f, 5.0f, 0.01f, 2);
        AddLineBreak();
        AddAction(static_cast<uint32_t>(ChristmasSetting::SAVE_TO_DISK), "Save to disk");
        AddAction(static_cast<uint32_t>(ChristmasSetting::LOAD_FROM_DISK), "Load from disk");
    }

    void ApplyChristmasEdit(uint32_t id, const Value& value) {
        const ChristmasSetting setting = static_cast<ChristmasSetting>(id);
        if (setting == ChristmasSetting::SAVE_TO_DISK) {
            const bool saved = Config::Christmas::SaveToDisk();
            Debug::BlitQuickDebugMessage(saved ? "Saved " + Config::Christmas::GetFilePath() : "Failed to save christmas config");
            return;
        }
        if (setting == ChristmasSetting::LOAD_FROM_DISK) {
            const bool loaded = Config::Christmas::LoadFromDisk();
            Debug::BlitQuickDebugMessage(loaded ? "Loaded " + Config::Christmas::GetFilePath() : "Failed to load christmas config");
            return;
        }

        Config::Christmas::Settings settings = Config::Christmas::GetSettings();
        switch (setting) {
            case ChristmasSetting::LIGHT_RADIUS:   settings.lightRadius = value.floatValue; break;
            case ChristmasSetting::LIGHT_STRENGTH: settings.lightStrength = value.floatValue; break;
            default: return;
        }
        Config::Christmas::SetSettings(settings);
    }

    void BuildMoonlightMenu() {
        const Config::Moonlight::Settings& settings = Config::Moonlight::GetSettings();

        AddFloat(static_cast<uint32_t>(MoonlightSetting::COLOR_R), "Color R", settings.color.r, 0.0f, 1.0f, 0.01f, 3);
        AddFloat(static_cast<uint32_t>(MoonlightSetting::COLOR_G), "Color G", settings.color.g, 0.0f, 1.0f, 0.01f, 3);
        AddFloat(static_cast<uint32_t>(MoonlightSetting::COLOR_B), "Color B", settings.color.b, 0.0f, 1.0f, 0.01f, 3);
        AddFloat(static_cast<uint32_t>(MoonlightSetting::STRENGTH), "Strength", settings.strength, 0.0f, 1.0f, 0.005f, 3);
        AddLineBreak();
        AddAction(static_cast<uint32_t>(MoonlightSetting::SAVE_TO_DISK), "Save to disk");
        AddAction(static_cast<uint32_t>(MoonlightSetting::LOAD_FROM_DISK), "Load from disk");
    }

    void ApplyMoonlightEdit(uint32_t id, const Value& value) {
        const MoonlightSetting setting = static_cast<MoonlightSetting>(id);
        if (setting == MoonlightSetting::SAVE_TO_DISK) {
            const bool saved = Config::Moonlight::SaveToDisk();
            Debug::BlitQuickDebugMessage(saved ? "Saved " + Config::Moonlight::GetFilePath() : "Failed to save moonlight config");
            return;
        }
        if (setting == MoonlightSetting::LOAD_FROM_DISK) {
            const bool loaded = Config::Moonlight::LoadFromDisk();
            Debug::BlitQuickDebugMessage(loaded ? "Loaded " + Config::Moonlight::GetFilePath() : "Failed to load moonlight config");
            return;
        }

        Config::Moonlight::Settings settings = Config::Moonlight::GetSettings();

        switch (setting) {
            case MoonlightSetting::COLOR_R:  settings.color.r = value.floatValue; break;
            case MoonlightSetting::COLOR_G:  settings.color.g = value.floatValue; break;
            case MoonlightSetting::COLOR_B:  settings.color.b = value.floatValue; break;
            case MoonlightSetting::STRENGTH: settings.strength = value.floatValue; break;
            default: return;
        }

        Config::Moonlight::SetSettings(settings);
    }

    void BuildGrassMenu() {
        const Config::Grass::Settings& settings = Config::Grass::GetSettings();

        AddInt(static_cast<uint32_t>(GrassSetting::SEGMENT_COUNT), "Segment Count", settings.segmentCount, 1, 8, 1);
        AddFloat(static_cast<uint32_t>(GrassSetting::CURVE_AMOUNT), "Curve Amount", settings.curveAmount, 0.0f, 0.5f, 0.005f, 3);
        AddFloat(static_cast<uint32_t>(GrassSetting::SPACING), "Spacing", settings.spacing, 0.0185185185185185f, 0.1f, 0.001f, 4);
        AddFloat(static_cast<uint32_t>(GrassSetting::MIN_CULL_DISTANCE), "Min Cull Distance", settings.minCullDistance, 0.0f, 29.99f, 0.25f, 2);
        AddFloat(static_cast<uint32_t>(GrassSetting::MAX_CULL_DISTANCE), "Max Cull Distance", settings.maxCullDistance, 0.01f, 30.0f, 0.25f, 2);
        AddFloat(static_cast<uint32_t>(GrassSetting::CULL_EXPONENT), "Cull Exp", settings.cullExponent, 0.01f, 32.0f, 0.1f, 2);
        AddFloat(static_cast<uint32_t>(GrassSetting::BLADE_HEIGHT), "Blade Height", settings.bladeHeight, 0.01f, 0.5f, 0.005f, 3);
        AddFloat(static_cast<uint32_t>(GrassSetting::BLADE_WIDTH), "Blade Width", settings.bladeWidth, 0.0001f, 0.02f, 0.0001f, 4);
        AddLineBreak();
        AddFloat(static_cast<uint32_t>(GrassSetting::COLOR_1_R), "Color 1 R", settings.color1.r, 0.0f, 1.0f, 0.005f, 3);
        AddFloat(static_cast<uint32_t>(GrassSetting::COLOR_1_G), "Color 1 G", settings.color1.g, 0.0f, 1.0f, 0.005f, 3);
        AddFloat(static_cast<uint32_t>(GrassSetting::COLOR_1_B), "Color 1 B", settings.color1.b, 0.0f, 1.0f, 0.005f, 3);
        AddFloat(static_cast<uint32_t>(GrassSetting::COLOR_1_DARKNESS_FACTOR), "Color 1 Darkness", settings.color1DarknessFactor, 0.0f, 1.0f, 0.01f, 3);
        AddFloat(static_cast<uint32_t>(GrassSetting::COLOR_2_R), "Color 2 R", settings.color2.r, 0.0f, 1.0f, 0.005f, 3);
        AddFloat(static_cast<uint32_t>(GrassSetting::COLOR_2_G), "Color 2 G", settings.color2.g, 0.0f, 1.0f, 0.005f, 3);
        AddFloat(static_cast<uint32_t>(GrassSetting::COLOR_2_B), "Color 2 B", settings.color2.b, 0.0f, 1.0f, 0.005f, 3);
        AddFloat(static_cast<uint32_t>(GrassSetting::COLOR_2_DARKNESS_FACTOR), "Color 2 Darkness", settings.color2DarknessFactor, 0.0f, 1.0f, 0.01f, 3);
        AddFloat(static_cast<uint32_t>(GrassSetting::NOISE_SQUARE_MULTIPLIER), "Noise Square Mult", settings.noiseSquareMultiplier, 0.0f, 8.0f, 0.05f, 2);
        AddFloat(static_cast<uint32_t>(GrassSetting::NOISE_MIX_MULTIPLIER), "Noise Mix Mult", settings.noiseMixMultiplier, 0.0f, 1.0f, 0.01f, 2);
        AddFloat(static_cast<uint32_t>(GrassSetting::ROUGHNESS), "Roughness", settings.roughness, 0.0f, 1.0f, 0.01f, 3);
        AddFloat(static_cast<uint32_t>(GrassSetting::SUB_SURFACE_FACTOR), "Sub Surface Factor", settings.subSurfaceFactor, 0.0f, 0.98f, 0.01f, 3);
        AddLineBreak();
        AddFloat(static_cast<uint32_t>(GrassSetting::NORMAL_UP_BLEND), "Normal Up Blend", settings.normalUpBlend, 0.0f, 1.0f, 0.01f, 3);
        AddFloat(static_cast<uint32_t>(GrassSetting::NORMAL_BLEND_START_DISTANCE), "Normal Blend Start", settings.normalBlendStartDistance, 0.0f, 255.99f, 0.25f, 2);
        AddFloat(static_cast<uint32_t>(GrassSetting::NORMAL_BLEND_END_DISTANCE), "Normal Blend End", settings.normalBlendEndDistance, 0.01f, 256.0f, 0.25f, 2);
        AddFloat(static_cast<uint32_t>(GrassSetting::DIFFUSE_WRAP), "Diffuse Wrap", settings.diffuseWrap, 0.0f, 1.0f, 0.01f, 3);
        AddFloat(static_cast<uint32_t>(GrassSetting::TRANSMISSION_POWER), "Transmission Power", settings.transmissionPower, 0.25f, 32.0f, 0.25f, 2);
        AddFloat(static_cast<uint32_t>(GrassSetting::SPECULAR_STRENGTH), "Specular Strength", settings.specularStrength, 0.0f, 1.0f, 0.01f, 3);
        AddLineBreak();
        AddAction(static_cast<uint32_t>(GrassSetting::SAVE_TO_DISK), "Save to disk");
        AddAction(static_cast<uint32_t>(GrassSetting::LOAD_FROM_DISK), "Load from disk");
    }

    void ApplyGrassEdit(uint32_t id, const Value& value) {
        const GrassSetting setting = static_cast<GrassSetting>(id);
        if (setting == GrassSetting::SAVE_TO_DISK) {
            const bool saved = Config::Grass::SaveToDisk();
            Debug::BlitQuickDebugMessage(saved ? "Saved " + Config::Grass::GetFilePath() : "Failed to save grass config");
            return;
        }
        if (setting == GrassSetting::LOAD_FROM_DISK) {
            const bool loaded = Config::Grass::LoadFromDisk();
            Debug::BlitQuickDebugMessage(loaded ? "Loaded " + Config::Grass::GetFilePath() : "Failed to load grass config");
            return;
        }

        Config::Grass::Settings settings = Config::Grass::GetSettings();
        switch (setting) {
            case GrassSetting::SEGMENT_COUNT: settings.segmentCount = value.intValue; break;
            case GrassSetting::CURVE_AMOUNT:  settings.curveAmount = value.floatValue; break;
            case GrassSetting::SPACING:       settings.spacing = value.floatValue; break;
            case GrassSetting::MIN_CULL_DISTANCE: settings.minCullDistance = value.floatValue; break;
            case GrassSetting::MAX_CULL_DISTANCE: settings.maxCullDistance = value.floatValue; break;
            case GrassSetting::CULL_EXPONENT:     settings.cullExponent = value.floatValue; break;
            case GrassSetting::BLADE_HEIGHT:      settings.bladeHeight = value.floatValue; break;
            case GrassSetting::BLADE_WIDTH:       settings.bladeWidth = value.floatValue; break;
            case GrassSetting::COLOR_1_R:         settings.color1.r = value.floatValue; break;
            case GrassSetting::COLOR_1_G:         settings.color1.g = value.floatValue; break;
            case GrassSetting::COLOR_1_B:         settings.color1.b = value.floatValue; break;
            case GrassSetting::COLOR_1_DARKNESS_FACTOR: settings.color1DarknessFactor = value.floatValue; break;
            case GrassSetting::COLOR_2_R:         settings.color2.r = value.floatValue; break;
            case GrassSetting::COLOR_2_G:         settings.color2.g = value.floatValue; break;
            case GrassSetting::COLOR_2_B:         settings.color2.b = value.floatValue; break;
            case GrassSetting::COLOR_2_DARKNESS_FACTOR: settings.color2DarknessFactor = value.floatValue; break;
            case GrassSetting::NOISE_SQUARE_MULTIPLIER: settings.noiseSquareMultiplier = value.floatValue; break;
            case GrassSetting::NOISE_MIX_MULTIPLIER:    settings.noiseMixMultiplier = value.floatValue; break;
            case GrassSetting::ROUGHNESS:                settings.roughness = value.floatValue; break;
            case GrassSetting::SUB_SURFACE_FACTOR:       settings.subSurfaceFactor = value.floatValue; break;
            case GrassSetting::NORMAL_UP_BLEND:          settings.normalUpBlend = value.floatValue; break;
            case GrassSetting::NORMAL_BLEND_START_DISTANCE: settings.normalBlendStartDistance = value.floatValue; break;
            case GrassSetting::NORMAL_BLEND_END_DISTANCE:   settings.normalBlendEndDistance = value.floatValue; break;
            case GrassSetting::DIFFUSE_WRAP:             settings.diffuseWrap = value.floatValue; break;
            case GrassSetting::TRANSMISSION_POWER:       settings.transmissionPower = value.floatValue; break;
            case GrassSetting::SPECULAR_STRENGTH:        settings.specularStrength = value.floatValue; break;
            default: return;
        }
        Config::Grass::SetSettings(settings);
    }

    void BuildFlashlightMenu() {
        const Config::Flashlight::Settings& settings = Config::Flashlight::GetSettings();
        const std::vector<std::string> profileNames = GetIESProfileNames();

        AddEnum(static_cast<uint32_t>(FlashlightSetting::IES_PROFILE), "IES Profile", GetIESProfileIndex(profileNames, settings.iesProfile), profileNames);
        AddBool(static_cast<uint32_t>(FlashlightSetting::IES_ENABLED), "IES Enabled", settings.iesEnabled);
        AddFloat(static_cast<uint32_t>(FlashlightSetting::RANGE), "Range", settings.range, 1.0f, 100.0f, 0.1f, 2);
        AddFloat(static_cast<uint32_t>(FlashlightSetting::FALLOFF_EXPONENT), "Falloff Exponent", settings.falloffExponent, 0.01f, 8.0f, 0.01f, 2);
        AddFloat(static_cast<uint32_t>(FlashlightSetting::BRIGHTNESS), "Brightness", settings.brightness, 0.0f, 10.0f, 0.01f, 2);
        AddFloat(static_cast<uint32_t>(FlashlightSetting::COLOR_R), "Color R", settings.color.r, 0.0f, 1.0f, 0.01f, 3);
        AddFloat(static_cast<uint32_t>(FlashlightSetting::COLOR_G), "Color G", settings.color.g, 0.0f, 1.0f, 0.01f, 3);
        AddFloat(static_cast<uint32_t>(FlashlightSetting::COLOR_B), "Color B", settings.color.b, 0.0f, 1.0f, 0.01f, 3);
        AddFloat(static_cast<uint32_t>(FlashlightSetting::IES_CONE_SCALE), "IES Cone Scale", settings.iesConeScale, 0.1f, 1.2f, 0.01f, 2);
        AddFloat(static_cast<uint32_t>(FlashlightSetting::IES_INNER_ANGLE), "IES Inner Angle", settings.iesInnerAngle, 0.0f, 89.0f, 0.01f, 2);
        AddFloat(static_cast<uint32_t>(FlashlightSetting::IES_OUTER_ANGLE), "IES Outer Angle", settings.iesOuterAngle, 0.0f, 89.0f, 0.01f, 2);
        AddFloat(static_cast<uint32_t>(FlashlightSetting::IES_CONTRAST), "IES Contrast", settings.iesContrast, 0.1f, 8.0f, 0.01f, 2);

        AddLineBreak();
        AddBool(static_cast<uint32_t>(FlashlightSetting::CENTER_SPOT_ENABLED), "Center Spot Enabled", settings.centerSpotEnabled);
        AddFloat(static_cast<uint32_t>(FlashlightSetting::CENTER_SPOT_RANGE), "Center Spot Range", settings.centerSpotRange, 0.1f, 100.0f, 0.1f, 2);
        AddFloat(static_cast<uint32_t>(FlashlightSetting::CENTER_SPOT_FALLOFF_EXPONENT), "Center Spot Falloff Exponent", settings.centerSpotFalloffExponent, 0.01f, 8.0f, 0.01f, 2);
        AddFloat(static_cast<uint32_t>(FlashlightSetting::CENTER_SPOT_BRIGHTNESS), "Center Spot Brightness", settings.centerSpotBrightness, 0.0f, 10.0f, 0.01f, 2);
        AddFloat(static_cast<uint32_t>(FlashlightSetting::CENTER_SPOT_INNER_ANGLE), "Center Spot Inner Angle", settings.centerSpotInnerAngle, 0.0f, 89.0f, 0.01f, 2);
        AddFloat(static_cast<uint32_t>(FlashlightSetting::CENTER_SPOT_OUTER_ANGLE), "Center Spot Outer Angle", settings.centerSpotOuterAngle, 0.0f, 89.0f, 0.01f, 2);

        AddLineBreak();
        AddAction(static_cast<uint32_t>(FlashlightSetting::SAVE_TO_DISK), "Save to disk");
        AddAction(static_cast<uint32_t>(FlashlightSetting::LOAD_FROM_DISK), "Load from disk");
    }

    void ApplyFlashlightEdit(uint32_t id, const Value& value) {
        const FlashlightSetting setting = static_cast<FlashlightSetting>(id);
        if (setting == FlashlightSetting::SAVE_TO_DISK) {
            const bool saved = Config::Flashlight::SaveToDisk();
            Debug::BlitQuickDebugMessage(saved ? "Saved " + Config::Flashlight::GetFilePath() : "Failed to save flashlight config");
            return;
        }
        if (setting == FlashlightSetting::LOAD_FROM_DISK) {
            const bool loaded = Config::Flashlight::LoadFromDisk();
            Debug::BlitQuickDebugMessage(loaded ? "Loaded " + Config::Flashlight::GetFilePath() : "Failed to load flashlight config");
            return;
        }

        Config::Flashlight::Settings settings = Config::Flashlight::GetSettings();
        switch (setting) {
            case FlashlightSetting::IES_PROFILE: {
                const std::vector<std::string> profileNames = GetIESProfileNames();
                if (value.intValue < 0 || value.intValue >= static_cast<int32_t>(profileNames.size())) return;
                settings.iesProfile = profileNames[value.intValue];
                break;
            }
            case FlashlightSetting::IES_ENABLED:        settings.iesEnabled = value.boolValue;       break;
            case FlashlightSetting::RANGE:              settings.range = value.floatValue;           break;
            case FlashlightSetting::FALLOFF_EXPONENT:   settings.falloffExponent = value.floatValue; break;
            case FlashlightSetting::BRIGHTNESS:         settings.brightness = value.floatValue;      break;
            case FlashlightSetting::COLOR_R:            settings.color.r = value.floatValue;         break;
            case FlashlightSetting::COLOR_G:            settings.color.g = value.floatValue;         break;
            case FlashlightSetting::COLOR_B:            settings.color.b = value.floatValue;         break;
            case FlashlightSetting::IES_CONE_SCALE:     settings.iesConeScale = value.floatValue;    break;
            case FlashlightSetting::IES_INNER_ANGLE:    settings.iesInnerAngle = value.floatValue;   break;
            case FlashlightSetting::IES_OUTER_ANGLE:    settings.iesOuterAngle = value.floatValue;   break;
            case FlashlightSetting::IES_CONTRAST:       settings.iesContrast = value.floatValue;     break;
            case FlashlightSetting::CENTER_SPOT_ENABLED:          settings.centerSpotEnabled = value.boolValue;                  break;
            case FlashlightSetting::CENTER_SPOT_RANGE:            settings.centerSpotRange = value.floatValue;                   break;
            case FlashlightSetting::CENTER_SPOT_FALLOFF_EXPONENT: settings.centerSpotFalloffExponent = value.floatValue;         break;
            case FlashlightSetting::CENTER_SPOT_BRIGHTNESS:       settings.centerSpotBrightness = value.floatValue;              break;
            case FlashlightSetting::CENTER_SPOT_INNER_ANGLE:      settings.centerSpotInnerAngle = value.floatValue;              break;
            case FlashlightSetting::CENTER_SPOT_OUTER_ANGLE:      settings.centerSpotOuterAngle = value.floatValue;              break;
            default: return;
        }

        Config::Flashlight::SetSettings(settings);
    }
}

namespace Debug::Menu::FogMenu {

    enum struct Setting : uint32_t {
        ENABLED,
        APPEARANCE_PAGE,
        DENSITY_PAGE,
        HEIGHT_PAGE,
        DISTANCE_PAGE,
        NOISE_SHAPE_PAGE,
        NOISE_DETAIL_PAGE,
        MOTION_PAGE,
        SAVE_TO_DISK,
        REVERT_TO_DISK,

        COLOR_R,
        COLOR_G,
        COLOR_B,
        COLOR_STRENGTH,

        MAX_RAY_DISTANCE,
        STEP_COUNT,
        DENSITY_BIAS,
        DENSITY_SCALE,
        EXTINCTION_SCALE,
        AMBIENT_START_DISTANCE,
        AMBIENT_END_DISTANCE,
        AMBIENT_EXPONENT,

        HEIGHT_FADE_START,
        HEIGHT_FADE_END,
        HEIGHT_EXPONENT,
        LOW_HEIGHT_SCATTER_MULTIPLIER,

        DISTANCE_FOG_START,
        DISTANCE_FOG_END,
        DISTANCE_FOG_EXPONENT,
        DISTANCE_FOG_STRENGTH,

        CLUMP_SIZE_XZ,
        CLUMP_SIZE_Y,
        NOISE_SCALE_NEAR_MULTIPLIER,
        NOISE_SCALE_FAR_MULTIPLIER,
        NOISE_SCALE_START_DISTANCE,
        NOISE_SCALE_END_DISTANCE,
        NOISE_SCALE_EXPONENT,

        NOISE_MIP_BIAS,
        NOISE_MIP_SCALE,
        NOISE_MIN_MIP,
        NOISE_MAX_MIP,
        NOISE_NEAR_MIP,
        NOISE_FAR_MIP,
        NOISE_MIP_NEAR_DISTANCE,
        NOISE_MIP_FAR_DISTANCE,
        NOISE_MIP_EXPONENT,
        NOISE_MIP_RESPECT_STEP,

        WIND_X,
        WIND_Y,
        WIND_Z,
        TIME_SCROLL_SPEED,
        X_MORPH_SPEED,
        Z_MORPH_SPEED,
        Y_SCROLL_SPEED,
    };

    PageId g_mainPage = ROOT_PAGE_ID;
    PageId g_appearancePage = ROOT_PAGE_ID;
    PageId g_densityPage = ROOT_PAGE_ID;
    PageId g_heightPage = ROOT_PAGE_ID;
    PageId g_distancePage = ROOT_PAGE_ID;
    PageId g_noiseShapePage = ROOT_PAGE_ID;
    PageId g_noiseDetailPage = ROOT_PAGE_ID;
    PageId g_motionPage = ROOT_PAGE_ID;

    void BuildMainMenu();
    void BuildAppearanceMenu();
    void BuildDensityMenu();
    void BuildHeightMenu();
    void BuildDistanceMenu();
    void BuildNoiseShapeMenu();
    void BuildNoiseDetailMenu();
    void BuildMotionMenu();
    void ApplyEdit(uint32_t id, const Value& value);

    PageId RegisterMenu(PageId parentPage) {
        g_mainPage = RegisterPage("FOG", parentPage, BuildMainMenu, ApplyEdit);
        g_appearancePage = RegisterPage("FOG APPEARANCE", g_mainPage, BuildAppearanceMenu, ApplyEdit);
        g_densityPage = RegisterPage("FOG DENSITY", g_mainPage, BuildDensityMenu, ApplyEdit);
        g_heightPage = RegisterPage("FOG HEIGHT", g_mainPage, BuildHeightMenu, ApplyEdit);
        g_distancePage = RegisterPage("FOG DISTANCE", g_mainPage, BuildDistanceMenu, ApplyEdit);
        g_noiseShapePage = RegisterPage("FOG NOISE SHAPE", g_mainPage, BuildNoiseShapeMenu, ApplyEdit);
        g_noiseDetailPage = RegisterPage("FOG NOISE DETAIL", g_mainPage, BuildNoiseDetailMenu, ApplyEdit);
        g_motionPage = RegisterPage("FOG MOTION", g_mainPage, BuildMotionMenu, ApplyEdit);
        return g_mainPage;
    }

    void BuildMainMenu() {
        const Config::Fog::Settings& settings = Config::Fog::GetSettings();
        AddBool(static_cast<uint32_t>(Setting::ENABLED), "Enabled", settings.enabled);
        AddSubMenu(static_cast<uint32_t>(Setting::APPEARANCE_PAGE), "Appearance", g_appearancePage);
        AddSubMenu(static_cast<uint32_t>(Setting::DENSITY_PAGE), "Density & Sampling", g_densityPage);
        AddSubMenu(static_cast<uint32_t>(Setting::HEIGHT_PAGE), "Height Response", g_heightPage);
        AddSubMenu(static_cast<uint32_t>(Setting::DISTANCE_PAGE), "Distance Fade", g_distancePage);
        AddSubMenu(static_cast<uint32_t>(Setting::NOISE_SHAPE_PAGE), "Noise Shape", g_noiseShapePage);
        AddSubMenu(static_cast<uint32_t>(Setting::NOISE_DETAIL_PAGE), "Noise Detail", g_noiseDetailPage);
        AddSubMenu(static_cast<uint32_t>(Setting::MOTION_PAGE), "Motion", g_motionPage);
        AddLineBreak();
        AddAction(static_cast<uint32_t>(Setting::SAVE_TO_DISK), "Save to disk");
        AddAction(static_cast<uint32_t>(Setting::REVERT_TO_DISK), "Revert to disk");
    }

    void BuildAppearanceMenu() {
        const Config::Fog::Settings& settings = Config::Fog::GetSettings();
        AddFloat(static_cast<uint32_t>(Setting::COLOR_R), "Color R", settings.color.r, 0.0f, 4.0f, 0.005f, 3);
        AddFloat(static_cast<uint32_t>(Setting::COLOR_G), "Color G", settings.color.g, 0.0f, 4.0f, 0.005f, 3);
        AddFloat(static_cast<uint32_t>(Setting::COLOR_B), "Color B", settings.color.b, 0.0f, 4.0f, 0.005f, 3);
        AddFloat(static_cast<uint32_t>(Setting::COLOR_STRENGTH), "Color Strength", settings.colorStrength, 0.0f, 8.0f, 0.01f, 3);
    }

    void BuildDensityMenu() {
        const Config::Fog::Settings& settings = Config::Fog::GetSettings();
        AddFloat(static_cast<uint32_t>(Setting::MAX_RAY_DISTANCE), "Max Ray Distance", settings.maxRayDistance, 0.1f, 256.0f, 0.5f, 2);
        AddInt(static_cast<uint32_t>(Setting::STEP_COUNT), "Step Count", settings.stepCount, 1, 128, 1);
        AddFloat(static_cast<uint32_t>(Setting::DENSITY_BIAS), "Density Bias", settings.densityBias, 0.0f, 1.0f, 0.001f, 4);
        AddFloat(static_cast<uint32_t>(Setting::DENSITY_SCALE), "Density Scale", settings.densityScale, 0.0f, 16.0f, 0.05f, 3);
        AddFloat(static_cast<uint32_t>(Setting::EXTINCTION_SCALE), "Extinction Scale", settings.extinctionScale, 0.0f, 4.0f, 0.001f, 4);
        AddFloat(static_cast<uint32_t>(Setting::AMBIENT_START_DISTANCE), "Ambient Fade Start", settings.ambientStartDistance, 0.0f, 256.0f, 0.25f, 2);
        AddFloat(static_cast<uint32_t>(Setting::AMBIENT_END_DISTANCE), "Ambient Fade End", settings.ambientEndDistance, 0.0f, 512.0f, 0.25f, 2);
        AddFloat(static_cast<uint32_t>(Setting::AMBIENT_EXPONENT), "Ambient Exponent", settings.ambientExponent, 0.01f, 16.0f, 0.05f, 2);
    }

    void BuildHeightMenu() {
        const Config::Fog::Settings& settings = Config::Fog::GetSettings();
        AddFloat(static_cast<uint32_t>(Setting::HEIGHT_FADE_START), "Fade Start Height", settings.heightFadeStart, -256.0f, 256.0f, 0.5f, 2);
        AddFloat(static_cast<uint32_t>(Setting::HEIGHT_FADE_END), "Fade End Height", settings.heightFadeEnd, -256.0f, 512.0f, 0.5f, 2);
        AddFloat(static_cast<uint32_t>(Setting::HEIGHT_EXPONENT), "Height Exponent", settings.heightExponent, 0.01f, 16.0f, 0.05f, 2);
        AddFloat(static_cast<uint32_t>(Setting::LOW_HEIGHT_SCATTER_MULTIPLIER), "Low Height Scatter", settings.lowHeightScatterMultiplier, 0.0f, 16.0f, 0.05f, 2);
    }

    void BuildDistanceMenu() {
        const Config::Fog::Settings& settings = Config::Fog::GetSettings();
        AddFloat(static_cast<uint32_t>(Setting::DISTANCE_FOG_START), "Start Distance", settings.distanceFogStart, 0.0f, 512.0f, 0.5f, 2);
        AddFloat(static_cast<uint32_t>(Setting::DISTANCE_FOG_END), "End Distance", settings.distanceFogEnd, 0.0f, 1024.0f, 0.5f, 2);
        AddFloat(static_cast<uint32_t>(Setting::DISTANCE_FOG_EXPONENT), "Exponent", settings.distanceFogExponent, 0.01f, 16.0f, 0.05f, 2);
        AddFloat(static_cast<uint32_t>(Setting::DISTANCE_FOG_STRENGTH), "Strength", settings.distanceFogStrength, 0.0f, 1.0f, 0.01f, 3);
    }

    void BuildNoiseShapeMenu() {
        const Config::Fog::Settings& settings = Config::Fog::GetSettings();
        AddFloat(static_cast<uint32_t>(Setting::CLUMP_SIZE_XZ), "Clump Size XZ", settings.clumpSizeXZ, 0.1f, 256.0f, 0.25f, 2);
        AddFloat(static_cast<uint32_t>(Setting::CLUMP_SIZE_Y), "Clump Size Y", settings.clumpSizeY, 0.1f, 256.0f, 0.25f, 2);
        AddFloat(static_cast<uint32_t>(Setting::NOISE_SCALE_NEAR_MULTIPLIER), "Near Size Multiplier", settings.noiseScaleNearMultiplier, 0.05f, 32.0f, 0.05f, 2);
        AddFloat(static_cast<uint32_t>(Setting::NOISE_SCALE_FAR_MULTIPLIER), "Far Size Multiplier", settings.noiseScaleFarMultiplier, 0.05f, 32.0f, 0.05f, 2);
        AddFloat(static_cast<uint32_t>(Setting::NOISE_SCALE_START_DISTANCE), "Size Fade Start", settings.noiseScaleStartDistance, 0.0f, 512.0f, 0.5f, 2);
        AddFloat(static_cast<uint32_t>(Setting::NOISE_SCALE_END_DISTANCE), "Size Fade End", settings.noiseScaleEndDistance, 0.0f, 1024.0f, 0.5f, 2);
        AddFloat(static_cast<uint32_t>(Setting::NOISE_SCALE_EXPONENT), "Size Fade Exponent", settings.noiseScaleExponent, 0.01f, 16.0f, 0.05f, 2);
    }

    void BuildNoiseDetailMenu() {
        const Config::Fog::Settings& settings = Config::Fog::GetSettings();
        AddFloat(static_cast<uint32_t>(Setting::NOISE_MIP_BIAS), "Mip Bias", settings.noiseMipBias, -8.0f, 8.0f, 0.05f, 2);
        AddFloat(static_cast<uint32_t>(Setting::NOISE_MIP_SCALE), "Mip Scale", settings.noiseMipScale, 0.0f, 8.0f, 0.05f, 2);
        AddFloat(static_cast<uint32_t>(Setting::NOISE_MIN_MIP), "Minimum Mip", settings.noiseMinMip, 0.0f, 16.0f, 0.1f, 2);
        AddFloat(static_cast<uint32_t>(Setting::NOISE_MAX_MIP), "Maximum Mip", settings.noiseMaxMip, 0.0f, 16.0f, 0.1f, 2);
        AddFloat(static_cast<uint32_t>(Setting::NOISE_NEAR_MIP), "Near Mip", settings.noiseNearMip, 0.0f, 16.0f, 0.1f, 2);
        AddFloat(static_cast<uint32_t>(Setting::NOISE_FAR_MIP), "Far Mip", settings.noiseFarMip, 0.0f, 16.0f, 0.1f, 2);
        AddFloat(static_cast<uint32_t>(Setting::NOISE_MIP_NEAR_DISTANCE), "Mip Fade Start", settings.noiseMipNearDistance, 0.0f, 512.0f, 0.1f, 2);
        AddFloat(static_cast<uint32_t>(Setting::NOISE_MIP_FAR_DISTANCE), "Mip Fade End", settings.noiseMipFarDistance, 0.0f, 1024.0f, 0.1f, 2);
        AddFloat(static_cast<uint32_t>(Setting::NOISE_MIP_EXPONENT), "Mip Exponent", settings.noiseMipExponent, 0.01f, 16.0f, 0.1f, 2);
        AddFloat(static_cast<uint32_t>(Setting::NOISE_MIP_RESPECT_STEP), "Respect Step LOD", settings.noiseMipRespectStep, 0.0f, 1.0f, 0.05f, 2);
    }

    void BuildMotionMenu() {
        const Config::Fog::Settings& settings = Config::Fog::GetSettings();
        AddFloat(static_cast<uint32_t>(Setting::WIND_X), "Wind X", settings.windVelocity.x, -16.0f, 16.0f, 0.01f, 2);
        AddFloat(static_cast<uint32_t>(Setting::WIND_Y), "Wind Y", settings.windVelocity.y, -16.0f, 16.0f, 0.01f, 2);
        AddFloat(static_cast<uint32_t>(Setting::WIND_Z), "Wind Z", settings.windVelocity.z, -16.0f, 16.0f, 0.01f, 2);
        AddFloat(static_cast<uint32_t>(Setting::TIME_SCROLL_SPEED), "Time Scroll Speed", settings.timeScrollSpeed, -32.0f, 32.0f, 0.1f, 2);
        AddFloat(static_cast<uint32_t>(Setting::X_MORPH_SPEED), "X Morph Speed", settings.xMorphSpeed, -32.0f, 32.0f, 0.1f, 2);
        AddFloat(static_cast<uint32_t>(Setting::Z_MORPH_SPEED), "Z Morph Speed", settings.zMorphSpeed, -32.0f, 32.0f, 0.1f, 2);
        AddFloat(static_cast<uint32_t>(Setting::Y_SCROLL_SPEED), "Y Scroll Speed", settings.yScrollSpeed, -32.0f, 32.0f, 0.1f, 2);
    }

    void ApplyEdit(uint32_t id, const Value& value) {
        const Setting setting = static_cast<Setting>(id);
        if (setting == Setting::SAVE_TO_DISK) {
            const bool saved = Config::Fog::SaveToDisk();
            Debug::BlitQuickDebugMessage(saved ? "Saved " + Config::Fog::GetFilePath() : "Failed to save fog config");
            return;
        }
        if (setting == Setting::REVERT_TO_DISK) {
            const bool loaded = Config::Fog::LoadFromDisk();
            Debug::BlitQuickDebugMessage(loaded ? "Reverted to " + Config::Fog::GetFilePath() : "Failed to revert fog config");
            return;
        }

        Config::Fog::Settings settings = Config::Fog::GetSettings();
        switch (setting) {
            case Setting::ENABLED:                         settings.enabled = value.boolValue; break;
            case Setting::COLOR_R:                         settings.color.r = value.floatValue; break;
            case Setting::COLOR_G:                         settings.color.g = value.floatValue; break;
            case Setting::COLOR_B:                         settings.color.b = value.floatValue; break;
            case Setting::COLOR_STRENGTH:                  settings.colorStrength = value.floatValue; break;
            case Setting::MAX_RAY_DISTANCE:                settings.maxRayDistance = value.floatValue; break;
            case Setting::STEP_COUNT:                      settings.stepCount = value.intValue; break;
            case Setting::DENSITY_BIAS:                    settings.densityBias = value.floatValue; break;
            case Setting::DENSITY_SCALE:                   settings.densityScale = value.floatValue; break;
            case Setting::EXTINCTION_SCALE:                settings.extinctionScale = value.floatValue; break;
            case Setting::AMBIENT_START_DISTANCE:          settings.ambientStartDistance = value.floatValue; break;
            case Setting::AMBIENT_END_DISTANCE:            settings.ambientEndDistance = value.floatValue; break;
            case Setting::AMBIENT_EXPONENT:                settings.ambientExponent = value.floatValue; break;
            case Setting::HEIGHT_FADE_START:               settings.heightFadeStart = value.floatValue; break;
            case Setting::HEIGHT_FADE_END:                 settings.heightFadeEnd = value.floatValue; break;
            case Setting::HEIGHT_EXPONENT:                 settings.heightExponent = value.floatValue; break;
            case Setting::LOW_HEIGHT_SCATTER_MULTIPLIER:   settings.lowHeightScatterMultiplier = value.floatValue; break;
            case Setting::DISTANCE_FOG_START:              settings.distanceFogStart = value.floatValue; break;
            case Setting::DISTANCE_FOG_END:                settings.distanceFogEnd = value.floatValue; break;
            case Setting::DISTANCE_FOG_EXPONENT:           settings.distanceFogExponent = value.floatValue; break;
            case Setting::DISTANCE_FOG_STRENGTH:           settings.distanceFogStrength = value.floatValue; break;
            case Setting::CLUMP_SIZE_XZ:                   settings.clumpSizeXZ = value.floatValue; break;
            case Setting::CLUMP_SIZE_Y:                    settings.clumpSizeY = value.floatValue; break;
            case Setting::NOISE_SCALE_NEAR_MULTIPLIER:     settings.noiseScaleNearMultiplier = value.floatValue; break;
            case Setting::NOISE_SCALE_FAR_MULTIPLIER:      settings.noiseScaleFarMultiplier = value.floatValue; break;
            case Setting::NOISE_SCALE_START_DISTANCE:      settings.noiseScaleStartDistance = value.floatValue; break;
            case Setting::NOISE_SCALE_END_DISTANCE:        settings.noiseScaleEndDistance = value.floatValue; break;
            case Setting::NOISE_SCALE_EXPONENT:            settings.noiseScaleExponent = value.floatValue; break;
            case Setting::NOISE_MIP_BIAS:                  settings.noiseMipBias = value.floatValue; break;
            case Setting::NOISE_MIP_SCALE:                 settings.noiseMipScale = value.floatValue; break;
            case Setting::NOISE_MIN_MIP:                   settings.noiseMinMip = value.floatValue; break;
            case Setting::NOISE_MAX_MIP:                   settings.noiseMaxMip = value.floatValue; break;
            case Setting::NOISE_NEAR_MIP:                  settings.noiseNearMip = value.floatValue; break;
            case Setting::NOISE_FAR_MIP:                   settings.noiseFarMip = value.floatValue; break;
            case Setting::NOISE_MIP_NEAR_DISTANCE:         settings.noiseMipNearDistance = value.floatValue; break;
            case Setting::NOISE_MIP_FAR_DISTANCE:          settings.noiseMipFarDistance = value.floatValue; break;
            case Setting::NOISE_MIP_EXPONENT:              settings.noiseMipExponent = value.floatValue; break;
            case Setting::NOISE_MIP_RESPECT_STEP:          settings.noiseMipRespectStep = value.floatValue; break;
            case Setting::WIND_X:                          settings.windVelocity.x = value.floatValue; break;
            case Setting::WIND_Y:                          settings.windVelocity.y = value.floatValue; break;
            case Setting::WIND_Z:                          settings.windVelocity.z = value.floatValue; break;
            case Setting::TIME_SCROLL_SPEED:               settings.timeScrollSpeed = value.floatValue; break;
            case Setting::X_MORPH_SPEED:                   settings.xMorphSpeed = value.floatValue; break;
            case Setting::Z_MORPH_SPEED:                   settings.zMorphSpeed = value.floatValue; break;
            case Setting::Y_SCROLL_SPEED:                  settings.yScrollSpeed = value.floatValue; break;
            default: return;
        }

        Config::Fog::SetSettings(settings);
    }
}
