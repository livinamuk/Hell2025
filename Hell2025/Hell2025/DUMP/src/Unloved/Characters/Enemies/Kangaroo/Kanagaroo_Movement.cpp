#include "Kangaroo.h"
#include "Hell/Math/Easing.h"
#include "Unloved/Session/Session.h"
#include "Hell/Physics/Physics.h"
#include <glm/gtc/quaternion.hpp>
#include "Unloved/Systems/Pathfinding/AStarMap.h"
#include "Unloved/Render/Renderer.h"

namespace Unloved {

    void Kangaroo::GoToTarget(glm::vec3 targetPosition) {
        m_targetPosition = targetPosition;

        if (m_animationState == KanagarooAnimationState::IDLE) {
            m_animationState = KanagarooAnimationState::IDLE_TO_HOP;
            PlayAnimation("Kangaroo_IdleToHop", 1.0f);
        }
    }

    void Kangaroo::UpdateMovementLogic(float deltaTime) {
        CharacterController* characterController = GetCharacterController();
        Unloved::SkinnedGameObject* skinnedGameObject = GetSkinnedGameObject();

        if (!characterController) return;
        if (!skinnedGameObject) return;

        std::vector<glm::ivec2> path = GetPath();

        //if (path.size() <= 2) {
        //    // You need to actually transition into idle
        //    // You need to actually transition into idle
        //    // You need to actually transition into idle
        //    // You need to actually transition into idle
        //    m_animationState = KanagarooAnimationState::IDLE;
        //    PlayAndLoopAnimation("Kangaroo", 1.0f);
        //}

        // Chilling idle
        if (m_animationState == KanagarooAnimationState::IDLE) {

        }

        // Zero out displacement
        glm::vec3 displacement = glm::vec3(0.0f);

        // Update displacement if roo is in a moving state
        if (m_animationState == KanagarooAnimationState::HOP || m_animationState == KanagarooAnimationState::IDLE_TO_HOP || m_animationState == KanagarooAnimationState::BITE) {

            if (HasValidPath()) {
                Cell* nextPathCell = m_aStar.m_finalPath[1];
                glm::ivec2 nextPathCellCoords = glm::ivec2(nextPathCell->x, nextPathCell->y);
                glm::vec3 nextPathWorldPosition = AStarMap::GetWorldSpacePositionFromCellCoords(nextPathCellCoords);

                // Compute forward vector from 2d direction
                glm::vec3 normalizedPosition = m_position * glm::vec3(1.0f, 0.0f, 1.0f);
                glm::vec3 targetForward = glm::normalize(nextPathWorldPosition - normalizedPosition);
                float turnSpeed = 2.5f;
                float alpha = glm::clamp(turnSpeed * deltaTime, 0.0f, 1.0f);
                m_forward = glm::normalize(m_forward * (1.0f - alpha) + targetForward * alpha);

                // Calculate kangaroo speed
                float speed = 7.5f;

                if (m_animationState == KanagarooAnimationState::BITE) {
                    // Reduce speed as bite ends
                    float t = m_timeSinceBiteBegan / 1.5f;
                    float easedT = Hell::Easing::EaseOut(t, 3.0f);
                    easedT = glm::clamp(easedT, 0.0f, 1.0f);
                    float newSpeed = glm::mix(speed, 0.0f, easedT);
                    speed = newSpeed;
                }

                displacement = m_forward * speed * deltaTime;
            }
        }

        // Gravity
        if (m_grounded) {
            m_yVelocity = -3.5f; // Keep the player on the ground
        }
        else {
            float gravity = 15.75f; // 9.8 feels like the moon
            m_yVelocity -= gravity * deltaTime;
        }
        displacement.y += m_yVelocity * deltaTime;

        // Move character controller
        Hell::Physics::MoveCharacterController(m_characterControllerId, displacement);
        m_position = Hell::Physics::GetCharacterControllerPosition(m_characterControllerId);


        // Check grounded state
        m_grounded = false;
        for (CharacterCollisionReport& report : Hell::Physics::GetCharacterCollisionReports()) {
            m_grounded = (report.characterController == characterController->GetPxController() && report.hitNormal.y > 0.5f);
        }


      // // Check grounded state
      // for (CharacterCollisionReport& report : Hell::Physics::GetCharacterCollisionReports()) {
      //     m_grounded = (report.characterController == characterController->GetPxController() && report.hitNormal.y > 0.5f);
      // }

        Hell::Physics::ClearCharacterControllerCollsionReports();
    }
}
