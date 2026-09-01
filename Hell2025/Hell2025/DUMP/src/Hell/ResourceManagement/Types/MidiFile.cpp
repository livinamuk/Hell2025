#include "MidiFile.h"

#include "Hell/File.h"

#include "midifile/MidiEvent.h"
#include "midifile/MidiEventList.h"
#include "midifile/MidiFile.h"

#include <algorithm>

namespace Hell {

    MidiFile::MidiFile(const std::string& name) {
        m_name = name;
    }

    bool MidiFile::LoadFromFile(const std::string& path) {
        smf::MidiFile smfMidiFile;

        if (!smfMidiFile.read(path)) {
            return false;
        }

        m_ticksPerQuarterNote = smfMidiFile.getTicksPerQuarterNote();
        m_totalDurationSeconds = smfMidiFile.getFileDurationInSeconds();

        if (m_ticksPerQuarterNote <= 0) {
            return false;
        }

        smfMidiFile.linkNotePairs();
        smfMidiFile.doTimeAnalysis();

        if (smfMidiFile.getTrackCount() > 1) {
            smfMidiFile.joinTracks();
        }

        int velocityMax = 0;
        m_scheduledEvents.clear();

        if (smfMidiFile.getTrackCount() > 0 && smfMidiFile[0].size() > 0) {
            for (int eventIndex = 0; eventIndex < smfMidiFile[0].size(); eventIndex++) {
                smf::MidiEvent* midiEvent = &smfMidiFile[0][eventIndex];

                if (midiEvent->isNoteOn() && (*midiEvent)[2] > 0) {
                    ScheduledMidiEvent scheduledEvent;
                    scheduledEvent.type = MidiEventType::NOTE_ON;
                    scheduledEvent.timestamp = midiEvent->seconds;
                    scheduledEvent.note = (*midiEvent)[1];
                    scheduledEvent.velocity = (*midiEvent)[2];
                    scheduledEvent.duration = midiEvent->isLinked() ? midiEvent->getDurationInSeconds() : 0.1;

                    if (scheduledEvent.duration < 0.0) {
                        scheduledEvent.duration = 0.1;
                    }

                    m_scheduledEvents.push_back(scheduledEvent);
                    velocityMax = std::max(velocityMax, scheduledEvent.velocity);
                }
                else if (midiEvent->isController() && midiEvent->getControllerNumber() == 64) {
                    ScheduledMidiEvent scheduledEvent;
                    scheduledEvent.type = MidiEventType::SUSTAIN;
                    scheduledEvent.timestamp = midiEvent->seconds;
                    scheduledEvent.sustainValue = midiEvent->getControllerValue() >= 64;

                    m_scheduledEvents.push_back(scheduledEvent);
                }
            }
        }

        if (m_scheduledEvents.empty()) {
            return false;
        }

        const int velocityNormalizer = 127 - velocityMax;
        for (ScheduledMidiEvent& scheduledEvent : m_scheduledEvents) {
            if (scheduledEvent.type == MidiEventType::NOTE_ON) {
                scheduledEvent.velocity += velocityNormalizer;
            }
        }

        std::sort(m_scheduledEvents.begin(), m_scheduledEvents.end());

        m_initialTime = m_scheduledEvents.front().timestamp;
        m_name = File::GetName(path);
        m_path = path;

        return true;
    }
}
