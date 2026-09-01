#include "Debug_menu.h"

#include "Unloved/Debug/Debug.h"
#include "Unloved/Systems/Ocean/Ocean.h"

#include <cstdint>
#include <limits>

namespace Debug::Menu::OceanMenu {

    enum struct Setting : uint32_t {
        SIMULATE,
        ALTERNATE_BAND_UPDATES,
        DISPLAY_MODE,
        WIND_SPEED,
        GRAVITY,
        HEIGHT_SCALE,
        DISPLACEMENT_SCALE,
        TIME_SCALE,
        TIME_OFFSET,
        BAND_0,
        BAND_1,
        SURFACE,
        RESET_DEFAULTS,
        SAVE_TO_DISK,
        LOAD_FROM_DISK,
    };

    enum struct SurfaceSetting : uint32_t {
        NORMALS,
        RIPPLES,
        MATERIAL,
        SUBSURFACE,
        FOG,
        REFRACTION,
        UNDERWATER_FOG,
        UNDERWATER_DISTORTION,
        UNDERWATER_COLOR,
    };

    enum struct NormalSetting : uint32_t {
        STRENGTH,
        CONVERGE_START_DISTANCE,
        CONVERGE_END_DISTANCE,
        CONVERGE_MAX_FACTOR,
        CONVERGE_EXPONENT,
        SOFTENING,
    };

    enum struct RippleSetting : uint32_t {
        TILING,
        STRENGTH,
        SECOND_LAYER_SCALE,
        VELOCITY_0_X,
        VELOCITY_0_Y,
        VELOCITY_1_X,
        VELOCITY_1_Y,
    };

    enum struct MaterialSetting : uint32_t {
        SPECULAR_ANTI_ALIASING,
        ALBEDO_R,
        ALBEDO_G,
        ALBEDO_B,
        ROUGHNESS,
        REFLECTANCE,
        REFLECTION_GAMMA,
        DIFFUSE_STRENGTH,
    };

    enum struct SubsurfaceSetting : uint32_t {
        HEIGHT_RANGE,
        STRENGTH,
        UNDERWATER_STRENGTH,
        RADIUS_MINIMUM,
        RADIUS_MAXIMUM,
        INTENSITY,
        FALLOFF,
        SATURATION,
    };

    enum struct FogSetting : uint32_t {
        COLOR_R,
        COLOR_G,
        COLOR_B,
        START_DISTANCE,
        END_DISTANCE,
        EXPONENT,
        STRENGTH,
    };

    enum struct RefractionSetting : uint32_t {
        PLANE_HEIGHT_OFFSET,
        DISTORTION_SPEED,
        DISTORTION_STRENGTH,
        DISTORTION_TILING,
        TINT_STRENGTH,
    };

    enum struct UnderwaterFogSetting : uint32_t {
        COLOR_R,
        COLOR_G,
        COLOR_B,
        STRENGTH,
        DARKNESS_CURVE,
    };

    enum struct UnderwaterDistortionSetting : uint32_t {
        SPEED,
        STRENGTH,
    };

    enum struct UnderwaterColorSetting : uint32_t {
        TINT_R,
        TINT_G,
        TINT_B,
        DEPTH_TINT_STRENGTH,
        DEPTH_TINT_ORIGINAL_WEIGHT,
        GEOMETRY_COLOR_SQUARED_STRENGTH,
        GEOMETRY_COLOR_STRENGTH,
        GEOMETRY_TINT_STRENGTH,
        OPEN_WATER_TINT_STRENGTH,
        OPEN_WATER_BRIGHTNESS,
    };

    enum struct BandSetting : uint32_t {
        DOMAIN_SIZE,
        MINIMUM_WAVELENGTH,
        MAXIMUM_WAVELENGTH,
        MINIMUM_WAVELENGTH_FADE,
        MAXIMUM_WAVELENGTH_FADE,
        WIND_DIRECTION_X,
        WIND_DIRECTION_Y,
        AMPLITUDE,
        WIND_ALIGNMENT_EXPONENT,
        OPPOSING_WAVES_DAMPING,
        SMALL_WAVES_DAMPING,
        RANDOM_SEED,
    };

    PageId g_oceanPage = ROOT_PAGE_ID;
    PageId g_band0Page = ROOT_PAGE_ID;
    PageId g_band1Page = ROOT_PAGE_ID;
    PageId g_surfacePage = ROOT_PAGE_ID;
    PageId g_normalPage = ROOT_PAGE_ID;
    PageId g_ripplePage = ROOT_PAGE_ID;
    PageId g_materialPage = ROOT_PAGE_ID;
    PageId g_subsurfacePage = ROOT_PAGE_ID;
    PageId g_fogPage = ROOT_PAGE_ID;
    PageId g_refractionPage = ROOT_PAGE_ID;
    PageId g_underwaterFogPage = ROOT_PAGE_ID;
    PageId g_underwaterDistortionPage = ROOT_PAGE_ID;
    PageId g_underwaterColorPage = ROOT_PAGE_ID;

