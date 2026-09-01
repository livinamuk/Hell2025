#include "Debug_menu.h"

#include "Hell/Backend/BackEnd.h"
#include "Hell/Common/Enum.h"

#include "Unloved/Render/Renderer.h"

#include <cstdint>
#include <limits>

namespace Debug::Menu::Renderer {

    enum struct Setting : uint32_t {
        DDGI,
        DDGI_REFLECTIONS,
        INDIRECT_SPECULAR,
        FXAA,
        TAA,
        ADDITIONAL_SETTINGS,
        DEBUG_SETTINGS,
        RESET_DEFAULTS
    };

    enum struct AdditionalSetting : uint32_t {
        STATIC_SHADOW_MAP_CACHING,
        VULKAN_DIRECT_POINT_SHADOW_MODE,
        TAA_JITTER_SCALE,
        EMISSIVE_STRENGTH,
        IRRADIANCE_DAMPENING,
        INDIRECT_SPECULAR_RAYS_PER_QUAD,
        INDIRECT_SPECULAR_FACTOR,
        INDIRECT_SPECULAR_ROUGHNESS_DAMPENING,
    };

    enum struct DebugSetting : uint32_t {
        DRAW_POINT_CLOUD,
        DRAW_POINT_CLOUD_GRID,
        DRAW_IRRADIANCE_PROBES,
        DRAW_NAV_MESH,
        DRAW_RAGDOLLS,
        DEBUG_VIEW,
    };

    PageId g_homepageId = ROOT_PAGE_ID;
    PageId g_additionalSettingsId = ROOT_PAGE_ID;
    PageId g_debugSettingsId = ROOT_PAGE_ID;

    void BuildMainMenu();
    void BuildAdditionalSettingsMenu();
    void BuildDebugSettingsMenu();

    void ApplyEdit(uint32_t id, const Value& value);
    void ApplyAdditionalSettingsEdit(uint32_t id, const Value& value);
    void ApplyDebugSettingsEdit(uint32_t id, const Value& value);

    void RegisterMenu() {
        g_homepageId = RegisterRootPage("Renderer", "RENDERER", BuildMainMenu, ApplyEdit);
        g_additionalSettingsId = RegisterPage("ADDITIONAL SETTINGS", g_homepageId, BuildAdditionalSettingsMenu, ApplyAdditionalSettingsEdit);
        g_debugSettingsId = RegisterPage("DEBUG SETTINGS", g_debugSettingsId, BuildDebugSettingsMenu, ApplyDebugSettingsEdit);
    }

    Registrar g_registrar(RegisterMenu);

    void BuildMainMenu() {
        const RendererSettings settings = Unloved::Renderer::GetCurrentRendererSettings();

        AddBool(static_cast<uint32_t>(Setting::DDGI), "DDGI", settings.enableDDGI);
        AddBool(static_cast<uint32_t>(Setting::DDGI_REFLECTIONS), "DDGI Reflections", settings.enableDDGIReflections);
        AddBool(static_cast<uint32_t>(Setting::INDIRECT_SPECULAR), "Indirect Specular", settings.enableIndirectSpecular);
        AddBool(static_cast<uint32_t>(Setting::FXAA), "FXAA", settings.enableFXAA);
        AddBool(static_cast<uint32_t>(Setting::TAA), "TAA", settings.enableTAA);

        AddLineBreak();

        AddSubMenu(static_cast<uint32_t>(Setting::ADDITIONAL_SETTINGS), "Additional Settings", g_additionalSettingsId);
        AddSubMenu(static_cast<uint32_t>(Setting::DEBUG_SETTINGS), "Debug Settings", g_debugSettingsId);

        AddLineBreak();

        AddAction(static_cast<uint32_t>(Setting::RESET_DEFAULTS), "Reset defaults");
    }

    void BuildAdditionalSettingsMenu() {
        const RendererSettings settings = Unloved::Renderer::GetCurrentRendererSettings();

        AddBool(static_cast<uint32_t>(AdditionalSetting::STATIC_SHADOW_MAP_CACHING), "Static Shadow Map Caching", settings.enableStaticShadowMapCaching);
        if (Hell::BackEnd::GetAPI() == API::OPENGL) AddOpenGLFunctionTiming("PointLightShadowPass");
        if (Hell::BackEnd::GetAPI() == API::VULKAN) AddVulkanFunctionTiming("PointLightShadowPass");
        AddEnum(static_cast<uint32_t>(AdditionalSetting::VULKAN_DIRECT_POINT_SHADOW_MODE), "Vulkan Direct Point Shadows", static_cast<int32_t>(settings.directPointShadowMode), { "Shadow Maps", "Ray Queries" });
        AddLineBreak();
        AddInt(static_cast<uint32_t>(AdditionalSetting::TAA_JITTER_SCALE), "TAA Jitter Scale", settings.taaJitterScale, 1, 3, 1);
        AddFloat(static_cast<uint32_t>(AdditionalSetting::EMISSIVE_STRENGTH), "Emissive Strength", settings.emissiveStrength, 0.0f, 10.0f, 0.1f, 2);
        AddFloat(static_cast<uint32_t>(AdditionalSetting::IRRADIANCE_DAMPENING), "Irradiance Dampening", settings.irradianceDampening, 0.0f, 1.0f, 0.0025f, 4);
        AddEnum(static_cast<uint32_t>(AdditionalSetting::INDIRECT_SPECULAR_RAYS_PER_QUAD), "Indirect Specular Rays Per Quad", static_cast<int32_t>(settings.indirectSpecularRaysPerQuad), { "1", "2", "4" });
        AddFloat(static_cast<uint32_t>(AdditionalSetting::INDIRECT_SPECULAR_FACTOR), "Indirect Specular Factor", settings.indirectSpecularFactor, 0.0f, 5.0f, 0.1f, 2);
        AddFloat(static_cast<uint32_t>(AdditionalSetting::INDIRECT_SPECULAR_ROUGHNESS_DAMPENING), "Indirect Specular Roughness Dampening", settings.indirectSpecularRoughnessDampening, 0.0f, 1.0f, 0.1f, 2);
}

