#include "Renderer.h"
#include "Renderer_settings.h"

#include "Hell/Audio.h"
#include "Hell/Common/Enum.h"

#include "Unloved/Common/Constants.h"
#include "Unloved/Debug/Debug.h"
#include "Unloved/EditorSession/EditorSession.h"

namespace Unloved::Renderer {
    struct RendererSettingsSet {
        RendererSettings game;
        RendererSettings houseEditor;
        RendererSettings mapHeightEditor;
        RendererSettings mapObjectEditor;
    } g_rendererSettingsSet;

    RendererMode g_rendererMode = RendererMode::RE_STYLE;

    RendererSettings& GetCurrentRendererSettings() {
        if (!EditorSession::IsActive() || !EditorSession::HasMode()) return g_rendererSettingsSet.game;

        if (EditorSession::GetMode() == EditorSession::EditorSessionMode::HOUSE) return g_rendererSettingsSet.houseEditor;
        if (EditorSession::GetMode() == EditorSession::EditorSessionMode::MAP) {
            return EditorSession::IsHeightMapEditorActive() ? g_rendererSettingsSet.mapHeightEditor : g_rendererSettingsSet.mapObjectEditor;
        }
        return g_rendererSettingsSet.game;
    }

    void SetCurrentRendererSettings(const RendererSettings& settings) {
        RendererSettings& currentSettings = GetCurrentRendererSettings();
        currentSettings = settings;
    }

    void ResetCurrentRendererSettings() {
        RendererSettings& currentSettings = GetCurrentRendererSettings();
        currentSettings = RendererSettings();
    }

    void ToggleLighting() {
        RendererSettings& rendererSettings = GetCurrentRendererSettings();
        rendererSettings.enableLighting = !rendererSettings.enableLighting;

        std::string onOff = rendererSettings.enableLighting ? "ON" : "OFF";
        Debug::BlitQuickDebugMessage("Lighting: " + onOff);

        Hell::Audio::PlayAudio(AUDIO_SELECT, 1.00f);
    }

    void TogglePointCloud() {
        RendererSettings& rendererSettings = GetCurrentRendererSettings();
        rendererSettings.debugDrawPointCloud = !rendererSettings.debugDrawPointCloud;

        if (rendererSettings.debugDrawPointCloud) {
            rendererSettings.debugDrawPointCloudGrid = false;
        }

        std::string onOff = rendererSettings.debugDrawPointCloud ? "ON" : "OFF";
        Debug::BlitQuickDebugMessage("Point Cloud: " + onOff);

        Hell::Audio::PlayAudio(AUDIO_SELECT, 1.00f);
    }

    void TogglePointCloudGrid() {
        RendererSettings& rendererSettings = GetCurrentRendererSettings();
        rendererSettings.debugDrawPointCloudGrid = !rendererSettings.debugDrawPointCloudGrid;

        if (rendererSettings.debugDrawPointCloudGrid) {
            rendererSettings.debugDrawPointCloud = false;
        }

        std::string onOff = rendererSettings.debugDrawPointCloudGrid ? "ON" : "OFF";
        Debug::BlitQuickDebugMessage("Point Cloud Grid: " + onOff);

        Hell::Audio::PlayAudio(AUDIO_SELECT, 1.00f);
    }


    void ToggleRagdollRendering() {
        RendererSettings& rendererSettings = GetCurrentRendererSettings();
        rendererSettings.debugDrawRagdolls = !rendererSettings.debugDrawRagdolls;

        std::string onOff = rendererSettings.debugDrawRagdolls ? "ON" : "OFF";
        Debug::BlitQuickDebugMessage("Draw Ragdolls: " + onOff);

        Hell::Audio::PlayAudio(AUDIO_SELECT, 1.00f);
    }

    void ToggleDebugDraw() {
        RendererSettings& rendererSettings = GetCurrentRendererSettings();
        rendererSettings.debugDrawNavMesh = !rendererSettings.debugDrawNavMesh;

        std::string onOff = rendererSettings.debugDrawNavMesh ? "ON" : "OFF";
        Debug::BlitQuickDebugMessage("Nav Mesh: " + onOff);

        Hell::Audio::PlayAudio(AUDIO_SELECT, 1.00f);
    }

    void ToggleScreenSpaceReflections() {
        RendererSettings& rendererSettings = GetCurrentRendererSettings();
        rendererSettings.screenspaceReflections = !rendererSettings.screenspaceReflections;

        std::string onOff = rendererSettings.screenspaceReflections ? "ON" : "OFF";
        Debug::BlitQuickDebugMessage("Screenspace Reflections: " + onOff);

        Hell::Audio::PlayAudio(AUDIO_SELECT, 1.00f);
    }

    void ToggleIrradianceProbeSampling() {
        RendererSettings& rendererSettings = GetCurrentRendererSettings();
        rendererSettings.enableDDGI = !rendererSettings.enableDDGI;

        std::string onOff = rendererSettings.enableDDGI ? "ON" : "OFF";
        Debug::BlitQuickDebugMessage("Irradiance Probe Sampling: " + onOff);

        Hell::Audio::PlayAudio(AUDIO_SELECT, 1.00f);
    }

    void ToggleOverrideState(RendererOverrideState state) {
        RendererSettings& rendererSettings = GetCurrentRendererSettings();
        if (rendererSettings.rendererOverrideState == state) {
            SetRendererOverrideState(RendererOverrideState::NONE);
        }
        else {
            SetRendererOverrideState(state);
        }

        Hell::Audio::PlayAudio(AUDIO_SELECT, 1.00f);
    }

