#include "Config.h"
#include "FlashlightConfig.h"
#include "PhysicsConfig.h"

#include "Hell/Logging.h"
#include "Hell/Serialization/Json.h"

#include <algorithm>
#include <filesystem>
#include <system_error>

namespace Config {
	Resolutions g_resolutions;
	float g_nearPlane = 0.005f;
	float g_farPlane = 256.00f;

    void Init() {
        g_resolutions.gBuffer = { 1920, 1080 };
        g_resolutions.gBufferHalfRes = g_resolutions.gBuffer / 2;
        g_resolutions.finalImage = { 1920 / 2, 1080 / 2 };
        g_resolutions.ui = { 1920, 1080 };
        g_resolutions.hair = { 1920 / 2, 1080 / 2 };
        Christmas::LoadFromDisk();
        Flashlight::LoadFromDisk();
        Fog::LoadFromDisk();
        Grass::LoadFromDisk();
        Moonlight::LoadFromDisk();
        Physics::LoadFromDisk();
    }

    const Resolutions& GetResolutions() {
        return g_resolutions;
    }

    const float GetNearPlane() {
        return g_nearPlane;
    }

    const float GetFarPlane() {
        return g_farPlane;
    }
}

namespace Config::Christmas {
    namespace {
        const std::string CONFIG_FILE_PATH = "res/config/christmas.json";
        Settings g_settings;

        void Sanitize(Settings& settings) {
            settings.lightRadius = std::clamp(settings.lightRadius, 0.0f, 5.0f);
            settings.lightStrength = std::clamp(settings.lightStrength, 0.0f, 5.0f);
        }
    }

    const Settings& GetSettings() {
        return g_settings;
    }

    void SetSettings(const Settings& settings) {
        g_settings = settings;
        Sanitize(g_settings);
    }

    bool LoadFromDisk() {
        nlohmann::json json;
        if (!Hell::Json::LoadFromFile(json, CONFIG_FILE_PATH)) return false;

        Settings loadedSettings;
        try {
            const auto lightRadius = json.find("lightRadius");
            if (lightRadius != json.end() && !lightRadius->is_null()) lightRadius->get_to(loadedSettings.lightRadius);

            const auto lightStrength = json.find("lightStrength");
            if (lightStrength != json.end() && !lightStrength->is_null()) lightStrength->get_to(loadedSettings.lightStrength);
        }
        catch (const nlohmann::json::exception& e) {
            Logging::Error() << "Config::Christmas::LoadFromDisk() failed to read '" << CONFIG_FILE_PATH << "': " << e.what() << "\n";
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
            Logging::Error() << "Config::Christmas::SaveToDisk() failed to create config directory: " << errorCode.message() << "\n";
            return false;
        }

        const nlohmann::json json = {
            { "lightRadius", g_settings.lightRadius },
            { "lightStrength", g_settings.lightStrength }
        };
        return Hell::Json::SaveToFile(json, CONFIG_FILE_PATH);
    }

    const std::string& GetFilePath() {
        return CONFIG_FILE_PATH;
    }
}

namespace Config::Grass {
    namespace {
        const std::string CONFIG_FILE_PATH = "res/config/grass.json";
        Settings g_settings;

