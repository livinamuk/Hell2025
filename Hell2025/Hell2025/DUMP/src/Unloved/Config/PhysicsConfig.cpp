#include "PhysicsConfig.h"

#include "Hell/Logging.h"
#include "Hell/Physics/Physics.h"
#include "Hell/Serialization/Json.h"

#include <algorithm>
#include <filesystem>
#include <system_error>

namespace Config::Physics {
    namespace {
        const std::string CONFIG_FILE_PATH = "res/config/physics.json";
        Settings g_settings;

        void Sanitize(Settings& settings) {
            settings.substeps = std::clamp(settings.substeps, 1u, 16u);
            settings.positionIterations = std::clamp(settings.positionIterations, 1u, 255u);
            settings.velocityIterations = std::clamp(settings.velocityIterations, 1u, 255u);
            settings.bulletImpactImpulse = std::clamp(settings.bulletImpactImpulse, 0.0f, 100.0f);
            settings.shotgunPelletImpactImpulse = std::clamp(settings.shotgunPelletImpactImpulse, 0.0f, 100.0f);
            settings.meleeImpactImpulse = std::clamp(settings.meleeImpactImpulse, 0.0f, 100.0f);
            settings.ragdollImpactTranslationScale = std::clamp(settings.ragdollImpactTranslationScale, 0.0f, 10.0f);
            settings.ragdollImpactRotationScale = std::clamp(settings.ragdollImpactRotationScale, 0.0f, 10.0f);
            settings.defaultMaterialStaticFriction = std::clamp(settings.defaultMaterialStaticFriction, 0.0f, 10.0f);
            settings.defaultMaterialDynamicFriction = std::clamp(settings.defaultMaterialDynamicFriction, 0.0f, 10.0f);
            settings.defaultMaterialRestitution = std::clamp(settings.defaultMaterialRestitution, 0.0f, 1.0f);
        }

        template<typename T>
        void ReadIfPresent(const nlohmann::json& json, const char* name, T& value) {
            const auto it = json.find(name);
            if (it != json.end() && !it->is_null()) it->get_to(value);
        }
    }

    const Settings& GetSettings() {
        return g_settings;
    }

    void SetSettings(const Settings& settings) {
        g_settings = settings;
        Sanitize(g_settings);
        Hell::Physics::SetDefaultMaterialProperties(
            g_settings.defaultMaterialStaticFriction,
            g_settings.defaultMaterialDynamicFriction,
            g_settings.defaultMaterialRestitution
        );
    }

    void ResetSettings() {
        SetSettings(Settings{});
    }

    bool LoadFromDisk() {
        nlohmann::json json;
        if (!Hell::Json::LoadFromFile(json, CONFIG_FILE_PATH)) return false;

        Settings loadedSettings;
        try {
            ReadIfPresent(json, "gravityEnabled", loadedSettings.gravityEnabled);
            ReadIfPresent(json, "substeps", loadedSettings.substeps);
            ReadIfPresent(json, "positionIterations", loadedSettings.positionIterations);
            ReadIfPresent(json, "velocityIterations", loadedSettings.velocityIterations);
            ReadIfPresent(json, "bulletImpactImpulse", loadedSettings.bulletImpactImpulse);
            ReadIfPresent(json, "shotgunPelletImpactImpulse", loadedSettings.shotgunPelletImpactImpulse);
            ReadIfPresent(json, "meleeImpactImpulse", loadedSettings.meleeImpactImpulse);
            ReadIfPresent(json, "ragdollImpactTranslationScale", loadedSettings.ragdollImpactTranslationScale);
            ReadIfPresent(json, "ragdollImpactRotationScale", loadedSettings.ragdollImpactRotationScale);
            ReadIfPresent(json, "defaultMaterialStaticFriction", loadedSettings.defaultMaterialStaticFriction);
            ReadIfPresent(json, "defaultMaterialDynamicFriction", loadedSettings.defaultMaterialDynamicFriction);
            ReadIfPresent(json, "defaultMaterialRestitution", loadedSettings.defaultMaterialRestitution);
        }
        catch (const nlohmann::json::exception& e) {
            Logging::Error() << "Config::Physics::LoadFromDisk() failed to read '" << CONFIG_FILE_PATH << "': " << e.what() << "\n";
            return false;
        }

        SetSettings(loadedSettings);
        Logging::Debug() << "Loaded " << CONFIG_FILE_PATH << "\n";
        return true;
    }

    bool SaveToDisk() {
        std::error_code errorCode;
        std::filesystem::create_directories(std::filesystem::path(CONFIG_FILE_PATH).parent_path(), errorCode);
        if (errorCode) {
            Logging::Error() << "Config::Physics::SaveToDisk() failed to create config directory: " << errorCode.message() << "\n";
            return false;
        }

        const nlohmann::json json = {
            { "gravityEnabled", g_settings.gravityEnabled },
            { "substeps", g_settings.substeps },
            { "positionIterations", g_settings.positionIterations },
            { "velocityIterations", g_settings.velocityIterations },
            { "bulletImpactImpulse", g_settings.bulletImpactImpulse },
            { "shotgunPelletImpactImpulse", g_settings.shotgunPelletImpactImpulse },
            { "meleeImpactImpulse", g_settings.meleeImpactImpulse },
            { "ragdollImpactTranslationScale", g_settings.ragdollImpactTranslationScale },
            { "ragdollImpactRotationScale", g_settings.ragdollImpactRotationScale },
            { "defaultMaterialStaticFriction", g_settings.defaultMaterialStaticFriction },
            { "defaultMaterialDynamicFriction", g_settings.defaultMaterialDynamicFriction },
            { "defaultMaterialRestitution", g_settings.defaultMaterialRestitution }
        };

        return Hell::Json::SaveToFile(json, CONFIG_FILE_PATH);
    }

    const std::string& GetFilePath() {
        return CONFIG_FILE_PATH;
    }
}
