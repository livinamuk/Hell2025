#pragma once
#include <string>

namespace Hell {
    struct SoundFont;

    namespace Synth {
        void Init();
        void Update(float deltaTime);
        void CleanUp();
        bool SelectSoundFont(const std::string& soundFontName, int channel = 0);
        void PlayNote(int note, int velocity = 127);
        void ReleaseNote(int note);
        SoundFont LoadSoundFont(const std::string& path);
        void SetSustain(bool value);
        bool LogToConsoleEnabled();
    }
}