    void BuildDebugSettingsMenu() {
        const RendererSettings settings = Unloved::Renderer::GetCurrentRendererSettings();

        AddEnum(static_cast<uint32_t>(DebugSetting::DEBUG_VIEW), "Debug View", static_cast<int32_t>(settings.rendererOverrideState), Hell::Enum::GetNames<RendererOverrideState>(), Hell::Enum::GetCount<RendererOverrideState>() - 1);
        AddBool(static_cast<uint32_t>(DebugSetting::DRAW_POINT_CLOUD), "Draw Point Cloud", settings.debugDrawPointCloud);
        AddBool(static_cast<uint32_t>(DebugSetting::DRAW_POINT_CLOUD_GRID), "Draw Point Cloud Grid", settings.debugDrawPointCloudGrid);
        AddBool(static_cast<uint32_t>(DebugSetting::DRAW_IRRADIANCE_PROBES), "Draw Irradiance Probes", settings.debugDrawIrradianceProbes);
        AddBool(static_cast<uint32_t>(DebugSetting::DRAW_NAV_MESH), "Draw NAV Mesh", settings.debugDrawNavMesh);
        AddBool(static_cast<uint32_t>(DebugSetting::DRAW_RAGDOLLS), "Draw Ragdolls", settings.debugDrawRagdolls);
    }


    void ApplyEdit(uint32_t id, const Value& value) {
        if (id == static_cast<uint32_t>(Setting::RESET_DEFAULTS)) {
            Unloved::Renderer::ResetCurrentRendererSettings();
            return;
        }

        RendererSettings settings = Unloved::Renderer::GetCurrentRendererSettings();

        switch (static_cast<Setting>(id)) {
            case Setting::DDGI:             settings.enableDDGI = value.boolValue;              break;
            case Setting::DDGI_REFLECTIONS: settings.enableDDGIReflections = value.boolValue;   break;
            case Setting::INDIRECT_SPECULAR: settings.enableIndirectSpecular = value.boolValue; break;
            case Setting::FXAA:             settings.enableFXAA = value.boolValue;              break;
            case Setting::TAA:              settings.enableTAA = value.boolValue;               break;
            break;
            default: return;
        }

        Unloved::Renderer::SetCurrentRendererSettings(settings);
    }

    void ApplyAdditionalSettingsEdit(uint32_t id, const Value& value) {
        RendererSettings settings = Unloved::Renderer::GetCurrentRendererSettings();

        switch (static_cast<AdditionalSetting>(id)) {
            case AdditionalSetting::STATIC_SHADOW_MAP_CACHING:             settings.enableStaticShadowMapCaching = value.boolValue;                                      break;
            case AdditionalSetting::VULKAN_DIRECT_POINT_SHADOW_MODE:       settings.directPointShadowMode = static_cast<DirectPointShadowMode>(value.intValue);           break;
            case AdditionalSetting::TAA_JITTER_SCALE:                      settings.taaJitterScale = value.intValue;                                                        break;
            case AdditionalSetting::EMISSIVE_STRENGTH:                     settings.emissiveStrength = value.floatValue;                                                   break;
            case AdditionalSetting::IRRADIANCE_DAMPENING:                  settings.irradianceDampening = value.floatValue;                                                break;
            case AdditionalSetting::INDIRECT_SPECULAR_RAYS_PER_QUAD:       settings.indirectSpecularRaysPerQuad = static_cast<IndirectSpecularRaysPerQuad>(value.intValue); break;
            case AdditionalSetting::INDIRECT_SPECULAR_FACTOR:              settings.indirectSpecularFactor = value.floatValue;                                              break;
            case AdditionalSetting::INDIRECT_SPECULAR_ROUGHNESS_DAMPENING: settings.indirectSpecularRoughnessDampening = value.floatValue;                                  break;
            default: return;
        }

        Unloved::Renderer::SetCurrentRendererSettings(settings);
    }

    void ApplyDebugSettingsEdit(uint32_t id, const Value& value) {
        RendererSettings settings = Unloved::Renderer::GetCurrentRendererSettings();

        switch (static_cast<DebugSetting>(id)) {
            case DebugSetting::DEBUG_VIEW:             settings.rendererOverrideState = static_cast<RendererOverrideState>(value.intValue); break;
            case DebugSetting::DRAW_POINT_CLOUD:       settings.debugDrawPointCloud = value.boolValue;                                      break;
            case DebugSetting::DRAW_POINT_CLOUD_GRID:  settings.debugDrawPointCloudGrid = value.boolValue;                                  break;
            case DebugSetting::DRAW_IRRADIANCE_PROBES: settings.debugDrawIrradianceProbes = value.boolValue;                                break;
            case DebugSetting::DRAW_NAV_MESH:          settings.debugDrawNavMesh = value.boolValue;                                         break;
            case DebugSetting::DRAW_RAGDOLLS:          settings.debugDrawRagdolls = value.boolValue;                                        break;
            break;
            default: return;
        }

        Unloved::Renderer::SetCurrentRendererSettings(settings);
    }
    
}
