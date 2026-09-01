#include "Ocean.h"

#include "Hell/Logging.h"
#include "Hell/Physics/Physics.h"
#include "Hell/Serialization/Json.h"
#include "Hell/Time.h"

#include "Unloved/Common/Constants.h"
#include "Unloved/ObjectId.h"

#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <random>
#include <system_error>

namespace Ocean {

    const std::string CONFIG_FILE_PATH = "res/config/ocean.json";

    struct FFTBandData {
        std::vector<std::complex<float>> h0;
        bool h0UploadRequired = false;
    };

    uint64_t g_waterPlaneUpFacingPhysicsID = 0;
    uint64_t g_waterPlaneDownFacingPhysicsID = 0;

    Settings g_settings;
    Settings g_generatedSettings;
    FFTBandData g_fftBandData[FFT_BAND_COUNT];
    bool g_spectrumGenerated = false;
    OceanReadbackData g_oceanReadbackData;
    float g_animationTime = 0.0f;
    float g_simulationTime = 0.0f;

    const float g_oceanOriginY = 29.5f;

    float SmoothStep(float edge0, float edge1, float value);
    float SpectrumBandWindow(float wavelength, const BandSettings& settings);
    float SpectrumBandWeight(float wavelength, int bandIndex);
    float PhillipsSpectrum(const glm::vec2& k, int bandIndex);
    glm::vec2 KVector(int x, int z, float domainSize);
    std::vector<std::complex<float>> ComputeH0(int bandIndex);
    Settings CreateDefaultSettings();
    bool BandSpectrumSettingsChanged(int bandIndex);
    void SanitizeSettings();

    template<typename T>
    void ReadIfPresent(const nlohmann::json& json, const char* name, T& value) {
        const auto it = json.find(name);
        if (it != json.end() && !it->is_null()) it->get_to(value);
    }

    void ReadVec2IfPresent(const nlohmann::json& json, const char* name, glm::vec2& value) {
        const auto it = json.find(name);
        if (it == json.end() || it->is_null()) return;
        value.x = it->at(0).get<float>();
        value.y = it->at(1).get<float>();
    }

    void ReadVec3IfPresent(const nlohmann::json& json, const char* name, glm::vec3& value) {
        const auto it = json.find(name);
        if (it == json.end() || it->is_null()) return;
        value.x = it->at(0).get<float>();
        value.y = it->at(1).get<float>();
        value.z = it->at(2).get<float>();
    }

    void Init() {
        g_spectrumGenerated = false;
        g_animationTime = 0.0f;
        g_simulationTime = 0.0f;
        g_settings = CreateDefaultSettings();
        if (!LoadFromDisk()) UpdateSpectrum();

        g_oceanReadbackData.heightPlayer0 = g_oceanOriginY;
        g_oceanReadbackData.heightPlayer1 = g_oceanOriginY;
        g_oceanReadbackData.heightPlayer2 = g_oceanOriginY;
        g_oceanReadbackData.heightPlayer3 = g_oceanOriginY;
    }

    Settings CreateDefaultSettings() {
        Settings settings;

        // Big water matched to the old 13 metre layer
        settings.bands[0].domainSize = 13.123f;
        settings.bands[0].minimumWavelength = 1.0f;
        settings.bands[0].maximumWavelength = 13.123f;
        settings.bands[0].minimumWavelengthFade = 0.5f;
        settings.bands[0].maximumWavelengthFade = 0.25f;
        settings.bands[0].windDirection = glm::vec2(0.9f, -0.4f);
        settings.bands[0].amplitude = 0.09873434f;
        settings.bands[0].opposingWavesDamping = 1.0f;
        settings.bands[0].smallWavesDamping = 0.000000001423249f;
        settings.bands[0].randomSeed = 42;

        // Small water matched to the old 8 metre layer
        settings.bands[1].domainSize = 8.0f;
        settings.bands[1].minimumWavelength = 0.03125f;
        settings.bands[1].maximumWavelength = 2.0f;
        settings.bands[1].minimumWavelengthFade = 0.015625f;
        settings.bands[1].maximumWavelengthFade = 0.5f;
        settings.bands[1].windDirection = glm::vec2(1.0f, 0.1f);
        settings.bands[1].amplitude = 2.4719238f;
        settings.bands[1].opposingWavesDamping = 1.0f;
        settings.bands[1].smallWavesDamping = 0.00000000028444445f;
        settings.bands[1].randomSeed = 1337;

        return settings;
    }

