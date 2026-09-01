#include "EditorViewports.h"

#include "Unloved/EditorSession/HeightMap/EditorHeightMap.h"
#include "Unloved/EditorSession/UI/EditorLayout.h"
#include "Unloved/EditorSession/EditorSession.h"
#include "Unloved/EditorSession/UI/EditorStyle.h"
#include "Unloved/EditorSession/UI/EditorUI.h"

#include "Hell/Audio.h"
#include "Hell/Input.h"
#include "Hell/Time/Time.h"
#include "Hell/UI/UIBackEnd.h"

#include "Unloved/Common/Constants.h"
#include "Unloved/Common/CreateInfo.h"
#include "Unloved/EditorSession/Gizmo/Gizmo.h"
#include "Unloved/Viewport/ViewportManager.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <glm/common.hpp>
#include <limits>
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/matrix.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <string>

// Viewport camera
namespace Unloved::EditorSession {

    EditorCamera::EditorCamera() {
        LookAt(glm::vec3(0.0f), glm::normalize(glm::vec3(-1.0f, -0.75f, -1.0f)), glm::vec3(0.0f, 1.0f, 0.0f));
    }

    void EditorCamera::LookAt(const glm::vec3& pivot, const glm::vec3& forward, const glm::vec3& up) {
        const glm::vec3 normalizedForward = glm::normalize(forward);
        const glm::vec3 normalizedRight = glm::normalize(glm::cross(normalizedForward, up));
        const glm::vec3 normalizedUp = glm::normalize(glm::cross(normalizedRight, normalizedForward));

        glm::mat3 rotation(1.0f);
        rotation[0] = normalizedRight;
        rotation[1] = normalizedUp;
        rotation[2] = -normalizedForward;

        m_pivot = pivot;
        m_orientation = glm::normalize(glm::quat_cast(rotation));
        UpdateViewMatrix();
    }

    void EditorCamera::SetPivot(const glm::vec3& pivot) {
        m_pivot = pivot;
        UpdateViewMatrix();
    }

    void EditorCamera::SetOrientation(const glm::quat& orientation) {
        m_orientation = glm::normalize(orientation);
        UpdateViewMatrix();
    }

    void EditorCamera::SetDistance(float distance) {
        m_distance = std::max(distance, 0.01f);
        UpdateViewMatrix();
    }

    void EditorCamera::UpdateViewMatrix() {
        m_right = glm::normalize(m_orientation * glm::vec3(1.0f, 0.0f, 0.0f));
        m_up = glm::normalize(m_orientation * glm::vec3(0.0f, 1.0f, 0.0f));
        m_forward = glm::normalize(m_orientation * glm::vec3(0.0f, 0.0f, -1.0f));
        m_position = m_pivot - m_forward * m_distance;
        m_viewMatrix = glm::lookAt(m_position, m_pivot, m_up);
    }

    const glm::mat4& EditorCamera::GetViewMatrix() const {
        return m_viewMatrix;
    }

    const glm::vec3& EditorCamera::GetPivot() const {
        return m_pivot;
    }

    const glm::vec3& EditorCamera::GetPosition() const {
        return m_position;
    }

    const glm::vec3& EditorCamera::GetForward() const {
        return m_forward;
    }

    const glm::vec3& EditorCamera::GetUp() const {
        return m_up;
    }

    const glm::vec3& EditorCamera::GetRight() const {
        return m_right;
    }

    const glm::quat& EditorCamera::GetOrientation() const {
        return m_orientation;
    }

    float EditorCamera::GetDistance() const {
        return m_distance;
    }
}

namespace Unloved::EditorSession::Viewports {
    namespace {
        constexpr uint32_t VIEWPORT_COUNT = 4;
        constexpr float ORBIT_SENSITIVITY = 0.006981317f;
        constexpr float FREE_LOOK_SENSITIVITY = 0.002617994f;
        constexpr float FREE_LOOK_TOP_LIMIT = 1.483529864f;
        constexpr float FREE_LOOK_BOTTOM_LIMIT = -1.396263402f;
        constexpr float FREE_MOVE_SPEED = 3.125f;
        constexpr float FLY_EXIT_ORBIT_DISTANCE = 1.5f;
        constexpr float ZOOM_STEP = 1.2f;
        constexpr float MINIMUM_ZOOM = 0.01f;
        constexpr float CAMERA_ORIENTATION_TRANSITION_DURATION = 0.2f;
        constexpr float CAMERA_FOCUS_TRANSITION_DURATION = CAMERA_ORIENTATION_TRANSITION_DURATION / 2.0f;
        const glm::vec3 EDITOR_WORLD_UP(0.0f, 1.0f, 0.0f);
        const glm::vec3 DEFAULT_INITIAL_VIEW_FORWARD = glm::normalize(glm::vec3(1.0f, -0.75f, -1.0f));
        const glm::vec3 RAGDOLL_INITIAL_VIEW_FORWARD = glm::normalize(glm::vec3(0.0f, -0.35f, -1.0f));