    void BuildOceanMenu();
    void BuildBand0Menu();
    void BuildBand1Menu();
    void BuildSurfaceMenu();
    void BuildNormalMenu();
    void BuildRippleMenu();
    void BuildMaterialMenu();
    void BuildSubsurfaceMenu();
    void BuildFogMenu();
    void BuildRefractionMenu();
    void BuildUnderwaterFogMenu();
    void BuildUnderwaterDistortionMenu();
    void BuildUnderwaterColorMenu();
    void ApplyOceanEdit(uint32_t id, const Value& value);
    void ApplyBand0Edit(uint32_t id, const Value& value);
    void ApplyBand1Edit(uint32_t id, const Value& value);
    void ApplyNormalEdit(uint32_t id, const Value& value);
    void ApplyRippleEdit(uint32_t id, const Value& value);
    void ApplyMaterialEdit(uint32_t id, const Value& value);
    void ApplySubsurfaceEdit(uint32_t id, const Value& value);
    void ApplyFogEdit(uint32_t id, const Value& value);
    void ApplyRefractionEdit(uint32_t id, const Value& value);
    void ApplyUnderwaterFogEdit(uint32_t id, const Value& value);
    void ApplyUnderwaterDistortionEdit(uint32_t id, const Value& value);
    void ApplyUnderwaterColorEdit(uint32_t id, const Value& value);

    PageId RegisterMenu(PageId parentPage) {
        g_oceanPage = RegisterPage("OCEAN", parentPage, BuildOceanMenu, ApplyOceanEdit);
        g_band0Page = RegisterPage("OCEAN BAND 0", g_oceanPage, BuildBand0Menu, ApplyBand0Edit);
        g_band1Page = RegisterPage("OCEAN BAND 1", g_oceanPage, BuildBand1Menu, ApplyBand1Edit);
        g_surfacePage = RegisterPage("OCEAN SURFACE", g_oceanPage, BuildSurfaceMenu, nullptr);
        g_normalPage = RegisterPage("OCEAN SURFACE NORMALS", g_surfacePage, BuildNormalMenu, ApplyNormalEdit);
        g_ripplePage = RegisterPage("OCEAN SURFACE RIPPLES", g_surfacePage, BuildRippleMenu, ApplyRippleEdit);
        g_materialPage = RegisterPage("OCEAN SURFACE MATERIAL", g_surfacePage, BuildMaterialMenu, ApplyMaterialEdit);
        g_subsurfacePage = RegisterPage("OCEAN SURFACE SUBSURFACE", g_surfacePage, BuildSubsurfaceMenu, ApplySubsurfaceEdit);
        g_fogPage = RegisterPage("OCEAN SURFACE FOG", g_surfacePage, BuildFogMenu, ApplyFogEdit);
        g_refractionPage = RegisterPage("OCEAN SURFACE REFRACTION", g_surfacePage, BuildRefractionMenu, ApplyRefractionEdit);
        g_underwaterFogPage = RegisterPage("OCEAN UNDERWATER FOG", g_surfacePage, BuildUnderwaterFogMenu, ApplyUnderwaterFogEdit);
        g_underwaterDistortionPage = RegisterPage("OCEAN UNDERWATER DISTORTION", g_surfacePage, BuildUnderwaterDistortionMenu, ApplyUnderwaterDistortionEdit);
        g_underwaterColorPage = RegisterPage("OCEAN UNDERWATER COLOR", g_surfacePage, BuildUnderwaterColorMenu, ApplyUnderwaterColorEdit);
        return g_oceanPage;
    }

    void BuildOceanMenu() {
        const Ocean::Settings settings = Ocean::GetSettings();

        AddBool(static_cast<uint32_t>(Setting::SIMULATE), "Simulate", settings.simulate);
        AddBool(static_cast<uint32_t>(Setting::ALTERNATE_BAND_UPDATES), "Alternate FFT band updates", settings.alternateBandUpdates);
        AddEnum(static_cast<uint32_t>(Setting::DISPLAY_MODE), "Display mode", static_cast<int32_t>(settings.displayMode), { "Combined", "Band 0", "Band 1" });
        AddFloat(static_cast<uint32_t>(Setting::WIND_SPEED), "Wind speed", settings.windSpeed, 0.1f, 200.0f, 0.5f, 1);
        AddFloat(static_cast<uint32_t>(Setting::GRAVITY), "Gravity", settings.gravity, 0.1f, 30.0f, 0.1f, 2);
        AddFloat(static_cast<uint32_t>(Setting::HEIGHT_SCALE), "Height", settings.heightScale, 0.0f, 10.0f, 0.05f, 2);
        AddFloat(static_cast<uint32_t>(Setting::DISPLACEMENT_SCALE), "Horizontal chop", settings.displacementScale, 0.0f, 10.0f, 0.05f, 2);
        AddFloat(static_cast<uint32_t>(Setting::TIME_SCALE), "Time scale", settings.simulationTimeScale, 0.0f, 10.0f, 0.05f, 2);
        AddFloat(static_cast<uint32_t>(Setting::TIME_OFFSET), "Time offset", settings.simulationTimeOffset, -10000.0f, 10000.0f, 1.0f, 1);
        AddSubMenu(static_cast<uint32_t>(Setting::BAND_0), "Band 0", g_band0Page);
        AddSubMenu(static_cast<uint32_t>(Setting::BAND_1), "Band 1", g_band1Page);
        AddSubMenu(static_cast<uint32_t>(Setting::SURFACE), "Surface", g_surfacePage);

        AddLineBreak();
        AddAction(static_cast<uint32_t>(Setting::RESET_DEFAULTS), "Reset ocean defaults");
        AddAction(static_cast<uint32_t>(Setting::SAVE_TO_DISK), "Save to disk");
        AddAction(static_cast<uint32_t>(Setting::LOAD_FROM_DISK), "Load from disk");
    }