    void UpdateSpectrum() {
        SanitizeSettings();

        for (int i = 0; i < FFT_BAND_COUNT; i++) {
            if (!g_spectrumGenerated || BandSpectrumSettingsChanged(i)) {
                g_fftBandData[i].h0 = ComputeH0(i);
                g_fftBandData[i].h0UploadRequired = true;
            }
        }

        g_generatedSettings = g_settings;
        g_spectrumGenerated = true;
    }

    void CreatePhysicsPlane() {
        if (g_waterPlaneUpFacingPhysicsID != 0) DestroyPhysicsPlane();

        PhysicsFilterData filterData;
        filterData.raycastGroup = RaycastGroup::RAYCAST_ENABLED;
        filterData.collisionGroup = CollisionGroup::NO_COLLISION;
        filterData.collidesWith = CollisionGroup::NO_COLLISION;

        glm::vec3 planePosition = glm::vec3(0.0f, g_oceanOriginY, 0.0f);
        glm::vec3 planeNormal = glm::vec3(0.0f, 1.0f, 0.0f);

        g_waterPlaneUpFacingPhysicsID = Hell::Physics::CreateRigidStaticPlane(planePosition, planeNormal, filterData);
        g_waterPlaneDownFacingPhysicsID = Hell::Physics::CreateRigidStaticPlane(planePosition, planeNormal * glm::vec3(-1.0f), filterData);

        PhysicsUserData physicsUserData;
        physicsUserData.objectId = Unloved::GetNextObjectId(ObjectType::WATER_PLANE_TOP);
        Hell::Physics::SetRigidStaticUserData(g_waterPlaneUpFacingPhysicsID, physicsUserData);

        physicsUserData.objectId = Unloved::GetNextObjectId(ObjectType::WATER_PLANE_BOTTOM);
        Hell::Physics::SetRigidStaticUserData(g_waterPlaneDownFacingPhysicsID, physicsUserData);
    }

    void DestroyPhysicsPlane() {
        if (g_waterPlaneUpFacingPhysicsID != 0) Hell::Physics::RemoveRigidStatic(g_waterPlaneUpFacingPhysicsID);
        if (g_waterPlaneDownFacingPhysicsID != 0) Hell::Physics::RemoveRigidStatic(g_waterPlaneDownFacingPhysicsID);
        g_waterPlaneUpFacingPhysicsID = 0;
        g_waterPlaneDownFacingPhysicsID = 0;
    }

