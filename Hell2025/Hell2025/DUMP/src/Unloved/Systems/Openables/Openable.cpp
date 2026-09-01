#include "Openable.h"

#include "Hell/Audio.h"
#include "Hell/Input.h"

#include "Legacy/World/LegacyWorld.h"

#include "Unloved/Systems/Openables/OpenableManager.h"
#include "Unloved/World/World.h"

namespace Unloved {

void Openable::Init(const OpenableCreateInfo& createInfo, uint64_t parentObjectId) {
    m_currentOpenState = createInfo.initialOpenState;
    m_openAxis = createInfo.openAxis;
    m_minOpenValue = createInfo.minOpenValue;
    m_maxOpenValue = createInfo.maxOpenValue;
    m_openSpeed = createInfo.openSpeed;
    m_closeSpeed = createInfo.closeSpeed;
    m_openingAudio = createInfo.openingAudio;
    m_closingAudio = createInfo.closingAudio;
    m_openedAudio = createInfo.openedAudio;
    m_closedAudio = createInfo.closedAudio;
    m_lockedAudio = createInfo.lockedAudio;
    m_audioVolume = createInfo.audioVolume;
    m_prerequisiteOpenMeshName = createInfo.prerequisiteOpenMeshName;
    m_prerequisiteClosedMeshName = createInfo.prerequisiteClosedMeshName;
    m_isDeadLock = createInfo.isDeadLock;
    m_parentObjectId = parentObjectId;

    if (m_currentOpenState == OpenState::OPEN ||
        m_currentOpenState == OpenState::OPENING) {
        m_currentOpenValue = m_maxOpenValue;

        switch (m_openAxis) {
            case OpenAxis::TRANSLATE_X:     m_transform.position.x = m_currentOpenValue;  break;
            case OpenAxis::TRANSLATE_Y:     m_transform.position.y = m_currentOpenValue;  break;
            case OpenAxis::TRANSLATE_Z:     m_transform.position.z = m_currentOpenValue;  break;
            case OpenAxis::TRANSLATE_Y_NEG: m_transform.position.y = -m_currentOpenValue; break;
            case OpenAxis::TRANSLATE_X_NEG: m_transform.position.x = -m_currentOpenValue; break;
            case OpenAxis::TRANSLATE_Z_NEG: m_transform.position.z = -m_currentOpenValue; break;
            case OpenAxis::ROTATE_X:        m_transform.rotation.x = m_currentOpenValue;  break;
            case OpenAxis::ROTATE_Y:        m_transform.rotation.y = m_currentOpenValue;  break;
            case OpenAxis::ROTATE_Z:        m_transform.rotation.z = m_currentOpenValue;  break;
            case OpenAxis::ROTATE_X_NEG:    m_transform.rotation.x = -m_currentOpenValue; break;
            case OpenAxis::ROTATE_Y_NEG:    m_transform.rotation.y = -m_currentOpenValue; break;
            case OpenAxis::ROTATE_Z_NEG:    m_transform.rotation.z = -m_currentOpenValue; break;
            default: break;
        }
    }
    else if (m_currentOpenState == OpenState::CLOSED ||
             m_currentOpenState == OpenState::CLOSING) {
        m_currentOpenValue = m_minOpenValue;
    }
}

void Openable::SetParentObjectId(uint64_t parentObjectId) {
    m_parentObjectId = parentObjectId;
}

bool Openable::IsInteractable(const glm::vec3& viewPos) {
    // Already opening or closing?
    if (m_currentOpenState == OpenState::CLOSING) return false;
    if (m_currentOpenState == OpenState::OPENING) return false;

    // Check distance to view pos
    // TODO!

    // Check prerequisites
    if (m_prerequisiteOpenMeshName != UNDEFINED_STRING || m_prerequisiteClosedMeshName != UNDEFINED_STRING) {
       
        // Parent is generic object
        if (GenericObject* genericObject = Unloved::World::GetGenericObjectById(m_parentObjectId)) {
            MeshNodes& meshNodes = genericObject->GetMeshNodes();
            if (m_currentOpenState == OpenState::OPEN) {
                if (meshNodes.MeshNodeIsClosed(m_prerequisiteClosedMeshName)) return false;
            }
            if (m_currentOpenState == OpenState::CLOSED) {
                if (meshNodes.MeshNodeIsOpen(m_prerequisiteOpenMeshName)) return false;
            }
        }
    }

    return true;
}

std::string Openable::Interact(const glm::vec3& cameraPosition, const glm::vec3& cameraForward) {
    // Unlock deadlock from "the other side"
    if (m_locked && m_isDeadLock) {
        if (Door* door = Unloved::World::GetDoorByObjectId(m_parentObjectId)) {
            if (door->CameraFacingDoorWorldForward(cameraPosition, cameraForward)) {
                m_locked = false;
                m_isDeadLock = false;
                Hell::Audio::PlayAudio("Unlocked.wav", 1.0f);
                return "You unlocked it.";
            }
        }
    }

    if (m_locked && m_lockedAudio != UNDEFINED_STRING) {
        Hell::Audio::PlayAudio(m_lockedAudio, 0.75f);

        if (Door* door = Unloved::World::GetDoorByObjectId(m_parentObjectId)) {
            if (door->GetDeadLockState()) {
                return "It's locked from the other side.";
            }
        }
        return "";
    }

    glm::vec3 dummyViewPos = glm::vec3(0.0f);
    if (!IsInteractable(dummyViewPos)) {
        return "";
    }

    if (m_currentOpenState == OpenState::OPEN) {
        m_currentOpenState = OpenState::CLOSING;
        if (m_closingAudio != UNDEFINED_STRING) {
            Hell::Audio::PlayAudio(m_closingAudio, m_audioVolume);
        }
    }

    if (m_currentOpenState == OpenState::CLOSED) {
        m_currentOpenState = OpenState::OPENING;
        if (m_openingAudio != UNDEFINED_STRING) {
            Hell::Audio::PlayAudio(m_openingAudio, m_audioVolume);
        }
    }

    return "";
}

void Openable::Update(float deltaTime) {
    m_dirty = false;

    if (m_currentOpenState == OpenState::CLOSING) {
        m_dirty = true;
        m_currentOpenValue -= m_closeSpeed * deltaTime;

        if (m_currentOpenValue < m_minOpenValue) {
            m_currentOpenValue = m_minOpenValue;
            m_currentOpenState = OpenState::CLOSED;

            if (m_closedAudio != UNDEFINED_STRING) {
                Hell::Audio::PlayAudio(m_closedAudio, m_audioVolume);
            }
        }
    }
    if (m_currentOpenState == OpenState::OPENING) {
        m_dirty = true;
        m_currentOpenValue += m_openSpeed * deltaTime;

        if (m_currentOpenValue > m_maxOpenValue) {
            m_currentOpenValue = m_maxOpenValue;
            m_currentOpenState = OpenState::OPEN;

            if (m_openedAudio != UNDEFINED_STRING) {
                Hell::Audio::PlayAudio(m_openedAudio, m_audioVolume);
            }
        }
    }
    if (m_currentOpenState == OpenState::OPEN) {
        m_currentOpenValue = m_maxOpenValue;
    }
    if (m_currentOpenState == OpenState::CLOSED) {
        m_currentOpenValue = m_minOpenValue;
    }

    m_transform.position.x = 0.0f;
    m_transform.position.y = 0.0f;
    m_transform.position.z = 0.0f;
    m_transform.rotation.x = 0.0f;
    m_transform.rotation.y = 0.0f;
    m_transform.rotation.z = 0.0f;

    switch (m_openAxis) {
        case OpenAxis::TRANSLATE_X:     m_transform.position.x = m_currentOpenValue;  break;
        case OpenAxis::TRANSLATE_Y:     m_transform.position.y = m_currentOpenValue;  break;
        case OpenAxis::TRANSLATE_Z:     m_transform.position.z = m_currentOpenValue;  break;
        case OpenAxis::TRANSLATE_Y_NEG: m_transform.position.y = -m_currentOpenValue; break;
        case OpenAxis::TRANSLATE_X_NEG: m_transform.position.x = -m_currentOpenValue; break;
        case OpenAxis::TRANSLATE_Z_NEG: m_transform.position.z = -m_currentOpenValue; break;
        case OpenAxis::ROTATE_X:        m_transform.rotation.x = m_currentOpenValue;  break;
        case OpenAxis::ROTATE_Y:        m_transform.rotation.y = m_currentOpenValue;  break;
        case OpenAxis::ROTATE_Z:        m_transform.rotation.z = m_currentOpenValue;  break;
        case OpenAxis::ROTATE_X_NEG:    m_transform.rotation.x = -m_currentOpenValue; break;
        case OpenAxis::ROTATE_Y_NEG:    m_transform.rotation.y = -m_currentOpenValue; break;
        case OpenAxis::ROTATE_Z_NEG:    m_transform.rotation.z = -m_currentOpenValue; break;
        default: break;
    }

    if (m_firstFrame) {
        m_dirty = true;
        m_firstFrame = false;
    }
}

bool Openable::IsDirty() {
    return m_dirty;
}

bool Openable::IsOpen() {
    return (m_currentOpenState == OpenState::OPEN);
}

bool Openable::IsClosed() {
    return (m_currentOpenState == OpenState::CLOSED);
}

} // namespace Unloved
