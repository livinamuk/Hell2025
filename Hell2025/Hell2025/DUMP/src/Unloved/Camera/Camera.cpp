#include "Camera.h"

#include "Hell/Input.h"


#include "Unloved/Session/Session.h"

#include <glm/gtc/type_ptr.hpp>

#include <algorithm>

namespace Input = Hell::Input;

namespace Unloved {

void Camera::Update() {

    // Wrap yaw to 0 to PI * 2 range
    m_rotation.y = glm::mod(m_rotation.y, HELL_PI * 2.0f);
    m_rotation.z = 0.0f;

    glm::mat4 animatedCameraMatrix = glm::mat4(1.0f);

    for (int i = 0; i < Unloved::Session::GetLocalPlayerCount(); i++) {
        Unloved::Player* player = Unloved::Session::GetLocalPlayerByViewportIndex(i);
        if (&player->GetCamera() == this) {
            animatedCameraMatrix = player->GetAnimatedCameraMatrix();
            break;
        }
    }

    // Build the view matrix
    glm::mat4 m = glm::translate(glm::mat4(1), m_position);
    m *= glm::mat4_cast(glm::normalize(glm::quat(m_rotation)));
    glm::mat4 baseViewMatrix = glm::inverse(m);

    // Then apply weapon camera matrix
    m_viewMatrix = animatedCameraMatrix * baseViewMatrix;

    // Now recreate the inverse view matrix from that above
    m_inverseViewMatrix = glm::inverse(m_viewMatrix);

    m_right = glm::vec3(m_inverseViewMatrix[0]);
    m_up = glm::vec3(m_inverseViewMatrix[1]);
    m_forward = -glm::vec3(m_inverseViewMatrix[2]);
}

void Camera::SetPosition(glm::vec3 position) {
    m_position = position;
    Update();
}

void Camera::SetEulerRotation(glm::vec3 rotation) {
    m_rotation = rotation;
    m_rotationQ = glm::quat(m_rotation);
    Update();
}

void Camera::AddPitch(float value) {
    m_rotation.x += value;
    m_rotation.x = std::clamp(m_rotation.x, m_minPitch, m_maxPitch);
    m_rotationQ = glm::quat(m_rotation);
    Update();
}

void Camera::AddYaw(float value) {
    m_rotation.y += value;
    Update();
}

void Camera::AddHeight(float value) {
    m_position.y += value;
    Update();
}

void Camera::SetMinPitch(float value) {
    m_minPitch = value;
    Update();
}

void Camera::SetMaxPitch(float value) {
    m_maxPitch = value;
    Update();
}

const glm::mat4& Camera::GetViewMatrix() const {
    return m_viewMatrix;
}

const glm::mat4& Camera::GetInverseViewMatrix() const {
    return m_inverseViewMatrix;
}

const glm::vec3& Camera::GetPosition() const {
    return m_position;
}

const glm::vec3& Camera::GetEulerRotation() const {
    return m_rotation;
}

const glm::quat& Camera::GetQuaternionRotation() const {
    return m_rotationQ;
}

const glm::vec3& Camera::GetForward() const {
    return m_forward;
}

const glm::vec3& Camera::GetUp() const {
    return m_up;
}

const glm::vec3& Camera::GetRight() const {
    return m_right;
}

const glm::vec3 Camera::GetForwardXZ() const {
    return glm::normalize(glm::vec3(m_forward.x, 0.0f, m_forward.z));
}

void Camera::Orbit(float pitchOffset, float yawOffset) {

    float sensitivity = 0.002f;
    AddPitch(-Input::GetMouseOffsetY() * sensitivity);
    AddYaw(-Input::GetMouseOffsetX() * sensitivity);
}

}