        struct EditorViewportState {
            EditorCamera camera;
            EditorViewportMode mode = EditorViewportMode::PERSPECTIVE;
            glm::vec3 mouseRayOrigin = glm::vec3(0.0f);
            glm::vec3 mouseRayDirection = glm::vec3(0.0f, 0.0f, -1.0f);
            glm::vec3 orbitPoint = glm::vec3(0.0f);
            glm::vec3 orbitInitialPivot = glm::vec3(0.0f);
            glm::vec3 pivotTransitionStart = glm::vec3(0.0f);
            glm::vec3 pivotTransitionTarget = glm::vec3(0.0f);
            glm::quat orbitInitialOrientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
            glm::quat orientationTransitionStart = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
            glm::quat orientationTransitionTarget = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
            float orthographicSize = 5.0f;
            float perspectiveFov = 1.0f;
            float orientationTransitionElapsed = 0.0f;
            float pivotTransitionElapsed = 0.0f;
            float orbitReverse = 1.0f;
            float panWorldUnitsPerPixel = 0.0f;
            bool orientationTransitionActive = false;
            bool pivotTransitionActive = false;
            bool orbitActive = false;
            bool panActive = false;
            bool hasCustomOrbitPoint = false;
        };

        std::array<EditorViewportState, VIEWPORT_COUNT> g_viewports;
        int32_t g_hoveredViewportIndex = -1;
        int32_t g_cameraDragViewportIndex = -1;
        int32_t g_flyModeViewportIndex = -1;
        bool g_flyModeViewChanged = false;
        glm::vec3 g_initialViewBoundsMin = glm::vec3(0.0f);
        glm::vec3 g_initialViewBoundsMax = glm::vec3(0.0f);
        glm::vec3 g_initialViewForward = DEFAULT_INITIAL_VIEW_FORWARD;
        bool g_initialViewPending = false;

        const char* GetModeLabel(uint32_t viewportIndex) {
            if (g_flyModeViewportIndex == static_cast<int32_t>(viewportIndex)) {
                return "Perspective (Fly mode)";
            }

            const EditorViewportMode mode = g_viewports[viewportIndex].mode;
            switch (mode) {
                case EditorViewportMode::PERSPECTIVE:  return "Perspective";
                case EditorViewportMode::ORTHOGRAPHIC: return "Ortho";
                case EditorViewportMode::TOP:          return "Top";
                case EditorViewportMode::BOTTOM:       return "Bottom";
                case EditorViewportMode::FRONT:        return "Front";
                case EditorViewportMode::BACK:         return "Back";
                case EditorViewportMode::LEFT:         return "Left";
                case EditorViewportMode::RIGHT:        return "Right";
            }
            return "";
        }

        glm::quat CreateOrientation(const glm::vec3& forward, const glm::vec3& up) {
            // Build a camera rotation from the direction it should face
            const glm::vec3 normalizedForward = glm::normalize(forward);
            const glm::vec3 normalizedRight = glm::normalize(glm::cross(normalizedForward, up));
            const glm::vec3 normalizedUp = glm::normalize(glm::cross(normalizedRight, normalizedForward));
            glm::mat3 rotation(1.0f);
            rotation[0] = normalizedRight;
            rotation[1] = normalizedUp;
            rotation[2] = -normalizedForward;
            return glm::normalize(glm::quat_cast(rotation));
        }

