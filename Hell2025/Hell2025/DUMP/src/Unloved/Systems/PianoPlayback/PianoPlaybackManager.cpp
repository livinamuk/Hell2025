#include "PianoPlaybackManager.h"

#include "Hell/Input.h"
#include "Hell/Logging.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/ResourceManagement/Types/MidiFile.h"
#include "Hell/Common/String.h"
#include "Hell/Time.h"
#include "Unloved/Objects/Interior/Piano.h"
#include "Unloved/Player/Player.h"
#include "Unloved/Session/Session.h"
#include "Unloved/World/World.h"

#include <array>
#include <string>
#include <vector>

namespace Input = Hell::Input;
namespace Time = Hell::Time;

namespace Unloved::PianoPlaybackManager {

    namespace {
        constexpr std::array<const char*, 3> TRACK_NAMES = {
            "Goat",
            "Nocturne",
            "Czardas"
        };

        const Hell::MidiFile* g_midiFile = nullptr;
        size_t g_nextEventIndex = 0;
        double g_playbackTime = 0.0;
        bool g_playing = false;

        std::vector<std::string> g_debugTextTime;
        std::vector<std::string> g_debugTextEvents;
        std::vector<std::string> g_debugTextTimeDurations;
        std::vector<std::string> g_debugTextTimeVelocities;

        void ClearDebugText() {
            g_debugTextTime.clear();
            g_debugTextEvents.clear();
            g_debugTextTimeDurations.clear();
            g_debugTextTimeVelocities.clear();
        }

        void AddRollingDebugText(std::vector<std::string>& textSet, const std::string& text) {
            textSet.push_back(text);
            while (textSet.size() > 10) {
                textSet.erase(textSet.begin());
            }
        }

        std::string JoinDebugText(const std::vector<std::string>& textSet) {
            std::string result;
            for (const std::string& text : textSet) {
                result += text + "\n";
            }
            return result;
        }

        Piano* GetPlaybackPiano() {
            Player* player = Session::GetLocalPlayerByViewportIndex(0);
            if (!player) return nullptr;

            Piano* closestPiano = nullptr;
            float closestDistanceSquared = 0.0f;
            for (Piano& piano : World::GetPianos()) {
                const glm::vec3 offset = piano.GetPosition() - player->GetFootPosition();
                const float distanceSquared = glm::dot(offset, offset);
                if (!closestPiano || distanceSquared < closestDistanceSquared) {
                    closestPiano = &piano;
                    closestDistanceSquared = distanceSquared;
                }
            }
            return closestPiano;
        }
    }

    void Init() {
        Logging::Init() << "Initialized the Piano Playback Manager\n";
        Stop();
    }

    void Update() {
        if (Input::KeyPressed(HELL_KEY_HOME)) {
            PlayTrack(0);
        }

        if (Input::KeyPressed(HELL_KEY_PAUSE)) {
            PlayTrack(1);
        }

        if (Input::KeyPressed(HELL_KEY_PAGE_UP)) {
            PlayTrack(2);
        }

        if (Input::KeyPressed(HELL_KEY_END)) {
            Stop();
        }

        if (!g_playing || !g_midiFile) {
            return;
        }

        Piano* piano = GetPlaybackPiano();
        if (!piano) {
            Logging::Error() << "PianoPlaybackManager::Update() failed because no MIDI playback piano exists\n";
            Stop();
            return;
        }

        const float deltaTime = Time::DeltaTime();
        g_playbackTime += deltaTime * 1.05f;

        const std::vector<Hell::ScheduledMidiEvent>& scheduledEvents = g_midiFile->GetScheduledEvents();
        while (g_nextEventIndex < scheduledEvents.size() && scheduledEvents[g_nextEventIndex].timestamp <= g_playbackTime) {
            const Hell::ScheduledMidiEvent& currentEvent = scheduledEvents[g_nextEventIndex];

            switch (currentEvent.type) {
                case Hell::MidiEventType::NOTE_ON: {
                    AddDebugTextTimes("Time: " + Hell::String::FormatDouble(g_playbackTime) + "s");
                    AddDebugTextEvent("Note On: " + std::to_string(currentEvent.note));
                    AddDebugTextDurations("Dur: " + Hell::String::FormatDouble(currentEvent.duration) + "s");
                    AddDebugTextVelocity("Vel: " + std::to_string(currentEvent.velocity));

                    piano->PlayKey(currentEvent.note, currentEvent.velocity, static_cast<float>(currentEvent.duration));
                    break;
                }

                case Hell::MidiEventType::SUSTAIN: {
                    AddDebugTextTimes("Time: " + Hell::String::FormatDouble(g_playbackTime) + "s");
                    AddDebugTextEvent(currentEvent.sustainValue ? "Sustain pedal on" : "Sustain pedal off");
                    AddDebugTextDurations("  ");
                    AddDebugTextVelocity(" ");

                    piano->SetSustain(currentEvent.sustainValue);
                    break;
                }
            }

            g_nextEventIndex++;
        }

        if (g_playbackTime >= g_midiFile->GetDuration()) {
            Stop();
        }
    }

    void CleanUp() {
        Stop();
    }

    void PlayTrack(int trackIndex) {
        if (trackIndex < 0 || trackIndex >= static_cast<int>(TRACK_NAMES.size())) {
            return;
        }

        Piano* piano = GetPlaybackPiano();
        if (!piano) {
            Logging::Error() << "PianoPlaybackManager::PlayTrack() failed because no MIDI playback piano exists\n";
            return;
        }

        Hell::MidiFile* midiFile = Hell::ResourceManager::GetMidiFilePtr(TRACK_NAMES[trackIndex]);
        if (!midiFile || !midiFile->IsValid()) {
            Logging::Error() << "PianoPlaybackManager::PlayTrack() failed because MIDI file '" << TRACK_NAMES[trackIndex] << "' is not loaded\n";
            return;
        }

        g_midiFile = midiFile;
        g_nextEventIndex = 0;
        g_playbackTime = g_midiFile->GetInitialTime();
        g_playing = true;
        ClearDebugText();
    }

    void Stop() {
        g_playing = false;
        g_midiFile = nullptr;
        g_nextEventIndex = 0;
        g_playbackTime = 0.0;
        ClearDebugText();
    }

    void AddDebugTextTimes(const std::string& text) {
        AddRollingDebugText(g_debugTextTime, text);
    }

    void AddDebugTextEvent(const std::string& text) {
        AddRollingDebugText(g_debugTextEvents, text);
    }

    void AddDebugTextVelocity(const std::string& text) {
        AddRollingDebugText(g_debugTextTimeVelocities, text);
    }

    void AddDebugTextDurations(const std::string& text) {
        AddRollingDebugText(g_debugTextTimeDurations, text);
    }

    bool IsPlaying() {
        return g_playing;
    }

    std::string GetDebugTextTime() {
        return JoinDebugText(g_debugTextTime);
    }

    std::string GetDebugTextEvents() {
        return JoinDebugText(g_debugTextEvents);
    }

    std::string GetDebugTextVelocity() {
        return JoinDebugText(g_debugTextTimeVelocities);
    }

    std::string GetDebugTextTimeDurations() {
        return JoinDebugText(g_debugTextTimeDurations);
    }
}
