#pragma once

#include <cstdint>
#include <string>

namespace Unloved::PianoPlaybackManager {
    void Init();
    void Update();
    void CleanUp();

    void PlayTrack(int trackIndex);
    void Stop();

    void AddDebugTextTimes(const std::string& text);
    void AddDebugTextEvent(const std::string& text);
    void AddDebugTextVelocity(const std::string& text);
    void AddDebugTextDurations(const std::string& text);

    bool IsPlaying();

    std::string GetDebugTextTime();
    std::string GetDebugTextEvents();
    std::string GetDebugTextVelocity();
    std::string GetDebugTextTimeDurations();
}
