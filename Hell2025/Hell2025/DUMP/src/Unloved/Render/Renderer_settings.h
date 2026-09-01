#pragma once

#include "Renderer.h"
#include "RendererEnums.h"

struct RendererSettings {
    int depthPeelCount = 3;
    bool drawGrass = true;
    bool screenspaceReflections = true;
    bool debugDrawPointCloud = false;
    bool debugDrawPointCloudGrid = false;
    bool debugDrawIrradianceProbes = false;
    bool debugDrawNavMesh = false;
    bool debugDrawRagdolls = false;
    bool enableDDGI = true;
    bool enableDDGIReflections = false;
    bool enableIndirectSpecular = true;
    bool enableFXAA = true;
    bool enableTAA = true;
    bool enableLighting = true;
    bool enableStaticShadowMapCaching = true;
    DirectPointShadowMode directPointShadowMode = DirectPointShadowMode::SHADOW_MAP;
    int taaJitterScale = 2;
    float emissiveStrength = 1.0f;
    float irradianceDampening = 0.0325f;
    float indirectSpecularFactor = 1.3f;
    float indirectSpecularRoughnessDampening = 0.3f;
    IndirectSpecularRaysPerQuad indirectSpecularRaysPerQuad = IndirectSpecularRaysPerQuad::ONE;
    RendererOverrideState rendererOverrideState = RendererOverrideState::NONE;
    ProbeDebugState probeDebugState = ProbeDebugState::HIDDEN;
};

namespace Unloved::Renderer {
    void ResetCurrentRendererSettings();
    void SetCurrentRendererSettings(const RendererSettings& settings);
    RendererSettings& GetCurrentRendererSettings();

    bool DDGIEnabled();
    bool IndirectSpecularEnabled();
    uint32_t GetIndirectSpecularRaysPerQuad();
}