        void Sanitize(Settings& settings) {
            settings.segmentCount = std::clamp(settings.segmentCount, 1, 8);
            settings.curveAmount = std::clamp(settings.curveAmount, 0.0f, 0.5f);
            settings.spacing = std::clamp(settings.spacing, 0.0185185185185185f, 0.1f);
            settings.minCullDistance = std::clamp(settings.minCullDistance, 0.0f, 29.99f);
            settings.maxCullDistance = std::clamp(settings.maxCullDistance, settings.minCullDistance + 0.01f, 30.0f);
            settings.cullExponent = std::clamp(settings.cullExponent, 0.01f, 32.0f);
            settings.bladeHeight = std::clamp(settings.bladeHeight, 0.01f, 0.5f);
            settings.bladeWidth = std::clamp(settings.bladeWidth, 0.0001f, 0.02f);
            settings.color1 = glm::clamp(settings.color1, glm::vec3(0.0f), glm::vec3(1.0f));
            settings.color2 = glm::clamp(settings.color2, glm::vec3(0.0f), glm::vec3(1.0f));
            settings.color1DarknessFactor = std::clamp(settings.color1DarknessFactor, 0.0f, 1.0f);
            settings.color2DarknessFactor = std::clamp(settings.color2DarknessFactor, 0.0f, 1.0f);
            settings.noiseSquareMultiplier = std::clamp(settings.noiseSquareMultiplier, 0.0f, 8.0f);
            settings.noiseMixMultiplier = std::clamp(settings.noiseMixMultiplier, 0.0f, 1.0f);
            settings.roughness = std::clamp(settings.roughness, 0.0f, 1.0f);
            settings.subSurfaceFactor = std::clamp(settings.subSurfaceFactor, 0.0f, 0.98f);
            settings.normalUpBlend = std::clamp(settings.normalUpBlend, 0.0f, 1.0f);
            settings.normalBlendStartDistance = std::clamp(settings.normalBlendStartDistance, 0.0f, 255.99f);
            settings.normalBlendEndDistance = std::clamp(settings.normalBlendEndDistance, settings.normalBlendStartDistance + 0.01f, 256.0f);
            settings.diffuseWrap = std::clamp(settings.diffuseWrap, 0.0f, 1.0f);
            settings.transmissionPower = std::clamp(settings.transmissionPower, 0.25f, 32.0f);
            settings.specularStrength = std::clamp(settings.specularStrength, 0.0f, 1.0f);
        }
    }

    const Settings& GetSettings() {
        return g_settings;
    }

    void SetSettings(const Settings& settings) {
        g_settings = settings;
        Sanitize(g_settings);
    }