    void BuildSurfaceMenu() {
        AddSubMenu(static_cast<uint32_t>(SurfaceSetting::NORMALS), "Normals", g_normalPage);
        AddSubMenu(static_cast<uint32_t>(SurfaceSetting::RIPPLES), "Ripples", g_ripplePage);
        AddSubMenu(static_cast<uint32_t>(SurfaceSetting::MATERIAL), "Material", g_materialPage);
        AddSubMenu(static_cast<uint32_t>(SurfaceSetting::SUBSURFACE), "Subsurface", g_subsurfacePage);
        AddSubMenu(static_cast<uint32_t>(SurfaceSetting::FOG), "Fog", g_fogPage);
        AddSubMenu(static_cast<uint32_t>(SurfaceSetting::REFRACTION), "Refraction", g_refractionPage);
        AddSubMenu(static_cast<uint32_t>(SurfaceSetting::UNDERWATER_FOG), "Underwater fog", g_underwaterFogPage);
        AddSubMenu(static_cast<uint32_t>(SurfaceSetting::UNDERWATER_DISTORTION), "Underwater distortion", g_underwaterDistortionPage);
        AddSubMenu(static_cast<uint32_t>(SurfaceSetting::UNDERWATER_COLOR), "Underwater color", g_underwaterColorPage);
    }

    void BuildNormalMenu() {
        const Ocean::SurfaceSettings surface = Ocean::GetSettings().surface;

        AddFloat(static_cast<uint32_t>(NormalSetting::STRENGTH), "Strength", surface.normalScale, 0.0f, 20.0f, 0.1f, 2);
        AddFloat(static_cast<uint32_t>(NormalSetting::CONVERGE_START_DISTANCE), "Converge start", surface.normalConvergeStartDistance, 0.0f, 5000.0f, 5.0f, 1);
        AddFloat(static_cast<uint32_t>(NormalSetting::CONVERGE_END_DISTANCE), "Converge end", surface.normalConvergeEndDistance, 0.001f, 10000.0f, 5.0f, 1);
        AddFloat(static_cast<uint32_t>(NormalSetting::CONVERGE_MAX_FACTOR), "Converge factor", surface.normalConvergeMaxFactor, 0.0f, 1.0f, 0.05f, 2);
        AddFloat(static_cast<uint32_t>(NormalSetting::CONVERGE_EXPONENT), "Converge exponent", surface.normalConvergeExponent, 0.001f, 8.0f, 0.05f, 3);
        AddFloat(static_cast<uint32_t>(NormalSetting::SOFTENING), "Softening", surface.normalSoftening, 0.0f, 1.0f, 0.05f, 2);
    }

    void BuildRippleMenu() {
        const Ocean::SurfaceSettings surface = Ocean::GetSettings().surface;

        AddFloat(static_cast<uint32_t>(RippleSetting::TILING), "Tiling", surface.rippleTiling, 0.0f, 10.0f, 0.025f, 3);
        AddFloat(static_cast<uint32_t>(RippleSetting::STRENGTH), "Strength", surface.rippleStrength, 0.0f, 1.0f, 0.005f, 3);
        AddFloat(static_cast<uint32_t>(RippleSetting::SECOND_LAYER_SCALE), "Second layer scale", surface.rippleSecondLayerScale, 0.0f, 10.0f, 0.05f, 2);
        AddFloat(static_cast<uint32_t>(RippleSetting::VELOCITY_0_X), "Velocity 0 X", surface.rippleVelocity0.x, -10.0f, 10.0f, 0.01f, 2);
        AddFloat(static_cast<uint32_t>(RippleSetting::VELOCITY_0_Y), "Velocity 0 Y", surface.rippleVelocity0.y, -10.0f, 10.0f, 0.01f, 2);
        AddFloat(static_cast<uint32_t>(RippleSetting::VELOCITY_1_X), "Velocity 1 X", surface.rippleVelocity1.x, -10.0f, 10.0f, 0.01f, 2);
        AddFloat(static_cast<uint32_t>(RippleSetting::VELOCITY_1_Y), "Velocity 1 Y", surface.rippleVelocity1.y, -10.0f, 10.0f, 0.01f, 2);
    }

