#include "Shark.h"

#include "Hell/Input.h"
#include "Hell/Math/Math.h"

#include "Unloved/Session/Session.h"
#include "Unloved/Systems/Ocean/Ocean.h"
#include "Unloved/World/World.h"

#include <algorithm>

namespace Input = Hell::Input;

namespace Unloved {

    void Shark::UpdateMovement(float deltaTime) {
        if (m_movementState == SharkMovementState::FOLLOWING_PATH_ANGRY) {
            Unloved::Player* player = Unloved::Session::GetPlayerById(m_huntedPlayerId);
            if (player && player->FeetBelowWater()) {
                m_movementState = SharkMovementState::HUNT_PLAYER;
                m_huntingState = SharkHuntingState::CHARGE_PLAYER;
            }
        }

        if (IsAlive()) {
            if (World::HasOcean()) {
                float magicNumber = 1.05f;
                float lerpSpeed = 1.0f;
                float targetHeight = Ocean::GetOceanOriginY() - magicNumber;

                if (m_movementState == SharkMovementState::HUNT_PLAYER) {
                    if (Unloved::Player* player = Unloved::Session::GetPlayerById(m_huntedPlayerId)) {
                        targetHeight = std::min(targetHeight, player->GetCameraPosition().y) - (magicNumber * 0.0f);
                    }
                }

                m_yHeight = Hell::Math::InterpTo(m_yHeight, targetHeight, deltaTime, lerpSpeed);
            }

            if (m_movementState == SharkMovementState::ARROW_KEYS) {
                if (Input::KeyDown(HELL_KEY_UP)) {
                    CalculateForwardVectorFromArrowKeys(deltaTime);

                    for (int i = 0; i < m_logicSubStepCount; i++) {
                        MoveShark(deltaTime);
                    }
                }
            }
            else if (m_movementState == SharkMovementState::FOLLOWING_PATH ||
                     m_movementState == SharkMovementState::FOLLOWING_PATH_ANGRY) {
                CalculateTargetFromPath();
                CalculateForwardVectorFromTarget(deltaTime);

                for (int i = 0; i < m_logicSubStepCount; i++) {
                    MoveShark(deltaTime);
                }
            }
            else if (m_movementState == SharkMovementState::HUNT_PLAYER) {
                if (m_huntingState == SharkHuntingState::CHARGE_PLAYER ||
                    m_huntingState == SharkHuntingState::BITING_PLAYER && GetAnimationFrameNumber() > 17 ||
                    m_huntingState == SharkHuntingState::BITING_PLAYER && GetAnimationFrameNumber() > 7 && !IsBehindEvadePoint(m_targetPosition)) {
                    CalculateTargetFromPlayer();
                    CalculateForwardVectorFromTarget(deltaTime);
                }

                for (int i = 0; i < m_logicSubStepCount; i++) {
                    UpdateHuntingLogic(deltaTime);
                    MoveShark(deltaTime);
                }
            }
        }

        if (m_health > 0) {
            m_alive = true;
        }
        if (IsAlive() && m_health <= 0) {
            Kill();
        }
        m_health = std::max(m_health, 0);

        Unloved::SkinnedGameObject* skinnedGameObject = GetSkinnedGameObject();
        if (!skinnedGameObject) return;

        if (IsDead()) {
            StraightenSpine(deltaTime, 0.25f);

            const glm::vec3& currentPosition = skinnedGameObject->GetPosition();
            float sinkAmount = deltaTime * 1.36f;
            glm::vec3 newPosition = currentPosition + glm::vec3(0.0f, -sinkAmount, 0.0f);
            skinnedGameObject->SetPosition(newPosition);

            if (currentPosition.y < 10.0f) {
                Respawn();
                float spawnHeight = 28.85f;
                glm::vec3 spawnPos = glm::vec3(-50.0f, spawnHeight, 40.5f);
                skinnedGameObject->SetPosition(spawnPos);
                ResetSpine(spawnPos);
            }
        }
    }

    void Shark::Update(float deltaTime) {
        m_right = glm::cross(m_forward, glm::vec3(0, 1, 0));
        m_left = -m_right;
        UpdateSpinePose();
    }
}
