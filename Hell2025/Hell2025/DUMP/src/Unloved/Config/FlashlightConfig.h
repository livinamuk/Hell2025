#pragma once

#include <glm/glm.hpp>

#include <string>

namespace Config::Flashlight {

    struct Settings {
        std::string iesProfile = "ThreeJS_2";
        bool iesEnabled = true;
        float range = 19.1f;
        float falloffExponent = 4.29f;
        float brightness = 1.0f;
        glm::vec3 color = glm::vec3(0.780f, 0.778f, 0.797f);
        float iesConeScale = 1.0f;
        float iesInnerAngle = 14.0f;
        float iesOuterAngle = 40.0f;
        float iesContrast = 0.2f;

        bool centerSpotEnabled = true;
        float centerSpotRange = 15.0f;
        float centerSpotFalloffExponent = 4.0f;
        float centerSpotBrightness = 1.0f;
        float centerSpotInnerAngle = 1.5f;
        float centerSpotOuterAngle = 5.0f;
    };

    const Settings& GetSettings();
    void SetSettings(const Settings& settings);
    void ResetSettings();

    bool LoadFromDisk();
    bool SaveToDisk();
    const std::string& GetFilePath();
}
