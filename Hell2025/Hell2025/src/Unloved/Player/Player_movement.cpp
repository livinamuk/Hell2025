#include "Player.h"

#include "Hell/Input.h"
#include "Hell/Math/Math.h"

#include "Unloved/EditorSession/EditorSession.h"
#include "Unloved/Systems/Ocean/Ocean.h"
#include "Unloved/Session/Session.h" // for total time
#include "Unloved/World/World.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

namespace Input = Hell::Input;

namespace Unloved {

void Player::UpdateMovementMode() {
    if (m_movementMode == PlayerMovementMode::LADDER_TRANSITION) {
        return;
    }

    if (m_movementMode == PlayerMovementMode::LADDER) {
        if (World::GetLadderByObjectId(m_ladderMoveData.ladderId)) {
            return;
        }
        m_ladderMoveData = {};
        m_movementMode = PlayerMovementMode::NONE;
    }

    if (!m_controlEnabled || EditorSession::IsActive()) {
        m_movementMode = PlayerMovementMode::NONE;
        return;
    }

    if (TryAutoMountLadder()) {
        if (m_movementMode != PlayerMovementMode::LADDER_TRANSITION) {
            m_movementMode = PlayerMovementMode::NONE;
        }
        return;
    }

    if (World::HasOcean() && GetCameraPosition().y < Ocean::GetWaterHeightAtPlayer(m_viewportIndex) + 0.1f) {
        m_movementMode = PlayerMovementMode::SWIMMING;
        return;
    }
    else {
        m_movementMode = PlayerMovementMode::WALKING;
    }
}

void Player::UpdateMovement(float deltaTime) {
    UpdateMovementMode();

    switch (m_movementMode) {
        case PlayerMovementMode::NONE:                                                 break;
        case PlayerMovementMode::WALKING:           UpdateWalkingMovement(deltaTime);  break;
        case PlayerMovementMode::SWIMMING:          UpdateSwimmingMovement(deltaTime); break;
        case PlayerMovementMode::LADDER:            UpdateLadderMovement(deltaTime);   break;
        case PlayerMovementMode::LADDER_TRANSITION: UpdateLadderTransition(deltaTime); break;
    }

    m_characterControllerAABB = Hell::Physics::GetCharacterControllerAABB(m_characterControllerId);
}

void Player::UpdateWalkingMovement(float deltaTime) {
    m_crouching = PressingCrouch();
    m_groundedLastFrame = m_grounded;

    if (EditorSession::IsInactive() && m_controlEnabled) {

        // TO DO!!!!!!!!!!!!!!
        // You need to find a way to cleanly determine character controller grounded states, and whether its head is touching the ceiling
        // FIND ME
        PxController* m_characterController = nullptr;
        CharacterController* characterControler = Hell::Physics::GetCharacterControllerById(m_characterControllerId);
        if (characterControler) {
            m_characterController = characterControler->GetPxController();
        }


        float accelerationSpeed = 7.5f;
        float decelerationSpeed = 4.0f;
        float airbornDamping = 2.5f;

        glm::vec3 inputDirection = glm::vec3(0.0f);

        // WSAD input
        if (PressingWalkLeft()) inputDirection -= m_camera.GetRight();
        if (PressingWalkRight()) inputDirection += m_camera.GetRight();
        if (PressingWalkForward()) inputDirection += m_camera.GetForwardXZ();
        if (PressingWalkBackward()) inputDirection -= m_camera.GetForwardXZ();

        // Accelerate
        if (PressingWalkLeft() || PressingWalkRight() || PressingWalkForward() || PressingWalkBackward()) {
            m_movementDirection = glm::normalize(inputDirection);
            m_acceleration += accelerationSpeed * deltaTime;
        }
        // Walking deceleration
        else if (m_grounded) {
            m_acceleration -= accelerationSpeed * deltaTime;
        }
        // Airborne deceleration
        else {
            m_acceleration -= airbornDamping * deltaTime;
        }     

        // Clamp acceleration between 0 and 1
        m_acceleration = glm::clamp(m_acceleration, 0.0f, 1.0f);

        // Zero out movement direction when no longer moving
        if (m_acceleration == 0.0f) {
            m_movementDirection = glm::vec3(0.0f);
        }

        // Determine if moving
        if (glm::length(m_movementDirection) > 0.001f) {
            m_moving = true;
        }
        
        // Calculate movement speed
        float targetSpeed = GetTargetWalkingSpeed();
        float interpolationSpeed = 18.0f;
        if (!IsMoving()) {
            targetSpeed = 0.0f;
            interpolationSpeed = 22.0f;
        }
        m_currentSpeed = Hell::Math::InterpTo(m_currentSpeed, targetSpeed, deltaTime, interpolationSpeed);

        // Jump
        if (PressingJump() && HasControl() && m_grounded) {
            m_animatedHumanoid.PlayJumpAnimation();
            m_jumpAnimationActive = true;
            m_stationaryJumpTailEligible = !m_moving && !m_crouching;
            m_yVelocity = 4.5f;  // Magic value for jump strength
            m_grounded = false;
        }

        // Gravity
        if (m_grounded) {
            m_yVelocity = -3.5f; // Keep the player on the ground
        }
        else {
            float gravity = 15.75f; // 9.8 feels like the moon
            m_yVelocity -= gravity * deltaTime;
        }

        // Move character controller
        glm::vec3 displacement = m_movementDirection * m_acceleration;
        displacement *= m_currentSpeed * deltaTime;
        displacement.y += m_yVelocity * deltaTime;

        // Update character controller
        Hell::Physics::MoveCharacterController(m_characterControllerId, displacement);
        //m_position = Hell::Physics::GetCharacterControllerPosition(m_characterControllerId);

        // Check grounded state
        m_grounded = false;
        for (CharacterCollisionReport& report : Hell::Physics::GetCharacterCollisionReports()) {
            m_grounded = (report.characterController == m_characterController && report.hitNormal.y > 0.5f);
        }

        // Check for head hitting ceiling
        bool ceilingHit = false;
        for (CharacterCollisionReport& report : Hell::Physics::GetCharacterCollisionReports()) {
            if (report.characterController == m_characterController && report.hitNormal.y < -0.5f) {
                ceilingHit = true;
                break;
            }
        }
        if (ceilingHit && m_yVelocity > 0.0f) {
            m_yVelocity = 0.0f;
        }

        Hell::Physics::ClearCharacterControllerCollsionReports();

        // Piano hacks
        if (IsPlayingPiano()) {
            m_crouching = true;
            m_moving = false;
        }
    }
}

inline float SmoothLerp(float current, float target, float deltaTime, float smoothTime) {
    // t: fraction of the gap closed this frame (smoothTime in seconds to close ~63%)
    float t = 1.0f - std::exp(-deltaTime / smoothTime);
    return current + (target - current) * t;
}

void Player::UpdateSwimmingMovement(float deltaTime) {
    m_crouching = false;
    m_grounded = false;
    m_groundedLastFrame = false;


    float accelerationSpeed = 7.5f;
    float decelerationSpeed = 4.0f;
    float airbornDamping = 2.5f;

    m_walkingSpeed = 4.25f;
    m_crouchingSpeed = 2.325f;

    glm::vec3 inputDirection = glm::vec3(0.0f);

    // WSAD input
    if (PressingWalkLeft()) inputDirection -= m_camera.GetRight();
    if (PressingWalkRight()) inputDirection += m_camera.GetRight();
    if (PressingWalkForward()) inputDirection += m_camera.GetForward();
    if (PressingWalkBackward()) inputDirection -= m_camera.GetForward();

    // Accelerate
    if (PressingWalkLeft() || PressingWalkRight() || PressingWalkForward() || PressingWalkBackward()) {
        m_movementDirection = glm::normalize(inputDirection);
        m_acceleration += accelerationSpeed * deltaTime;
    }
    // Walking deceleration
    else if (m_grounded) {
        m_acceleration -= accelerationSpeed * deltaTime;
    }
    // Airborne deceleration
    else {
        m_acceleration -= airbornDamping * deltaTime;
    }

    // Clamp acceleration between 0 and 1
    m_acceleration = glm::clamp(m_acceleration, 0.0f, 1.0f);

    // Zero out movement direction when no longer moving
    if (m_acceleration == 0.0f) {
        m_movementDirection = glm::vec3(0.0f);
    }

    // Determine if moving
    if (glm::length(m_movementDirection) > 0.001f) {
        m_moving = true;
    }

    // Calculate movement speed
    float targetSpeed = m_swimmingSpeed;
    float interpolationSpeed = 18.0f;
    if (!IsMoving()) {
        targetSpeed = 0.0f;
        interpolationSpeed = 22.0f;
    }
    m_currentSpeed = Hell::Math::InterpTo(m_currentSpeed, targetSpeed, deltaTime, interpolationSpeed);


    // Move character controller
    glm::vec3 displacement = m_movementDirection * m_acceleration;
    displacement *= m_currentSpeed * deltaTime;
    displacement.y += m_yVelocity * deltaTime;

    //float yDisplacement = m_yVelocity * deltaTime;
    //displacement.y += m_yVelocity * deltaTime;;
    float yVelocityCancelationInterpolationSpeed = 15;
    m_yVelocity = Hell::Math::InterpTo(m_yVelocity, 0.0f, deltaTime, yVelocityCancelationInterpolationSpeed);

    float m_swimVerticalInterpolationSpeed = 15.0f;
    float m_swimMaxVerticalAcceleration = 0.05f;

    // Vertical movement
    if (PressingCrouch()) {
        m_swimVerticalAcceleration = Hell::Math::InterpTo(m_swimVerticalAcceleration, -m_swimMaxVerticalAcceleration, deltaTime, m_swimVerticalInterpolationSpeed);
    }
    else if (PressingJump()) {
        m_swimVerticalAcceleration = Hell::Math::InterpTo(m_swimVerticalAcceleration, m_swimMaxVerticalAcceleration, deltaTime, m_swimVerticalInterpolationSpeed);
    }
    else {
        m_swimVerticalAcceleration = Hell::Math::InterpTo(m_swimVerticalAcceleration, 0.0f, deltaTime, 20.0f);
    }
    displacement.y += m_swimVerticalAcceleration;

    // Update character controller
    Hell::Physics::MoveCharacterController(m_characterControllerId, displacement);
    //m_position = Hell::Physics::GetCharacterControllerPosition(m_characterControllerId);

    Hell::Physics::ClearCharacterControllerCollsionReports();

    //static bool test = false;
    //if (Input::KeyPressed(HELL_KEY_L)) {
    //    test = !test;
    //}
    //test = true;

    m_swimmingSpeed = 3.0;

    // Snap to ocean surface
    if (!PressingCrouch()) {
        const float surfaceThreshold = 0.05f;
        float targetY = Ocean::GetWaterHeightAtPlayer(m_viewportIndex) - m_viewHeightStanding + 0.05f;
        m_smoothedWaterY = SmoothLerp(m_smoothedWaterY, targetY, deltaTime, 0.1f);
        glm::vec3 footPos = GetFootPosition();
        float dy = m_smoothedWaterY - footPos.y;
        glm::vec3 snapDisplacement{ 0.0f, dy, 0.0f };
        if (glm::abs(dy) < surfaceThreshold) {

            float downDot = glm::dot(m_camera.GetForward(), glm::vec3(0.0f, -1.0f, 0.0f));
            const float lookDownThreshold = 0.01f;
            if (downDot < lookDownThreshold) {
                Hell::Physics::MoveCharacterController(m_characterControllerId, snapDisplacement);
            }
        }
    }
}

