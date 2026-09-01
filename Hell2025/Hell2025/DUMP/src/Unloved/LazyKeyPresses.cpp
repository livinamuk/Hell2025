#include "Unloved.h"

#include "Unloved/Render/Renderer.h"

#include "Hell/Audio.h"
#include "Hell/Backend/BackEnd.h"
#include "Hell/Input.h"

#include "Unloved/Debug/Debug.h"
#include "Unloved/Debug/Scratch.h"
#include "Unloved/EditorSession/EditorSession.h"
#include "Unloved/Session/Session.h"

namespace Unloved {

void UpdateLazyKeypresses() {
    if (EditorSession::WantsKeyboardCapture()) return;

    if (Hell::Input::KeyPressed(HELL_KEY_GRAVE_ACCENT)) {
        if (EditorSession::IsActive() && EditorSession::HasMode() && EditorSession::GetMode() == EditorSession::EditorSessionMode::RAGDOLL) return;
        if (EditorSession::IsActive()) { Hell::Audio::PlayAudio(AUDIO_SELECT, 1.0f); EditorSession::Close(); }
        else Debug::ToggleMenuVisiblity();
    }

    // Function keys
    if (Hell::Input::KeyPressed(HELL_KEY_F2)) Session::RequestNewGame(GameMode::CAMPAIGN, "Shit");
    if (Hell::Input::KeyPressed(HELL_KEY_F4)) EditorSession::Open(EditorSession::EditorSessionMode::HOUSE);
    if (Hell::Input::KeyPressed(HELL_KEY_F5)) EditorSession::Open(EditorSession::EditorSessionMode::MAP);;

    // Core
    if (Hell::Input::KeyPressed(HELL_KEY_ESCAPE)) Hell::BackEnd::ForceCloseWindow();
    if (Hell::Input::KeyPressed(HELL_KEY_X))      Hell::BackEnd::ToggleFullscreen();

    // Game
    if (Hell::Input::KeyPressed(HELL_KEY_K)) Unloved::Session::RespawnPlayers();
    if (Hell::Input::KeyPressed(HELL_KEY_3)) {
        const bool topVisible = !Debug::Scratch::GetBool("Mermaid Top", true);
        Debug::Scratch::SetBool("Mermaid Top", topVisible);
        Hell::Audio::PlayAudio(AUDIO_SELECT, 1.00f);
    }

    // Renderer
    if (Renderer::GameIsRendering() && !EditorSession::IsActive()) {
        if (Hell::Input::KeyPressed(HELL_KEY_H))             Renderer::HotloadShaders();
        if (Hell::Input::KeyPressed(HELL_KEY_I))             Renderer::ToggleRagdollRendering();
        if (Hell::Input::KeyPressed(HELL_KEY_M))             Renderer::ToggleScreenSpaceReflections();
        if (Hell::Input::KeyPressed(HELL_KEY_O))             Renderer::ToggleDebugDraw();
        if (Hell::Input::KeyPressed(HELL_KEY_L))             Renderer::ToggleLighting();
        if (Hell::Input::KeyPressed(HELL_KEY_COMMA))         Renderer::TogglePointCloud();
        if (Hell::Input::KeyPressed(HELL_KEY_PERIOD))        Renderer::NextProbeDebugState();
        if (Hell::Input::KeyPressed(HELL_KEY_SLASH))         Renderer::ToggleIrradianceProbeSampling();
        if (Hell::Input::KeyPressed(HELL_KEY_RIGHT_CONTROL)) Renderer::ToggleOverrideState(RendererOverrideState::INDIRECT_SPECULAR_AMD_INPUT);
        if (Hell::Input::KeyPressed(HELL_KEY_RIGHT_ALT))     Renderer::ToggleOverrideState(RendererOverrideState::INDIRECT_SPECULAR_AMD_TEMPORAL);
        if (Hell::Input::KeyPressed(HELL_KEY_N))             Renderer::ToggleOverrideState(RendererOverrideState::VELOCITY);
        if (Hell::Input::KeyPressed(HELL_KEY_RIGHT_SHIFT))   Renderer::ToggleOverrideState(RendererOverrideState::INDIRECT_DIFFUSE);
        if (Hell::Input::KeyPressed(HELL_KEY_DELETE))        Renderer::ToggleOverrideState(RendererOverrideState::VELOCITY);
        if (Hell::Input::KeyPressed(HELL_KEY_APOSTROPHE))    Renderer::TogglePointCloudGrid();
        if (Hell::Input::KeyPressed(HELL_KEY_LEFT_BRACKET))  Renderer::PrevRendererOverrideState();
        if (Hell::Input::KeyPressed(HELL_KEY_RIGHT_BRACKET)) Renderer::NextRendererOverrideState();
        if (Hell::Input::KeyPressed(HELL_KEY_BACKSLASH))     Renderer::NextRendererMode();
    }

    // Editor only
    if (EditorSession::IsInactive()) {
        if (Hell::Input::KeyPressed(HELL_KEY_C)) {
            Session::NextSplitScreenMode();
        }
        if (Hell::Input::KeyPressed(HELL_KEY_1) && Session::GetLocalPlayerCount() >= 1) {
            Session::SetPlayerKeyboardAndMouseIndex(0, 0, 0);
            Session::SetPlayerKeyboardAndMouseIndex(1, 1, 1);
            Session::SetPlayerKeyboardAndMouseIndex(2, 1, 1);
            Session::SetPlayerKeyboardAndMouseIndex(3, 1, 1);
        }
        if (Hell::Input::KeyPressed(HELL_KEY_2) && Unloved::Session::GetLocalPlayerCount() >= 2) {
            Session::SetPlayerKeyboardAndMouseIndex(0, 1, 1);
            Session::SetPlayerKeyboardAndMouseIndex(1, 0, 0);
            Session::SetPlayerKeyboardAndMouseIndex(2, 1, 1);
            Session::SetPlayerKeyboardAndMouseIndex(3, 1, 1);
        }
        if (Hell::Input::KeyPressed(HELL_KEY_3) && Unloved::Session::GetLocalPlayerCount() >= 3) {
            Session::SetPlayerKeyboardAndMouseIndex(0, 1, 1);
            Session::SetPlayerKeyboardAndMouseIndex(1, 1, 1);
            Session::SetPlayerKeyboardAndMouseIndex(2, 0, 0);
            Session::SetPlayerKeyboardAndMouseIndex(3, 1, 1);
        }
        if (Hell::Input::KeyPressed(HELL_KEY_4) && Unloved::Session::GetLocalPlayerCount() >= 4) {
            Session::SetPlayerKeyboardAndMouseIndex(0, 1, 1);
            Session::SetPlayerKeyboardAndMouseIndex(1, 1, 1);
            Session::SetPlayerKeyboardAndMouseIndex(2, 1, 1);
            Session::SetPlayerKeyboardAndMouseIndex(3, 0, 0);
        }
        if (Hell::Input::KeyPressed(HELL_KEY_B)) {
            Hell::Audio::PlayAudio(AUDIO_SELECT, 1.00f);
            Debug::NextDebugRenderMode();
        }
    }
}

}