    bool LoadFromDisk() {
        nlohmann::json json;
        if (!Hell::Json::LoadFromFile(json, CONFIG_FILE_PATH)) return false;

        Settings loadedSettings;
        try {
            const auto segmentCount = json.find("segmentCount");
            if (segmentCount != json.end() && !segmentCount->is_null()) segmentCount->get_to(loadedSettings.segmentCount);

            const auto curveAmount = json.find("curveAmount");
            if (curveAmount != json.end() && !curveAmount->is_null()) curveAmount->get_to(loadedSettings.curveAmount);

            const auto spacing = json.find("spacing");
            if (spacing != json.end() && !spacing->is_null()) spacing->get_to(loadedSettings.spacing);

            const auto minCullDistance = json.find("minCullDistance");
            if (minCullDistance != json.end() && !minCullDistance->is_null()) minCullDistance->get_to(loadedSettings.minCullDistance);

            const auto maxCullDistance = json.find("maxCullDistance");
            if (maxCullDistance != json.end() && !maxCullDistance->is_null()) maxCullDistance->get_to(loadedSettings.maxCullDistance);

            const auto cullExponent = json.find("cullExponent");
            if (cullExponent != json.end() && !cullExponent->is_null()) cullExponent->get_to(loadedSettings.cullExponent);

            const auto bladeHeight = json.find("bladeHeight");
            if (bladeHeight != json.end() && !bladeHeight->is_null()) bladeHeight->get_to(loadedSettings.bladeHeight);

            const auto bladeWidth = json.find("bladeWidth");
            if (bladeWidth != json.end() && !bladeWidth->is_null()) bladeWidth->get_to(loadedSettings.bladeWidth);

            const auto color1 = json.find("color1");
            if (color1 != json.end() && color1->is_array() && color1->size() >= 3) {
                loadedSettings.color1.r = color1->at(0).get<float>();
                loadedSettings.color1.g = color1->at(1).get<float>();
                loadedSettings.color1.b = color1->at(2).get<float>();
            }

            const auto color2 = json.find("color2");
            if (color2 != json.end() && color2->is_array() && color2->size() >= 3) {
                loadedSettings.color2.r = color2->at(0).get<float>();
                loadedSettings.color2.g = color2->at(1).get<float>();
                loadedSettings.color2.b = color2->at(2).get<float>();
            }

            const auto color1DarknessFactor = json.find("color1DarknessFactor");
            if (color1DarknessFactor != json.end() && !color1DarknessFactor->is_null()) {
                color1DarknessFactor->get_to(loadedSettings.color1DarknessFactor);
            }

            const auto color2DarknessFactor = json.find("color2DarknessFactor");
            if (color2DarknessFactor != json.end() && !color2DarknessFactor->is_null()) {
                color2DarknessFactor->get_to(loadedSettings.color2DarknessFactor);
            }

            const auto noiseSquareMultiplier = json.find("noiseSquareMultiplier");
            if (noiseSquareMultiplier != json.end() && !noiseSquareMultiplier->is_null()) {
                noiseSquareMultiplier->get_to(loadedSettings.noiseSquareMultiplier);
            }

            const auto noiseMixMultiplier = json.find("noiseMixMultiplier");
            if (noiseMixMultiplier != json.end() && !noiseMixMultiplier->is_null()) {
                noiseMixMultiplier->get_to(loadedSettings.noiseMixMultiplier);
            }

            const auto roughness = json.find("roughness");
            if (roughness != json.end() && !roughness->is_null()) roughness->get_to(loadedSettings.roughness);

            const auto subSurfaceFactor = json.find("subSurfaceFactor");
            if (subSurfaceFactor != json.end() && !subSurfaceFactor->is_null()) {
                subSurfaceFactor->get_to(loadedSettings.subSurfaceFactor);
            }

            const auto normalUpBlend = json.find("normalUpBlend");
            if (normalUpBlend != json.end() && !normalUpBlend->is_null()) {
                normalUpBlend->get_to(loadedSettings.normalUpBlend);
            }

            const auto normalBlendStartDistance = json.find("normalBlendStartDistance");
            if (normalBlendStartDistance != json.end() && !normalBlendStartDistance->is_null()) {
                normalBlendStartDistance->get_to(loadedSettings.normalBlendStartDistance);
            }

            const auto normalBlendEndDistance = json.find("normalBlendEndDistance");
            if (normalBlendEndDistance != json.end() && !normalBlendEndDistance->is_null()) {
                normalBlendEndDistance->get_to(loadedSettings.normalBlendEndDistance);
            }

            const auto diffuseWrap = json.find("diffuseWrap");
            if (diffuseWrap != json.end() && !diffuseWrap->is_null()) {
                diffuseWrap->get_to(loadedSettings.diffuseWrap);
            }

            const auto transmissionPower = json.find("transmissionPower");
            if (transmissionPower != json.end() && !transmissionPower->is_null()) {
                transmissionPower->get_to(loadedSettings.transmissionPower);
            }

            const auto specularStrength = json.find("specularStrength");
            if (specularStrength != json.end() && !specularStrength->is_null()) {
                specularStrength->get_to(loadedSettings.specularStrength);
            }
        }
        catch (const nlohmann::json::exception& e) {
            Logging::Error() << "Config::Grass::LoadFromDisk() failed to read '" << CONFIG_FILE_PATH << "': " << e.what() << "\n";
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
            Logging::Error() << "Config::Grass::SaveToDisk() failed to create config directory: " << errorCode.message() << "\n";
            return false;
        }

        const nlohmann::json json = {
            { "segmentCount", g_settings.segmentCount },
            { "curveAmount", g_settings.curveAmount },
            { "spacing", g_settings.spacing },
            { "minCullDistance", g_settings.minCullDistance },
            { "maxCullDistance", g_settings.maxCullDistance },
            { "cullExponent", g_settings.cullExponent },
            { "bladeHeight", g_settings.bladeHeight },
            { "bladeWidth", g_settings.bladeWidth },
            { "color1", { g_settings.color1.r, g_settings.color1.g, g_settings.color1.b } },
            { "color2", { g_settings.color2.r, g_settings.color2.g, g_settings.color2.b } },
            { "color1DarknessFactor", g_settings.color1DarknessFactor },
            { "color2DarknessFactor", g_settings.color2DarknessFactor },
            { "noiseSquareMultiplier", g_settings.noiseSquareMultiplier },
            { "noiseMixMultiplier", g_settings.noiseMixMultiplier },
            { "roughness", g_settings.roughness },
            { "subSurfaceFactor", g_settings.subSurfaceFactor },
            { "normalUpBlend", g_settings.normalUpBlend },
            { "normalBlendStartDistance", g_settings.normalBlendStartDistance },
            { "normalBlendEndDistance", g_settings.normalBlendEndDistance },
            { "diffuseWrap", g_settings.diffuseWrap },
            { "transmissionPower", g_settings.transmissionPower },
            { "specularStrength", g_settings.specularStrength }
        };
        return Hell::Json::SaveToFile(json, CONFIG_FILE_PATH);
    }

