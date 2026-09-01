#pragma once

#include "Hell/Math/GLM.h"

namespace Unloved {

struct Camera {
    void Update();
    void SetPosition(glm::vec3 position);
    void SetEulerRotation(glm::vec3 rotation);
    void Orbit(float pitchOffset, float yawOffset);
    void AddPitch(float value);
    void AddYaw(float value);
    void AddHeight(float value);
    void SetMinPitch(float value);
    void SetMaxPitch(float value);
    const glm::mat4& GetViewMatrix() const;
    const glm::mat4& GetInverseViewMatrix() const;
    const glm::vec3& GetPosition() const;
    const glm::vec3& GetEulerRotation() const;
    const glm::quat& GetQuaternionRotation() const;
    const glm::vec3& GetForward() const;
    const glm::vec3& GetUp() const;
    const glm::vec3& GetRight() const;
    const glm::vec3 GetForwardXZ() const;
    const float GetPitch() const { return m_rotation.x; }
    const float GetYaw() const { return m_rotation.y; }

private:
    glm::vec3 m_position = glm::vec3(0.0f);
    glm::vec3 m_rotation = glm::vec3(0.0f);
    glm::quat m_rotationQ = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::mat4 m_viewMatrix = glm::mat4(1.0f);
    glm::mat4 m_inverseViewMatrix = glm::mat4(1.0f);
    glm::vec3 m_forward = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 m_forwardXZ = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 m_up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 m_right = glm::vec3(1.0f, 0.0f, 0.0f);
    float m_minPitch = -1.5f;
    float m_maxPitch = 1.5f;
};

}