        glm::quat GetAxisViewOrientation(EditorViewportMode mode) {
            switch (mode) {
                case EditorViewportMode::TOP:    return CreateOrientation(glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
                case EditorViewportMode::BOTTOM: return CreateOrientation(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
                case EditorViewportMode::FRONT:  return CreateOrientation(glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                case EditorViewportMode::BACK:   return CreateOrientation(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                case EditorViewportMode::LEFT:   return CreateOrientation(glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                case EditorViewportMode::RIGHT:  return CreateOrientation(glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                default:                         return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
            }
        }

        void BeginAxisViewTransition(EditorViewportState& viewport) {
            viewport.orientationTransitionStart = viewport.camera.GetOrientation();
            viewport.orientationTransitionTarget = GetAxisViewOrientation(viewport.mode);

            // q and -q are the same rotation, pick the closer one so the camera does not spin the long way
            if (glm::dot(viewport.orientationTransitionStart, viewport.orientationTransitionTarget) < 0.0f) {
                viewport.orientationTransitionTarget = -viewport.orientationTransitionTarget;
            }

            viewport.orientationTransitionElapsed = 0.0f;
            viewport.orientationTransitionActive = true;
        }

        void BeginPivotTransition(EditorViewportState& viewport) {
            viewport.pivotTransitionStart = viewport.camera.GetPivot();
            viewport.pivotTransitionTarget = Gizmo::GetPosition();
            viewport.pivotTransitionElapsed = 0.0f;
            viewport.pivotTransitionActive = true;
            viewport.hasCustomOrbitPoint = false;
        }

        void UpdateCameraTransitions() {
            for (EditorViewportState& viewport : g_viewports) {
                if (viewport.orientationTransitionActive) {
                    viewport.orientationTransitionElapsed = std::min(viewport.orientationTransitionElapsed + Hell::Time::DeltaTime(), CAMERA_ORIENTATION_TRANSITION_DURATION);
                    const float interpolation = viewport.orientationTransitionElapsed / CAMERA_ORIENTATION_TRANSITION_DURATION;
                    viewport.camera.SetOrientation(glm::slerp(viewport.orientationTransitionStart, viewport.orientationTransitionTarget, interpolation));

                    if (viewport.orientationTransitionElapsed >= CAMERA_ORIENTATION_TRANSITION_DURATION) {
                        viewport.camera.SetOrientation(viewport.orientationTransitionTarget);
                        viewport.orientationTransitionActive = false;
                    }
                }

                if (viewport.pivotTransitionActive) {
                    viewport.pivotTransitionElapsed = std::min(viewport.pivotTransitionElapsed + Hell::Time::DeltaTime(), CAMERA_FOCUS_TRANSITION_DURATION);
                    const float interpolation = viewport.pivotTransitionElapsed / CAMERA_FOCUS_TRANSITION_DURATION;
                    viewport.camera.SetPivot(viewport.pivotTransitionStart + (viewport.pivotTransitionTarget - viewport.pivotTransitionStart) * interpolation);

                    if (viewport.pivotTransitionElapsed >= CAMERA_FOCUS_TRANSITION_DURATION) {
                        viewport.camera.SetPivot(viewport.pivotTransitionTarget);
                        viewport.pivotTransitionActive = false;
                    }
                }
            }
        }

        void SetModeImmediately(uint32_t viewportIndex, EditorViewportMode mode) {
            EditorViewportState& viewport = g_viewports[viewportIndex];
            viewport.mode = mode;
            viewport.orientationTransitionActive = false;

            // Startup presets should not animate in
            switch (mode) {
                case EditorViewportMode::TOP:
                case EditorViewportMode::BOTTOM:
                case EditorViewportMode::FRONT:
                case EditorViewportMode::BACK:
                case EditorViewportMode::LEFT:
                case EditorViewportMode::RIGHT:
                    viewport.camera.SetOrientation(GetAxisViewOrientation(mode));
                    break;
                default: break;
            }
        }

        void UpdateMouseRay(uint32_t viewportIndex, const EditorViewportRegion& region, const glm::ivec2& mousePosition) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(viewportIndex);
            if (!viewport || !viewport->IsVisible() || !region.rect.HasArea()) return;

            const float localMouseX = static_cast<float>(mousePosition.x - region.rect.x);
            const float localMouseY = static_cast<float>(mousePosition.y - region.rect.y);
            const float ndcX = 2.0f * localMouseX / static_cast<float>(region.rect.width) - 1.0f;
            const float ndcY = 1.0f - 2.0f * localMouseY / static_cast<float>(region.rect.height);
            const glm::mat4 viewMatrix = g_viewports[viewportIndex].camera.GetViewMatrix();
            const glm::mat4 inverseProjectionView = glm::inverse(viewport->GetProjectionMatrix() * viewMatrix);

            if (viewport->IsOrthographic()) {
                // Ortho rays start under the cursor and all point straight through the view
                const glm::vec4 worldPoint = inverseProjectionView * glm::vec4(ndcX, ndcY, 0.0f, 1.0f);
                g_viewports[viewportIndex].mouseRayOrigin = glm::vec3(worldPoint) / worldPoint.w;
                g_viewports[viewportIndex].mouseRayDirection = -glm::vec3(viewMatrix[0][2], viewMatrix[1][2], viewMatrix[2][2]);
            }
            else {
                // Perspective rays start on the near plane and spread through the view
                glm::vec4 nearWorld = inverseProjectionView * glm::vec4(ndcX, ndcY, 0.0f, 1.0f);
                glm::vec4 farWorld = inverseProjectionView * glm::vec4(ndcX, ndcY, 1.0f, 1.0f);
                nearWorld /= nearWorld.w;
                farWorld /= farWorld.w;
                g_viewports[viewportIndex].mouseRayOrigin = glm::vec3(nearWorld);
                g_viewports[viewportIndex].mouseRayDirection = glm::normalize(glm::vec3(farWorld - nearWorld));
            }
        }

        void BeginPanCamera(uint32_t viewportIndex) {
            EditorViewportState& viewport = g_viewports[viewportIndex];
            const EditorViewportRegion* region = Layout::GetViewportRegionByIndex(viewportIndex);
            if (!region || !region->rect.HasArea()) return;

            // Cache world units per pixel so pan speed follows the current zoom
            const float verticalWorldSpan = UsesOrthographicProjection(viewport.mode) ? viewport.orthographicSize * 2.0f : viewport.camera.GetDistance() * std::tan(viewport.perspectiveFov * 0.5f) * 2.0f;
            viewport.panWorldUnitsPerPixel = verticalWorldSpan / static_cast<float>(region->rect.height);
            viewport.orientationTransitionActive = false;
            viewport.pivotTransitionActive = false;
            viewport.hasCustomOrbitPoint = false;
            viewport.panActive = true;
        }

        void PanCamera(uint32_t viewportIndex) {
            EditorViewportState& viewport = g_viewports[viewportIndex];
            const glm::vec3 translation = -viewport.camera.GetRight() * viewport.panWorldUnitsPerPixel * Hell::Input::GetMouseOffsetX() + viewport.camera.GetUp() * viewport.panWorldUnitsPerPixel * Hell::Input::GetMouseOffsetY();
            viewport.camera.SetPivot(viewport.camera.GetPivot() + translation);
        }

        void BeginFlyMode(uint32_t viewportIndex) {
            EditorViewportState& viewport = g_viewports[viewportIndex];
            viewport.mode = EditorViewportMode::PERSPECTIVE;
            viewport.orientationTransitionActive = false;
            viewport.pivotTransitionActive = false;

            g_cameraDragViewportIndex = -1;

            for (EditorViewportState& cameraViewport : g_viewports) {
                cameraViewport.orbitActive = false;
                cameraViewport.panActive = false;
            }

            g_flyModeViewportIndex = static_cast<int32_t>(viewportIndex);
            g_flyModeViewChanged = false;

            Hell::Input::DisableCursor();
        }

        void EndFlyMode() {
            if (g_flyModeViewportIndex < 0) return;
            EditorViewportState& viewport = g_viewports[static_cast<uint32_t>(g_flyModeViewportIndex)];

            // Rebuild a nearby pivot without moving the camera
            if (g_flyModeViewChanged && viewport.camera.GetDistance() > FLY_EXIT_ORBIT_DISTANCE) {
                const glm::vec3 cameraPosition = viewport.camera.GetPosition();
                const glm::vec3 cameraForward = viewport.camera.GetForward();
                viewport.camera.SetDistance(FLY_EXIT_ORBIT_DISTANCE);
                viewport.camera.SetPivot(cameraPosition + cameraForward * FLY_EXIT_ORBIT_DISTANCE);
            }

            g_flyModeViewportIndex = -1;
            g_flyModeViewChanged = false;

            Hell::Input::ShowCursor();
        }

        void UpdateFlyLook(EditorViewportState& viewport) {
            const float mouseOffsetX = Hell::Input::GetMouseOffsetX();
            const float mouseOffsetY = Hell::Input::GetMouseOffsetY();

            if (mouseOffsetX == 0.0f && mouseOffsetY == 0.0f) return;
            g_flyModeViewChanged = true;

            const glm::vec3 cameraPosition = viewport.camera.GetPosition();
            const float cameraDistance = viewport.camera.GetDistance();
            glm::quat orientation = viewport.camera.GetOrientation();
            glm::vec3 forward = viewport.camera.GetForward();
            float pitchDelta = -mouseOffsetY * FREE_LOOK_SENSITIVITY;
            const float currentPitch = std::asin(std::clamp(glm::dot(forward, EDITOR_WORLD_UP), -1.0f, 1.0f));

            // Stop free look from flipping over
            if ((currentPitch >= FREE_LOOK_TOP_LIMIT && pitchDelta > 0.0f) || (currentPitch <= FREE_LOOK_BOTTOM_LIMIT && pitchDelta < 0.0f)) {
                pitchDelta = 0.0f;
            }
            else {
                pitchDelta = std::clamp(currentPitch + pitchDelta, FREE_LOOK_BOTTOM_LIMIT, FREE_LOOK_TOP_LIMIT) - currentPitch;
            }

            const glm::quat yawRotation = glm::angleAxis(-mouseOffsetX * FREE_LOOK_SENSITIVITY, EDITOR_WORLD_UP);
            orientation = glm::normalize(yawRotation * orientation);

            if (pitchDelta != 0.0f) {
                const glm::vec3 pitchAxis = glm::normalize(yawRotation * viewport.camera.GetRight());
                orientation = glm::normalize(glm::angleAxis(pitchDelta, pitchAxis) * orientation);
            }

            viewport.camera.SetOrientation(orientation);

            // Move the pivot with the view so the camera stays put
            viewport.camera.SetPivot(cameraPosition + viewport.camera.GetForward() * cameraDistance);
        }

        void UpdateFlyMovement(EditorViewportState& viewport) {
            // Read movement input
            glm::vec3 movement(0.0f);
            if (Hell::Input::KeyDown(HELL_KEY_W)) {
                movement += viewport.camera.GetForward();
            }
            if (Hell::Input::KeyDown(HELL_KEY_S)) {
                movement -= viewport.camera.GetForward();
            }
            if (Hell::Input::KeyDown(HELL_KEY_A)) {
                movement -= viewport.camera.GetRight();
            }
            if (Hell::Input::KeyDown(HELL_KEY_D)) {
                movement += viewport.camera.GetRight();
            }
            if (Hell::Input::KeyDown(HELL_KEY_Q)) {
                movement += EDITOR_WORLD_UP;
            }
            if (Hell::Input::KeyDown(HELL_KEY_E)) {
                movement -= EDITOR_WORLD_UP;
            }

            if (glm::dot(movement, movement) <= 0.0f) return;

            g_flyModeViewChanged = true;
            const float movementSpeed = Hell::Input::KeyDown(HELL_KEY_LEFT_SHIFT_GLFW) ? FREE_MOVE_SPEED * 5.0f : FREE_MOVE_SPEED;

            // Normalize diagonal movement so it is not faster
            movement = glm::normalize(movement) * movementSpeed * Hell::Time::DeltaTime();
            viewport.camera.SetPivot(viewport.camera.GetPivot() + movement);
        }

        void UpdateFlyMode(uint32_t viewportIndex) {
            EditorViewportState& viewport = g_viewports[viewportIndex];
            UpdateFlyLook(viewport);
            UpdateFlyMovement(viewport);
        }

        void ZoomViewport(uint32_t viewportIndex, bool zoomIn) {
            EditorViewportState& viewport = g_viewports[viewportIndex];
            float stepStrength = 1.0f;

            // Shift zooms slower and control zooms faster
            if (Hell::Input::KeyDown(HELL_KEY_LEFT_SHIFT_GLFW)) {
                stepStrength *= 0.5f;
            }
            else if (Hell::Input::KeyDown(HELL_KEY_LEFT_CONTROL_GLFW)) {
                stepStrength *= 2.5f;
            }

            float zoomFactor = std::pow(ZOOM_STEP, stepStrength);

            if (zoomIn) {
                zoomFactor = 1.0f / zoomFactor;
            }

            Unloved::Viewport* renderViewport = Unloved::ViewportManager::GetViewportByIndex(viewportIndex);
            const float maximumZoom = renderViewport ? renderViewport->GetFarPlane() * 10.0f : 2560.0f;

            if (UsesOrthographicProjection(viewport.mode)) {
                viewport.orthographicSize = std::clamp(viewport.orthographicSize * zoomFactor, MINIMUM_ZOOM, maximumZoom);
            }
            else {
                viewport.camera.SetDistance(std::clamp(viewport.camera.GetDistance() * zoomFactor, MINIMUM_ZOOM, maximumZoom));
            }
        }

        void BeginOrbitCamera(uint32_t viewportIndex) {
            EditorViewportState& viewport = g_viewports[viewportIndex];
            const bool heightMapMode = Unloved::EditorSession::HasMode() && Unloved::EditorSession::GetMode() == EditorSessionMode::MAP && HeightMapEditor::IsActive();
            if (!heightMapMode) {
                viewport.hasCustomOrbitPoint = false;
            }
            if (!viewport.hasCustomOrbitPoint) {
                viewport.orbitPoint = viewport.camera.GetPivot();
            }

            viewport.orbitInitialPivot = viewport.camera.GetPivot();
            viewport.orbitInitialOrientation = viewport.camera.GetOrientation();

            // Keep horizontal dragging sane when the camera is upside down
            viewport.orbitReverse = glm::dot(viewport.camera.GetUp(), EDITOR_WORLD_UP) < 0.0f ? -1.0f : 1.0f;
            viewport.orientationTransitionActive = false;
            viewport.pivotTransitionActive = false;
            viewport.orbitActive = true;

            // Orbiting an axis preset turns it into free ortho
            if (viewport.mode != EditorViewportMode::PERSPECTIVE) {
                viewport.mode = EditorViewportMode::ORTHOGRAPHIC;
            }
        }

        void OrbitCamera(uint32_t viewportIndex) {
            EditorViewportState& viewport = g_viewports[viewportIndex];
            const glm::quat currentOrientation = viewport.camera.GetOrientation();
            const glm::vec3 cameraBackward = currentOrientation * glm::vec3(0.0f, 0.0f, 1.0f);
            const glm::vec3 cameraRight = currentOrientation * glm::vec3(1.0f, 0.0f, 0.0f);
            glm::vec3 pitchAxis = cameraRight;

            // Blend the pitch axis near the poles so orbiting does not flip
            const glm::vec3 upToBackward = EDITOR_WORLD_UP - cameraBackward;
            if (glm::dot(upToBackward, upToBackward) > 0.001f) {
                pitchAxis = glm::cross(EDITOR_WORLD_UP, cameraBackward);

                if (glm::dot(pitchAxis, cameraRight) < 0.0f) {
                    pitchAxis = -pitchAxis;
                }

                float horizonBlend = std::acos(std::clamp(glm::dot(EDITOR_WORLD_UP, cameraBackward), -1.0f, 1.0f)) / 3.141592654f;
                horizonBlend = std::abs(horizonBlend - 0.5f) * 2.0f;
                horizonBlend *= horizonBlend;
                pitchAxis = pitchAxis * (1.0f - horizonBlend) + cameraRight * horizonBlend;
            }

            if (glm::dot(pitchAxis, pitchAxis) <= 0.000001f) {
                pitchAxis = cameraRight;
            }

            pitchAxis = glm::normalize(pitchAxis);

            glm::quat viewOrientation = glm::conjugate(currentOrientation);
            const glm::quat pitchRotation = glm::angleAxis(ORBIT_SENSITIVITY * Hell::Input::GetMouseOffsetY(), pitchAxis);
            const glm::quat yawRotation = glm::angleAxis(ORBIT_SENSITIVITY * viewport.orbitReverse * Hell::Input::GetMouseOffsetX(), EDITOR_WORLD_UP);

            viewOrientation = glm::normalize(viewOrientation * pitchRotation * yawRotation);

            const glm::quat newOrientation = glm::conjugate(viewOrientation);
            const glm::quat orbitRotation = glm::normalize(newOrientation * glm::conjugate(viewport.orbitInitialOrientation));

            glm::vec3 newPivot = viewport.orbitPoint + orbitRotation * (viewport.orbitInitialPivot - viewport.orbitPoint);

            // Stop ortho orbit from sliding away from its orbit point
            if (UsesOrthographicProjection(viewport.mode)) {
                const glm::vec3 initialBackward = viewport.orbitInitialOrientation * glm::vec3(0.0f, 0.0f, 1.0f);
                const glm::vec3 currentBackward = newOrientation * glm::vec3(0.0f, 0.0f, 1.0f);
                const float angleCosine = std::max(0.0f, glm::dot(initialBackward, currentBackward));

                if (angleCosine < 1.0f) {
                    const float correctionFactor = std::acos(angleCosine) / 1.570796327f;
                    const float depthOffset = glm::dot(currentBackward, newPivot - viewport.orbitPoint);
                    newPivot -= currentBackward * depthOffset * correctionFactor;
                }
            }

            viewport.camera.SetPivot(newPivot);
            viewport.camera.SetOrientation(newOrientation);
        }
    }

    void Init() {
        g_viewports = {};
        g_hoveredViewportIndex = -1;
        g_cameraDragViewportIndex = -1;
        g_flyModeViewportIndex = -1;
        g_flyModeViewChanged = false;
        g_initialViewPending = false;
        SetModeImmediately(0, EditorViewportMode::PERSPECTIVE);
        SetModeImmediately(1, EditorViewportMode::TOP);
        SetModeImmediately(2, EditorViewportMode::FRONT);
        SetModeImmediately(3, EditorViewportMode::RIGHT);
        SetPivot(glm::vec3(0.0f));
    }

    void CancelNavigation() {
        EndFlyMode();
        g_cameraDragViewportIndex = -1;

        for (EditorViewportState& viewport : g_viewports) {
            viewport.orbitActive = false;
            viewport.panActive = false;
        }
    }

    void PrepareInitialView(const CreateInfoCollection& createInfoCollection) {
        glm::vec3 boundsMin(std::numeric_limits<float>::max());
        glm::vec3 boundsMax(std::numeric_limits<float>::lowest());
        bool hasBounds = false;

        const auto growBounds = [&](const glm::vec3& point) {
            boundsMin = glm::min(boundsMin, point);
            boundsMax = glm::max(boundsMax, point);
            hasBounds = true;
        };

        // Only walls and world planes decide the initial framing
        for (const WallCreateInfo& wall : createInfoCollection.walls) {
            for (const SequencePoint& sequencePoint : wall.sequencePoints) {
                growBounds(sequencePoint.position);
                // Wall points sit on the floor so include the top edge too
                growBounds(sequencePoint.position + glm::vec3(0.0f, sequencePoint.customFloat, 0.0f));
            }
        }
        for (const WorldPlaneCreateInfo& plane : createInfoCollection.worldPlanes) {
            growBounds(plane.p0);
            growBounds(plane.p1);
            growBounds(plane.p2);
            growBounds(plane.p3);
        }

        g_initialViewBoundsMin = boundsMin;
        g_initialViewBoundsMax = boundsMax;
        g_initialViewForward = DEFAULT_INITIAL_VIEW_FORWARD;
        g_initialViewPending = hasBounds;
    }

    void PrepareInitialMapView(uint32_t chunkCountX, uint32_t chunkCountZ, float originHeight) {
        g_initialViewBoundsMin = glm::vec3(0.0f, originHeight, 0.0f);
        g_initialViewBoundsMax = glm::vec3(chunkCountX * HEIGHT_MAP_CHUNK_WORLD_SPACE_SIZE, originHeight, chunkCountZ * HEIGHT_MAP_CHUNK_WORLD_SPACE_SIZE);
        g_initialViewForward = DEFAULT_INITIAL_VIEW_FORWARD;
        g_initialViewPending = chunkCountX > 0 && chunkCountZ > 0;
    }

    void PrepareInitialRagdollView() {
        constexpr float FRAME_HEIGHT = 1.8f;
        constexpr float FRAME_CENTER_HEIGHT = 1.0f;
        constexpr float HALF_FRAME_HEIGHT = FRAME_HEIGHT * 0.5f;

        // Aim above the midpoint to balance the downward view
        SetModeImmediately(0, EditorViewportMode::PERSPECTIVE);
        g_initialViewBoundsMin = glm::vec3(0.0f, FRAME_CENTER_HEIGHT - HALF_FRAME_HEIGHT, 0.0f);
        g_initialViewBoundsMax = glm::vec3(0.0f, FRAME_CENTER_HEIGHT + HALF_FRAME_HEIGHT, 0.0f);
        g_initialViewForward = RAGDOLL_INITIAL_VIEW_FORWARD;
        g_initialViewPending = true;
    }

    void ApplyInitialView() {
        // Consume this once after the editor layout is ready
        if (!g_initialViewPending) return;
        g_initialViewPending = false;

        constexpr float padding = 1.1f;
        const glm::vec3 center = (g_initialViewBoundsMin + g_initialViewBoundsMax) * 0.5f;
        const float radius = std::max(glm::length(g_initialViewBoundsMax - g_initialViewBoundsMin) * 0.5f * padding, MINIMUM_ZOOM);
        g_viewports[0].camera.LookAt(center, g_initialViewForward, EDITOR_WORLD_UP);

        // Fit the same bounds to each viewport using its own aspect ratio
        for (uint32_t i = 0; i < g_viewports.size(); i++) {
            EditorViewportState& viewport = g_viewports[i];
            const EditorViewportRegion* region = Layout::GetViewportRegionByIndex(i);
            const float aspect = region && region->rect.HasArea() ? static_cast<float>(region->rect.width) / static_cast<float>(region->rect.height) : 1.0f;
            const float verticalHalfFov = viewport.perspectiveFov * 0.5f;
            const float horizontalHalfFov = std::atan(std::tan(verticalHalfFov) * aspect);
            const float fitHalfFov = std::clamp(std::min(verticalHalfFov, horizontalHalfFov), 0.01f, 1.5f);

            viewport.camera.SetPivot(center);
            viewport.camera.SetDistance(radius / std::sin(fitHalfFov));
            viewport.orthographicSize = radius / std::min(aspect, 1.0f);
        }
    }

    void Update() {
        UpdateCameraTransitions();
        g_hoveredViewportIndex = -1;
        const glm::ivec2 mousePosition = Coordinates::GetMousePositionUI();

        for (uint32_t i = 0; i < g_viewports.size(); i++) {
            const EditorViewportRegion* region = Layout::GetViewportRegionByIndex(i);
            if (!region || !region->visible) continue;

            UpdateMouseRay(i, *region, mousePosition);
            if (region->rect.Contains(mousePosition)) {
                g_hoveredViewportIndex = static_cast<int32_t>(i);
            }
        }
    }

    void UpdateInput(bool allowKeyboardInput, bool allowMouseInput) {
        // Fly mode
        if (g_flyModeViewportIndex >= 0) {
            if (Hell::Input::KeyPressed(HELL_KEY_F)) {
                Hell::Audio::PlayAudio(AUDIO_SELECT, 1.0f);
                EndFlyMode();
            }
            else {
                UpdateFlyMode(static_cast<uint32_t>(g_flyModeViewportIndex));
            }
            return;
        }
        else if (allowKeyboardInput && allowMouseInput && Hell::Input::KeyPressed(HELL_KEY_F) && g_hoveredViewportIndex >= 0 && Gizmo::GetAction() != GizmoAction::DRAGGING) {
            Hell::Audio::PlayAudio(AUDIO_SELECT, 1.0f);
            BeginFlyMode(static_cast<uint32_t>(g_hoveredViewportIndex));
            return;
        }

        // View commands
        if (allowKeyboardInput && g_hoveredViewportIndex >= 0) {
            const uint32_t viewportIndex = static_cast<uint32_t>(g_hoveredViewportIndex);
            bool commandPressed = false;

            if (Hell::Input::KeyPressed(HELL_KEY_NUMPAD_0)) {
                const EditorViewportMode currentMode = g_viewports[viewportIndex].mode;
                SetMode(viewportIndex, currentMode == EditorViewportMode::PERSPECTIVE ? EditorViewportMode::ORTHOGRAPHIC : EditorViewportMode::PERSPECTIVE);
                commandPressed = true;
            }
            else if (Hell::Input::KeyPressed(HELL_KEY_NUMPAD_1) || Hell::Input::KeyPressed(HELL_KEY_8)) {
                SetMode(viewportIndex, EditorViewportMode::FRONT);
                commandPressed = true;
            }
            else if (Hell::Input::KeyPressed(HELL_KEY_NUMPAD_3)) {
                SetMode(viewportIndex, EditorViewportMode::BACK);
                commandPressed = true;
            }
            else if (Hell::Input::KeyPressed(HELL_KEY_NUMPAD_4)) {
                SetMode(viewportIndex, EditorViewportMode::LEFT);
                commandPressed = true;
            }
            else if (Hell::Input::KeyPressed(HELL_KEY_NUMPAD_6) || Hell::Input::KeyPressed(HELL_KEY_0)) {
                SetMode(viewportIndex, EditorViewportMode::RIGHT);
                commandPressed = true;
            }
            else if (Hell::Input::KeyPressed(HELL_KEY_NUMPAD_5) || Hell::Input::KeyPressed(HELL_KEY_9)) {
                SetMode(viewportIndex, EditorViewportMode::TOP);
                commandPressed = true;
            }
            else if (Hell::Input::KeyPressed(HELL_KEY_NUMPAD_2) || Hell::Input::KeyPressed(HELL_KEY_7)) {
                SetMode(viewportIndex, EditorViewportMode::BOTTOM);
                commandPressed = true;
            }
            else if (Hell::Input::KeyPressed(HELL_KEY_C)) {
                BeginPivotTransition(g_viewports[viewportIndex]);
                commandPressed = true;
            }

            if (commandPressed) {
                Hell::Audio::PlayAudio(AUDIO_SELECT, 1.0f);
            }
        }

        // Mouse wheel zoom
        if (allowMouseInput && g_hoveredViewportIndex >= 0 && !Hell::Input::KeyDown(HELL_KEY_LEFT_ALT)) {
            const uint32_t viewportIndex = static_cast<uint32_t>(g_hoveredViewportIndex);

            if (Hell::Input::MouseWheelUp()) {
                ZoomViewport(viewportIndex, true);
            }
            else if (Hell::Input::MouseWheelDown()) {
                ZoomViewport(viewportIndex, false);
            }
        }

        // Camera drag state
        if (!Hell::Input::MiddleMouseDown()) {
            if (g_cameraDragViewportIndex >= 0) {
                EditorViewportState& viewport = g_viewports[static_cast<uint32_t>(g_cameraDragViewportIndex)];
                viewport.orbitActive = false;
                viewport.panActive = false;
            }

            g_cameraDragViewportIndex = -1;
        }
        if (allowMouseInput && Hell::Input::MiddleMousePressed() && g_hoveredViewportIndex >= 0) {
            g_cameraDragViewportIndex = g_hoveredViewportIndex;
        }

        // Pan or orbit the active camera
        if (g_cameraDragViewportIndex >= 0 && Hell::Input::MiddleMouseDown()) {
            const uint32_t viewportIndex = static_cast<uint32_t>(g_cameraDragViewportIndex);
            EditorViewportState& viewport = g_viewports[viewportIndex];

            if (Hell::Input::KeyDown(HELL_KEY_LEFT_SHIFT_GLFW)) {
                viewport.orbitActive = false;

                if (!viewport.panActive) {
                    BeginPanCamera(viewportIndex);
                }
                if (viewport.panActive) {
                    PanCamera(viewportIndex);
                }
            }
            else {
                viewport.panActive = false;

                if (!viewport.orbitActive) {
                    BeginOrbitCamera(viewportIndex);
                }

                OrbitCamera(viewportIndex);
            }
        }
    }

    void RenderLabels() {
        const EditorStyle& style = GetStyle();

        for (uint32_t i = 0; i < g_viewports.size(); i++) {
            const EditorViewportRegion* region = Layout::GetViewportRegionByIndex(i);
            if (!region || !region->visible) continue;

            const glm::ivec2 position(region->rect.x + style.viewport.labelPadding, region->rect.y + style.viewport.labelPadding);
            UIBackEnd::BlitText(UICanvas::NATIVE, std::string(style.font.viewportTextColorTag) + GetModeLabel(i), style.font.name, position, Alignment::TOP_LEFT, style.font.scale, TextureFilter::NEAREST);
        }
    }

    void SetMode(uint32_t viewportIndex, EditorViewportMode mode) {
        if (viewportIndex >= g_viewports.size()) return;

        EditorViewportState& viewport = g_viewports[viewportIndex];
        viewport.mode = mode;
        viewport.orientationTransitionActive = false;

        // Perspective and free ortho keep their current rotation
        switch (mode) {
            case EditorViewportMode::TOP:
            case EditorViewportMode::BOTTOM:
            case EditorViewportMode::FRONT:
            case EditorViewportMode::BACK:
            case EditorViewportMode::LEFT:
            case EditorViewportMode::RIGHT:
                BeginAxisViewTransition(viewport);
                break;
            default: break;
        }
    }

    void SetPivot(const glm::vec3& pivot) {
        for (EditorViewportState& viewport : g_viewports) {
            viewport.camera.SetPivot(pivot);
            viewport.hasCustomOrbitPoint = false;
        }
    }

    void SetOrbitPoint(uint32_t viewportIndex, const glm::vec3& orbitPoint) {
        if (viewportIndex >= g_viewports.size()) return;
        if (!Unloved::EditorSession::HasMode() || Unloved::EditorSession::GetMode() != EditorSessionMode::MAP || !HeightMapEditor::IsActive()) return;
        g_viewports[viewportIndex].orbitPoint = orbitPoint;
        g_viewports[viewportIndex].hasCustomOrbitPoint = true;
    }

    EditorCamera* GetCameraByIndex(uint32_t viewportIndex) {
        return viewportIndex < g_viewports.size() ? &g_viewports[viewportIndex].camera : nullptr;
    }

    const glm::mat4& GetViewMatrix(uint32_t viewportIndex) {
        static const glm::mat4 identity(1.0f);
        const EditorCamera* camera = GetCameraByIndex(viewportIndex);
        return camera ? camera->GetViewMatrix() : identity;
    }

    const glm::vec3& GetMouseRayOrigin(uint32_t viewportIndex) {
        static const glm::vec3 fallback(0.0f);
        return viewportIndex < g_viewports.size() ? g_viewports[viewportIndex].mouseRayOrigin : fallback;
    }

    const glm::vec3& GetMouseRayDirection(uint32_t viewportIndex) {
        static const glm::vec3 fallback(0.0f, 0.0f, -1.0f);
        return viewportIndex < g_viewports.size() ? g_viewports[viewportIndex].mouseRayDirection : fallback;
    }

    EditorViewportMode GetMode(uint32_t viewportIndex) {
        return viewportIndex < g_viewports.size() ? g_viewports[viewportIndex].mode : EditorViewportMode::PERSPECTIVE;
    }

    int32_t GetHoveredViewportIndex() {
        return g_hoveredViewportIndex;
    }

    bool IsPanning() {
        return g_cameraDragViewportIndex >= 0 && Hell::Input::MiddleMouseDown() && Hell::Input::KeyDown(HELL_KEY_LEFT_SHIFT_GLFW);
    }

    bool IsOrbiting() {
        return g_cameraDragViewportIndex >= 0 && Hell::Input::MiddleMouseDown() && !Hell::Input::KeyDown(HELL_KEY_LEFT_SHIFT_GLFW);
    }

    bool IsFlyMode() {
        return g_flyModeViewportIndex >= 0;
    }

    float GetOrthographicSize(uint32_t viewportIndex) {
        return viewportIndex < g_viewports.size() ? g_viewports[viewportIndex].orthographicSize : 5.0f;
    }

    float GetPerspectiveFov(uint32_t viewportIndex) {
        return viewportIndex < g_viewports.size() ? g_viewports[viewportIndex].perspectiveFov : 1.0f;
    }
}