    const std::string& GetFilePath() {
        return CONFIG_FILE_PATH;
    }
}

namespace Config::Moonlight {
    namespace {
        const std::string CONFIG_FILE_PATH = "res/config/moonlight.json";
        Settings g_settings;

        void Sanitize(Settings& settings) {
            settings.color = glm::clamp(settings.color, glm::vec3(0.0f), glm::vec3(1.0f));
            settings.strength = std::clamp(settings.strength, 0.0f, 1.0f);
        }
    }

    const Settings& GetSettings() {
        return g_settings;
    }

    void SetSettings(const Settings& settings) {
        g_settings = settings;
        Sanitize(g_settings);
    }

    bool LoadFromDisk() {
        nlohmann::json json;
        if (!Hell::Json::LoadFromFile(json, CONFIG_FILE_PATH)) return false;

        Settings loadedSettings;
        try {
            const auto color = json.find("color");
            if (color != json.end() && color->is_array() && color->size() >= 3) {
                loadedSettings.color.r = color->at(0).get<float>();
                loadedSettings.color.g = color->at(1).get<float>();
                loadedSettings.color.b = color->at(2).get<float>();
            }

            const auto strength = json.find("strength");
            if (strength != json.end() && !strength->is_null()) strength->get_to(loadedSettings.strength);
        }
        catch (const nlohmann::json::exception& e) {
            Logging::Error() << "Config::Moonlight::LoadFromDisk() failed to read '" << CONFIG_FILE_PATH << "': " << e.what() << "\n";
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
            Logging::Error() << "Config::Moonlight::SaveToDisk() failed to create config directory: " << errorCode.message() << "\n";
            return false;
        }

        const nlohmann::json json = {
            { "color", { g_settings.color.r, g_settings.color.g, g_settings.color.b } },
            { "strength", g_settings.strength }
        };

        return Hell::Json::SaveToFile(json, CONFIG_FILE_PATH);
    }

    const std::string& GetFilePath() {
        return CONFIG_FILE_PATH;
    }
}

namespace Config::Fog {
    namespace {
        const std::string CONFIG_FILE_PATH = "res/config/fog.json";
        Settings g_settings;

