#include "Kangaroo.h"
#include "Hell/Math/Rotation.h"
#include "Unloved/Systems/Pathfinding/AStarMap.h"

#include "Unloved/Session/Session.h"   // remove me
#include "Unloved/Render/Renderer.h" // TODO get me out of here

namespace Unloved {

    void Kangaroo::UpdateMovement(float deltaTime) {
        if (m_animationState == KanagarooAnimationState::BITE) {
            m_timeSinceBiteBegan += deltaTime;
        }
        else {
            m_timeSinceBiteBegan = 0.0f;
        }
        if (m_animationState == KanagarooAnimationState::IDLE) {
            m_timeSinceIdleBegan += deltaTime;
        }
        else {
            m_timeSinceIdleBegan = 0.0f;
        }

        //if (Input::KeyPressed(HELL_KEY_PERIOD)) {
        //    Respawn();
        //}

        const bool isMoving =
            m_animationState == KanagarooAnimationState::IDLE_TO_HOP ||
            m_animationState == KanagarooAnimationState::HOP ||
            m_animationState == KanagarooAnimationState::BITE;

        if (isMoving) {
            FindPathToTarget();
        }

        UpdateAnimationStateMachine();
        UpdateMovementLogic(deltaTime);

        UpdateAudio();

        //DebugDraw();
    }

    void Kangaroo::Update(float deltaTime) {
        (void)deltaTime;

        Unloved::SkinnedGameObject* skinnedGameObject = GetSkinnedGameObject();
        Ragdoll* ragdoll = GetRagdoll();

        if (skinnedGameObject && ragdoll) {
            if (Renderer::GetCurrentRendererSettings().debugDrawRagdolls) {
                skinnedGameObject->DisableRendering();
            }
            else {
                skinnedGameObject->EnableRendering();
            }
        }

        UpdateSkinnedGameObjectPositionRotation();

        // Death check
        if (m_health <= 0) {
            Kill();
        }
        m_health = glm::clamp(m_health, 0, 9999999);
    }

    void Kangaroo::UpdateSkinnedGameObjectPositionRotation() {
        Unloved::SkinnedGameObject* skinnedGameObject = GetSkinnedGameObject();
        CharacterController* characterController = GetCharacterController();

        if (!skinnedGameObject) return;
        if (!characterController) return;

        // TODO:
        // Get position from PhysX character controller

        m_position = characterController->GetFootPosition();

        // Compute euler from forward vector
        glm::vec3 start = m_position;
        glm::vec3 end = m_position + m_forward;
        m_rotation.x = 0.0f;
        m_rotation.y = Hell::Math::YawBetweenPoints(start, end) + (HELL_PI * 0.5f);
        m_rotation.z = 0.0f;

        skinnedGameObject->SetPosition(m_position);
        skinnedGameObject->SetRotationX(m_rotation.x);
        skinnedGameObject->SetRotationY(m_rotation.y);
        skinnedGameObject->SetRotationZ(m_rotation.z);
    }
}