   //m_moving = false;
   //
   //// WSAD movement
   //glm::vec3 displacement = glm::vec3(0);
   //if (PressingWalkForward()) {
   //    displacement += m_camera.GetForward();
   //    m_moving = true;
   //}
   //if (PressingWalkBackward()) {
   //    displacement -= m_camera.GetForward();
   //    m_moving = true;
   //}
   //if (PressingWalkLeft()) {
   //    displacement -= m_camera.GetRight();
   //    m_moving = true;
   //}
   //if (PressingWalkRight()) {
   //    displacement += m_camera.GetRight();;
   //    m_moving = true;
   //}
   //// Calculate speed
   //float targetSpeed = m_swimmingSpeed;
   //float interpolationSpeed = 18.0f;
   //if (!IsMoving()) {
   //    targetSpeed = 0.0f;
   //    interpolationSpeed = 22.0f;
   //}
   //m_currentSpeed = Hell::Math::InterpTo(m_currentSpeed, targetSpeed, deltaTime, interpolationSpeed);
   //
   //// Normalize displacement vector and include player speed
   //float len = length(displacement);
   //if (len != 0.0) {
   //    displacement = (displacement / len) * m_currentSpeed * deltaTime;
   //}
   //float yDisplacement = m_yVelocity * deltaTime;
   //displacement.y += yDisplacement;
   //float yVelocityCancelationInterpolationSpeed = 15;
   //m_yVelocity = Hell::Math::InterpTo(m_yVelocity, 0, deltaTime, yVelocityCancelationInterpolationSpeed);
   //
   //float m_swimVerticalInterpolationSpeed = 15.0f;
   //float m_swimMaxVerticalAcceleration = 0.05f;
   //
   //// Vertical movement
   //if (PressingCrouch()) {
   //    m_swimVerticalAcceleration = Hell::Math::InterpTo(m_swimVerticalAcceleration, -m_swimMaxVerticalAcceleration, deltaTime, m_swimVerticalInterpolationSpeed);
   //}
   //else if (PresingJump()) {
   //    m_swimVerticalAcceleration = Hell::Math::InterpTo(m_swimVerticalAcceleration, m_swimMaxVerticalAcceleration, deltaTime, m_swimVerticalInterpolationSpeed);
   //}
   //else {
   //    m_swimVerticalAcceleration = Hell::Math::InterpTo(m_swimVerticalAcceleration, 0.0f, deltaTime, 20.0f);
   //}
   //displacement.y += m_swimVerticalAcceleration;
   //
   //MoveCharacterController(glm::vec3(displacement.x, displacement.y, displacement.z));
//}

//bool Player::IsUnderWater() {
//    return m_underWater;
//}

bool Player::IsPlayingPiano() {
    return m_isPlayingPiano;
}

bool Player::IsMoving() {
    return m_moving;
}

bool Player::IsCrouching() {
    return m_crouching;
}

bool Player::IsGrounded() {
    return m_grounded;
}

bool Player::FeetEnteredUnderwater() {
    return !m_waterState.feetUnderWaterPrevious && m_waterState.feetUnderWater;
}

bool Player::FeetExitedUnderwater() {
    return m_waterState.feetUnderWaterPrevious && !m_waterState.feetUnderWater;
}

bool Player::CameraEnteredUnderwater() {
    return !m_waterState.cameraUnderWaterPrevious && m_waterState.cameraUnderWater;
}

bool Player::CameraExitedUnderwater() {
    return m_waterState.cameraUnderWaterPrevious && !m_waterState.cameraUnderWater;
}

bool Player::CameraIsUnderwater() {
    return m_waterState.cameraUnderWater;
}

bool Player::FeetBelowWater() {
    return m_waterState.feetUnderWater;
}

bool Player::IsSwimming() {
    return m_waterState.cameraUnderWater && IsMoving();
}

bool Player::IsWading() {
    return m_waterState.wading;
}

bool Player::StartedSwimming() {
    return !m_waterState.swimmingPrevious && m_waterState.swimming;
}

bool Player::StoppedSwimming() {
    return m_waterState.swimmingPrevious && !m_waterState.swimming;
}

bool Player::StartingWading() {
    return !m_waterState.wadingPrevious && m_waterState.wading;
}

bool Player::StoppedWading() {
    return m_waterState.wadingPrevious && !m_waterState.wading;
}

float Player::GetFeetDistanceBeneathWater() {
    float waterHeight = Ocean::GetWaterHeightAtPlayer(m_viewportIndex);
    float feetHeight = GetFootPosition().y;
    return waterHeight - feetHeight;
}

} // namespace Unloved
