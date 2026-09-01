#pragma once
#include "Unloved/Common/Types.h"

#include <string>

namespace Config {
    void Init();
	const Resolutions& GetResolutions();
	const float GetNearPlane();
	const float GetFarPlane();
    //void SetDepthPeelCount(int count);
}

namespace Config::Christmas {
    struct Settings {
        float lightRadius = 0.2f;
        float lightStrength = 0.1f;
    };

    const Settings& GetSettings();
    void SetSettings(const Settings& settings);

    bool LoadFromDisk();
    bool SaveToDisk();
    const std::string& GetFilePath();
}

namespace Config::Moonlight {
    struct Settings {
        glm::vec3 color = glm::vec3(0.881875f, 0.894375f, 0.73525f);
        float strength = 0.05f;
    };

    const Settings& GetSettings();
    void SetSettings(const Settings& settings);

    bool LoadFromDisk();
    bool SaveToDisk();
    const std::string& GetFilePath();
}

namespace Config::Grass {
    struct Settings {
        int segmentCount = 3;
        float curveAmount = 0.1f;
        float spacing = 0.0185185185185185f;
        float minCullDistance = 5.0f;
        float maxCullDistance = 30.0f;
        float cullExponent = 8.0f;
        float bladeHeight = 0.075f;
        float bladeWidth = 0.003f;
        glm::vec3 color1 = glm::vec3(0.45f, 0.40f, 0.12f);
        glm::vec3 color2 = glm::vec3(175.0f, 200.0f, 92.0f) / 255.0f;
        float color1DarknessFactor = 1.0f;
        float color2DarknessFactor = 1.0f;
        float noiseSquareMultiplier = 2.5f;
        float noiseMixMultiplier = 0.3f;
        float roughness = 0.4f;
        float subSurfaceFactor = 0.45f;
        float normalUpBlend = 0.35f;
        float normalBlendStartDistance = 3.0f;
        float normalBlendEndDistance = 10.0f;
        float diffuseWrap = 0.35f;
        float transmissionPower = 4.0f;
        float specularStrength = 0.25f;
    };

    const Settings& GetSettings();
    void SetSettings(const Settings& settings);

    bool LoadFromDisk();
    bool SaveToDisk();
    const std::string& GetFilePath();
}

namespace Config::Fog {
    struct Settings {
        bool enabled = true;

        glm::vec3 color = glm::vec3(0.222f, 0.233f, 0.270f);
        float colorStrength = 1.0f;

        float maxRayDistance = 30.0f;
        int stepCount = 36;
        float densityBias = 0.005f;
        float densityScale = 1.25f;
        float extinctionScale = 0.02f;
        float ambientStartDistance = 1.0f;
        float ambientEndDistance = 5.0f;
        float ambientExponent = 1.0f;

        float heightFadeStart = 29.0f;
        float heightFadeEnd = 31.0f;
        float heightExponent = 1.0f;
        float lowHeightScatterMultiplier = 3.0f;

        float distanceFogStart = 30.0f;
        float distanceFogEnd = 80.0f;
        float distanceFogExponent = 1.0f;
        float distanceFogStrength = 1.0f;

        float clumpSizeXZ = 10.666667f;
        float clumpSizeY = 8.0f;
        float noiseScaleNearMultiplier = 1.0f;
        float noiseScaleFarMultiplier = 1.0f;
        float noiseScaleStartDistance = 6.0f;
        float noiseScaleEndDistance = 22.0f;
        float noiseScaleExponent = 2.0f;

        float noiseMipBias = 0.0f;
        float noiseMipScale = 1.0f;
        float noiseMinMip = 0.0f;
        float noiseMaxMip = 2.0f;
        float noiseNearMip = 0.0f;
        float noiseFarMip = 2.0f;
        float noiseMipNearDistance = 1.175f;
        float noiseMipFarDistance = 5.0f;
        float noiseMipExponent = 5.6f;
        float noiseMipRespectStep = 0.0f;

        glm::vec3 windVelocity = glm::vec3(0.10f, 0.02f, 0.10f);
        float timeScrollSpeed = 4.4f;
        float xMorphSpeed = 5.5f;
        float zMorphSpeed = 5.5f;
        float yScrollSpeed = 2.2f;
    };

    const Settings& GetSettings();
    void SetSettings(const Settings& settings);

    bool LoadFromDisk();
    bool SaveToDisk();
    const std::string& GetFilePath();
}