    void BuildMaterialMenu() {
        const Ocean::SurfaceSettings surface = Ocean::GetSettings().surface;

        AddBool(static_cast<uint32_t>(MaterialSetting::SPECULAR_ANTI_ALIASING), "Specular AA", surface.specularAntiAliasing);
        AddFloat(static_cast<uint32_t>(MaterialSetting::ALBEDO_R), "Albedo R", surface.albedo.r, 0.0f, 2.0f, 0.005f, 3);
        AddFloat(static_cast<uint32_t>(MaterialSetting::ALBEDO_G), "Albedo G", surface.albedo.g, 0.0f, 2.0f, 0.005f, 3);
        AddFloat(static_cast<uint32_t>(MaterialSetting::ALBEDO_B), "Albedo B", surface.albedo.b, 0.0f, 2.0f, 0.005f, 3);
        AddFloat(static_cast<uint32_t>(MaterialSetting::ROUGHNESS), "Roughness", surface.roughness, 0.001f, 1.0f, 0.01f, 3);
        AddFloat(static_cast<uint32_t>(MaterialSetting::REFLECTANCE), "Reflectance", surface.reflectance, 0.0f, 1.0f, 0.01f, 3);
        AddFloat(static_cast<uint32_t>(MaterialSetting::REFLECTION_GAMMA), "Reflection gamma", surface.reflectionGamma, 0.001f, 8.0f, 0.05f, 3);
        AddFloat(static_cast<uint32_t>(MaterialSetting::DIFFUSE_STRENGTH), "Diffuse strength", surface.diffuseStrength, 0.0f, 2.0f, 0.0025f, 4);
    }

    void BuildSubsurfaceMenu() {
        const Ocean::SurfaceSettings surface = Ocean::GetSettings().surface;

        AddFloat(static_cast<uint32_t>(SubsurfaceSetting::HEIGHT_RANGE), "Height range", surface.sssHeightRange, 0.001f, 10.0f, 0.05f, 3);
        AddFloat(static_cast<uint32_t>(SubsurfaceSetting::STRENGTH), "Strength", surface.sssStrength, 0.0f, 20.0f, 0.1f, 2);
        AddFloat(static_cast<uint32_t>(SubsurfaceSetting::UNDERWATER_STRENGTH), "Underwater strength", surface.underwaterSssStrength, 0.0f, 20.0f, 0.1f, 2);
        AddFloat(static_cast<uint32_t>(SubsurfaceSetting::RADIUS_MINIMUM), "Radius minimum", surface.sssRadiusMinimum, 0.001f, 10.0f, 0.01f, 3);
        AddFloat(static_cast<uint32_t>(SubsurfaceSetting::RADIUS_MAXIMUM), "Radius maximum", surface.sssRadiusMaximum, 0.001f, 10.0f, 0.01f, 3);
        AddFloat(static_cast<uint32_t>(SubsurfaceSetting::INTENSITY), "Intensity", surface.sssIntensity, 0.0f, 20.0f, 0.05f, 2);
        AddFloat(static_cast<uint32_t>(SubsurfaceSetting::FALLOFF), "Falloff", surface.sssFalloff, 0.0f, 20.0f, 0.1f, 2);
        AddFloat(static_cast<uint32_t>(SubsurfaceSetting::SATURATION), "Saturation", surface.sssSaturation, 0.0f, 20.0f, 0.1f, 2);
    }

    void BuildFogMenu() {
        const Ocean::SurfaceSettings surface = Ocean::GetSettings().surface;

        AddFloat(static_cast<uint32_t>(FogSetting::COLOR_R), "Color R", surface.fogColor.r, 0.0f, 2.0f, 0.0001f, 5);
        AddFloat(static_cast<uint32_t>(FogSetting::COLOR_G), "Color G", surface.fogColor.g, 0.0f, 2.0f, 0.0001f, 5);
        AddFloat(static_cast<uint32_t>(FogSetting::COLOR_B), "Color B", surface.fogColor.b, 0.0f, 2.0f, 0.0001f, 5);
        AddFloat(static_cast<uint32_t>(FogSetting::START_DISTANCE), "Start distance", surface.fogStartDistance, 0.0f, 5000.0f, 5.0f, 1);
        AddFloat(static_cast<uint32_t>(FogSetting::END_DISTANCE), "End distance", surface.fogEndDistance, 0.001f, 10000.0f, 10.0f, 1);
        AddFloat(static_cast<uint32_t>(FogSetting::EXPONENT), "Exponent", surface.fogExponent, 0.001f, 8.0f, 0.05f, 3);
        AddFloat(static_cast<uint32_t>(FogSetting::STRENGTH), "Strength", surface.fogStrength, 0.0f, 10.0f, 0.05f, 2);
    }

    void BuildRefractionMenu() {
        const Ocean::SurfaceCompositeSettings surface = Ocean::GetSettings().composite.surface;

        AddFloat(static_cast<uint32_t>(RefractionSetting::PLANE_HEIGHT_OFFSET), "Plane height offset", surface.planeHeightOffset, -1000.0f, 1000.0f, 5.0f, 1);
        AddFloat(static_cast<uint32_t>(RefractionSetting::DISTORTION_SPEED), "Distortion speed", surface.distortionSpeed, -10.0f, 10.0f, 0.005f, 3);
        AddFloat(static_cast<uint32_t>(RefractionSetting::DISTORTION_STRENGTH), "Distortion strength", surface.distortionStrength, 0.0f, 1.0f, 0.0005f, 4);
        AddFloat(static_cast<uint32_t>(RefractionSetting::DISTORTION_TILING), "Distortion tiling", surface.distortionTiling, 0.0f, 10.0f, 0.01f, 3);
        AddFloat(static_cast<uint32_t>(RefractionSetting::TINT_STRENGTH), "Tint strength", surface.refractionTintStrength, 0.0f, 10.0f, 0.05f, 2);
    }

