#include "Shark.h"

#include "Hell/Audio.h"

#include "Unloved/Session/Session.h"

#include <iostream>

namespace Audio = Hell::Audio;

namespace Unloved {

    void Shark::UpdateHuntingLogic(float deltaTime) {
        Unloved::SkinnedGameObject* skinnedGameObject = GetSkinnedGameObject();
        if (!skinnedGameObject) return;

        Unloved::Player* player = Unloved::Session::GetPlayerById(m_huntedPlayerId);

        // Does the player not exist for some reason?
        if (!player) {
            SetMovementState(SharkMovementState::FOLLOWING_PATH);
            return;
        }

        // Did the player leave the water?
        if (!player->FeetBelowWater()) {
            m_movementState = SharkMovementState::FOLLOWING_PATH_ANGRY;
            m_huntingState = SharkHuntingState::UNDEFINED;
        }

        // Is the player within biting range?
        if (m_huntingState == SharkHuntingState::CHARGE_PLAYER) {
            float bitingRange = 3.25f;
            if (GetDistanceMouthToTarget3D() < bitingRange) {
                m_huntingState = SharkHuntingState::BITING_PLAYER;
                if (TargetIsOnLeft(m_targetPosition)) {
                    PlayAnimation(Bible::AnimationSlot::SHARK_ATTACK_LEFT, 1.0f);
                    std::cout << "Shark left bite\n";
                }
                else {
                    PlayAnimation(Bible::AnimationSlot::SHARK_ATTACK_RIGHT, 1.0f);
                    std::cout << "Shark right bite\n";
                }
                Audio::PlayAudio("Shark_Bite_Overwater_Edited.wav", 1.0f);
                m_hasBitPlayer = false;
            }
        }

        // Issue bite to player in range
        if (m_huntingState == SharkHuntingState::BITING_PLAYER && !m_hasBitPlayer) {
            if (m_huntedPlayerId != 0) {
                Unloved::Player* player = Unloved::Session::GetPlayerById(m_huntedPlayerId);
                glm::vec3 playerPos = player->GetFootPosition() * glm::vec3(1.0f, 0.0f, 1.0f);

                float distanceFromHead2D = glm::distance(playerPos, GetHeadPosition2D());
                glm::vec3 dirToPlayer = glm::normalize(playerPos - GetHeadPosition2D());
                float dotToPlayer = glm::dot(GetForwardVector(), dirToPlayer);

                bool playerSafe = true;
                float safeHeadDistance = 1.20f;

                if (GetAnimationFrameNumber() == 11) {
                    safeHeadDistance = 1.25f;
                }
                if (GetAnimationFrameNumber() == 12) {
                    safeHeadDistance = 1.3f;
                }
                if (GetAnimationFrameNumber() == 13) {
                    safeHeadDistance = 1.35f;
                }
                if (GetAnimationFrameNumber() > 14) {
                    safeHeadDistance = 1.45f;
                }

                if (GetAnimationFrameNumber() <= 9) {
                    playerSafe = true;
                }
                else if (GetAnimationFrameNumber() < 20) {
                    if (!IsBehindEvadePoint(playerPos)) {
                        if (distanceFromHead2D < safeHeadDistance) {
                            playerSafe = false;
                        }
                        else {
                            playerSafe = true;
                        }
                    }
                    else {
                        playerSafe = true;
                    }

                    // But the player still has to be this far away.
                    if (GetAnimationFrameNumber() < 12 && distanceFromHead2D < 1.3f) {
                        playerSafe = false;
                    }
                    if (GetAnimationFrameNumber() > 12 && !IsBehindEvadePoint(playerPos) && distanceFromHead2D < 2.0f) {
                        playerSafe = false;
                    }
                }
                if (dotToPlayer < 0.25f) {
                    playerSafe = true;
                }

                m_logicSubStepCount = 6;

                if (!playerSafe) {
                    m_hasBitPlayer = true;
                    player->Kill(false);

                    if (Ragdoll* ragdoll = player->GetRagdoll()) {
                        glm::vec3 biteForce = glm::normalize(GetForwardVector() + glm::vec3(0.0f, 0.15f, 0.35f)) * 2.5f;
                        glm::vec3 biteSpinAxis = glm::normalize(glm::cross(GetForwardVector(), glm::vec3(0.0f, 1.0f, 0.5f)));
                        ragdoll->AddForce("CC_Base_Head", biteForce, false);
                        ragdoll->SetAngularVelocity("CC_Base_Hip", biteSpinAxis * 14.0f, false);
                    }

                    m_huntedPlayerId = 0;
                    m_movementState = SharkMovementState::FOLLOWING_PATH_ANGRY;
                }
            }
        }

        // Is the bite over?
        if (m_huntingState == SharkHuntingState::BITING_PLAYER) {
            AnimatorInstance* animatorInstance = GetAnimatorInstance();
            if (animatorInstance && animatorInstance->IsAnimationComplete(m_animationLayerIndex)) {
                m_huntingState = SharkHuntingState::CHARGE_PLAYER;
                PlayAndLoopAnimation(Bible::AnimationSlot::SHARK_SWIM, 1.0f);
            }
        }
    }
}