    void SetRendererOverrideState(RendererOverrideState state) {
        RendererSettings& rendererSettings = GetCurrentRendererSettings();
        rendererSettings.rendererOverrideState = state;

        Debug::BlitQuickDebugMessage("Renderer Override State: " + Hell::Enum::ToString(state));
        Hell::Audio::PlayAudio(AUDIO_SELECT, 1.00f);
    }

    void PrevRendererOverrideState() {
        RendererSettings& rendererSettings = GetCurrentRendererSettings();
        int stateCount = static_cast<int>(RendererOverrideState::STATE_COUNT);
        int i = static_cast<int>(rendererSettings.rendererOverrideState);
        i = (i - 1 + stateCount) % stateCount;

        SetRendererOverrideState(static_cast<RendererOverrideState>(i));
    }

    void NextRendererOverrideState() {
        RendererSettings& rendererSettings = GetCurrentRendererSettings();
        int stateCount = static_cast<int>(RendererOverrideState::STATE_COUNT);
        int i = static_cast<int>(rendererSettings.rendererOverrideState);
        i = (i + 1) % stateCount;

        SetRendererOverrideState(static_cast<RendererOverrideState>(i));
    }

    bool OverrideStateUsesDebugViewPass() {
        const RendererSettings& rendererSettings = GetCurrentRendererSettings();

        switch (rendererSettings.rendererOverrideState) {
            case RendererOverrideState::BASE_COLOR:
            case RendererOverrideState::NORMALS:
            case RendererOverrideState::RMA:
            case RendererOverrideState::ROUGHNESS:
            case RendererOverrideState::METALIC:
            case RendererOverrideState::AO:
            case RendererOverrideState::CAMERA_NDOTL:
            case RendererOverrideState::INDIRECT_DIFFUSE:
            case RendererOverrideState::HIZ:
            case RendererOverrideState::OCCLUSION_HIZ:
            case RendererOverrideState::INDIRECT_SPECULAR_AMD_SAMPLE_COUNT:
            case RendererOverrideState::INDIRECT_SPECULAR_AMD_INPUT:
            case RendererOverrideState::INDIRECT_SPECULAR_AMD_REPROJECTED:
            case RendererOverrideState::INDIRECT_SPECULAR_AMD_PREFILTERED:
            case RendererOverrideState::INDIRECT_SPECULAR_AMD_PREFILTERED_VARIANCE:
            case RendererOverrideState::INDIRECT_SPECULAR_AMD_TEMPORAL:
            case RendererOverrideState::INDIRECT_SPECULAR_AMD_TEMPORAL_VARIANCE:
            case RendererOverrideState::VELOCITY:
            case RendererOverrideState::VIS_BUFFER:
            case RendererOverrideState::DEPTH:
            case RendererOverrideState::WORLD_POSITION:
            case RendererOverrideState::EMISSIVE:
                return true;
            default:
                return false;
        }
    }

    bool OverrideStateUsesDebugTileViewPass() {
        const RendererSettings& rendererSettings = GetCurrentRendererSettings();

        switch (rendererSettings.rendererOverrideState) {
        case RendererOverrideState::TILE_HEATMAP_LIGHTS:
        case RendererOverrideState::TILE_HEATMAP_BLOOD_DECALS:
        case RendererOverrideState::TILE_HEATMAP_CHRISTMAS_LIGHTS:
            return true;
        default:
            return false;
        }
    }

	void NextProbeDebugState() {
		RendererSettings& rendererSettings = GetCurrentRendererSettings();
		int i = static_cast<int>(rendererSettings.probeDebugState);
		i = (i + 1) % static_cast<int>(ProbeDebugState::STATE_COUNT);

		SetProbeDebugState(static_cast<ProbeDebugState>(i));
    }

	void NextRendererMode() {
		int i = static_cast<int>(g_rendererMode);
		i = (i + 1) % static_cast<int>(RendererMode::RENDERER_MODE_COUNT);
		SetRendererMode(static_cast<RendererMode>(i));
	}

    void SetRendererMode(RendererMode rendererMode) {
        g_rendererMode = rendererMode;

        Debug::BlitQuickDebugMessage("Renderer Mode: " + Hell::Enum::ToString(rendererMode));
        Hell::Audio::PlayAudio(AUDIO_SELECT, 1.00f);
	}

    RendererMode GetRendererMode() {
        return g_rendererMode;
    }

	void SetProbeDebugState(ProbeDebugState state) {
		RendererSettings& rendererSettings = GetCurrentRendererSettings();
		rendererSettings.probeDebugState = state;

        rendererSettings.debugDrawIrradianceProbes = rendererSettings.probeDebugState != ProbeDebugState::HIDDEN;

        Debug::BlitQuickDebugMessage("Irradiance Probes: " + Hell::Enum::ToString(state));
        Hell::Audio::PlayAudio(AUDIO_SELECT, 1.00f);
	}

    bool DDGIEnabled()             { return GetCurrentRendererSettings().enableDDGI; }
    bool IndirectSpecularEnabled() { return GetCurrentRendererSettings().enableIndirectSpecular; }
    bool FXAAEnabled()             { return GetCurrentRendererSettings().enableFXAA; }
    bool TAAEnabled()              { return GetCurrentRendererSettings().enableTAA; }

    uint32_t GetIndirectSpecularRaysPerQuad() {
        switch (Unloved::Renderer::GetCurrentRendererSettings().indirectSpecularRaysPerQuad) {
            case IndirectSpecularRaysPerQuad::ONE:  return 1;
            case IndirectSpecularRaysPerQuad::TWO:  return 2;
            case IndirectSpecularRaysPerQuad::FOUR: return 4;
        }
        return 4;
    }

}
