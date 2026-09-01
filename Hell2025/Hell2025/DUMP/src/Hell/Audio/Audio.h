#pragma once

#include <cstdint>
#include <iostream>
#include <string>
#include <unordered_map>

#include "fmod.hpp"
#include <fmod_errors.h>

namespace Hell::Audio {
    struct AudioHandle {
        FMOD::Sound* sound = nullptr;
        FMOD::Channel* channel = nullptr;
        std::string filename;
        float fadeOutDuration = -1.0f;
        float fadeOutTimeRemaining = 0.0f;
        float decay = 0.0f;

        enum class State {
            PLAYING,
            LOOPING,
            PAUSED,
            FADING_OUT
        } state = State::PLAYING;
    };

    struct AudioEffectInfo {
        std::string filename = "";
        float volume = 0.0f;
    };

    void Init();
    void Update();
    void LoadAudio(const std::string& filename);
    void StopAudio(const std::string& filename);
    uint64_t LoopAudio(const std::string& filename, float volume);
    uint64_t PlayAudio(const std::string& filename, float volume, float frequency = 1.0f);
    uint64_t LoopAudioIfNotPlaying(const std::string& filename, float volume);
    uint64_t PlayAudioIfNotPlaying(const std::string& filename, float volume, float frequency = 1.0f);

    void StopAudio(uint64_t audioId); 
    bool AudioIsPlaying(uint64_t audioId);
    bool AudioIsPlaying(const std::string& filename);
    void FadeOut(uint64_t audioId, float duration);

    float GetVolume(uint64_t audioId);
    void SetVolume(uint64_t audioId, float volume);
};