    void BuildUnderwaterFogMenu() {
        const Ocean::UnderwaterCompositeSettings underwater = Ocean::GetSettings().composite.underwater;

        AddFloat(static_cast<uint32_t>(UnderwaterFogSetting::COLOR_R), "Color R", underwater.rayFogColor.r, 0.0f, 4.0f, 0.01f, 3);
        AddFloat(static_cast<uint32_t>(UnderwaterFogSetting::COLOR_G), "Color G", underwater.rayFogColor.g, 0.0f, 4.0f, 0.01f, 3);
        AddFloat(static_cast<uint32_t>(UnderwaterFogSetting::COLOR_B), "Color B", underwater.rayFogColor.b, 0.0f, 4.0f, 0.01f, 3);
        AddFloat(static_cast<uint32_t>(UnderwaterFogSetting::STRENGTH), "Strength", underwater.rayFogStrength, 0.0f, 1.0f, 0.00025f, 5);
        AddFloat(static_cast<uint32_t>(UnderwaterFogSetting::DARKNESS_CURVE), "Darkness curve", underwater.darknessCurve, 0.001f, 8.0f, 0.05f, 3);
    }

    void BuildUnderwaterDistortionMenu() {
        const Ocean::UnderwaterCompositeSettings underwater = Ocean::GetSettings().composite.underwater;

        AddFloat(static_cast<uint32_t>(UnderwaterDistortionSetting::SPEED), "Speed", underwater.distortionSpeed, -10.0f, 10.0f, 0.005f, 3);
        AddFloat(static_cast<uint32_t>(UnderwaterDistortionSetting::STRENGTH), "Strength", underwater.distortionStrength, 0.0f, 1.0f, 0.0001f, 4);
    }

    void BuildUnderwaterColorMenu() {
        const Ocean::CompositeSettings composite = Ocean::GetSettings().composite;
        const Ocean::UnderwaterCompositeSettings& underwater = composite.underwater;

        AddFloat(static_cast<uint32_t>(UnderwaterColorSetting::TINT_R), "Tint R", composite.underwaterTint.r, 0.0f, 4.0f, 0.01f, 3);
        AddFloat(static_cast<uint32_t>(UnderwaterColorSetting::TINT_G), "Tint G", composite.underwaterTint.g, 0.0f, 4.0f, 0.01f, 3);
        AddFloat(static_cast<uint32_t>(UnderwaterColorSetting::TINT_B), "Tint B", composite.underwaterTint.b, 0.0f, 4.0f, 0.01f, 3);
        AddFloat(static_cast<uint32_t>(UnderwaterColorSetting::DEPTH_TINT_STRENGTH), "Depth tint strength", underwater.depthTintStrength, 0.0f, 10.0f, 0.05f, 2);
        AddFloat(static_cast<uint32_t>(UnderwaterColorSetting::DEPTH_TINT_ORIGINAL_WEIGHT), "Depth original weight", underwater.depthTintOriginalWeight, 0.0f, 1.0f, 0.05f, 2);
        AddFloat(static_cast<uint32_t>(UnderwaterColorSetting::GEOMETRY_COLOR_SQUARED_STRENGTH), "Geometry color squared", underwater.geometryWaterColorSquaredStrength, 0.0f, 20.0f, 0.25f, 2);
        AddFloat(static_cast<uint32_t>(UnderwaterColorSetting::GEOMETRY_COLOR_STRENGTH), "Geometry color", underwater.geometryWaterColorStrength, 0.0f, 20.0f, 0.05f, 2);
        AddFloat(static_cast<uint32_t>(UnderwaterColorSetting::GEOMETRY_TINT_STRENGTH), "Geometry tint", underwater.geometryTintStrength, 0.0f, 1.0f, 0.05f, 2);
        AddFloat(static_cast<uint32_t>(UnderwaterColorSetting::OPEN_WATER_TINT_STRENGTH), "Open water tint", underwater.openWaterTintStrength, 0.0f, 1.0f, 0.01f, 2);
        AddFloat(static_cast<uint32_t>(UnderwaterColorSetting::OPEN_WATER_BRIGHTNESS), "Open water brightness", underwater.openWaterBrightness, 0.0f, 10.0f, 0.05f, 2);
    }

