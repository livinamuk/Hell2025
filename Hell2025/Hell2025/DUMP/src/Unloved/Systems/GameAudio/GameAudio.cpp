#include "GameAudio.h"

#include "Hell/Audio.h"
#include "Hell/Time.h"

#include "Unloved/EditorSession/EditorSession.h"
#include "Unloved/Player/Player.h"

#include "Unloved/Session/Session.h"

#include <algorithm>
#include <cstdlib>
#include <string>
#include <vector>

namespace Unloved::GameAudio {
    bool g_glassHitAudioPlayedThisFrame = false;
    float g_fleshImpactAudioTimer = 0.0f;
    constexpr float g_fleshImpactAudioDelay = 0.2f;

    void PlayFleshImpactAudio();

    void BeginFrame() {
        g_glassHitAudioPlayedThisFrame = false;
        float deltaTime = Hell::Time::DeltaTime();
        g_fleshImpactAudioTimer -= deltaTime;
        g_fleshImpactAudioTimer = std::max(g_fleshImpactAudioTimer, 0.0f);
    }

    void Update() {
        bool playersUnderWater = false;
        bool playersWading = false;

        for (int32_t i = 0; i < Session::GetLocalPlayerCount(); i++) {
            Unloved::Player* player = Session::GetLocalPlayerByViewportIndex(i);
            if (!player) {
                continue;
            }

            if (player->CameraIsUnderwater() && player->ViewportIsVisible() && player->IsAlive()) {
                playersUnderWater = true;
            }

            if (player->IsWading()) {
                playersWading = true;
            }
        }

        if (EditorSession::IsActive()) {
            playersUnderWater = false;
            playersWading = false;
        }

        if (playersUnderWater && Session::GetSessionTime() > 1.0f) {
            Hell::Audio::LoopAudioIfNotPlaying("Water_AmbientLoop.wav", 1.0);
        }
        else {
            Hell::Audio::StopAudio("Water_AmbientLoop.wav");
        }

        if (playersWading) {
            Hell::Audio::LoopAudioIfNotPlaying("Water_PaddlingLoop_1.wav", 1.0);
        }
        else {
            Hell::Audio::StopAudio("Water_PaddlingLoop_1.wav");
        }
    }

    void PlayGlassHitAudio() {
        if (!g_glassHitAudioPlayedThisFrame) {
            Hell::Audio::PlayAudio("GlassImpact.wav", 2.0f);
        }
        g_glassHitAudioPlayedThisFrame = true;
    }

    void PlayFootstepIndoorAudio() {
        const std::vector<const char*> indoorFootstepFilenames = {
                    "player_step_1.wav",
                    "player_step_2.wav",
                    "player_step_3.wav",
                    "player_step_4.wav",
        };
        int random = rand() % 4;
        Hell::Audio::PlayAudio(indoorFootstepFilenames[random], 0.5f);
    }

    void PlayFootstepOutdoorAudio() {
        const std::vector<const char*> indoorFootstepFilenames = {
                "player_step_grass_1.wav",
                "player_step_grass_2.wav",
                "player_step_grass_3.wav",
                "player_step_grass_4.wav",
        };
        int random = rand() % 4;
        Hell::Audio::PlayAudio(indoorFootstepFilenames[random], 0.5f);
    }

    void TryPlayFleshImpactAudio() {
        if (g_fleshImpactAudioTimer > 0.0f) {
            return;
        }

        g_fleshImpactAudioTimer = g_fleshImpactAudioDelay;
        PlayFleshImpactAudio();
    }

    void PlayFleshImpactAudio() {
        const std::vector<std::string> filenames = {
                "FLY_Bullet_Impact_Flesh_00.wav",
                "FLY_Bullet_Impact_Flesh_01.wav",
                "FLY_Bullet_Impact_Flesh_02.wav",
                "FLY_Bullet_Impact_Flesh_03.wav",
                "FLY_Bullet_Impact_Flesh_04.wav",
                "FLY_Bullet_Impact_Flesh_05.wav",
                "FLY_Bullet_Impact_Flesh_06.wav",
                "FLY_Bullet_Impact_Flesh_07.wav"
        };

        int random = rand() % filenames.size();
        Hell::Audio::PlayAudio(filenames[random], 1.0f);
    }
}