        void Sanitize(Settings& settings) {
            settings.color = glm::clamp(settings.color, glm::vec3(0.0f), glm::vec3(4.0f));
            settings.colorStrength = std::clamp(settings.colorStrength, 0.0f, 8.0f);

            settings.maxRayDistance = std::clamp(settings.maxRayDistance, 0.1f, 256.0f);
            settings.stepCount = std::clamp(settings.stepCount, 1, 128);
            settings.densityBias = std::clamp(settings.densityBias, 0.0f, 1.0f);
            settings.densityScale = std::clamp(settings.densityScale, 0.0f, 16.0f);
            settings.extinctionScale = std::clamp(settings.extinctionScale, 0.0f, 4.0f);
            settings.ambientStartDistance = std::clamp(settings.ambientStartDistance, 0.0f, 256.0f);
            settings.ambientEndDistance = std::clamp(settings.ambientEndDistance, settings.ambientStartDistance + 0.001f, 512.0f);
            settings.ambientExponent = std::clamp(settings.ambientExponent, 0.01f, 16.0f);

            settings.heightFadeStart = std::clamp(settings.heightFadeStart, -256.0f, 256.0f);
            settings.heightFadeEnd = std::clamp(settings.heightFadeEnd, settings.heightFadeStart + 0.001f, 512.0f);
            settings.heightExponent = std::clamp(settings.heightExponent, 0.01f, 16.0f);
            settings.lowHeightScatterMultiplier = std::clamp(settings.lowHeightScatterMultiplier, 0.0f, 16.0f);

            settings.distanceFogStart = std::clamp(settings.distanceFogStart, 0.0f, 512.0f);
            settings.distanceFogEnd = std::clamp(settings.distanceFogEnd, settings.distanceFogStart + 0.001f, 1024.0f);
            settings.distanceFogExponent = std::clamp(settings.distanceFogExponent, 0.01f, 16.0f);
            settings.distanceFogStrength = std::clamp(settings.distanceFogStrength, 0.0f, 1.0f);

            settings.clumpSizeXZ = std::clamp(settings.clumpSizeXZ, 0.1f, 256.0f);
            settings.clumpSizeY = std::clamp(settings.clumpSizeY, 0.1f, 256.0f);
            settings.noiseScaleNearMultiplier = std::clamp(settings.noiseScaleNearMultiplier, 0.05f, 32.0f);
            settings.noiseScaleFarMultiplier = std::clamp(settings.noiseScaleFarMultiplier, 0.05f, 32.0f);
            settings.noiseScaleStartDistance = std::clamp(settings.noiseScaleStartDistance, 0.0f, 512.0f);
            settings.noiseScaleEndDistance = std::clamp(settings.noiseScaleEndDistance, settings.noiseScaleStartDistance + 0.001f, 1024.0f);
            settings.noiseScaleExponent = std::clamp(settings.noiseScaleExponent, 0.01f, 16.0f);

            settings.noiseMipBias = std::clamp(settings.noiseMipBias, -8.0f, 8.0f);
            settings.noiseMipScale = std::clamp(settings.noiseMipScale, 0.0f, 8.0f);
            settings.noiseMinMip = std::clamp(settings.noiseMinMip, 0.0f, 16.0f);
            settings.noiseMaxMip = std::clamp(settings.noiseMaxMip, settings.noiseMinMip, 16.0f);
            settings.noiseNearMip = std::clamp(settings.noiseNearMip, 0.0f, 16.0f);
            settings.noiseFarMip = std::clamp(settings.noiseFarMip, 0.0f, 16.0f);
            settings.noiseMipNearDistance = std::clamp(settings.noiseMipNearDistance, 0.0f, 512.0f);
            settings.noiseMipFarDistance = std::clamp(settings.noiseMipFarDistance, settings.noiseMipNearDistance + 0.001f, 1024.0f);
            settings.noiseMipExponent = std::clamp(settings.noiseMipExponent, 0.01f, 16.0f);
            settings.noiseMipRespectStep = std::clamp(settings.noiseMipRespectStep, 0.0f, 1.0f);

            settings.windVelocity = glm::clamp(settings.windVelocity, glm::vec3(-16.0f), glm::vec3(16.0f));
            settings.timeScrollSpeed = std::clamp(settings.timeScrollSpeed, -32.0f, 32.0f);
            settings.xMorphSpeed = std::clamp(settings.xMorphSpeed, -32.0f, 32.0f);
            settings.zMorphSpeed = std::clamp(settings.zMorphSpeed, -32.0f, 32.0f);
            settings.yScrollSpeed = std::clamp(settings.yScrollSpeed, -32.0f, 32.0f);
        }

        template<typename T>
        void ReadIfPresent(const nlohmann::json& json, const char* name, T& value) {
            const auto it = json.find(name);
            if (it != json.end() && !it->is_null()) it->get_to(value);
        }

        void ReadVec3IfPresent(const nlohmann::json& json, const char* name, glm::vec3& value) {
            const auto it = json.find(name);
            if (it == json.end() || !it->is_array() || it->size() < 3) return;
            value.r = it->at(0).get<float>();
            value.g = it->at(1).get<float>();
            value.b = it->at(2).get<float>();
        }
    }

    const Settings& GetSettings() {
        return g_settings;
    }