    void BuildBandMenu(int32_t bandIndex) {
        const Ocean::Settings settings = Ocean::GetSettings();
        const Ocean::BandSettings& band = settings.bands[bandIndex];
        const float amplitudeIncrement = bandIndex == 0 ? 0.005f : 0.05f;

        AddFloat(static_cast<uint32_t>(BandSetting::DOMAIN_SIZE), "Domain size", band.domainSize, 0.25f, 1024.0f, 0.25f, 3);
        AddFloat(static_cast<uint32_t>(BandSetting::MINIMUM_WAVELENGTH), "Minimum wavelength", band.minimumWavelength, 0.0f, 1024.0f, 0.015625f, 6);
        AddFloat(static_cast<uint32_t>(BandSetting::MAXIMUM_WAVELENGTH), "Maximum wavelength", band.maximumWavelength, 0.001f, 1024.0f, 0.25f, 3);
        AddFloat(static_cast<uint32_t>(BandSetting::MINIMUM_WAVELENGTH_FADE), "Minimum fade", band.minimumWavelengthFade, 0.0f, 128.0f, 0.015625f, 6);
        AddFloat(static_cast<uint32_t>(BandSetting::MAXIMUM_WAVELENGTH_FADE), "Maximum fade", band.maximumWavelengthFade, 0.0f, 128.0f, 0.015625f, 6);
        AddFloat(static_cast<uint32_t>(BandSetting::WIND_DIRECTION_X), "Wind direction X", band.windDirection.x, -1.0f, 1.0f, 0.05f, 3);
        AddFloat(static_cast<uint32_t>(BandSetting::WIND_DIRECTION_Y), "Wind direction Y", band.windDirection.y, -1.0f, 1.0f, 0.05f, 3);
        AddFloat(static_cast<uint32_t>(BandSetting::AMPLITUDE), "Amplitude", band.amplitude, 0.0f, 10.0f, amplitudeIncrement, 8);
        AddFloat(static_cast<uint32_t>(BandSetting::WIND_ALIGNMENT_EXPONENT), "Wind alignment", band.windAlignmentExponent, 0.0f, 16.0f, 0.25f, 2);
        AddFloat(static_cast<uint32_t>(BandSetting::OPPOSING_WAVES_DAMPING), "Opposing waves", band.opposingWavesDamping, 0.0f, 2.0f, 0.05f, 2);
        AddFloat(static_cast<uint32_t>(BandSetting::SMALL_WAVES_DAMPING), "Small wave damping", band.smallWavesDamping, 0.0f, 0.000001f, 0.0000000001f, 4, true);
        AddUInt(static_cast<uint32_t>(BandSetting::RANDOM_SEED), "Random seed", band.randomSeed, 0, std::numeric_limits<uint32_t>::max(), 1);
    }

    void BuildBand0Menu() {
        BuildBandMenu(0);
    }

    void BuildBand1Menu() {
        BuildBandMenu(1);
    }

    void ApplyOceanEdit(uint32_t id, const Value& value) {
        if (id == static_cast<uint32_t>(Setting::RESET_DEFAULTS)) {
            Ocean::ResetSettings();
            return;
        }
        if (id == static_cast<uint32_t>(Setting::SAVE_TO_DISK)) {
            const bool saved = Ocean::SaveToDisk();
            Debug::BlitQuickDebugMessage(saved ? "Saved " + Ocean::GetFilePath() : "Failed to save ocean config");
            return;
        }
        if (id == static_cast<uint32_t>(Setting::LOAD_FROM_DISK)) {
            const bool loaded = Ocean::LoadFromDisk();
            Debug::BlitQuickDebugMessage(loaded ? "Loaded " + Ocean::GetFilePath() : "Failed to load ocean config");
            return;
        }

        Ocean::Settings settings = Ocean::GetSettings();

        switch (static_cast<Setting>(id)) {
            case Setting::SIMULATE:                    settings.simulate = value.boolValue; break;
            case Setting::ALTERNATE_BAND_UPDATES:      settings.alternateBandUpdates = value.boolValue; break;
            case Setting::DISPLAY_MODE:                settings.displayMode = static_cast<Ocean::DisplayMode>(value.intValue); break;
            case Setting::WIND_SPEED:                  settings.windSpeed = value.floatValue; break;
            case Setting::GRAVITY:                     settings.gravity = value.floatValue; break;
            case Setting::HEIGHT_SCALE:                settings.heightScale = value.floatValue; break;
            case Setting::DISPLACEMENT_SCALE:          settings.displacementScale = value.floatValue; break;
            case Setting::TIME_SCALE:                  settings.simulationTimeScale = value.floatValue; break;
            case Setting::TIME_OFFSET:                 settings.simulationTimeOffset = value.floatValue; break;
            default:                                   return;
        }

        Ocean::SetSettings(settings);
    }

    void ApplyNormalEdit(uint32_t id, const Value& value) {
        Ocean::Settings settings = Ocean::GetSettings();
        Ocean::SurfaceSettings& surface = settings.surface;

        switch (static_cast<NormalSetting>(id)) {
            case NormalSetting::STRENGTH:                surface.normalScale = value.floatValue; break;
            case NormalSetting::CONVERGE_START_DISTANCE: surface.normalConvergeStartDistance = value.floatValue; break;
            case NormalSetting::CONVERGE_END_DISTANCE:   surface.normalConvergeEndDistance = value.floatValue; break;
            case NormalSetting::CONVERGE_MAX_FACTOR:     surface.normalConvergeMaxFactor = value.floatValue; break;
            case NormalSetting::CONVERGE_EXPONENT:       surface.normalConvergeExponent = value.floatValue; break;
            case NormalSetting::SOFTENING:               surface.normalSoftening = value.floatValue; break;
            default:                                     return;
        }

        Ocean::SetSettings(settings);
    }

