#include "FlashlightConfig.h"

#include "Hell/Logging.h"
#include "Hell/Serialization/Json.h"

#include <algorithm>
#include <filesystem>
#include <system_error>

namespace Config::Flashlight {
    namespace {
        const std::string CONFIG_FILE_PATH = "res/config/flashlight.json";
        Settings g_settings;

        void Sanitize(Settings& settings) {
            if (settings.iesProfile.empty()) settings.iesProfile = Settings{}.iesProfile;
            settings.range = std::clamp(settings.range, 1.0f, 100.0f);
            settings.falloffExponent = std::clamp(settings.falloffExponent, 0.01f, 8.0f);
            settings.brightness = std::clamp(settings.brightness, 0.0f, 10.0f);
            settings.color = glm::clamp(settings.color, glm::vec3(0.0f), glm::vec3(1.0f));
            settings.iesConeScale = std::clamp(settings.iesConeScale, 0.1f, 1.2f);
            settings.iesInnerAngle = std::clamp(settings.iesInnerAngle, 0.0f, 89.0f);
            settings.iesOuterAngle = std::clamp(settings.iesOuterAngle, 0.0f, 89.0f);
            settings.iesContrast = std::clamp(settings.iesContrast, 0.1f, 8.0f);
            settings.centerSpotRange = std::clamp(settings.centerSpotRange, 0.1f, 100.0f);
            settings.centerSpotFalloffExponent = std::clamp(settings.centerSpotFalloffExponent, 0.01f, 8.0f);
            settings.centerSpotBrightness = std::clamp(settings.centerSpotBrightness, 0.0f, 10.0f);
            settings.centerSpotInnerAngle = std::clamp(settings.centerSpotInnerAngle, 0.0f, 89.0f);
            settings.centerSpotOuterAngle = std::clamp(settings.centerSpotOuterAngle, 0.0f, 89.0f);
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
    }

    void ResetSettings() {
        g_settings = Settings{};
    }

    bool LoadFromDisk() {
        nlohmann::json json;
        if (!Hell::Json::LoadFromFile(json, CONFIG_FILE_PATH)) return false;

        Settings loadedSettings;
        try {
            ReadIfPresent(json, "iesProfile", loadedSettings.iesProfile);
            ReadIfPresent(json, "range", loadedSettings.range);
            ReadIfPresent(json, "falloffExponent", loadedSettings.falloffExponent);
            ReadIfPresent(json, "brightness", loadedSettings.brightness);

            const auto colorIt = json.find("color");
            if (colorIt != json.end() && !colorIt->is_null()) {
                loadedSettings.color.r = colorIt->at(0).get<float>();
                loadedSettings.color.g = colorIt->at(1).get<float>();
                loadedSettings.color.b = colorIt->at(2).get<float>();
            }

            const auto iesIt = json.find("ies");
            if (iesIt != json.end() && iesIt->is_object()) {
                ReadIfPresent(*iesIt, "enabled", loadedSettings.iesEnabled);
                ReadIfPresent(*iesIt, "coneScale", loadedSettings.iesConeScale);
                ReadIfPresent(*iesIt, "innerAngle", loadedSettings.iesInnerAngle);
                ReadIfPresent(*iesIt, "outerAngle", loadedSettings.iesOuterAngle);
                ReadIfPresent(*iesIt, "contrast", loadedSettings.iesContrast);
            }

            const auto centerSpotIt = json.find("centerSpot");
            if (centerSpotIt != json.end() && centerSpotIt->is_object()) {
                ReadIfPresent(*centerSpotIt, "enabled", loadedSettings.centerSpotEnabled);
                ReadIfPresent(*centerSpotIt, "range", loadedSettings.centerSpotRange);
                ReadIfPresent(*centerSpotIt, "falloffExponent", loadedSettings.centerSpotFalloffExponent);
                ReadIfPresent(*centerSpotIt, "brightness", loadedSettings.centerSpotBrightness);
                ReadIfPresent(*centerSpotIt, "innerAngle", loadedSettings.centerSpotInnerAngle);
                ReadIfPresent(*centerSpotIt, "outerAngle", loadedSettings.centerSpotOuterAngle);
            }
        }
        catch (const nlohmann::json::exception& e) {
            Logging::Error() << "Config::Flashlight::LoadFromDisk() failed to read '" << CONFIG_FILE_PATH << "': " << e.what() << "\n";
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
            Logging::Error() << "Config::Flashlight::SaveToDisk() failed to create config directory: " << errorCode.message() << "\n";
            return false;
        }

        const nlohmann::json json = {
            { "iesProfile", g_settings.iesProfile },
            { "range", g_settings.range },
            { "falloffExponent", g_settings.falloffExponent },
            { "brightness", g_settings.brightness },
            { "color", { g_settings.color.r, g_settings.color.g, g_settings.color.b } },
            { "ies", {
                { "enabled", g_settings.iesEnabled },
                { "coneScale", g_settings.iesConeScale },
                { "innerAngle", g_settings.iesInnerAngle },
                { "outerAngle", g_settings.iesOuterAngle },
                { "contrast", g_settings.iesContrast }
            } },
            { "centerSpot", {
                { "enabled", g_settings.centerSpotEnabled },
                { "range", g_settings.centerSpotRange },
                { "falloffExponent", g_settings.centerSpotFalloffExponent },
                { "brightness", g_settings.centerSpotBrightness },
                { "innerAngle", g_settings.centerSpotInnerAngle },
                { "outerAngle", g_settings.centerSpotOuterAngle }
            } }
        };

        return Hell::Json::SaveToFile(json, CONFIG_FILE_PATH);
    }

    const std::string& GetFilePath() {
        return CONFIG_FILE_PATH;
    }
}
