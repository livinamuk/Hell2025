#pragma once
#include "Hell/Common.h"

#include <string>

namespace Hell {

    struct SoundFont {
        SoundFont() = default;
        SoundFont(const std::string& name);
        SoundFont(const SoundFont&) = delete;
        SoundFont& operator=(const SoundFont&) = delete;
        SoundFont(SoundFont&&) noexcept = default;
        SoundFont& operator=(SoundFont&&) noexcept = default;
        ~SoundFont() = default;

        void SetFluidSynthId(int fluidSynthId)     { m_fluidSynthId = fluidSynthId; }

        const std::string& GetName() const         { return m_name; }
        int GetFluidSynthId() const                { return m_fluidSynthId; }

    private:
        std::string m_name = UNDEFINED_STRING;
        int m_fluidSynthId = -1;
    };
}