    void ApplyRippleEdit(uint32_t id, const Value& value) {
        Ocean::Settings settings = Ocean::GetSettings();
        Ocean::SurfaceSettings& surface = settings.surface;

        switch (static_cast<RippleSetting>(id)) {
            case RippleSetting::TILING:             surface.rippleTiling = value.floatValue; break;
            case RippleSetting::STRENGTH:           surface.rippleStrength = value.floatValue; break;
            case RippleSetting::SECOND_LAYER_SCALE: surface.rippleSecondLayerScale = value.floatValue; break;
            case RippleSetting::VELOCITY_0_X:       surface.rippleVelocity0.x = value.floatValue; break;
            case RippleSetting::VELOCITY_0_Y:       surface.rippleVelocity0.y = value.floatValue; break;
            case RippleSetting::VELOCITY_1_X:       surface.rippleVelocity1.x = value.floatValue; break;
            case RippleSetting::VELOCITY_1_Y:       surface.rippleVelocity1.y = value.floatValue; break;
            default:                                return;
        }

        Ocean::SetSettings(settings);
    }

    void ApplyMaterialEdit(uint32_t id, const Value& value) {
        Ocean::Settings settings = Ocean::GetSettings();
        Ocean::SurfaceSettings& surface = settings.surface;

        switch (static_cast<MaterialSetting>(id)) {
            case MaterialSetting::SPECULAR_ANTI_ALIASING: surface.specularAntiAliasing = value.boolValue; break;
            case MaterialSetting::ALBEDO_R:          surface.albedo.r = value.floatValue; break;
            case MaterialSetting::ALBEDO_G:          surface.albedo.g = value.floatValue; break;
            case MaterialSetting::ALBEDO_B:          surface.albedo.b = value.floatValue; break;
            case MaterialSetting::ROUGHNESS:         surface.roughness = value.floatValue; break;
            case MaterialSetting::REFLECTANCE:       surface.reflectance = value.floatValue; break;
            case MaterialSetting::REFLECTION_GAMMA:  surface.reflectionGamma = value.floatValue; break;
            case MaterialSetting::DIFFUSE_STRENGTH:  surface.diffuseStrength = value.floatValue; break;
            default:                                 return;
        }

        Ocean::SetSettings(settings);
    }

    void ApplySubsurfaceEdit(uint32_t id, const Value& value) {
        Ocean::Settings settings = Ocean::GetSettings();
        Ocean::SurfaceSettings& surface = settings.surface;

        switch (static_cast<SubsurfaceSetting>(id)) {
            case SubsurfaceSetting::HEIGHT_RANGE:        surface.sssHeightRange = value.floatValue; break;
            case SubsurfaceSetting::STRENGTH:            surface.sssStrength = value.floatValue; break;
            case SubsurfaceSetting::UNDERWATER_STRENGTH: surface.underwaterSssStrength = value.floatValue; break;
            case SubsurfaceSetting::RADIUS_MINIMUM:      surface.sssRadiusMinimum = value.floatValue; break;
            case SubsurfaceSetting::RADIUS_MAXIMUM:      surface.sssRadiusMaximum = value.floatValue; break;
            case SubsurfaceSetting::INTENSITY:           surface.sssIntensity = value.floatValue; break;
            case SubsurfaceSetting::FALLOFF:             surface.sssFalloff = value.floatValue; break;
            case SubsurfaceSetting::SATURATION:          surface.sssSaturation = value.floatValue; break;
            default:                                     return;
        }

        Ocean::SetSettings(settings);
    }

    void ApplyFogEdit(uint32_t id, const Value& value) {
        Ocean::Settings settings = Ocean::GetSettings();
        Ocean::SurfaceSettings& surface = settings.surface;

        switch (static_cast<FogSetting>(id)) {
            case FogSetting::COLOR_R:        surface.fogColor.r = value.floatValue; break;
            case FogSetting::COLOR_G:        surface.fogColor.g = value.floatValue; break;
            case FogSetting::COLOR_B:        surface.fogColor.b = value.floatValue; break;
            case FogSetting::START_DISTANCE: surface.fogStartDistance = value.floatValue; break;
            case FogSetting::END_DISTANCE:   surface.fogEndDistance = value.floatValue; break;
            case FogSetting::EXPONENT:       surface.fogExponent = value.floatValue; break;
            case FogSetting::STRENGTH:       surface.fogStrength = value.floatValue; break;
            default:                         return;
        }

        Ocean::SetSettings(settings);
    }

    void ApplyRefractionEdit(uint32_t id, const Value& value) {
        Ocean::Settings settings = Ocean::GetSettings();
        Ocean::SurfaceCompositeSettings& surface = settings.composite.surface;

        switch (static_cast<RefractionSetting>(id)) {
            case RefractionSetting::PLANE_HEIGHT_OFFSET:  surface.planeHeightOffset = value.floatValue; break;
            case RefractionSetting::DISTORTION_SPEED:     surface.distortionSpeed = value.floatValue; break;
            case RefractionSetting::DISTORTION_STRENGTH:  surface.distortionStrength = value.floatValue; break;
            case RefractionSetting::DISTORTION_TILING:    surface.distortionTiling = value.floatValue; break;
            case RefractionSetting::TINT_STRENGTH:        surface.refractionTintStrength = value.floatValue; break;
            default:                                      return;
        }

        Ocean::SetSettings(settings);
    }

