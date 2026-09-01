#pragma once

#include "Unloved/Common/Enums.h"

#include "Unloved/Camera/Frustum.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Unloved {

struct SpaceCoords {
    float width;
    float height;
    float localMouseX;
    float localMouseY;
};

struct Viewport {
public:
    Viewport(uint32_t viewportIndex, const glm::vec2& position = { 0.0f, 0.0f }, const glm::vec2& size = { 1.0f, 1.0f }, bool isOrthographic = true);
    void Update();

    void SetOrthographic(float orthoSize, float nearPlane, float farPlane);
    void SetPerspective(float fov, float nearPlane, float farPlane);
    void SetPosition(const glm::vec2& position);
    void SetSize(const glm::vec2& size);
    void SetMirrorId(uint64_t mirrorId);
    void Show();
    void Hide();
    void SetViewportMode(ShadingMode viewportMode);
    void SetOrthoSize(float value);
    const bool IsVisible() const;
    const bool IsOrthographic() const;
    const bool IsHovered() const;
    const float GetOrthoSize() const;
    const float GetPerspectiveFOV() const;
    glm::vec2 GetPosition() const;
    glm::vec2 GetSize() const;
    glm::mat4 GetProjectionMatrix() const;
    glm::mat4 GetProjectionMatrixReverseZ() const;
    glm::mat4 GetPerpsectiveMatrix() const;
    glm::mat4 GetOrthographicMatrix() const;
    glm::vec2 WorldToScreen(const glm::mat4& viewMatrix, const glm::vec3& worldPosition) const;
    glm::ivec2 GetLocalMouseCoords();
    ShadingMode GetViewportMode() const;
    SpaceCoords GetWindowSpaceCoords() const;
    SpaceCoords GetGBufferSpaceCoords() const;

    uint64_t GetMirrorId()      { return m_mirrorId; }
    Unloved::Frustum& GetFrustum()       { return m_frustum; }
    uint32_t GetViewportIndex() { return m_viewportIndex; }
    int GetLeftPixel()          { return (int)m_leftPixel; }
    int GetRightPixel()         { return (int)m_rightPixel; }
    int GetTopPixel()           { return (int)m_topPixel; }
    int GetBottomPixel()        { return (int)m_bottomPixel; }
    float GetNearPlane()        { return m_nearPlane; }
    float GetFarPlane()         { return m_farPlane; }

private:
    glm::vec2 m_position;           // Top-left corner in normalized displayed-screen space (0-1)
    glm::vec2 m_size;               // Width and height in normalized screen space (0-1)
    bool m_isOrthographic = false;  // True for orthographic, false for perspective
    float m_orthoSize;
    float m_nearPlane;
    float m_farPlane;
    float m_fov;
    float m_aspect;
    float m_leftPixel;
    float m_rightPixel;
    float m_topPixel;
    float m_bottomPixel;
    bool m_isVisible = true;
    bool m_hasHover = false;
    uint32_t m_viewportIndex = 0;
    glm::mat4 m_perspectiveMatrix;
    glm::mat4 m_perspectiveMatrixReverseZ;
    glm::mat4 m_orthographicMatrix;
    glm::mat4 m_orthographicMatrixReverseZ;
    glm::vec3 m_mouseRayDirPerspective;
    glm::vec3 m_mouseRayDirOrthographic;
    Unloved::Frustum m_frustum;
    ShadingMode m_viewportMode;
    SpaceCoords m_windowSpaceCoords;
    SpaceCoords m_gBufferSpaceCoords;
    uint64_t m_mirrorId = 0;

    void UpdateProjectionMatrices();
    void UpdateSpaceCoords(SpaceCoords& spaceCoords, uint32_t fullResolutionWidth, uint32_t fullResolutionHeight);
};

}
