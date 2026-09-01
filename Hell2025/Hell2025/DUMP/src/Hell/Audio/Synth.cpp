#include "Synth.h"
#include "fluidsynth.h"
#include "Hell/File.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/ResourceManagement/Types/SoundFont.h"

#include <array>
#include <iostream>

void PrintSettings(void* data, const char* name, int type) {
    std::cout << "Setting: Name='" << name << "', Type=" << type << "\n";
}

void ErrorCallback(int level, const char* message, void* user_data) {
    if (Hell::Synth::LogToConsoleEnabled()) {
        fprintf(stderr, "%s\n", message);
    }
}

namespace Hell::Synth {
    fluid_audio_driver_t* g_driver;
    fluid_settings_t* g_settings;
    fluid_synth_t* g_synth;
    bool g_logToConsole = false;
    std::array<std::string, 16> g_selectedSoundFontNames;
        
    void Init() {
        g_settings = new_fluid_settings();
        g_synth = new_fluid_synth(g_settings);
        g_driver = new_fluid_audio_driver(g_settings, g_synth);

        fluid_settings_setstr(g_settings, "audio.driver", "wasapi");
        fluid_settings_setnum(g_settings, "synth.sample-rate", 44100);
        fluid_settings_setint(g_settings, "audio.wasapi.exclusive-mode", 1);
        fluid_settings_setint(g_settings, "audio.period-size", 128);
        fluid_settings_setint(g_settings, "audio.periods", 2);
        fluid_settings_setstr(g_settings, "audio.driver", "wasapi");
        fluid_settings_setnum(g_settings, "synth.sample-rate", 44100);
        fluid_settings_setint(g_settings, "synth.reverb.active", 0);
        fluid_settings_setint(g_settings, "synth.chorus.active", 0);
        fluid_settings_setint(g_settings, "synth.verbose", 0);

        fluid_set_log_function(FLUID_WARN, ErrorCallback, nullptr);
       

        //fluid_settings_foreach(g_settings, nullptr, PrintSettings);

        fluid_synth_set_gain(g_synth, 4.0);
        g_selectedSoundFontNames.fill(UNDEFINED_STRING);
    }

    SoundFont LoadSoundFont(const std::string& path) {
        SoundFont soundFont(File::GetName(path));
        int id = fluid_synth_sfload(g_synth, path.c_str(), 1);
        if (id == FLUID_FAILED) {
            if (LogToConsoleEnabled()) {
                std::cout << "Synth::LoadSoundFont() failed to load " << path << "\n";
            }
            return {};
        }
        if (LogToConsoleEnabled()) {
            std::cout << "Loaded sound font: " << path << "\n";
        }

        soundFont.SetFluidSynthId(id);
        return soundFont;
    }

    bool SelectSoundFont(const std::string& soundFontName, int channel) {
        if (!g_synth || soundFontName == UNDEFINED_STRING || channel < 0 || channel >= static_cast<int>(g_selectedSoundFontNames.size())) {
            return false;
        }

        if (g_selectedSoundFontNames[channel] == soundFontName) {
            return true;
        }

        SoundFont* soundFont = ResourceManager::GetSoundFontPtr(soundFontName);
        if (!soundFont || soundFont->GetFluidSynthId() < 0) {
            return false;
        }

        const int result = fluid_synth_program_select(g_synth, channel, soundFont->GetFluidSynthId(), 0, 0);
        if (result != FLUID_OK) {
            if (LogToConsoleEnabled()) {
                std::cout << "Synth::SelectSoundFont() failed to select '" << soundFontName << "'\n";
            }
            return false;
        }

        g_selectedSoundFontNames[channel] = soundFontName;
        return true;
    }

    void PlayNote(int note, int velocity) {
        fluid_synth_noteon(g_synth, 0, note, velocity);
    }

    void ReleaseNote(int note) {
        fluid_synth_noteoff(g_synth, 0, note);
    }

    void Update(float deltaTime) {
       
    }

    void CleanUp() {
        delete_fluid_audio_driver(g_driver);
        delete_fluid_synth(g_synth);
        delete_fluid_settings(g_settings);
    }

    void SetSustain(bool sustainOn) {
        if (!g_synth) {
            return; // Safety check
        }

        int MIDI_CHANNEL = 0;

        // MIDI CC 64: Sustain Pedal (Damper)
        // Value >= 64 is ON, Value < 64 is OFF.
        // Common values are 127 for ON and 0 for OFF.
        int value = sustainOn ? 127 : 0;
        int controller = 64; // Sustain pedal controller number

        // Send the Control Change message to FluidSynth
        int result = fluid_synth_cc(g_synth, MIDI_CHANNEL, controller, value);

        if (result != FLUID_OK && LogToConsoleEnabled()) {
            std::cerr << "FluidSynth Error: fluid_synth_cc failed for sustain." << std::endl;
        }
        else {
          // Optional log
          // std::cout << "Synth::SetSustain: Sent CC 64, Channel=" << MIDI_CHANNEL << ", Value=" << value << std::endl;
        }
    }

    bool LogToConsoleEnabled() {
        return g_logToConsole;
    }
}
