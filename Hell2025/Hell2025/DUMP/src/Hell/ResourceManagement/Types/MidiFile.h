#pragma once
#include "Hell/Common.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace Hell {

    enum class MidiEventType : uint8_t {
        NOTE_ON,
        SUSTAIN
    };

    struct ScheduledMidiEvent {
        double timestamp = 0.0;
        MidiEventType type = MidiEventType::NOTE_ON;
        int note = 0;
        int velocity = 0;
        double duration = 0.0;
        bool sustainValue = false;

        bool operator<(const ScheduledMidiEvent& other) const {
            return timestamp < other.timestamp;
        }
    };

    struct MidiFile {
        MidiFile() = default;
        MidiFile(const std::string& name);
        MidiFile(const MidiFile&) = delete;
        MidiFile& operator=(const MidiFile&) = delete;
        MidiFile(MidiFile&&) noexcept = default;
        MidiFile& operator=(MidiFile&&) noexcept = default;
        ~MidiFile() = default;

        bool LoadFromFile(const std::string& path);

        const std::string& GetName() const                                      { return m_name; }
        const std::string& GetPath() const                                      { return m_path; }
        const std::vector<ScheduledMidiEvent>& GetScheduledEvents() const       { return m_scheduledEvents; }
        double GetInitialTime() const                                           { return m_initialTime; }
        double GetDuration() const                                              { return m_totalDurationSeconds; }
        int GetTicksPerQuarterNote() const                                      { return m_ticksPerQuarterNote; }
        size_t GetEventCount() const                                            { return m_scheduledEvents.size(); }
        bool IsValid() const                                                    { return !m_scheduledEvents.empty(); }

    private:
        std::string m_name = UNDEFINED_STRING;
        std::string m_path;
        std::vector<ScheduledMidiEvent> m_scheduledEvents;
        double m_initialTime = 0.0;
        double m_totalDurationSeconds = 0.0;
        int m_ticksPerQuarterNote = 0;
    };
}
