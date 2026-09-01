#pragma once

#include "Unloved/EditorSession/EditorSessionTypes.h"

#include <cstdint>
#include <glm/mat4x4.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

struct CreateInfoCollection;

namespace Unloved::EditorSession {

    // Viewport camera
    struct EditorCamera {
        EditorCamera();

        void LookAt(const glm::vec3& pivot, const glm::vec3& forward, const glm::vec3& up);
        void SetPivot(const glm::vec3& pivot);
        void SetOrientation(const glm::quat& orientation);
        void SetDistance(float distance);

        const glm::mat4& GetViewMatrix() const;
        const glm::vec3& GetPivot() const;
        const glm::vec3& GetPosition() const;
        const glm::vec3& GetForward() const;
        const glm::vec3& GetUp() const;
        const glm::vec3& GetRight() const;
        const glm::quat& GetOrientation() const;
        float GetDistance() const;

    private:
        void UpdateViewMatrix();

        glm::vec3 m_pivot = glm::vec3(0.0f);
        glm::vec3 m_position = glm::vec3(0.0f);
        glm::vec3 m_forward = glm::vec3(0.0f, 0.0f, -1.0f);
        glm::vec3 m_up = glm::vec3(0.0f, 1.0f, 0.0f);
        glm::vec3 m_right = glm::vec3(1.0f, 0.0f, 0.0f);
        glm::quat m_orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        glm::mat4 m_viewMatrix = glm::mat4(1.0f);
        float m_distance = 10.0f;
    };
}

namespace Unloved::EditorSession::Viewports {

    void Init();
    void CancelNavigation();
    void PrepareInitialView(const CreateInfoCollection& createInfoCollection);
    void PrepareInitialMapView(uint32_t chunkCountX, uint32_t chunkCountZ, float originHeight);
    void PrepareInitialRagdollView();
    void ApplyInitialView();
    void Update();
    void UpdateInput(bool allowKeyboardInput, bool allowMouseInput);
    void RenderLabels();
    void SetMode(uint32_t viewportIndex, EditorViewportMode mode);
    void SetPivot(const glm::vec3& pivot);
    void SetOrbitPoint(uint32_t viewportIndex, const glm::vec3& orbitPoint);

    EditorCamera* GetCameraByIndex(uint32_t viewportIndex);
    const glm::mat4& GetViewMatrix(uint32_t viewportIndex);
    const glm::vec3& GetMouseRayOrigin(uint32_t viewportIndex);
    const glm::vec3& GetMouseRayDirection(uint32_t viewportIndex);
    EditorViewportMode GetMode(uint32_t viewportIndex);
    int32_t GetHoveredViewportIndex();
    bool IsPanning();
    bool IsOrbiting();
    bool IsFlyMode();
    float GetOrthographicSize(uint32_t viewportIndex);
    float GetPerspectiveFov(uint32_t viewportIndex);
}
