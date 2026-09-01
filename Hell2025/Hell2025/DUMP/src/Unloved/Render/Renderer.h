#pragma once
#include "Hell/Math/AABB.h"
#include "Hell/Math/OBB.h"

#include "Unloved/Render/RendererEnums.h"
#include "Unloved/Render/Renderer_settings.h"
#include "Unloved/Common/Types.h"

namespace Unloved::Renderer {
    void Init();
    void InitMain();
    void WaitIdle();
    void CleanUp();
    void InitWoundMaskArray();
    void RenderLoadingScreen();
    void RenderBlackFrame();
    void PreGameLogicComputePasses();
    void RenderGame();
    void HotloadShaders();

    // Override states
    void SetRendererOverrideState(RendererOverrideState state);
    void PrevRendererOverrideState();
    void NextRendererOverrideState();
    bool OverrideStateUsesDebugViewPass();
    bool OverrideStateUsesDebugTileViewPass();

    void SetProbeDebugState(ProbeDebugState state);
	void NextProbeDebugState();

    // Debug toggles
    void ToggleDebugDraw();
    void ToggleLighting();
    void ToggleOverrideState(RendererOverrideState state);
    void ToggleIrradianceProbeSampling();
    void TogglePointCloud();
    void TogglePointCloudGrid();
    void ToggleRagdollRendering();
    void ToggleScreenSpaceReflections();

    void NextRendererMode();
	void SetRendererMode(RendererMode rendererMode);
	RendererMode GetRendererMode();

    int32_t GetNextFreeWoundMaskIndexAndMarkItTaken();
    void MarkWoundMaskIndexAsAvailable(int32_t index);

    void RecalculateAllHeightMapData(bool uploadWorldHeightData, bool updatePhysics = true);

	uint32_t GetTileCount();
	uint32_t GetTileCountX();
	uint32_t GetTileCountY();

    // TODO: move me to Renderer_settings.h

    // TODO: move me to Renderer_settings.h

    const std::string& GetZoneNames();
    const std::string& GetZoneGPUTimings();
    const std::string& GetZoneCPUTimings();
    const std::string& GetTotalGPUTime();
    const std::string& GetTotalCPUTime();
    float GetTotalGPUTimeFloat();

    bool GameIsRendering();
}
