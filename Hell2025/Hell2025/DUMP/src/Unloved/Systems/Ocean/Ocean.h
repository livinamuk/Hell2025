#pragma once
#include "Unloved/Common/Types.h"

#include <complex>
#include <cstdint>
#include <string>
#include <vector>

namespace Ocean {
    constexpr int FFT_BAND_COUNT = 2;
    constexpr unsigned int FFT_RESOLUTION = 512; // Radix shaders only support 512

    enum struct DisplayMode : int32_t {
        COMBINED = 0,
        BAND_0 = 1,
        BAND_1 = 2,
    };

    struct BandSettings {
        float domainSize = 0.0f;                 // Size of the repeating simulation tile in metres
        float minimumWavelength = 0.0f;          // Kill waves shorter than this
        float maximumWavelength = 0.0f;          // Kill waves longer than this
        float minimumWavelengthFade = 0.0f;      // Soft edge around the short wave cutoff
        float maximumWavelengthFade = 0.0f;      // Soft edge around the long wave cutoff
        glm::vec2 windDirection = {};
        float amplitude = 0.0f;                  // Spectrum power not final wave height
        float windAlignmentExponent = 2.0f;      // Higher values point more waves downwind
        float opposingWavesDamping = 1.0f;       // Zero removes waves travelling against the wind
        float smallWavesDamping = 0.0000001f;    // Higher values erase the tiny waves
        uint32_t randomSeed = 0;
    };

    struct SurfaceSettings {
        bool specularAntiAliasing = true;
        glm::vec3 albedo = glm::vec3(0.0325f, 0.0675f, 0.0625f) * 0.95f;
        glm::vec3 fogColor = glm::vec3(0.00326f, 0.00217f, 0.00073f);
        float normalScale = 4.0f;
        float normalConvergeStartDistance = 0.0f;
        float normalConvergeEndDistance = 250.0f;
        float normalConvergeMaxFactor = 0.9f;
        float normalConvergeExponent = 0.95f;
        float normalSoftening = 0.5f;
        float rippleTiling = 0.325f;
        float rippleStrength = 0.025f;
        float rippleSecondLayerScale = 1.5f;
        glm::vec2 rippleVelocity0 = glm::vec2(0.15f, 0.10f);
        glm::vec2 rippleVelocity1 = glm::vec2(-0.12f, 0.08f);
        float roughness = 0.03f;
        float reflectance = 0.02f;
        float reflectionGamma = 2.2f;
        float diffuseStrength = 0.0125f;
        float sssHeightRange = 0.5f;
        float sssStrength = 0.5f;
        float underwaterSssStrength = 3.0f;
        float sssRadiusMinimum = 0.45f;
        float sssRadiusMaximum = 0.50f;
        float sssIntensity = 0.2f;
        float sssFalloff = 3.0f;
        float sssSaturation = 2.0f;
        float fogStartDistance = 0.0f;
        float fogEndDistance = 550.0f;
        float fogExponent = 0.5f;
        float fogStrength = 0.1f;
    };

    struct SurfaceCompositeSettings {
        float planeHeightOffset = 100.0f;
        float distortionSpeed = 0.05f;
        float distortionStrength = 0.004f;
        float distortionTiling = 0.15f;
        float refractionTintStrength = 0.2f;
    };

    struct UnderwaterCompositeSettings {
        glm::vec3 rayFogColor = glm::vec3(0.4f, 0.8f, 0.6f);
        float rayFogStrength = 0.00125f;
        float darknessCurve = 0.95f;
        float distortionSpeed = 0.075f;
        float distortionStrength = 0.0024f;
        float depthTintStrength = 1.5f;
        float depthTintOriginalWeight = 0.5f;
        float geometryWaterColorSquaredStrength = 5.75f;
        float geometryWaterColorStrength = 0.25f;
        float geometryTintStrength = 0.25f;
        float openWaterTintStrength = 0.99f;
        float openWaterBrightness = 0.5f;
    };

    struct CompositeSettings {
        glm::vec3 underwaterTint = glm::vec3(0.5275f, 1.0575f, 0.7975f);
        SurfaceCompositeSettings surface;
        UnderwaterCompositeSettings underwater;
    };

    struct Settings {
        bool simulate = true;
        bool alternateBandUpdates = false;
        DisplayMode displayMode = DisplayMode::COMBINED;
        float windSpeed = 75.0f;
        float gravity = 9.8f;
        float displacementScale = 1.0f;          // Horizontal chop
        float heightScale = 0.5f;                // Final wave height
        float simulationTimeScale = 0.6f;
        float simulationTimeOffset = 50.0f;
        BandSettings bands[FFT_BAND_COUNT];
        SurfaceSettings surface;
        CompositeSettings composite;
    };

    void Init();
    void UpdateSpectrum();
    void CreatePhysicsPlane();
    void DestroyPhysicsPlane();

    Settings GetSettings();
    void SetSettings(const Settings& settings);
    void ResetSettings();
    bool LoadFromDisk();
    bool SaveToDisk();
    const std::string& GetFilePath();
    const std::vector<std::complex<float>>& GetH0(int bandIndex);
    bool H0UploadRequired(int bandIndex);
    void MarkH0Uploaded(int bandIndex);

    const float GetDisplacementScale();
    const float GetHeightScale();
    float GetAnimationTime();
    float UpdateSimulationTime();
    DisplayMode GetDisplayMode();

    const float GetGravity();
    const float GetOceanOriginY();
    const float GetWaterHeightAtPlayer(int playerIndex);
    const float GetDomainSize(int bandIndex);

    OceanReadbackData& GetOceanReadBackData();
};
