#include "Player.h"

#include "Hell/Input.h"
#include "Hell/Math/Math.h"

#include "Unloved/Debug/Debug.h"
#include "Unloved/Session/Session.h"
#include "Unloved/EditorSession/EditorSession.h"

#include <glm/gtc/noise.hpp>

namespace Input = Hell::Input;
namespace InputMulti = Hell::InputMulti;

namespace Unloved {

void Player::UpdateHeadBob(float deltaTime) {
    if (!HasControl()) return;

    bool pressingMoveKey = PressingWalkLeft() || PressingWalkRight() || PressingWalkForward() || PressingWalkBackward();

    if (!CameraIsUnderwater() && !IsWading() && pressingMoveKey) {

        m_headBobTime += deltaTime;

        float frequency = 14.4;
        if (IsCrouching()) {
            frequency *= 0.75f;
        }
        else if (IsRunning()) {
            frequency *= 1.1f;
        }

        float bobIntensity = 0.05f;
        float noiseIntensity = 0.02f;
        m_bobOffsetY = glm::sin(m_headBobTime * frequency) * bobIntensity;
        m_bobOffsetX = glm::sin(m_headBobTime * frequency * 0.5f) * (bobIntensity * 0.5f);
        float noiseOffsetY = glm::perlin(glm::vec2(m_headBobTime * 0.1f, 0.0f)) * noiseIntensity;
        float noiseOffsetX = glm::perlin(glm::vec2(0.0f, m_headBobTime * 0.1f)) * noiseIntensity;
        m_headBob = glm::vec3(m_bobOffsetX + noiseOffsetX, m_bobOffsetY + noiseOffsetY, 0.0f);
    }
    else {
        // TODO: find the values that make a footstep always play on the first step u take
        m_headBobTime = Hell::Math::InterpTo(m_headBobTime, 0.2f, deltaTime, 10);
        m_headBob.x = Hell::Math::InterpTo(m_headBob.x, 0.0f, deltaTime, 10);
        m_headBob.y = Hell::Math::InterpTo(m_headBob.y, 0.0f, deltaTime, 10);
        m_headBob.z = Hell::Math::InterpTo(m_headBob.z, 0.0f, deltaTime, 10);
        m_bobOffsetX = Hell::Math::InterpTo(m_bobOffsetX, 0.0f, deltaTime, 10);
        m_bobOffsetY = Hell::Math::InterpTo(m_bobOffsetY, 0.0f, deltaTime, 10);
    }
}

void Player::UpdateBreatheBob(float deltaTime) {
    if (Hell::Math::IsNan(GetCameraUp()) || Hell::Math::IsNan(GetCameraRight())) return;

    m_breatheBobTime += deltaTime;

    m_breatheBobTime = 0;

    float breathSpeed = 0.5f;
    float horizontalBreathIntensity = 0.00025f;
    float verticalBreathIntensity = 0.002f;
    float noiseIntensity = 0.00f;

    float breathOffsetX = glm::sin(m_breatheBobTime * breathSpeed * glm::two_pi<float>()) * horizontalBreathIntensity;
    float breathOffsetY = glm::sin(m_breatheBobTime * breathSpeed * glm::two_pi<float>() * 0.5f) * verticalBreathIntensity;

    float noiseOffsetX = glm::perlin(glm::vec2(m_breatheBobTime * 0.05f, 0.0f)) * noiseIntensity;
    float noiseOffsetY = glm::perlin(glm::vec2(0.0f, m_breatheBobTime * 0.05f)) * noiseIntensity;

    m_breatheBob = GetCameraUp() * (breathOffsetY + noiseOffsetY);
    m_breatheBob += GetCameraRight() * glm::vec3(breathOffsetX + noiseOffsetX);
}

void Player::UpdateCamera(float deltaTime) {
    // Mouselook
    if (EditorSession::IsInactive() && m_controlEnabled) {
        float xOffset = (float)InputMulti::GetMouseXOffset(m_mouseIndex);
        float yOffset = (float)InputMulti::GetMouseYOffset(m_mouseIndex);
        m_camera.AddPitch(-yOffset * m_mouseSensitivity);
        m_camera.AddYaw(xOffset * m_mouseSensitivity);
    }

    // Height
    float crouchDownSpeed = 17.5f;
    float viewHeightTarget = m_crouching ? m_viewHeightCrouching : m_viewHeightStanding;
    m_currentViewHeight = Hell::Math::InterpTo(m_currentViewHeight, viewHeightTarget, deltaTime, crouchDownSpeed);

    if (!IsPlayingPiano()) {
        if (Input::KeyDown(HELL_KEY_EQUAL)) {
            m_cameraHeightModifier += 1.0f * deltaTime;
        }
        if (Input::KeyDown(HELL_KEY_MINUS)) {
            m_cameraHeightModifier -= 1.0f * deltaTime;
        }
        if (Input::KeyDown(HELL_KEY_BACKSPACE)) {
            m_cameraHeightModifier = 0.0f;
        }
    }

    // Set cosition position
    m_camera.SetPosition(GetFootPosition() + glm::vec3(0, m_currentViewHeight + m_cameraHeightModifier, 0) + m_headBob + m_breatheBob);

    // Calculate view weapon camera matrix
    SkinnedGameObject* viewWeapon = GetViewWeaponSkinnedGameObject();
    const glm::mat4 cameraAnimatedTransform = viewWeapon->GetNodeModelSpaceMatrix("camera");
    const glm::mat4& cameraLocalBindTransform = viewWeapon->GetLocalBindTransformByNodeName("camera");
    m_animatedCameraMatrix = cameraAnimatedTransform * glm::inverse(cameraLocalBindTransform);

    // HACK because the non-knife weapons are using the old rig which has fucked camera inverse transform
    //if (viewWeapon && GetCurrentWeaponInfo()->itemInfoName != "Knife") {
        m_animatedCameraMatrix[3][0] = 0.0f;
        m_animatedCameraMatrix[3][1] = 0.0f;
        m_animatedCameraMatrix[3][2] = 0.0f;
    //}

    //if (Hell::Input::KeyPressed(HELL_KEY_J)) {
    //    viewWeapon->PrintNodeNames();
    //}

    //if (GetCurrentWeaponInfo()->itemInfoName == "AKS74U") {
    //    m_animatedCameraMatrix = viewWeapon->GetAnimatedTransformByBoneName("camera");
    //    m_animatedCameraMatrix[3][0] = 0.0f;
    //    m_animatedCameraMatrix[3][1] = 0.0f;
    //    m_animatedCameraMatrix[3][2] = 0.0f;
    //}


    // Walk tilt
    const float walkSpeed = 5.0f;
    const float maxTilt = 0.005f; // Radians
    const float noiseScale = 0.4f;

    if (IsGrounded() && m_currentSpeed > 0.1f) {
        m_walkTiltTimer += deltaTime * walkSpeed;
    }
    else {
        m_walkTiltTimer = glm::mix(m_walkTiltTimer, 0.0f, deltaTime * 2.0f); // Smoothly settle back to 0 when idle
    }

    float baseSway = std::sin(m_walkTiltTimer) * maxTilt;
    float noise = glm::perlin(glm::vec2(m_walkTiltTimer * 0.5f, 1.0f)) * (maxTilt * noiseScale); // 0.5f makes the noise frequency different from the sine
    float movementFactor = glm::clamp(m_currentSpeed / 5.0f, 0.0f, 1.0f); // Scale by speed (more tilt the faster you move)
    float cameraRoll = (baseSway + noise) * movementFactor;

    Transform tilt;
    tilt.rotation.y = cameraRoll * 0.1f;
    tilt.rotation.z = cameraRoll;

    //std::cout << "m_walkTiltTimer: " << m_walkTiltTimer << "\n";
    //std::cout << "cameraRoll:      " << cameraRoll << "\n";
    //std::cout << "noise:           " << noise << "\n";
    //std::cout << "noise:           " << noise << "\n";
    //std::cout << "\n";

    //m_animatedCameraMatrix = tilt.to_mat4() * m_animatedCameraMatrix;


    if (!IsAlive()) {
        SkinnedGameObject* characterModel = GetCharacterModelSkinnedGameObject();
        Ragdoll* Ragdoll = GetRagdoll();

        if (characterModel && Ragdoll) {
            characterModel->SetAnimationModeToRagdoll();
        }

        if (Ragdoll) {
            glm::mat4 headMatrix = Ragdoll->GetRigidWorldTransform("CC_Base_Head");
            m_deathCamViewMatrix = glm::inverse(headMatrix);
        }
    }
}

} // namespace Unloved