    float SmoothStep(float edge0, float edge1, float value) {
        if (edge0 == edge1) return value < edge0 ? 0.0f : 1.0f;
        float t = std::clamp((value - edge0) / (edge1 - edge0), 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }

    float SpectrumBandWindow(float wavelength, const BandSettings& settings) {
        float minimumWeight = SmoothStep(settings.minimumWavelength - settings.minimumWavelengthFade, settings.minimumWavelength + settings.minimumWavelengthFade, wavelength);
        float maximumWeight = 1.0f - SmoothStep(settings.maximumWavelength - settings.maximumWavelengthFade, settings.maximumWavelength + settings.maximumWavelengthFade, wavelength);
        return minimumWeight * maximumWeight;
    }

    float SpectrumBandWeight(float wavelength, int bandIndex) {
        float bandWeight = SpectrumBandWindow(wavelength, g_settings.bands[bandIndex]);
        float totalWeight = 0.0f;

        for (int i = 0; i < FFT_BAND_COUNT; i++) {
            totalWeight += SpectrumBandWindow(wavelength, g_settings.bands[i]);
        }

        return totalWeight > 0.0f ? bandWeight / totalWeight : 0.0f;
    }

    glm::vec2 KVector(int x, int z, float domainSize) {
        return glm::vec2((x - FFT_RESOLUTION / 2.0f) * (2.0f * HELL_PI / domainSize), (z - FFT_RESOLUTION / 2.0f) * (2.0f * HELL_PI / domainSize));
    }

    float PhillipsSpectrum(const glm::vec2& k, int bandIndex) {
        const BandSettings& settings = g_settings.bands[bandIndex];
        const float lengthK = glm::length(k);
        if (lengthK <= 0.0f) return 0.0f;

        const float lengthKSquared = lengthK * lengthK;
        const float dotKWind = glm::dot(k / lengthK, settings.windDirection);
        const float directionalWeight = powf(std::clamp(std::abs(dotKWind), 0.0f, 1.0f), settings.windAlignmentExponent);
        const float wavelength = 2.0f * HELL_PI / lengthK;
        const float bandWeight = SpectrumBandWeight(wavelength, bandIndex);
        const float L = g_settings.windSpeed * g_settings.windSpeed / g_settings.gravity;

        float phillips = settings.amplitude * expf(-1.0f / (lengthKSquared * L * L)) * directionalWeight / (lengthKSquared * lengthKSquared);

        if (dotKWind < 0.0f) {
            phillips *= settings.opposingWavesDamping;
        }

        return phillips * expf(-lengthKSquared * L * L * settings.smallWavesDamping) * bandWeight;
    }

    std::vector<std::complex<float>> ComputeH0(int bandIndex) {
        const BandSettings& settings = g_settings.bands[bandIndex];
        std::vector<std::complex<float>> h0(FFT_RESOLUTION * FFT_RESOLUTION);
        std::mt19937 randomGen(settings.randomSeed);
        std::normal_distribution<float> normalDist(0.0f, 1.0f);

        // Every frequency gets its own noise
        for (unsigned int z = 0; z < FFT_RESOLUTION; ++z) {
            for (unsigned int x = 0; x < FFT_RESOLUTION; ++x) {
                int idx = z * FFT_RESOLUTION + x;
                glm::vec2 k = KVector(x, z, settings.domainSize);

                if (k == glm::vec2(0.0f)) {
                    h0[idx] = { 0.0f, 0.0f };
                }
                else {
                    float amp = sqrt(PhillipsSpectrum(k, bandIndex)) * HELL_SQRT_OF_HALF;
                    float a = normalDist(randomGen) * amp;
                    float b = normalDist(randomGen) * amp;
                    h0[idx] = { a, b };
                }
            }
        }

        return h0;
    }

    bool BandSpectrumSettingsChanged(int bandIndex) {
        const BandSettings& settings = g_settings.bands[bandIndex];
        const BandSettings& generated = g_generatedSettings.bands[bandIndex];
        if (g_settings.windSpeed != g_generatedSettings.windSpeed) return true;
        if (g_settings.gravity != g_generatedSettings.gravity) return true;
        if (settings.domainSize != generated.domainSize) return true;
        if (settings.minimumWavelength != generated.minimumWavelength) return true;
        if (settings.maximumWavelength != generated.maximumWavelength) return true;
        if (settings.minimumWavelengthFade != generated.minimumWavelengthFade) return true;
        if (settings.maximumWavelengthFade != generated.maximumWavelengthFade) return true;
        if (settings.windDirection.x != generated.windDirection.x || settings.windDirection.y != generated.windDirection.y) return true;
        if (settings.amplitude != generated.amplitude) return true;
        if (settings.windAlignmentExponent != generated.windAlignmentExponent) return true;
        if (settings.opposingWavesDamping != generated.opposingWavesDamping) return true;
        if (settings.smallWavesDamping != generated.smallWavesDamping) return true;
        if (settings.randomSeed != generated.randomSeed) return true;

        // One band moving changes the shared crossover
        for (int i = 0; i < FFT_BAND_COUNT; i++) {
            if (g_settings.bands[i].minimumWavelength != g_generatedSettings.bands[i].minimumWavelength) return true;
            if (g_settings.bands[i].maximumWavelength != g_generatedSettings.bands[i].maximumWavelength) return true;
            if (g_settings.bands[i].minimumWavelengthFade != g_generatedSettings.bands[i].minimumWavelengthFade) return true;
            if (g_settings.bands[i].maximumWavelengthFade != g_generatedSettings.bands[i].maximumWavelengthFade) return true;
        }

        return false;
    }

    void SanitizeSettings() {
        switch (g_settings.displayMode) {
            case DisplayMode::COMBINED:
            case DisplayMode::BAND_0:
            case DisplayMode::BAND_1:
                break;
            default:
                g_settings.displayMode = DisplayMode::COMBINED;
                break;
        }

        g_settings.windSpeed = std::max(g_settings.windSpeed, 0.001f);
        g_settings.gravity = std::max(g_settings.gravity, 0.001f);
        g_settings.displacementScale = std::max(g_settings.displacementScale, 0.0f);
        g_settings.heightScale = std::max(g_settings.heightScale, 0.0f);

        for (int i = 0; i < FFT_BAND_COUNT; i++) {
            BandSettings& settings = g_settings.bands[i];
            settings.domainSize = std::max(settings.domainSize, 0.001f);
            settings.minimumWavelength = std::max(settings.minimumWavelength, 0.0f);
            settings.maximumWavelength = std::max(settings.maximumWavelength, settings.minimumWavelength + 0.001f);
            settings.minimumWavelengthFade = std::max(settings.minimumWavelengthFade, 0.0f);
            settings.maximumWavelengthFade = std::max(settings.maximumWavelengthFade, 0.0f);
            settings.amplitude = std::max(settings.amplitude, 0.0f);
            settings.windAlignmentExponent = std::max(settings.windAlignmentExponent, 0.0f);
            settings.opposingWavesDamping = std::max(settings.opposingWavesDamping, 0.0f);
            settings.smallWavesDamping = std::max(settings.smallWavesDamping, 0.0f);

            float windLength = glm::length(settings.windDirection);
            if (windLength <= 0.0001f) settings.windDirection = glm::vec2(1.0f, 0.0f);
            else if (std::abs(windLength - 1.0f) > 0.0001f) settings.windDirection /= windLength;
        }

        SurfaceSettings& surface = g_settings.surface;
        surface.albedo = glm::max(surface.albedo, glm::vec3(0.0f));
        surface.fogColor = glm::max(surface.fogColor, glm::vec3(0.0f));
        surface.normalScale = std::max(surface.normalScale, 0.0f);
        surface.normalConvergeStartDistance = std::max(surface.normalConvergeStartDistance, 0.0f);
        surface.normalConvergeEndDistance = std::max(surface.normalConvergeEndDistance, surface.normalConvergeStartDistance + 0.001f);
        surface.normalConvergeMaxFactor = std::clamp(surface.normalConvergeMaxFactor, 0.0f, 1.0f);
        surface.normalConvergeExponent = std::max(surface.normalConvergeExponent, 0.001f);
        surface.normalSoftening = std::clamp(surface.normalSoftening, 0.0f, 1.0f);
        surface.rippleTiling = std::max(surface.rippleTiling, 0.0f);
        surface.rippleStrength = std::max(surface.rippleStrength, 0.0f);
        surface.rippleSecondLayerScale = std::max(surface.rippleSecondLayerScale, 0.0f);
        surface.roughness = std::clamp(surface.roughness, 0.001f, 1.0f);
        surface.reflectance = std::clamp(surface.reflectance, 0.0f, 1.0f);
        surface.reflectionGamma = std::max(surface.reflectionGamma, 0.001f);
        surface.diffuseStrength = std::max(surface.diffuseStrength, 0.0f);
        surface.sssHeightRange = std::max(surface.sssHeightRange, 0.001f);
        surface.sssStrength = std::max(surface.sssStrength, 0.0f);
        surface.underwaterSssStrength = std::max(surface.underwaterSssStrength, 0.0f);
        surface.sssRadiusMinimum = std::max(surface.sssRadiusMinimum, 0.001f);
        surface.sssRadiusMaximum = std::max(surface.sssRadiusMaximum, 0.001f);
        surface.sssIntensity = std::max(surface.sssIntensity, 0.0f);
        surface.sssFalloff = std::max(surface.sssFalloff, 0.0f);
        surface.sssSaturation = std::max(surface.sssSaturation, 0.0f);
        surface.fogStartDistance = std::max(surface.fogStartDistance, 0.0f);
        surface.fogEndDistance = std::max(surface.fogEndDistance, surface.fogStartDistance + 0.001f);
        surface.fogExponent = std::max(surface.fogExponent, 0.001f);
        surface.fogStrength = std::max(surface.fogStrength, 0.0f);

        CompositeSettings& composite = g_settings.composite;
        composite.underwaterTint = glm::max(composite.underwaterTint, glm::vec3(0.0f));
        composite.surface.distortionStrength = std::max(composite.surface.distortionStrength, 0.0f);
        composite.surface.distortionTiling = std::max(composite.surface.distortionTiling, 0.0f);
        composite.surface.refractionTintStrength = std::max(composite.surface.refractionTintStrength, 0.0f);
        composite.underwater.rayFogColor = glm::max(composite.underwater.rayFogColor, glm::vec3(0.0f));
        composite.underwater.rayFogStrength = std::max(composite.underwater.rayFogStrength, 0.0f);
        composite.underwater.darknessCurve = std::max(composite.underwater.darknessCurve, 0.001f);
        composite.underwater.distortionStrength = std::max(composite.underwater.distortionStrength, 0.0f);
        composite.underwater.depthTintStrength = std::max(composite.underwater.depthTintStrength, 0.0f);
        composite.underwater.depthTintOriginalWeight = std::clamp(composite.underwater.depthTintOriginalWeight, 0.0f, 1.0f);
        composite.underwater.geometryWaterColorSquaredStrength = std::max(composite.underwater.geometryWaterColorSquaredStrength, 0.0f);
        composite.underwater.geometryWaterColorStrength = std::max(composite.underwater.geometryWaterColorStrength, 0.0f);
        composite.underwater.geometryTintStrength = std::clamp(composite.underwater.geometryTintStrength, 0.0f, 1.0f);
        composite.underwater.openWaterTintStrength = std::clamp(composite.underwater.openWaterTintStrength, 0.0f, 1.0f);
        composite.underwater.openWaterBrightness = std::max(composite.underwater.openWaterBrightness, 0.0f);
    }

    Settings GetSettings() {
        return g_settings;
    }

    void SetSettings(const Settings& settings) {
        g_settings = settings;
        UpdateSpectrum();
    }

    void ResetSettings() {
        SetSettings(CreateDefaultSettings());
    }

    bool LoadFromDisk() {
        nlohmann::json json;
        if (!Hell::Json::LoadFromFile(json, CONFIG_FILE_PATH)) return false;

        Settings loadedSettings = CreateDefaultSettings();
        try {
            ReadIfPresent(json, "simulate", loadedSettings.simulate);
            ReadIfPresent(json, "alternateBandUpdates", loadedSettings.alternateBandUpdates);

            int32_t displayMode = static_cast<int32_t>(loadedSettings.displayMode);
            ReadIfPresent(json, "displayMode", displayMode);
            loadedSettings.displayMode = static_cast<DisplayMode>(displayMode);

            ReadIfPresent(json, "windSpeed", loadedSettings.windSpeed);
            ReadIfPresent(json, "gravity", loadedSettings.gravity);
            ReadIfPresent(json, "displacementScale", loadedSettings.displacementScale);
            ReadIfPresent(json, "heightScale", loadedSettings.heightScale);
            ReadIfPresent(json, "simulationTimeScale", loadedSettings.simulationTimeScale);
            ReadIfPresent(json, "simulationTimeOffset", loadedSettings.simulationTimeOffset);

            const auto bandsIt = json.find("bands");
            if (bandsIt != json.end() && bandsIt->is_array()) {
                for (int i = 0; i < FFT_BAND_COUNT && static_cast<size_t>(i) < bandsIt->size(); i++) {
                    const nlohmann::json& bandJson = bandsIt->at(i);
                    BandSettings& band = loadedSettings.bands[i];
                    ReadIfPresent(bandJson, "domainSize", band.domainSize);
                    ReadIfPresent(bandJson, "minimumWavelength", band.minimumWavelength);
                    ReadIfPresent(bandJson, "maximumWavelength", band.maximumWavelength);
                    ReadIfPresent(bandJson, "minimumWavelengthFade", band.minimumWavelengthFade);
                    ReadIfPresent(bandJson, "maximumWavelengthFade", band.maximumWavelengthFade);
                    ReadVec2IfPresent(bandJson, "windDirection", band.windDirection);
                    ReadIfPresent(bandJson, "amplitude", band.amplitude);
                    ReadIfPresent(bandJson, "windAlignmentExponent", band.windAlignmentExponent);
                    ReadIfPresent(bandJson, "opposingWavesDamping", band.opposingWavesDamping);
                    ReadIfPresent(bandJson, "smallWavesDamping", band.smallWavesDamping);
                    ReadIfPresent(bandJson, "randomSeed", band.randomSeed);
                }
            }

            const auto surfaceIt = json.find("surface");
            if (surfaceIt != json.end() && surfaceIt->is_object()) {
                const nlohmann::json& surfaceJson = *surfaceIt;
                SurfaceSettings& surface = loadedSettings.surface;
                ReadIfPresent(surfaceJson, "specularAntiAliasing", surface.specularAntiAliasing);
                ReadVec3IfPresent(surfaceJson, "albedo", surface.albedo);
                ReadVec3IfPresent(surfaceJson, "fogColor", surface.fogColor);
                ReadIfPresent(surfaceJson, "normalScale", surface.normalScale);
                ReadIfPresent(surfaceJson, "normalConvergeStartDistance", surface.normalConvergeStartDistance);
                ReadIfPresent(surfaceJson, "normalConvergeEndDistance", surface.normalConvergeEndDistance);
                ReadIfPresent(surfaceJson, "normalConvergeMaxFactor", surface.normalConvergeMaxFactor);
                ReadIfPresent(surfaceJson, "normalConvergeExponent", surface.normalConvergeExponent);
                ReadIfPresent(surfaceJson, "normalSoftening", surface.normalSoftening);
                ReadIfPresent(surfaceJson, "rippleTiling", surface.rippleTiling);
                ReadIfPresent(surfaceJson, "rippleStrength", surface.rippleStrength);
                ReadIfPresent(surfaceJson, "rippleSecondLayerScale", surface.rippleSecondLayerScale);
                ReadVec2IfPresent(surfaceJson, "rippleVelocity0", surface.rippleVelocity0);
                ReadVec2IfPresent(surfaceJson, "rippleVelocity1", surface.rippleVelocity1);
                ReadIfPresent(surfaceJson, "roughness", surface.roughness);
                ReadIfPresent(surfaceJson, "reflectance", surface.reflectance);
                ReadIfPresent(surfaceJson, "reflectionGamma", surface.reflectionGamma);
                ReadIfPresent(surfaceJson, "diffuseStrength", surface.diffuseStrength);
                ReadIfPresent(surfaceJson, "sssHeightRange", surface.sssHeightRange);
                ReadIfPresent(surfaceJson, "sssStrength", surface.sssStrength);
                ReadIfPresent(surfaceJson, "underwaterSssStrength", surface.underwaterSssStrength);
                ReadIfPresent(surfaceJson, "sssRadiusMinimum", surface.sssRadiusMinimum);
                ReadIfPresent(surfaceJson, "sssRadiusMaximum", surface.sssRadiusMaximum);
                ReadIfPresent(surfaceJson, "sssIntensity", surface.sssIntensity);
                ReadIfPresent(surfaceJson, "sssFalloff", surface.sssFalloff);
                ReadIfPresent(surfaceJson, "sssSaturation", surface.sssSaturation);
                ReadIfPresent(surfaceJson, "fogStartDistance", surface.fogStartDistance);
                ReadIfPresent(surfaceJson, "fogEndDistance", surface.fogEndDistance);
                ReadIfPresent(surfaceJson, "fogExponent", surface.fogExponent);
                ReadIfPresent(surfaceJson, "fogStrength", surface.fogStrength);
            }

            const auto compositeIt = json.find("composite");
            if (compositeIt != json.end() && compositeIt->is_object()) {
                const nlohmann::json& compositeJson = *compositeIt;
                CompositeSettings& composite = loadedSettings.composite;
                ReadVec3IfPresent(compositeJson, "underwaterTint", composite.underwaterTint);

                const auto surfaceCompositeIt = compositeJson.find("surface");
                if (surfaceCompositeIt != compositeJson.end() && surfaceCompositeIt->is_object()) {
                    const nlohmann::json& surfaceJson = *surfaceCompositeIt;
                    ReadIfPresent(surfaceJson, "planeHeightOffset", composite.surface.planeHeightOffset);
                    ReadIfPresent(surfaceJson, "distortionSpeed", composite.surface.distortionSpeed);
                    ReadIfPresent(surfaceJson, "distortionStrength", composite.surface.distortionStrength);
                    ReadIfPresent(surfaceJson, "distortionTiling", composite.surface.distortionTiling);
                    ReadIfPresent(surfaceJson, "refractionTintStrength", composite.surface.refractionTintStrength);
                }

                const auto underwaterIt = compositeJson.find("underwater");
                if (underwaterIt != compositeJson.end() && underwaterIt->is_object()) {
                    const nlohmann::json& underwaterJson = *underwaterIt;
                    UnderwaterCompositeSettings& underwater = composite.underwater;
                    ReadVec3IfPresent(underwaterJson, "rayFogColor", underwater.rayFogColor);
                    ReadIfPresent(underwaterJson, "rayFogStrength", underwater.rayFogStrength);
                    ReadIfPresent(underwaterJson, "darknessCurve", underwater.darknessCurve);
                    ReadIfPresent(underwaterJson, "distortionSpeed", underwater.distortionSpeed);
                    ReadIfPresent(underwaterJson, "distortionStrength", underwater.distortionStrength);
                    ReadIfPresent(underwaterJson, "depthTintStrength", underwater.depthTintStrength);
                    ReadIfPresent(underwaterJson, "depthTintOriginalWeight", underwater.depthTintOriginalWeight);
                    ReadIfPresent(underwaterJson, "geometryWaterColorSquaredStrength", underwater.geometryWaterColorSquaredStrength);
                    ReadIfPresent(underwaterJson, "geometryWaterColorStrength", underwater.geometryWaterColorStrength);
                    ReadIfPresent(underwaterJson, "geometryTintStrength", underwater.geometryTintStrength);
                    ReadIfPresent(underwaterJson, "openWaterTintStrength", underwater.openWaterTintStrength);
                    ReadIfPresent(underwaterJson, "openWaterBrightness", underwater.openWaterBrightness);
                }
            }
        }
        catch (const nlohmann::json::exception& e) {
            Logging::Error() << "Ocean::LoadFromDisk() failed to read '" << CONFIG_FILE_PATH << "': " << e.what() << "\n";
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
            Logging::Error() << "Ocean::SaveToDisk() failed to create config directory: " << errorCode.message() << "\n";
            return false;
        }

        nlohmann::json bands = nlohmann::json::array();
        for (int i = 0; i < FFT_BAND_COUNT; i++) {
            const BandSettings& band = g_settings.bands[i];
            const nlohmann::json bandJson = {
                { "domainSize", band.domainSize },
                { "minimumWavelength", band.minimumWavelength },
                { "maximumWavelength", band.maximumWavelength },
                { "minimumWavelengthFade", band.minimumWavelengthFade },
                { "maximumWavelengthFade", band.maximumWavelengthFade },
                { "windDirection", { band.windDirection.x, band.windDirection.y } },
                { "amplitude", band.amplitude },
                { "windAlignmentExponent", band.windAlignmentExponent },
                { "opposingWavesDamping", band.opposingWavesDamping },
                { "smallWavesDamping", band.smallWavesDamping },
                { "randomSeed", band.randomSeed }
            };
            bands.push_back(bandJson);
        }

        const SurfaceSettings& surface = g_settings.surface;
        const CompositeSettings& composite = g_settings.composite;
        const SurfaceCompositeSettings& surfaceComposite = composite.surface;
        const UnderwaterCompositeSettings& underwater = composite.underwater;
        const nlohmann::json json = {
            { "simulate", g_settings.simulate },
            { "alternateBandUpdates", g_settings.alternateBandUpdates },
            { "displayMode", static_cast<int32_t>(g_settings.displayMode) },
            { "windSpeed", g_settings.windSpeed },
            { "gravity", g_settings.gravity },
            { "displacementScale", g_settings.displacementScale },
            { "heightScale", g_settings.heightScale },
            { "simulationTimeScale", g_settings.simulationTimeScale },
            { "simulationTimeOffset", g_settings.simulationTimeOffset },
            { "bands", bands },
            { "surface", {
                { "specularAntiAliasing", surface.specularAntiAliasing },
                { "albedo", { surface.albedo.r, surface.albedo.g, surface.albedo.b } },
                { "fogColor", { surface.fogColor.r, surface.fogColor.g, surface.fogColor.b } },
                { "normalScale", surface.normalScale },
                { "normalConvergeStartDistance", surface.normalConvergeStartDistance },
                { "normalConvergeEndDistance", surface.normalConvergeEndDistance },
                { "normalConvergeMaxFactor", surface.normalConvergeMaxFactor },
                { "normalConvergeExponent", surface.normalConvergeExponent },
                { "normalSoftening", surface.normalSoftening },
                { "rippleTiling", surface.rippleTiling },
                { "rippleStrength", surface.rippleStrength },
                { "rippleSecondLayerScale", surface.rippleSecondLayerScale },
                { "rippleVelocity0", { surface.rippleVelocity0.x, surface.rippleVelocity0.y } },
                { "rippleVelocity1", { surface.rippleVelocity1.x, surface.rippleVelocity1.y } },
                { "roughness", surface.roughness },
                { "reflectance", surface.reflectance },
                { "reflectionGamma", surface.reflectionGamma },
                { "diffuseStrength", surface.diffuseStrength },
                { "sssHeightRange", surface.sssHeightRange },
                { "sssStrength", surface.sssStrength },
                { "underwaterSssStrength", surface.underwaterSssStrength },
                { "sssRadiusMinimum", surface.sssRadiusMinimum },
                { "sssRadiusMaximum", surface.sssRadiusMaximum },
                { "sssIntensity", surface.sssIntensity },
                { "sssFalloff", surface.sssFalloff },
                { "sssSaturation", surface.sssSaturation },
                { "fogStartDistance", surface.fogStartDistance },
                { "fogEndDistance", surface.fogEndDistance },
                { "fogExponent", surface.fogExponent },
                { "fogStrength", surface.fogStrength }
            } },
            { "composite", {
                { "underwaterTint", { composite.underwaterTint.r, composite.underwaterTint.g, composite.underwaterTint.b } },
                { "surface", {
                    { "planeHeightOffset", surfaceComposite.planeHeightOffset },
                    { "distortionSpeed", surfaceComposite.distortionSpeed },
                    { "distortionStrength", surfaceComposite.distortionStrength },
                    { "distortionTiling", surfaceComposite.distortionTiling },
                    { "refractionTintStrength", surfaceComposite.refractionTintStrength }
                } },
                { "underwater", {
                    { "rayFogColor", { underwater.rayFogColor.r, underwater.rayFogColor.g, underwater.rayFogColor.b } },
                    { "rayFogStrength", underwater.rayFogStrength },
                    { "darknessCurve", underwater.darknessCurve },
                    { "distortionSpeed", underwater.distortionSpeed },
                    { "distortionStrength", underwater.distortionStrength },
                    { "depthTintStrength", underwater.depthTintStrength },
                    { "depthTintOriginalWeight", underwater.depthTintOriginalWeight },
                    { "geometryWaterColorSquaredStrength", underwater.geometryWaterColorSquaredStrength },
                    { "geometryWaterColorStrength", underwater.geometryWaterColorStrength },
                    { "geometryTintStrength", underwater.geometryTintStrength },
                    { "openWaterTintStrength", underwater.openWaterTintStrength },
                    { "openWaterBrightness", underwater.openWaterBrightness }
                } }
            } }
        };

        return Hell::Json::SaveToFile(json, CONFIG_FILE_PATH);
    }

    const std::string& GetFilePath() {
        return CONFIG_FILE_PATH;
    }

    bool H0UploadRequired(int bandIndex) {
        return g_fftBandData[bandIndex].h0UploadRequired;
    }

    void MarkH0Uploaded(int bandIndex) {
        g_fftBandData[bandIndex].h0UploadRequired = false;
    }

    const float GetDisplacementScale() {
        return g_settings.displacementScale;
    }

    const float GetHeightScale() {
        return g_settings.heightScale;
    }

    float GetAnimationTime() {
        return g_animationTime;
    }

    float UpdateSimulationTime() {
        if (g_settings.simulate) {
            const float deltaTime = Hell::Time::DeltaTime();
            g_animationTime += deltaTime;
            g_simulationTime += deltaTime * g_settings.simulationTimeScale;
        }
        return g_settings.simulationTimeOffset + g_simulationTime;
    }

    DisplayMode GetDisplayMode() {
        return g_settings.displayMode;
    }

    const float GetGravity() {
        return g_settings.gravity;
    }

    const float GetOceanOriginY() {
        return g_oceanOriginY;
    }

    const float GetWaterHeightAtPlayer(int playerIndex) {
        switch (playerIndex) {
            case 0: return g_oceanReadbackData.heightPlayer0;
            case 1: return g_oceanReadbackData.heightPlayer1;
            case 2: return g_oceanReadbackData.heightPlayer2;
            case 3: return g_oceanReadbackData.heightPlayer3;
            default: return g_oceanOriginY;
        }
    }

    const float GetDomainSize(int bandIndex) {
        return g_settings.bands[bandIndex].domainSize;
    }

    const std::vector<std::complex<float>>& GetH0(int bandIndex) {
        return g_fftBandData[bandIndex].h0;
    }

    OceanReadbackData& GetOceanReadBackData() {
        return g_oceanReadbackData;
    }
}
