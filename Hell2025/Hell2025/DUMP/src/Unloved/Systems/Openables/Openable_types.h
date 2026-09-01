#pragma once

#include "Hell/Common/Constants.h"

#include <string>
#include <vector>

namespace Unloved {

enum struct OpenAxis {
    TRANSLATE_X,
    TRANSLATE_Y,
    TRANSLATE_Z,
    TRANSLATE_X_NEG,
    TRANSLATE_Y_NEG,
    TRANSLATE_Z_NEG,
    ROTATE_X,
    ROTATE_Y,
    ROTATE_Z,
    ROTATE_X_NEG,
    ROTATE_Y_NEG,
    ROTATE_Z_NEG,
};

enum class OpenState {
    OPEN,
    OPENING,
    CLOSED,
    CLOSING,
    UNDEFINED
};

struct OpenableCreateInfo {
    bool isOpenable = false;
    bool isDeadLock = false;
    OpenState initialOpenState = OpenState::CLOSED;
    OpenAxis openAxis = OpenAxis::TRANSLATE_Z;
    std::string lockedAudio = "Locked.wav";
    std::string openingAudio = UNDEFINED_STRING;
    std::string closingAudio = UNDEFINED_STRING;
    std::string openedAudio = UNDEFINED_STRING;
    std::string closedAudio = UNDEFINED_STRING;
    std::string prerequisiteOpenMeshName = UNDEFINED_STRING;
    std::string prerequisiteClosedMeshName = UNDEFINED_STRING;
    std::vector<std::string> additionalTriggerMeshNames;
    float minOpenValue = 0.0f;
    float maxOpenValue = HELL_PI * 0.5f;
    float openSpeed = 1.0f;
    float closeSpeed = 1.0f;
    float audioVolume = 2.0f;
};

} // namespace Unloved