    void ApplyUnderwaterFogEdit(uint32_t id, const Value& value) {
        Ocean::Settings settings = Ocean::GetSettings();
        Ocean::UnderwaterCompositeSettings& underwater = settings.composite.underwater;

        switch (static_cast<UnderwaterFogSetting>(id)) {
            case UnderwaterFogSetting::COLOR_R:        underwater.rayFogColor.r = value.floatValue; break;
            case UnderwaterFogSetting::COLOR_G:        underwater.rayFogColor.g = value.floatValue; break;
            case UnderwaterFogSetting::COLOR_B:        underwater.rayFogColor.b = value.floatValue; break;
            case UnderwaterFogSetting::STRENGTH:       underwater.rayFogStrength = value.floatValue; break;
            case UnderwaterFogSetting::DARKNESS_CURVE: underwater.darknessCurve = value.floatValue; break;
            default:                                   return;
        }

        Ocean::SetSettings(settings);
    }

    void ApplyUnderwaterDistortionEdit(uint32_t id, const Value& value) {
        Ocean::Settings settings = Ocean::GetSettings();
        Ocean::UnderwaterCompositeSettings& underwater = settings.composite.underwater;

        switch (static_cast<UnderwaterDistortionSetting>(id)) {
            case UnderwaterDistortionSetting::SPEED:    underwater.distortionSpeed = value.floatValue; break;
            case UnderwaterDistortionSetting::STRENGTH: underwater.distortionStrength = value.floatValue; break;
            default:                                    return;
        }

        Ocean::SetSettings(settings);
    }

    void ApplyUnderwaterColorEdit(uint32_t id, const Value& value) {
        Ocean::Settings settings = Ocean::GetSettings();
        Ocean::CompositeSettings& composite = settings.composite;
        Ocean::UnderwaterCompositeSettings& underwater = composite.underwater;

        switch (static_cast<UnderwaterColorSetting>(id)) {
            case UnderwaterColorSetting::TINT_R:                           composite.underwaterTint.r = value.floatValue; break;
            case UnderwaterColorSetting::TINT_G:                           composite.underwaterTint.g = value.floatValue; break;
            case UnderwaterColorSetting::TINT_B:                           composite.underwaterTint.b = value.floatValue; break;
            case UnderwaterColorSetting::DEPTH_TINT_STRENGTH:              underwater.depthTintStrength = value.floatValue; break;
            case UnderwaterColorSetting::DEPTH_TINT_ORIGINAL_WEIGHT:       underwater.depthTintOriginalWeight = value.floatValue; break;
            case UnderwaterColorSetting::GEOMETRY_COLOR_SQUARED_STRENGTH:  underwater.geometryWaterColorSquaredStrength = value.floatValue; break;
            case UnderwaterColorSetting::GEOMETRY_COLOR_STRENGTH:          underwater.geometryWaterColorStrength = value.floatValue; break;
            case UnderwaterColorSetting::GEOMETRY_TINT_STRENGTH:           underwater.geometryTintStrength = value.floatValue; break;
            case UnderwaterColorSetting::OPEN_WATER_TINT_STRENGTH:         underwater.openWaterTintStrength = value.floatValue; break;
            case UnderwaterColorSetting::OPEN_WATER_BRIGHTNESS:            underwater.openWaterBrightness = value.floatValue; break;
            default:                                                        return;
        }

        Ocean::SetSettings(settings);
    }

    void ApplyBandEdit(int32_t bandIndex, uint32_t id, const Value& value) {
        Ocean::Settings settings = Ocean::GetSettings();
        Ocean::BandSettings& band = settings.bands[bandIndex];

        switch (static_cast<BandSetting>(id)) {
            case BandSetting::DOMAIN_SIZE:             band.domainSize = value.floatValue; break;
            case BandSetting::MINIMUM_WAVELENGTH:      band.minimumWavelength = value.floatValue; break;
            case BandSetting::MAXIMUM_WAVELENGTH:      band.maximumWavelength = value.floatValue; break;
            case BandSetting::MINIMUM_WAVELENGTH_FADE: band.minimumWavelengthFade = value.floatValue; break;
            case BandSetting::MAXIMUM_WAVELENGTH_FADE: band.maximumWavelengthFade = value.floatValue; break;
            case BandSetting::WIND_DIRECTION_X:        band.windDirection.x = value.floatValue; break;
            case BandSetting::WIND_DIRECTION_Y:        band.windDirection.y = value.floatValue; break;
            case BandSetting::AMPLITUDE:               band.amplitude = value.floatValue; break;
            case BandSetting::WIND_ALIGNMENT_EXPONENT: band.windAlignmentExponent = value.floatValue; break;
            case BandSetting::OPPOSING_WAVES_DAMPING:  band.opposingWavesDamping = value.floatValue; break;
            case BandSetting::SMALL_WAVES_DAMPING:     band.smallWavesDamping = value.floatValue; break;
            case BandSetting::RANDOM_SEED:             band.randomSeed = value.uintValue; break;
            default:                                   return;
        }

        Ocean::SetSettings(settings);
    }

    void ApplyBand0Edit(uint32_t id, const Value& value) {
        ApplyBandEdit(0, id, value);
    }

    void ApplyBand1Edit(uint32_t id, const Value& value) {
        ApplyBandEdit(1, id, value);
    }
}
