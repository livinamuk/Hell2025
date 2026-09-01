#include "Player.h"

#include "Hell/Input.h"
#include "Hell/Logging.h"

#include "Legacy/World/LegacyWorld.h"
#include "Unloved/World/World.h"

namespace Input = Hell::Input;

namespace Unloved {

void Player::SitAtPiano(uint64_t pianoId) {
    m_pianoId = pianoId;
    m_isPlayingPiano = true;
    m_controlEnabled = false;
}

void Player::UpdatePlayingPiano(float deltaTime) {
    if (!m_isPlayingPiano) return;
        
    // Error check, in case your piano is nullptr somehow
    Piano* piano = Unloved::World::GetPianoByObjectId(m_pianoId);
    if (!piano) {
        Logging::Error() << "UpdatePlayingPiano() failed: tried to play a nullptr piano! m_pianoId is " << m_pianoId << "\n";
        m_isPlayingPiano = false;
        return;
    }

    // Move character control to piano seat position
    glm::vec3 targetPosition = piano->GetSeatPosition();

    CharacterController* characterControler = Hell::Physics::GetCharacterControllerById(m_characterControllerId);
    if (characterControler) {
        PxController* pxControler = characterControler->GetPxController();
        pxControler->setFootPosition(PxExtendedVec3(targetPosition.x, targetPosition.y, targetPosition.z));
    }

    // Leave the piano
    if (Input::MiddleMousePressed()) {
        m_isPlayingPiano = false;
    }

    int g3 = 55;
    int g4 = 67;
    int c4 = 60;
    int d3 = 50;
    int c3 = 48;
    int b3 = 59;
    int e3 = 52;


    int root = g4;
    int second = root + 2;
    int third = root + 4;
    int fourth = root + 5;
    int fifth = root + 7;
    int minorSixth = root + 8;
    int sixth = root + 9;

    // special notes
    int seventh = root - 1;
    int lowerSixth = sixth - 12;
    int lowerFifth = fifth - 12;

    // Chords
    if (Input::KeyDown(HELL_KEY_Z)) {
        piano->PlayMajorFirstInversion(g3);
    }
    if (Input::KeyDown(HELL_KEY_X)) {
        piano->PlayMajor7th(b3);
    }
    if (Input::KeyDown(HELL_KEY_C)) {
        piano->PlayMinor(e3);
    }
    if (Input::KeyDown(HELL_KEY_V)) {
        piano->PlayMajor(c3);
    }


    if (Input::KeyDown(HELL_KEY_R)) {
        piano->PlayKey(lowerFifth);
    }
    if (Input::KeyDown(HELL_KEY_T)) {
        piano->PlayKey(lowerSixth);
    }
    if (Input::KeyDown(HELL_KEY_Y)) {
        piano->PlayKey(seventh);
    }
    if (Input::KeyDown(HELL_KEY_U)) {
        piano->PlayKey(root);
    }
    if (Input::KeyDown(HELL_KEY_I)) {
        piano->PlayKey(second);
    }
    if (Input::KeyDown(HELL_KEY_O)) {
        piano->PlayKey(third);
    }
    if (Input::KeyDown(HELL_KEY_P)) {
        piano->PlayKey(fourth);
    }
    if (Input::KeyDown(HELL_KEY_LEFT_BRACKET)) {
        piano->PlayKey(fifth);
    }
    if (Input::KeyDown(HELL_KEY_EQUAL)) {
        piano->PlayKey(minorSixth);
    }
    if (Input::KeyDown(HELL_KEY_RIGHT_BRACKET)) {
        piano->PlayKey(sixth);
    }

}

} // namespace Unloved
