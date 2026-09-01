#pragma once

#include <cstdint>
#include <string>

namespace Config::Physics {

    struct Settings {
        bool gravityEnabled = true;
        uint32_t substeps = 4;
        uint32_t positionIterations = 1;
        uint32_t velocityIterations = 1;
        float bulletImpactImpulse = 8.0f;
        float shotgunPelletImpactImpulse = 2.5f;
        float meleeImpactImpulse = 8.0f;
        float ragdollImpactTranslationScale = 5.0f;
        float ragdollImpactRotationScale = 0.8f;
        float defaultMaterialStaticFriction = 1.0f;
        float defaultMaterialDynamicFriction = 0.85f;
        float defaultMaterialRestitution = 0.0f;
    };

    const Settings& GetSettings();
    void SetSettings(const Settings& settings);
    void ResetSettings();

    bool LoadFromDisk();
    bool SaveToDisk();
    const std::string& GetFilePath();
}
