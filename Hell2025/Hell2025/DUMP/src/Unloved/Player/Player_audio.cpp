#include "Player.h"

#include "Hell/Audio.h"

#include "Unloved/Systems/GameAudio/GameAudio.h"

namespace Unloved {

void Player::UpdateAudio(float deltaTime) {

    const std::vector<const char*> indoorFootstepFilenames = {
                    "player_step_1.wav",
                    "player_step_2.wav",
                    "player_step_3.wav",
                    "player_step_4.wav",
    };
    const std::vector<const char*> outdoorFootstepFilenames = {
                    "player_step_grass_1.wav",
                    "player_step_grass_2.wav",
                    "player_step_grass_3.wav",
                    "player_step_grass_4.wav",
    };
    const std::vector<const char*> swimFootstepFilenames = {
                    "player_step_swim_1.wav",
                    "player_step_swim_2.wav",
                    "player_step_swim_3.wav",
                    "player_step_swim_4.wav",
    };
    const std::vector<const char*> wadeEndFilenames = {
                    "Water_Wade_End_1.wav",
                    "Water_Wade_End_2.wav",
                    "Water_Wade_End_3.wav",
                    "Water_Wade_End_4.wav",
                    "Water_Wade_End_5.wav",
                    "Water_Wade_End_6.wav",
                    "Water_Wade_End_7.wav",
                    "Water_Wade_End_8.wav",
                    "Water_Wade_End_9.wav",
    };
    const std::vector<const char*> ladderFootstepFilenames = {
                    "player_step_ladder_1.wav",
                    "player_step_ladder_2.wav",
                    "player_step_ladder_3.wav",
                    "player_step_ladder_4.wav",
    };

    bool pressingMovementKey = PressingWalkForward() || PressingWalkBackward() || PressingWalkLeft() || PressingWalkRight();

    // Ladder
    const bool climbingLadder = m_movementMode == PlayerMovementMode::LADDER && pressingMovementKey;
    if (!climbingLadder) {
        m_ladderFootstepAudioTimer = 0;
    }
    if (m_ladderFootstepAudioTimer > 0.35f) {
        m_ladderFootstepAudioTimer = 0;
    }
    if (m_ladderFootstepAudioTimer == 0) {
        if (climbingLadder && PressingWalkForward()) {
            int random = rand() % ladderFootstepFilenames.size();
            Hell::Audio::PlayAudio(ladderFootstepFilenames[random], 1.0f);
        }
    }
    m_ladderFootstepAudioTimer += deltaTime;

    // Water
    if (StoppedWading() && false) {
        int random = rand() % 9;
        Hell::Audio::PlayAudio(wadeEndFilenames[random], 1.0);
    }

    if (FeetEnteredUnderwater()) {
        if (m_yVelocity < -5.0f) {
            Hell::Audio::PlayAudio("Water_Impact0.wav", 1.0);
        }
    }

    if (CameraExitedUnderwater()) {
        Hell::Audio::PlayAudio("Water_ExitAndPant0.wav", 1.0);
    }
    
    if (!IsWading()) {
        // Footsteps
        if (m_bobOffsetY < -0.04f && !m_footstepPlayed && IsGrounded()) {

            if (FeetBelowWater()) {
                std::vector<const char*> filenames = {
                    "Water_Wade_End_1.wav",
                    "Water_Wade_End_2.wav",
                    "Water_Wade_End_3.wav",
                    "Water_Wade_End_4.wav",
                    "Water_Wade_End_5.wav",
                    "Water_Wade_End_6.wav",
                    "Water_Wade_End_7.wav",
                    "Water_Wade_End_8.wav",
                    "Water_Wade_End_9.wav",
                };
                int random = rand() % filenames.size();
                Hell::Audio::PlayAudio(filenames[random], 0.75f);
            }
            else {
                if (AreFeetAboveHeightField()) {
                    Unloved::GameAudio::PlayFootstepOutdoorAudio();
                }
                else {
                    Unloved::GameAudio::PlayFootstepIndoorAudio();
                }
            }

            m_footstepPlayed = true;
        }
        if (m_bobOffsetY > 0.0f) {
            m_footstepPlayed = false;
        }

        // Landing on ground
        if (m_grounded && !m_groundedLastFrame) {

            if (AreFeetAboveHeightField()) {
                Unloved::GameAudio::PlayFootstepOutdoorAudio();
            } else {
                Unloved::GameAudio::PlayFootstepIndoorAudio();
            }

            m_headBobTime = 0.0f;
        }
    }
}

} // namespace Unloved
