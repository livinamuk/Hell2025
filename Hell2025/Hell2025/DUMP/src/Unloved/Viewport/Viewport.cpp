#include "Viewport.h"

#include "Hell/Backend/BackEnd.h"
#include "Hell/Input.h"
#include "Hell/Math/Range.h"
#include "Hell/Projection/Projection.h"

#include "Unloved/Common/Constants.h"

#include "Unloved/Config/Config.h"

namespace Unloved {


Viewport::Viewport(uint32_t viewportIndex, const glm::vec2& position, const glm::vec2& size, bool isOrthographic)
    : m_position(position),
    m_size(size),
    m_isOrthographic(isOrthographic),
    m_orthoSize(1.0f),
    m_nearPlane(Config::GetNearPlane()),
    m_farPlane(Config::GetFarPlane()),
    m_fov(1.0f),
    m_perspectiveMatrix(glm::mat4(1.0f)),
    m_orthographicMatrix(glm::mat4(1.0f)),
    m_viewportMode(ShadingMode::SHADED) {

    m_viewportIndex = viewportIndex;
    UpdateProjectionMatrices();
}

void Viewport::UpdateSpaceCoords(SpaceCoords& spaceCoords, uint32_t fullResolutionWidth, uint32_t fullResolutionHeight) {
    spaceCoords.width = fullResolutionWidth * m_size.x;
    spaceCoords.height = fullResolutionHeight * m_size.y;
    spaceCoords.localMouseX = Hell::Math::MapRange(Hell::Input::GetMouseX(), m_leftPixel, m_rightPixel, 0, spaceCoords.width);
    spaceCoords.localMouseY = Hell::Math::MapRange(Hell::Input::GetMouseY(), m_topPixel, m_bottomPixel, 0, spaceCoords.height);
}

void Viewport::Update() {
    const uint32_t windowWidth = Hell::BackEnd::GetCurrentWindowWidth();
    const uint32_t windowHeight = Hell::BackEnd::GetCurrentWindowHeight();

    // Pixel bounds
    m_leftPixel = m_position.x * windowWidth;
    m_rightPixel = m_leftPixel + m_size.x * windowWidth;
    m_topPixel = m_position.y * windowHeight;
    m_bottomPixel = m_topPixel + m_size.y * windowHeight;

    // Space co-ordinates
    const Resolutions& resolutions = Config::GetResolutions();
    UpdateSpaceCoords(m_windowSpaceCoords, windowWidth, windowHeight);
    UpdateSpaceCoords(m_gBufferSpaceCoords, resolutions.gBuffer.x, resolutions.gBuffer.y);

    // Viewport mouse hover state
    m_hasHover = (
        Hell::Input::GetMouseX() >= m_leftPixel &&
        Hell::Input::GetMouseX() < m_rightPixel &&
        Hell::Input::GetMouseY() >= m_topPixel &&
        Hell::Input::GetMouseY() < m_bottomPixel
    );
}

void Viewport::SetPerspective(float fov, float nearPlane, float farPlane) {
    m_isOrthographic = false;
    m_fov = fov;
    m_nearPlane = nearPlane;
    m_farPlane = farPlane;
    UpdateProjectionMatrices();
}

void Viewport::SetOrthographic(float orthoSize, float nearPlane, float farPlane) {
    m_isOrthographic = true;
    m_orthoSize = orthoSize;
    m_nearPlane = nearPlane;
    m_farPlane = farPlane;
    UpdateProjectionMatrices();
}

void Viewport::UpdateProjectionMatrices() {
    const Resolutions& resolutions = Config::GetResolutions();
    int renderTargetWidth = resolutions.gBuffer.x;
    int renderTargetHeight = resolutions.gBuffer.y;

    float viewportWidth = m_size.x * renderTargetWidth;
    float viewportHeight = m_size.y * renderTargetHeight;
    m_aspect = viewportWidth / viewportHeight;
    m_perspectiveMatrix = glm::perspective(m_fov, m_aspect, m_nearPlane, m_farPlane);
    m_perspectiveMatrixReverseZ = Hell::Projection::ReverseZPerspective(m_fov, m_aspect, m_nearPlane);

    float left = -m_orthoSize * m_aspect;
    float right = m_orthoSize * m_aspect;
    float bottom = -m_orthoSize;
    float top = m_orthoSize;
    m_orthographicMatrixReverseZ = glm::orthoZO(left, right, bottom, top, m_farPlane, m_nearPlane); // Reverse Z
    m_orthographicMatrix = glm::orthoZO(left, right, bottom, top, m_nearPlane, m_farPlane);         // Forward Z
}

glm::vec2 Viewport::WorldToScreen(const glm::mat4& viewMatrix, const glm::vec3& worldPosition) const {
    const Resolutions& resolutions = Config::GetResolutions();
    int renderTargetWidth = resolutions.gBuffer.x;
    int renderTargetHeight = resolutions.gBuffer.y;

    // Compute viewport dimensions in pixels.
    float viewportWidth = m_size.x * renderTargetWidth;
    float viewportHeight = m_size.y * renderTargetHeight;

    // Compute the aspect ratio.
    float aspect = viewportWidth / viewportHeight;

    // Use your orthographic settings.
    float left = -m_orthoSize * aspect;
    float right = m_orthoSize * aspect;
    float bottom = -m_orthoSize;
    float top = m_orthoSize;

    // Transform the world position by the view matrix.
    // (If your view matrix is identity, this is just the world position.)
    glm::vec4 pos = viewMatrix * glm::vec4(worldPosition, 1.0f);

    // Map the view-space coordinates to screen (pixel) coordinates.
    // For X: world x in [left, right] maps to [0, viewportWidth]
    // For Y: world y in [bottom, top] maps to [0, viewportHeight]
    float screenX = ((pos.x - left) / (right - left)) * viewportWidth;
    float screenY = ((pos.y - bottom) / (top - bottom)) * viewportHeight;

    return glm::vec2(screenX, viewportHeight - screenY);
}

glm::ivec2 Viewport::GetLocalMouseCoords() {
    int mx = Hell::Input::GetMouseX();
    int my = Hell::Input::GetMouseY();
    int x = mx - GetLeftPixel();
    int y = my - GetTopPixel();

    return glm::ivec2(x, y);
}

glm::mat4 Viewport::GetProjectionMatrix() const {
    if (IsOrthographic()) {
        return m_orthographicMatrix;
    }
    else {
        return m_perspectiveMatrix;
    }
}

glm::mat4 Viewport::GetProjectionMatrixReverseZ() const {
    if (IsOrthographic()) {
        return m_orthographicMatrixReverseZ;
    }
    else {
        return m_perspectiveMatrixReverseZ;
    }
}

glm::mat4 Viewport::GetPerpsectiveMatrix() const {
    return m_perspectiveMatrix;
}

glm::mat4 Viewport::GetOrthographicMatrix() const {
    return m_orthographicMatrix;
}

glm::vec2 Viewport::GetPosition() const {
    return m_position;
}

glm::vec2 Viewport::GetSize() const {
    return m_size;
}

SpaceCoords Viewport::GetWindowSpaceCoords() const {
    return m_windowSpaceCoords;
}

SpaceCoords Viewport::GetGBufferSpaceCoords() const {
    return m_gBufferSpaceCoords;
}

void Viewport::SetPosition(const glm::vec2& position) {
    m_position = position;
}

void Viewport::SetSize(const glm::vec2& size) {
    m_size = size;
    UpdateProjectionMatrices();
}

void Viewport::SetMirrorId(uint64_t mirrorId) {
    m_mirrorId = mirrorId;
}

void Viewport::Show() {
    m_isVisible = true;
}

void Viewport::Hide() {
    m_isVisible = false;
}

void Viewport::SetViewportMode(ShadingMode viewportMode) {
    m_viewportMode = viewportMode;
}

void Viewport::SetOrthoSize(float value) {
    m_orthoSize = value;
    m_orthoSize = std::max(m_orthoSize, 0.1f);
    UpdateProjectionMatrices();
}

const float Viewport::GetOrthoSize() const {
    return m_orthoSize;
}

const float Viewport::GetPerspectiveFOV() const {
    return m_fov;
}

const bool Viewport::IsVisible() const {
    return m_isVisible;
}

const bool Viewport::IsOrthographic() const {
    return m_isOrthographic;
}

ShadingMode Viewport::GetViewportMode() const {
    return m_viewportMode;
}

const bool Viewport::IsHovered() const {
    return m_hasHover;
}

}
