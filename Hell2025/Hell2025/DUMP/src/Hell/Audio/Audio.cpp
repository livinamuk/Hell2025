#include "Audio.h"

#include "Hell/Logging.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace Hell::Audio {
    std::unordered_map<std::string, FMOD::Sound*> g_loadedAudio;
    std::unordered_map<uint64_t, AudioHandle> g_playingAudio;
    std::vector<std::string> g_audioPlayedThisFrame;
    constexpr int AUDIO_CHANNEL_COUNT = 512;
    constexpr uint64_t INVALID_AUDIO_ID = 0;
    FMOD::System* g_system;
    uint64_t g_nextAudioId = 1;

    uint64_t GetNextAudioId() {
        uint64_t audioId = g_nextAudioId++;
        if (g_nextAudioId == INVALID_AUDIO_ID) {
            g_nextAudioId = 1;
        }
        return audioId;
    }

    void Init() {
        Logging::Init() << "Initialized audio";

        // Create the main system object
        FMOD_RESULT result = FMOD::System_Create(&g_system);
        if (result != FMOD_OK) {
            std::cerr << "FMOD: Failed to create system object: " << FMOD_ErrorString(result) << "\n";
            g_system = nullptr;
            return;
        }

        unsigned int dspBufferLength = 512;
        int dspNumBuffers = 4;

        //std::cout << "FMOD: Setting DSP buffer size to " << dspBufferLength << " samples * " << dspNumBuffers << " buffers\n";
        result = g_system->setDSPBufferSize(dspBufferLength, dspNumBuffers);
        if (result != FMOD_OK) {
            std::cerr << "FMOD: Warning - Failed to set custom DSP buffer size: " << FMOD_ErrorString(result) << "\n";
        }
  
        result = g_system->init(AUDIO_CHANNEL_COUNT, FMOD_INIT_NORMAL, nullptr);
        if (result != FMOD_OK) {
            std::cerr << "FMOD: Failed to initialize system object: " << FMOD_ErrorString(result) << "\n";
            // Clean up system if init fails
            if (g_system) {
                g_system->release();
                g_system = nullptr;
            }
            return;
        }

        //std::cout << "FMOD: System initialized successfully.\n";

        // Create the channel group.
        FMOD::ChannelGroup* channelGroup = nullptr; // Should this be stored globally or returned?
        result = g_system->createChannelGroup("inGameSoundEffects", &channelGroup);
        if (result != FMOD_OK) {
            // Log the error, but initialization itself was successful.
            std::cerr << "FMOD: Warning - Failed to create in-game sound effects channel group: " << FMOD_ErrorString(result) << "\n";
        }
    }

    void LoadAudio(const std::string& filename) {
        FMOD_MODE eMode = FMOD_DEFAULT;
        FMOD::Sound* sound = nullptr;
        g_system->createSound(("res/audio/" + filename).c_str(), eMode, nullptr, &sound);
        g_loadedAudio[filename] = sound;
    }

    void FMOD_ERRCHECK(FMOD_RESULT result) {
        if (result != FMOD_OK) {
            std::cout << "FMOD error! (" << result << ") " << FMOD_ErrorString(result) << "\n";
        }
    }

    void Update() {
        const float deltaTime = 1.0f / 60.0f;

        auto it = g_playingAudio.begin();
        while (it != g_playingAudio.end()) {
            uint64_t audioId = it->first;
            AudioHandle& handle = it->second;

            // Fade out any fading out audio
            if (handle.state == AudioHandle::State::FADING_OUT) {
                handle.fadeOutTimeRemaining -= deltaTime;
                float t = std::clamp(handle.fadeOutTimeRemaining / handle.fadeOutDuration, 0.0f, 1.0f);
                float fadedVolume = GetVolume(audioId) * powf(0.001f, 1.0f - t);
                SetVolume(audioId, fadedVolume);

                if (handle.fadeOutTimeRemaining <= 0.0f) {
                    handle.channel->stop();
                }
            }

            // Remove completed audio
            if (handle.channel) {
                bool isPlaying = false;
                FMOD_RESULT result = handle.channel->isPlaying(&isPlaying);
                if (result == FMOD_OK && !isPlaying) {
                    it = g_playingAudio.erase(it);
                }
                else if (result == FMOD_ERR_INVALID_HANDLE || result == FMOD_ERR_CHANNEL_STOLEN) {
                    it = g_playingAudio.erase(it);
                }
                else if (result != FMOD_OK) {
                    FMOD_ERRCHECK(result);
                    it = g_playingAudio.erase(it);
                }
                else {
                    it++;
                }
            }
            else {
                it = g_playingAudio.erase(it);
            }
        }

        // Update FMOD internal hive mind
        g_system->update();
        g_audioPlayedThisFrame.clear();
    }

    uint64_t PlayAudioIfNotPlaying(const std::string& filename, float volume, float frequency) {
        // Check if sound is already playing
        for (const auto& pair : g_playingAudio) {
            const AudioHandle& handle = pair.second;
            if (handle.filename == filename) {
                return 0; // Ummm, make sure you don't depend on this actually being a value
            }
        }
        // Otherwise, play the sound
        return PlayAudio(filename, volume, frequency);
    }

    uint64_t PlayAudio(const std::string& filename, float volume, float frequency) {
        // Load if needed
        if (g_loadedAudio.find(filename) == g_loadedAudio.end()) {
            LoadAudio(filename);
        }

        // Skip if this sound is already playing
        for (const std::string& existingFilename : g_audioPlayedThisFrame) {
            if (existingFilename == filename) {
                return 0;
            }
        }

        uint64_t uniqueId = GetNextAudioId();
        AudioHandle& handle = g_playingAudio[uniqueId];
        handle.state = AudioHandle::State::PLAYING;
        handle.sound = g_loadedAudio[filename];
        handle.filename = filename;
        
        FMOD_RESULT result = g_system->playSound(handle.sound, nullptr, false, &handle.channel);
        FMOD_ERRCHECK(result);

        if (result == FMOD_OK && handle.channel) {
            handle.channel->setVolume(volume);
            float currentFrequency = 0.0f;
            // Consider adding error checks for get/setFrequency too if needed
            result = handle.channel->getFrequency(&currentFrequency);
            FMOD_ERRCHECK(result);
            result = handle.channel->setFrequency(currentFrequency * frequency);
            FMOD_ERRCHECK(result);
        }
        else {
            std::cerr << "FMOD Error: Failed to play sound " << filename << " or got null channel.\n";
            // Cleanup the handle created if playback failed
            g_playingAudio.erase(uniqueId);
            return 0;
        }

        g_audioPlayedThisFrame.push_back(filename);
        return uniqueId;
    }


    uint64_t LoopAudioIfNotPlaying(const std::string& filename, float volume) {
        // Check if an instance of this filename is already looping
        for (const auto& pair : g_playingAudio) {
            const AudioHandle& handle = pair.second;
            if (handle.filename == filename && handle.state == AudioHandle::State::LOOPING) {
                return 0; // Already looping, so do nothing

                // REPLACE THIS 0 RETURN WITH THE ORIGINAL ID OF THE SOUND ALREADY PLAYING
                // REPLACE THIS 0 RETURN WITH THE ORIGINAL ID OF THE SOUND ALREADY PLAYING
                // REPLACE THIS 0 RETURN WITH THE ORIGINAL ID OF THE SOUND ALREADY PLAYING
                // REPLACE THIS 0 RETURN WITH THE ORIGINAL ID OF THE SOUND ALREADY PLAYING
                // REPLACE THIS 0 RETURN WITH THE ORIGINAL ID OF THE SOUND ALREADY PLAYING
                // REPLACE THIS 0 RETURN WITH THE ORIGINAL ID OF THE SOUND ALREADY PLAYING
                // REPLACE THIS 0 RETURN WITH THE ORIGINAL ID OF THE SOUND ALREADY PLAYING

            }
        }

        // If not looping, start it
        return LoopAudio(filename, volume);
    }

    uint64_t LoopAudio(const std::string& filename, float volume) {
        // Load if needed
        if (g_loadedAudio.find(filename) == g_loadedAudio.end()) LoadAudio(filename);

        uint64_t id = GetNextAudioId();
        AudioHandle& h = g_playingAudio[id];
        h.state = AudioHandle::State::LOOPING;
        h.sound = g_loadedAudio[filename];
        h.filename = filename;

        // Start paused to configure the channel safely
        FMOD_RESULT r = g_system->playSound(h.sound, nullptr, true, &h.channel);
        if (r != FMOD_OK || !h.channel) { std::cout << "playSound failed\n"; return 0; }

        // Per channel loop config
        h.channel->setMode(FMOD_LOOP_NORMAL);
        h.channel->setLoopCount(-1);
        h.channel->setVolume(volume);

        // Unpause to play
        h.channel->setPaused(false);
        return id;
    }

    void StopAudio(const std::string& filename) {
        for (const auto& pair : g_playingAudio) {
            const AudioHandle& handle = pair.second;
            if (handle.filename == filename) {
                FMOD_RESULT result = handle.channel->stop();
                if (result != FMOD_OK) {
                    std::cerr << "FMOD error: " << FMOD_ErrorString(result) << "\n";
                }
                else {
                    //std::cout << "Sound stopped.\n";
                }
            }
        }
    }

    void StopAudio(uint64_t audioId) {
        if (AudioIsPlaying(audioId)) {
            AudioHandle& handle = g_playingAudio[audioId];
            handle.channel->stop(); 
            g_playingAudio.erase(audioId);
        }
    }

    bool AudioIsPlaying(uint64_t audioId) {
        return (g_playingAudio.find(audioId) != g_playingAudio.end());
    }

    bool AudioIsPlaying(const std::string& filename) {
        for (auto& [key, val] : g_playingAudio) {
            AudioHandle& handle = val;
            if (handle.filename == filename) {
                return true;
            }
        }
        return false;
    }

    void FadeOut(uint64_t audioId, float duration) {
        if (!AudioIsPlaying(audioId)) return;

        AudioHandle& handle = g_playingAudio[audioId];

        if (handle.state == AudioHandle::State::PLAYING) {
            handle.state = AudioHandle::State::FADING_OUT;
            handle.fadeOutDuration = duration;
            handle.fadeOutTimeRemaining = duration;
        }
    }

    void SetVolume(uint64_t audioId, float volume) {
        if (!AudioIsPlaying(audioId)) {
            //std::cout << "Audio::SetVoulme() failed coz " << audioId << " was not found\n";
            return;
        }

        AudioHandle& handle = g_playingAudio[audioId];
        if (handle.channel) {
            handle.channel->setVolume(volume);
        }
    }

    float GetVolume(uint64_t audioId) {
        float result = 0.0f;
        if (!AudioIsPlaying(audioId)) return result;

        AudioHandle& handle = g_playingAudio[audioId];
        if (handle.channel) {
            handle.channel->getVolume(&result);
        }

        return result;
    }
}