    void SetSettings(const Settings& settings) {
        g_settings = settings;
        Sanitize(g_settings);
    }

    bool LoadFromDisk() {
        nlohmann::json json;
        if (!Hell::Json::LoadFromFile(json, CONFIG_FILE_PATH)) return false;

        Settings loadedSettings;
        try {
            ReadIfPresent(json, "enabled", loadedSettings.enabled);
            ReadVec3IfPresent(json, "color", loadedSettings.color);
            ReadIfPresent(json, "colorStrength", loadedSettings.colorStrength);

            const auto rayMarch = json.find("rayMarch");
            if (rayMarch != json.end() && rayMarch->is_object()) {
                ReadIfPresent(*rayMarch, "maxDistance", loadedSettings.maxRayDistance);
                ReadIfPresent(*rayMarch, "stepCount", loadedSettings.stepCount);
                ReadIfPresent(*rayMarch, "densityBias", loadedSettings.densityBias);
                ReadIfPresent(*rayMarch, "densityScale", loadedSettings.densityScale);
                ReadIfPresent(*rayMarch, "extinctionScale", loadedSettings.extinctionScale);
                ReadIfPresent(*rayMarch, "ambientStartDistance", loadedSettings.ambientStartDistance);
                ReadIfPresent(*rayMarch, "ambientEndDistance", loadedSettings.ambientEndDistance);
                ReadIfPresent(*rayMarch, "ambientExponent", loadedSettings.ambientExponent);
            }

            const auto height = json.find("height");
            if (height != json.end() && height->is_object()) {
                ReadIfPresent(*height, "fadeStart", loadedSettings.heightFadeStart);
                ReadIfPresent(*height, "fadeEnd", loadedSettings.heightFadeEnd);
                ReadIfPresent(*height, "exponent", loadedSettings.heightExponent);
                ReadIfPresent(*height, "lowScatterMultiplier", loadedSettings.lowHeightScatterMultiplier);
            }

            const auto distance = json.find("distanceFog");
            if (distance != json.end() && distance->is_object()) {
                ReadIfPresent(*distance, "start", loadedSettings.distanceFogStart);
                ReadIfPresent(*distance, "end", loadedSettings.distanceFogEnd);
                ReadIfPresent(*distance, "exponent", loadedSettings.distanceFogExponent);
                ReadIfPresent(*distance, "strength", loadedSettings.distanceFogStrength);
            }

            const auto noise = json.find("noise");
            if (noise != json.end() && noise->is_object()) {
                ReadIfPresent(*noise, "clumpSizeXZ", loadedSettings.clumpSizeXZ);
                ReadIfPresent(*noise, "clumpSizeY", loadedSettings.clumpSizeY);
                ReadIfPresent(*noise, "nearScaleMultiplier", loadedSettings.noiseScaleNearMultiplier);
                ReadIfPresent(*noise, "farScaleMultiplier", loadedSettings.noiseScaleFarMultiplier);
                ReadIfPresent(*noise, "scaleStartDistance", loadedSettings.noiseScaleStartDistance);
                ReadIfPresent(*noise, "scaleEndDistance", loadedSettings.noiseScaleEndDistance);
                ReadIfPresent(*noise, "scaleExponent", loadedSettings.noiseScaleExponent);
                ReadIfPresent(*noise, "mipBias", loadedSettings.noiseMipBias);
                ReadIfPresent(*noise, "mipScale", loadedSettings.noiseMipScale);
                ReadIfPresent(*noise, "minMip", loadedSettings.noiseMinMip);
                ReadIfPresent(*noise, "maxMip", loadedSettings.noiseMaxMip);
                ReadIfPresent(*noise, "nearMip", loadedSettings.noiseNearMip);
                ReadIfPresent(*noise, "farMip", loadedSettings.noiseFarMip);
                ReadIfPresent(*noise, "mipNearDistance", loadedSettings.noiseMipNearDistance);
                ReadIfPresent(*noise, "mipFarDistance", loadedSettings.noiseMipFarDistance);
                ReadIfPresent(*noise, "mipExponent", loadedSettings.noiseMipExponent);
                ReadIfPresent(*noise, "mipRespectStep", loadedSettings.noiseMipRespectStep);
            }

            const auto motion = json.find("motion");
            if (motion != json.end() && motion->is_object()) {
                ReadVec3IfPresent(*motion, "windVelocity", loadedSettings.windVelocity);
                ReadIfPresent(*motion, "timeScrollSpeed", loadedSettings.timeScrollSpeed);
                ReadIfPresent(*motion, "xMorphSpeed", loadedSettings.xMorphSpeed);
                ReadIfPresent(*motion, "zMorphSpeed", loadedSettings.zMorphSpeed);
                ReadIfPresent(*motion, "yScrollSpeed", loadedSettings.yScrollSpeed);
            }
        }
        catch (const nlohmann::json::exception& e) {
            Logging::Error() << "Config::Fog::LoadFromDisk() failed to read '" << CONFIG_FILE_PATH << "': " << e.what() << "\n";
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
            Logging::Error() << "Config::Fog::SaveToDisk() failed to create config directory: " << errorCode.message() << "\n";
            return false;
        }

        const nlohmann::json json = {
            { "enabled", g_settings.enabled },
            { "color", { g_settings.color.r, g_settings.color.g, g_settings.color.b } },
            { "colorStrength", g_settings.colorStrength },
            { "rayMarch", {
                { "maxDistance", g_settings.maxRayDistance },
                { "stepCount", g_settings.stepCount },
                { "densityBias", g_settings.densityBias },
                { "densityScale", g_settings.densityScale },
                { "extinctionScale", g_settings.extinctionScale },
                { "ambientStartDistance", g_settings.ambientStartDistance },
                { "ambientEndDistance", g_settings.ambientEndDistance },
                { "ambientExponent", g_settings.ambientExponent }
            } },
            { "height", {
                { "fadeStart", g_settings.heightFadeStart },
                { "fadeEnd", g_settings.heightFadeEnd },
                { "exponent", g_settings.heightExponent },
                { "lowScatterMultiplier", g_settings.lowHeightScatterMultiplier }
            } },
            { "distanceFog", {
                { "start", g_settings.distanceFogStart },
                { "end", g_settings.distanceFogEnd },
                { "exponent", g_settings.distanceFogExponent },
                { "strength", g_settings.distanceFogStrength }
            } },
            { "noise", {
                { "clumpSizeXZ", g_settings.clumpSizeXZ },
                { "clumpSizeY", g_settings.clumpSizeY },
                { "nearScaleMultiplier", g_settings.noiseScaleNearMultiplier },
                { "farScaleMultiplier", g_settings.noiseScaleFarMultiplier },
                { "scaleStartDistance", g_settings.noiseScaleStartDistance },
                { "scaleEndDistance", g_settings.noiseScaleEndDistance },
                { "scaleExponent", g_settings.noiseScaleExponent },
                { "mipBias", g_settings.noiseMipBias },
                { "mipScale", g_settings.noiseMipScale },
                { "minMip", g_settings.noiseMinMip },
                { "maxMip", g_settings.noiseMaxMip },
                { "nearMip", g_settings.noiseNearMip },
                { "farMip", g_settings.noiseFarMip },
                { "mipNearDistance", g_settings.noiseMipNearDistance },
                { "mipFarDistance", g_settings.noiseMipFarDistance },
                { "mipExponent", g_settings.noiseMipExponent },
                { "mipRespectStep", g_settings.noiseMipRespectStep }
            } },
            { "motion", {
                { "windVelocity", { g_settings.windVelocity.x, g_settings.windVelocity.y, g_settings.windVelocity.z } },
                { "timeScrollSpeed", g_settings.timeScrollSpeed },
                { "xMorphSpeed", g_settings.xMorphSpeed },
                { "zMorphSpeed", g_settings.zMorphSpeed },
                { "yScrollSpeed", g_settings.yScrollSpeed }
            } }
        };

        return Hell::Json::SaveToFile(json, CONFIG_FILE_PATH);
    }

    const std::string& GetFilePath() {
        return CONFIG_FILE_PATH;
    }
}
