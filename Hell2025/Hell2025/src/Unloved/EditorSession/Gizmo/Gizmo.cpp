#include "Gizmo.h"

#include "Hell/Audio.h"
#include "Hell/Geometry/PrimitiveMesh.h"
#include "Hell/Input.h"
#include "Hell/Logging.h"
#include "Hell/Math/Ray.h"
#include "Hell/Math/Rotation.h"
#include "Hell/Projection/Projection.h"

#include "Unloved/Common/Constants.h"
#include "Unloved/Common/Enums.h"
#include "Unloved/Viewport/ViewportManager.h"

#include "Unloved/Config/Config.h"
#include "Unloved/EditorSession/EditorSession.h"
#include "Unloved/EditorSession/Core/EditorViewports.h"

#ifndef GLM_ENABLE_EXPERIMENTAL
    #define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/intersect.hpp>

namespace Audio = Hell::Audio;
namespace Gizmo {
    using namespace Unloved;

    glm::mat4 GetActiveEditorViewportViewMatrix(int32_t viewportIndex) {
        return EditorSession::Viewports::GetViewMatrix(viewportIndex);
    }

    enum MeshIndex {
        RING = 0,
        SPHERE,
        CONE,
        CYLINDER,
        CUBE,
        MESH_COUNT
    };

    struct RotationDragState {
        bool active = false;
        GizmoFlag axisFlag = GizmoFlag::NONE;
        glm::vec3 center = glm::vec3(0);
        glm::vec3 axisWorld = glm::vec3(0, 1, 0); // Locked at mouse-down
        glm::vec3 basisU = glm::vec3(1, 0, 0);    // Spans plane with basisV
        glm::vec3 basisV = glm::vec3(0, 1, 0);
        glm::quat startRot = glm::quat(1, 0, 0, 0);
        float previousAngle = 0.0f;
        float accumulatedAngle = 0.0f;
        float direction = 1.0f;
    } g_rotDrag;

    struct TranslationDragState {
        bool active = false;
        glm::vec3 axis = glm::vec3(1.0f, 0.0f, 0.0f);
        glm::vec3 planeNormal = glm::vec3(0.0f, 0.0f, 1.0f);
        glm::vec3 startPosition = glm::vec3(0.0f);
        float startAxisCoordinate = 0.0f;
    } g_translateDrag;

    inline glm::vec3 ProjectOntoPlane(glm::vec3 v, glm::vec3 n) { return v - n * glm::dot(v, n); }

    inline void BuildPlaneBasis(const glm::vec3& planeNormal, const glm::vec3& cameraRight, glm::vec3& outU, glm::vec3& outV) {
        glm::vec3 u = ProjectOntoPlane(cameraRight, planeNormal);
        if (glm::dot(u, u) < 1e-6f) {
            // Fallback if cameraRight is nearly parallel to normal
            u = ProjectOntoPlane(glm::abs(planeNormal.y) > 0.5f ? glm::vec3(1, 0, 0) : glm::vec3(0, 1, 0), planeNormal);
        }
        outU = glm::normalize(u);
        outV = glm::normalize(glm::cross(planeNormal, outU));
    }

    inline bool TryGetAngleOnBasis(const glm::vec3& pOnPlane, const glm::vec3& center, const glm::vec3& U, const glm::vec3& V, float& outAngle) {
        glm::vec3 r = pOnPlane - center;
        if (glm::dot(r, r) < 1e-8f) return false;

        outAngle = std::atan2(glm::dot(r, V), glm::dot(r, U));
        return true;
    }

    inline bool ControlIsDown() {
        return Hell::Input::KeyDown(HELL_KEY_LEFT_CONTROL_GLFW) || Hell::Input::KeyDown(HELL_KEY_RIGHT_CONTROL);
    }
    
    inline glm::mat3 QuatToMat3(const glm::quat& q) {
        return glm::mat3_cast(q);
    }

    float g_gizmoSize = 1.0f;
    float g_armLength = 1.0f;
    glm::vec3 g_gizmoPosition = glm::vec3(0.0, 0.0f, 0.0f);
    glm::quat g_gizmoRotationQ = glm::quat(glm::vec3(0.0f));
    glm::vec3 g_gizmoRotationEuler = glm::vec3(0.0f);
    bool g_localAxes = false;
    bool g_worldRotationAxes = false;
    std::vector<GizmoRenderItem> g_renderItems[4];
    std::vector<MeshBufferOLD> g_meshBuffers;
    GizmoFlag g_hoverFlag = GizmoFlag::NONE;
    GizmoFlag g_actionFlag = GizmoFlag::NONE;
    GizmoAction g_action = GizmoAction::IDLE;
    GizmoMode g_mode = GizmoMode::TRANSLATE;
    int32_t g_interactionViewportIndex = -1;
    bool g_offsetNeedsUpdate = false;
    bool g_gizmoHasHover = false;
    bool g_visible = true;
    glm::ivec2 g_scaleOffset = glm::ivec2(0, 0);

    glm::vec3 g_localUpAxis = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 g_localRightAxis = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 g_localForwardAxis = glm::vec3(0.0f, 0.0f, 1.0f);

    glm::vec3 g_sourceObjectOffset = glm::vec3(0.0f);
    
    void UpdateInput(bool allowInput, bool allowModeSwitching);
    void UpdateLocalAxes();

    void Init() {
        Logging::Init() << "Initialized editor gizmo";

        g_meshBuffers.resize(MESH_COUNT);

        // Generate ring mesh
        float ringThickness = 0.03f;
        int ringSegments = 32;
        int ringThicknessSegments = 5;
        std::vector<Vertex> ringVertices = Hell::PrimitiveMesh::GenerateRingVertices(g_gizmoSize, ringThickness, ringSegments, ringThicknessSegments);
        std::vector<uint32_t> ringIndices = Hell::PrimitiveMesh::GenerateRingIndices(ringSegments, ringThicknessSegments);
        g_meshBuffers[RING].AddMesh(ringVertices, ringIndices);
        g_meshBuffers[RING].UpdateBuffers();

        // Generate sphere mesh
        float sphereRadius = g_gizmoSize - (ringThickness * 2);
        int sphereSegments = 32;
        std::vector<Vertex> sphereVerices = Hell::PrimitiveMesh::GenerateSphereVertices(sphereRadius, sphereSegments);
        std::vector<uint32_t> sphereIndices = Hell::PrimitiveMesh::GenerateSphereIndices(sphereSegments);
        g_meshBuffers[SPHERE].AddMesh(sphereVerices, sphereIndices);
        g_meshBuffers[SPHERE].UpdateBuffers();

        // Generate cone mesh
        int coneSegments = 12;
        float coneRadius = 0.125f;
        float coneHeight = 0.6f;
        std::vector<Vertex> coneVertices = Hell::PrimitiveMesh::GenerateConeVertices(coneRadius, coneHeight, coneSegments);
        std::vector<uint32_t> coneIndices = Hell::PrimitiveMesh::GenerateConeIndices(coneSegments);
        g_meshBuffers[CONE].AddMesh(coneVertices, coneIndices);
        g_meshBuffers[CONE].UpdateBuffers();

        // Generate cone mesh
        float cylinderRadius = 0.015f;
        float cylinderHeight = 1.0f;
        int cylinderSegments = 5;
        std::vector<Vertex> cylinderVertices = Hell::PrimitiveMesh::GenerateCylinderVertices(cylinderRadius, cylinderHeight, cylinderSegments);
        std::vector<uint32_t> cylinderIndices = Hell::PrimitiveMesh::GenerateCylinderIndices(cylinderSegments);
        g_meshBuffers[CYLINDER].AddMesh(cylinderVertices, cylinderIndices);
        g_meshBuffers[CYLINDER].UpdateBuffers();

        // Generate cube one mesh
        std::vector<Vertex> cubeVertices = Hell::PrimitiveMesh::GenerateCubeVertices();
        std::vector<uint32_t> cubeIndices = Hell::PrimitiveMesh::GenerateCubeIndices();
        g_meshBuffers[CUBE].AddMesh(cubeVertices, cubeIndices);
        g_meshBuffers[CUBE].UpdateBuffers();
    }

    MeshBufferOLD* GetMeshBufferByIndex(int index) {
        if (index >= 0 && index < static_cast<int>(g_meshBuffers.size())) {
            return g_meshBuffers.data() + index;
        }
        else {
            return nullptr;
        }
    }

    void Update(bool allowInput, bool allowModeSwitching) {
        if (!EditorSession::IsActive()) return;
        UpdateLocalAxes();
        UpdateRenderItems();
        UpdateInput(allowInput, allowModeSwitching);
        UpdateRenderItems();
    }

    void CancelInteraction() {
        g_hoverFlag = GizmoFlag::NONE;
        g_actionFlag = GizmoFlag::NONE;
        g_action = GizmoAction::IDLE;
        g_interactionViewportIndex = -1;
        g_gizmoHasHover = false;
        g_translateDrag = TranslationDragState{};
        g_rotDrag = RotationDragState{};
    }

    void UpdateLocalAxes() {
        if ((GetMode() == GizmoMode::ROTATE && !g_worldRotationAxes) || (GetMode() != GizmoMode::ROTATE && g_localAxes)) {
            glm::mat3 R = QuatToMat3(g_gizmoRotationQ);
            g_localRightAxis = R * glm::vec3(1, 0, 0);
            g_localUpAxis = R * glm::vec3(0, 1, 0);
            g_localForwardAxis = R * glm::vec3(0, 0, 1);
        }
        else {
            g_localUpAxis = glm::vec3(0, 1, 0);
            g_localRightAxis = glm::vec3(1, 0, 0);
            g_localForwardAxis = glm::vec3(0, 0, 1);
        }
    }

    bool BeginTranslationDrag(GizmoFlag flag, int32_t viewportIndex, const glm::vec3& rayOrigin, const glm::vec3& rayDirection) {
        const EditorSession::EditorCamera* camera = EditorSession::Viewports::GetCameraByIndex(viewportIndex);
        if (!camera) return false;

        glm::vec3 axis(0.0f);
        if (flag == GizmoFlag::TRANSLATE_X) axis = g_localRightAxis;
        if (flag == GizmoFlag::TRANSLATE_Y) axis = g_localUpAxis;
        if (flag == GizmoFlag::TRANSLATE_Z) axis = g_localForwardAxis;
        if (glm::dot(axis, axis) < 0.5f) return false;
        axis = glm::normalize(axis);

        glm::vec3 planeNormal = camera->GetForward() - axis * glm::dot(camera->GetForward(), axis);
        if (glm::dot(planeNormal, planeNormal) < 1e-6f) planeNormal = camera->GetUp() - axis * glm::dot(camera->GetUp(), axis);
        if (glm::dot(planeNormal, planeNormal) < 1e-6f) planeNormal = camera->GetRight() - axis * glm::dot(camera->GetRight(), axis);
        if (glm::dot(planeNormal, planeNormal) < 1e-6f) return false;
        planeNormal = glm::normalize(planeNormal);
        if (std::abs(glm::dot(rayDirection, planeNormal)) < 1e-6f) return false;

        float distanceToHit = 0.0f;
        if (!glm::intersectRayPlane(rayOrigin, rayDirection, g_gizmoPosition, planeNormal, distanceToHit)) return false;

        const glm::vec3 hitPosition = rayOrigin + rayDirection * distanceToHit;
        g_translateDrag.active = true;
        g_translateDrag.axis = axis;
        g_translateDrag.planeNormal = planeNormal;
        g_translateDrag.startPosition = g_gizmoPosition;
        g_translateDrag.startAxisCoordinate = glm::dot(hitPosition - g_gizmoPosition, axis);
        g_interactionViewportIndex = viewportIndex;
        g_action = GizmoAction::DRAGGING;
        g_actionFlag = flag;
        return true;
    }

    void UpdateTranslationDrag(const glm::vec3& rayOrigin, const glm::vec3& rayDirection) {
        if (!g_translateDrag.active || g_action != GizmoAction::DRAGGING) return;

        float distanceToHit = 0.0f;
        if (!glm::intersectRayPlane(rayOrigin, rayDirection, g_translateDrag.startPosition, g_translateDrag.planeNormal, distanceToHit)) return;

        const glm::vec3 hitPosition = rayOrigin + rayDirection * distanceToHit;
        float translation = glm::dot(hitPosition - g_translateDrag.startPosition, g_translateDrag.axis) - g_translateDrag.startAxisCoordinate;
        g_gizmoPosition = g_translateDrag.startPosition + g_translateDrag.axis * translation;

        if (ControlIsDown()) {
            const float axisPosition = glm::dot(g_gizmoPosition, g_translateDrag.axis);
            const float snappedAxisPosition = std::round(axisPosition * 10.0f) / 10.0f;
            g_gizmoPosition += g_translateDrag.axis * (snappedAxisPosition - axisPosition);
        }
    }

    void UpdateInput(bool allowInput, bool allowModeSwitching) {
        const int32_t hoveredViewportIndex = EditorSession::Viewports::GetHoveredViewportIndex();
        const int32_t viewportIndex = g_action == GizmoAction::DRAGGING ? g_interactionViewportIndex : hoveredViewportIndex;
        if (viewportIndex < 0) {
            g_gizmoHasHover = false;
            g_hoverFlag = GizmoFlag::NONE;
            return;
        }
        if (!allowInput && g_action == GizmoAction::IDLE) {
            g_gizmoHasHover = false;
            g_hoverFlag = GizmoFlag::NONE;
            return;
        }

        const Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(viewportIndex);
        if (!viewport) return;

        const glm::vec3 rayOrigin = EditorSession::Viewports::GetMouseRayOrigin(viewportIndex);
        const glm::vec3 rayDir = EditorSession::Viewports::GetMouseRayDirection(viewportIndex);

        glm::mat4 viewMatrix = GetActiveEditorViewportViewMatrix(viewportIndex);

        glm::mat4 projectionMatrix = viewport->GetProjectionMatrix();
        //glm::mat4 viewMatrix = camera->GetViewMatrix();
        glm::mat4 projectionView = projectionMatrix * viewMatrix;
        glm::mat4 inverseViewMatrix = glm::inverse(viewMatrix);
        glm::vec3 camRight = glm::vec3(inverseViewMatrix[0]);
        glm::vec3 camUp = glm::vec3(inverseViewMatrix[1]);
        glm::vec3 camForward = glm::vec3(inverseViewMatrix[2]);
        glm::vec3 viewPos = inverseViewMatrix[3];

        // Toggle mode
        if (allowModeSwitching && Hell::Input::KeyPressed(HELL_KEY_T) && g_mode != GizmoMode::TRANSLATE) {
            Audio::PlayAudio(AUDIO_SELECT, 1.0f);
            g_mode = GizmoMode::TRANSLATE;
        }
        if (allowModeSwitching && Hell::Input::KeyPressed(HELL_KEY_R) && g_mode != GizmoMode::ROTATE) {
            Audio::PlayAudio(AUDIO_SELECT, 1.0f);
            g_mode = GizmoMode::ROTATE;
        }
        if (allowModeSwitching && Hell::Input::KeyPressed(HELL_KEY_S) && g_mode != GizmoMode::SCALE) {
            Audio::PlayAudio(AUDIO_SELECT, 1.0f);
            g_mode = GizmoMode::SCALE;
        }

        // Get mouse ray direction
        int mouseX = Hell::Input::GetMouseX();
        int mouseY = Hell::Input::GetMouseY();
        int windowWidth = Hell::BackEnd::GetCurrentWindowWidth();
        int windowHeight = Hell::BackEnd::GetCurrentWindowHeight();

        // Raycast against all render item triangles to find hover
        if (g_action == GizmoAction::IDLE) {
            float closestDistance = 9999;
            g_gizmoHasHover = false;
            g_hoverFlag = GizmoFlag::NONE;
            for (GizmoRenderItem& renderItem : g_renderItems[viewportIndex]) {
                if (renderItem.flag == GizmoFlag::NONE) continue;

                MeshBufferOLD* mesh = Gizmo::GetMeshBufferByIndex(renderItem.meshIndex);
                if (mesh) {
                    std::vector<Vertex>& vertices = mesh->GetVertices();
                    std::vector<uint32_t>& indices = mesh->GetIndices();

                    for (int i = 0; i < indices.size(); i += 3) {
                        Vertex& v0 = vertices[mesh->GetIndices()[i + 0]];
                        Vertex& v1 = vertices[mesh->GetIndices()[i + 1]];
                        Vertex& v2 = vertices[mesh->GetIndices()[i + 2]];
                        glm::vec3 pos0 = renderItem.modelMatrix * glm::vec4(v0.position, 1.0f);
                        glm::vec3 pos1 = renderItem.modelMatrix * glm::vec4(v1.position, 1.0f);
                        glm::vec3 pos2 = renderItem.modelMatrix * glm::vec4(v2.position, 1.0f);

                        float t = 0;
                        if (Hell::Ray::IntersectTriangle(rayOrigin, rayDir, pos0, pos1, pos2, t)) {
                            if (t < closestDistance) {
                                closestDistance = t;
                                g_hoverFlag = renderItem.flag;
                                g_gizmoHasHover = true;
                            }
                        }
                    }
                }
            }
        }
        else {
            g_gizmoHasHover = true;
            g_hoverFlag = g_actionFlag;
        }

        // Translating
        if (g_actionFlag == GizmoFlag::TRANSLATE_X || g_actionFlag == GizmoFlag::TRANSLATE_Y || g_actionFlag == GizmoFlag::TRANSLATE_Z) UpdateTranslationDrag(rayOrigin, rayDir);

        if (g_actionFlag == GizmoFlag::SCALE_X ||
            g_actionFlag == GizmoFlag::SCALE_Y ||
            g_actionFlag == GizmoFlag::SCALE_Z)
        {
            if (g_action == GizmoAction::DRAGGING) {

                if (g_offsetNeedsUpdate) {
                    glm::vec3 armOffset = glm::vec3(
                        (g_actionFlag == GizmoFlag::SCALE_X) ? g_armLength * GetGizmoScalingFactorByViewportIndex(viewportIndex) : 0.0f,
                        (g_actionFlag == GizmoFlag::SCALE_Y) ? g_armLength * GetGizmoScalingFactorByViewportIndex(viewportIndex) : 0.0f,
                        (g_actionFlag == GizmoFlag::SCALE_Z) ? g_armLength * GetGizmoScalingFactorByViewportIndex(viewportIndex) : 0.0f
                    );
                    glm::mat4 mvpArm = projectionMatrix * viewMatrix * Transform(g_gizmoPosition + armOffset).to_mat4();
                    glm::ivec2 centerScreenCoords = Hell::Projection::WorldToScreen(g_gizmoPosition, projectionView, windowWidth, windowHeight, true);
                    glm::ivec2 armScreenCoords = Hell::Projection::WorldToScreen(g_gizmoPosition + armOffset, projectionView, windowWidth, windowHeight, true);
                    g_scaleOffset = armScreenCoords - centerScreenCoords;
                    g_offsetNeedsUpdate = false;

                    std::cout << "\n";
                    std::cout << "Mouse: " << mouseX << " " << mouseY << "\n";
                    std::cout << "Center: " << centerScreenCoords.x << " " << centerScreenCoords.y << "\n";
                    std::cout << "Arm: " << armScreenCoords.x << " " << armScreenCoords.y << "\n";
                    std::cout << "Offset: " << g_scaleOffset.x << " " << g_scaleOffset.y << "\n";
                }
            }            
        }

        // Begin rotate
        if ((g_hoverFlag == GizmoFlag::ROTATE_X || g_hoverFlag == GizmoFlag::ROTATE_Y || g_hoverFlag == GizmoFlag::ROTATE_Z) &&
            Hell::Input::LeftMousePressed() && g_action == GizmoAction::IDLE) {

            RotationDragState drag;
            drag.axisFlag = g_hoverFlag;
            drag.center = g_gizmoPosition;
            drag.startRot = g_gizmoRotationQ;

            glm::vec3 axis;
            switch (g_hoverFlag) {
                case GizmoFlag::ROTATE_X: axis = g_localRightAxis; break;
                case GizmoFlag::ROTATE_Y: axis = g_localUpAxis; break;
                default:                  axis = g_localForwardAxis; break;
            }
            drag.axisWorld = glm::normalize(axis);

            // Intersect with locked plane
            float t = 0.0f;
            if (glm::intersectRayPlane(rayOrigin, rayDir, drag.center, drag.axisWorld, t)) {
                glm::vec3 hit = rayOrigin + rayDir * t;
                // Choose a stable 2D basis on the plane using camera right
                BuildPlaneBasis(drag.axisWorld, camRight, drag.basisU, drag.basisV);
                if (TryGetAngleOnBasis(hit, drag.center, drag.basisU, drag.basisV, drag.previousAngle)) {
                    drag.direction = glm::dot(camForward, drag.axisWorld) < 0.0f ? -1.0f : 1.0f;
                    drag.active = true;
                    g_rotDrag = drag;
                    g_interactionViewportIndex = viewportIndex;
                    g_action = GizmoAction::DRAGGING;
                    g_actionFlag = g_hoverFlag;
                }
            }
        }

        // Rotate
        if (g_rotDrag.active && g_action == GizmoAction::DRAGGING &&
            (g_actionFlag == GizmoFlag::ROTATE_X || g_actionFlag == GizmoFlag::ROTATE_Y || g_actionFlag == GizmoFlag::ROTATE_Z)) {

            float t = 0.0f;
            if (glm::intersectRayPlane(rayOrigin, rayDir, g_rotDrag.center, g_rotDrag.axisWorld, t)) {
                glm::vec3 hit = rayOrigin + rayDir * t;
                float angleNow = 0.0f;
                if (TryGetAngleOnBasis(hit, g_rotDrag.center, g_rotDrag.basisU, g_rotDrag.basisV, angleNow)) {
                    // atan2 wraps at +/-pi, so unwrap each small frame delta before accumulating it.
                    float frameDelta = angleNow - g_rotDrag.previousAngle;
                    if (frameDelta > HELL_PI) frameDelta -= HELL_PI * 2.0f;
                    if (frameDelta < -HELL_PI) frameDelta += HELL_PI * 2.0f;

                    g_rotDrag.accumulatedAngle += frameDelta * g_rotDrag.direction;
                    g_rotDrag.previousAngle = angleNow;
                    float delta = g_rotDrag.accumulatedAngle;

                    if (ControlIsDown()) {
                        const float snap = glm::radians(22.5f);
                        delta = std::round(delta / snap) * snap;
                    }

                    // Apply about locked axis in world
                    glm::quat dq = glm::angleAxis(delta, g_rotDrag.axisWorld);
                    g_gizmoRotationQ = glm::normalize(dq * g_rotDrag.startRot);
                    g_gizmoRotationEuler = Hell::Math::NearestEulerEquivalent(g_gizmoRotationQ, g_gizmoRotationEuler);
                }
            }
        }

        // Gizmo selection
        bool rotateHovered = g_hoverFlag == GizmoFlag::ROTATE_X || g_hoverFlag == GizmoFlag::ROTATE_Y || g_hoverFlag == GizmoFlag::ROTATE_Z;
        bool translateHovered = g_hoverFlag == GizmoFlag::TRANSLATE_X || g_hoverFlag == GizmoFlag::TRANSLATE_Y || g_hoverFlag == GizmoFlag::TRANSLATE_Z;
        if (Hell::Input::LeftMousePressed() && g_hoverFlag != GizmoFlag::NONE && !rotateHovered) {
            if (translateHovered) {
                BeginTranslationDrag(g_hoverFlag, viewportIndex, rayOrigin, rayDir);
            }
            else {
                g_interactionViewportIndex = viewportIndex;
                g_action = GizmoAction::DRAGGING;
                g_actionFlag = g_hoverFlag;
                g_offsetNeedsUpdate = true;
            }
        }

        // User ended drag
        if (!Hell::Input::LeftMouseDown()) {
            if (g_action != GizmoAction::IDLE) {
                g_action = GizmoAction::IDLE;
                g_actionFlag = GizmoFlag::NONE;
                g_interactionViewportIndex = -1;
                g_translateDrag = TranslationDragState{};
                g_rotDrag = RotationDragState{};
            }
        }
    }

    

    void UpdateRenderItems() {

        if (!EditorSession::IsActive()) return;

        if (!g_visible) {
            for (int i = 0; i < 4; i++) g_renderItems[i].clear();
            return;
        }

        for (int i = 0; i < 4; i++) {
            g_renderItems[i].clear();

            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            glm::mat4 projectionMatrix = viewport->GetProjectionMatrix();

            glm::mat4 viewMatrix = GetActiveEditorViewportViewMatrix(i);
            glm::mat4 projectionView = projectionMatrix * viewMatrix;
            glm::mat4 inverseViewMatrix = glm::inverse(viewMatrix);
            glm::vec3 camRight = glm::vec3(inverseViewMatrix[0]);
            glm::vec3 camUp = glm::vec3(inverseViewMatrix[1]);
            glm::vec3 camForward = glm::vec3(inverseViewMatrix[2]);
            glm::vec3 camPos = inverseViewMatrix[3];

            float scaleCubeSize = 0.23f;
            float coneOffset = 0.9f;

            // Scale the gizmo based on camera distance.
            float scalingFactor = GetGizmoScalingFactorByViewportIndex(i);
            Transform transform;
            transform.position = g_gizmoPosition;
            transform.scale = glm::vec3(scalingFactor);

            if (g_mode == GizmoMode::TRANSLATE) {
                GizmoRenderItem& centerCube = g_renderItems[i].emplace_back();
                transform.position = glm::vec3(0.0f, 0.0f, 0.0f);
                transform.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
                transform.scale = glm::vec3(scalingFactor * 0.2f);
                centerCube.modelMatrix = transform.to_mat4();
                centerCube.meshIndex = CUBE;
                centerCube.flag = GizmoFlag::NONE;
                centerCube.color = YELLOW;

                GizmoRenderItem& cylinderX = g_renderItems[i].emplace_back();
                transform.position = glm::vec3(0.0f, 0.0f, 0.0f);
                transform.rotation = glm::vec3(0.0f, 0.0f, HELL_PI * -0.5f);
                transform.scale = glm::vec3(scalingFactor);
                cylinderX.modelMatrix = transform.to_mat4();
                cylinderX.meshIndex = CYLINDER;
                cylinderX.flag = GizmoFlag::TRANSLATE_X;
                cylinderX.color = RED;

                GizmoRenderItem& cylinderY = g_renderItems[i].emplace_back();
                transform.position = glm::vec3(0.0f, 0.0f, 0.0f);
                transform.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
                transform.scale = glm::vec3(scalingFactor);
                cylinderY.modelMatrix = transform.to_mat4();
                cylinderY.meshIndex = CYLINDER;
                cylinderY.flag = GizmoFlag::TRANSLATE_Y;
                cylinderY.color = GREEN;

                GizmoRenderItem& cylinderZ = g_renderItems[i].emplace_back();
                transform.position = glm::vec3(0.0f, 0.0f, 0.0f);
                transform.rotation = glm::vec3(HELL_PI * 0.5f, 0.0f, 0.0f);
                transform.scale = glm::vec3(scalingFactor);
                cylinderZ.modelMatrix = transform.to_mat4();
                cylinderZ.meshIndex = CYLINDER;
                cylinderZ.flag = GizmoFlag::TRANSLATE_Z;
                cylinderZ.color = BLUE;

                GizmoRenderItem& coneX = g_renderItems[i].emplace_back();
                transform.position = glm::vec3(coneOffset * scalingFactor, 0.0f, 0.0f);
                transform.rotation = glm::vec3(0.0f, 0.0f, HELL_PI * -0.5f);
                transform.scale = glm::vec3(scalingFactor);
                coneX.modelMatrix = transform.to_mat4();
                coneX.meshIndex = CONE;
                coneX.flag = GizmoFlag::TRANSLATE_X;
                coneX.color = RED;

                GizmoRenderItem& coneY = g_renderItems[i].emplace_back();
                transform.position = glm::vec3(0.0f, coneOffset * scalingFactor, 0.0f);
                transform.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
                transform.scale = glm::vec3(scalingFactor);
                coneY.modelMatrix = transform.to_mat4();
                coneY.meshIndex = CONE;
                coneY.flag = GizmoFlag::TRANSLATE_Y;
                coneY.color = GREEN;

                GizmoRenderItem& coneZ = g_renderItems[i].emplace_back();
                transform.position = glm::vec3(0.0f, 0.0f, coneOffset * scalingFactor);
                transform.rotation = glm::vec3(HELL_PI * 0.5f, 0.0f, 0.0f);
                transform.scale = glm::vec3(scalingFactor);
                coneZ.modelMatrix = transform.to_mat4();
                coneZ.meshIndex = CONE;
                coneZ.flag = GizmoFlag::TRANSLATE_Z;
                coneZ.color = BLUE;
            }

            if (g_mode == GizmoMode::SCALE) {
                GizmoRenderItem& centerCube = g_renderItems[i].emplace_back();
                transform.position = glm::vec3(0.0f, 0.0f, 0.0f);
                transform.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
                transform.scale = glm::vec3(scalingFactor * 0.2f);
                centerCube.modelMatrix = transform.to_mat4();
                centerCube.meshIndex = CUBE;
                centerCube.flag = GizmoFlag::SCALE;
                centerCube.color = YELLOW;

                GizmoRenderItem& cylinderX = g_renderItems[i].emplace_back();
                transform.position = glm::vec3(0.0f, 0.0f, 0.0f);
                transform.rotation = glm::vec3(0.0f, 0.0f, HELL_PI * -0.5f);
                transform.scale = glm::vec3(scalingFactor);
                cylinderX.modelMatrix = transform.to_mat4();
                cylinderX.meshIndex = CYLINDER;
                cylinderX.flag = GizmoFlag::SCALE_X;
                cylinderX.color = RED;

                GizmoRenderItem& cylinderY = g_renderItems[i].emplace_back();
                transform.position = glm::vec3(0.0f, 0.0f, 0.0f);
                transform.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
                transform.scale = glm::vec3(scalingFactor);
                cylinderY.modelMatrix = transform.to_mat4();
                cylinderY.meshIndex = CYLINDER;
                cylinderY.flag = GizmoFlag::SCALE_Y;
                cylinderY.color = GREEN;

                GizmoRenderItem& cylinderZ = g_renderItems[i].emplace_back();
                transform.position = glm::vec3(0.0f, 0.0f, 0.0f);
                transform.rotation = glm::vec3(HELL_PI * 0.5f, 0.0f, 0.0f);
                transform.scale = glm::vec3(scalingFactor);
                cylinderZ.modelMatrix = transform.to_mat4();
                cylinderZ.meshIndex = CYLINDER;
                cylinderZ.flag = GizmoFlag::SCALE_Z;
                cylinderZ.color = BLUE;

                GizmoRenderItem& coneX = g_renderItems[i].emplace_back();
                transform.position = glm::vec3(g_armLength * scalingFactor, 0.0f, 0.0f);
                transform.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
                transform.scale = glm::vec3(scalingFactor * scaleCubeSize);
                coneX.modelMatrix = transform.to_mat4();
                coneX.meshIndex = CUBE;
                coneX.flag = GizmoFlag::SCALE_X;
                coneX.color = RED;
                
                GizmoRenderItem& coneY = g_renderItems[i].emplace_back();
                transform.position = glm::vec3(0.0f, g_armLength * scalingFactor, 0.0f);
                transform.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
                transform.scale = glm::vec3(scalingFactor * scaleCubeSize);
                coneY.modelMatrix = transform.to_mat4();
                coneY.meshIndex = CUBE;
                coneY.flag = GizmoFlag::SCALE_Y;
                coneY.color = GREEN;

                GizmoRenderItem& coneZ = g_renderItems[i].emplace_back();
                transform.position = glm::vec3(0.0f, 0.0f, g_armLength * scalingFactor);
                transform.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
                transform.scale = glm::vec3(scalingFactor * scaleCubeSize);
                coneZ.modelMatrix = transform.to_mat4();
                coneZ.meshIndex = CUBE;
                coneZ.flag = GizmoFlag::SCALE_Z;
                coneZ.color = BLUE;
            }

            if (g_mode == GizmoMode::ROTATE) {
                // Rotate 
                GizmoRenderItem& sphere = g_renderItems[i].emplace_back();
                transform.position = glm::vec3(0.0f, 0.0f, 0.0f);
                transform.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
                transform.scale = glm::vec3(scalingFactor);
                sphere.modelMatrix = transform.to_mat4();
                sphere.meshIndex = SPHERE;
                sphere.flag = GizmoFlag::NONE;
                sphere.color = TRANSPARENT;
                
                // Rotate X
                GizmoRenderItem& rotateX = g_renderItems[i].emplace_back();
                transform.position = glm::vec3(0.0f, 0.0f, 0.0f);
                transform.rotation = glm::vec3(0.0f, HELL_PI * 0.5f, 0.0f);
                transform.scale = glm::vec3(scalingFactor);
                rotateX.modelMatrix = transform.to_mat4();
                rotateX.meshIndex = RING;
                rotateX.flag = GizmoFlag::ROTATE_X;
                rotateX.color = RED;

                // Rotate Y
                GizmoRenderItem& rotateY = g_renderItems[i].emplace_back();
                transform.position = glm::vec3(0.0f, 0.0f, 0.0f);
                transform.rotation = glm::vec3(HELL_PI * 0.5f, 0.0f, 0.0f);
                transform.scale = glm::vec3(scalingFactor);
                rotateY.modelMatrix = transform.to_mat4();
                rotateY.meshIndex = RING;
                rotateY.flag = GizmoFlag::ROTATE_Y;
                rotateY.color = GREEN;

                // Rotate Z
                GizmoRenderItem& rotateZ = g_renderItems[i].emplace_back();
                transform.position = glm::vec3(0.0f, 0.0f, 0.0f);
                transform.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
                transform.scale = glm::vec3(scalingFactor);
                rotateZ.modelMatrix = transform.to_mat4();
                rotateZ.meshIndex = RING;
                rotateZ.flag = GizmoFlag::ROTATE_Z;
                rotateZ.color = BLUE;
            }

            for (GizmoRenderItem& renderItem : g_renderItems[i]) {
                if (renderItem.flag == g_hoverFlag && renderItem.flag != GizmoFlag::NONE) {
                    renderItem.color = ORANGE;
                }
            }
        }

        // Final transform
        Transform transform;
        transform.position = g_gizmoPosition + g_sourceObjectOffset;
        
        if ((GetMode() == GizmoMode::ROTATE && !g_worldRotationAxes) || (GetMode() != GizmoMode::ROTATE && g_localAxes)) {
            transform.rotation = GetRotation();
        }

        for (int i = 0; i < 4; i++) {
            for (GizmoRenderItem& renderItem : g_renderItems[i]) {
                renderItem.modelMatrix = transform.to_mat4() * renderItem.modelMatrix;
            }
        }
    }

    std::vector<GizmoRenderItem>& GetRenderItemsByViewportIndex(int index) {
        return g_renderItems[index];
    }

    void SetPosition(const glm::vec3& position) {
        g_gizmoPosition = position;
    }

    void SetRotation(const glm::vec3& rotation) {
        g_gizmoRotationEuler = rotation;
        g_gizmoRotationQ = Hell::Math::EulerXYZToQuaternion(rotation);
    }

    void SetRotation(const glm::quat& rotation) {
        g_gizmoRotationQ = glm::normalize(rotation);
        g_gizmoRotationEuler = Hell::Math::QuaternionToEulerXYZ(g_gizmoRotationQ);
    }

    void SetMode(GizmoMode mode) {
        if (g_mode == mode) return;

        CancelInteraction();
        g_mode = mode;
    }

    void SetLocalAxes(bool enabled) {
        g_localAxes = enabled;
    }

    void SetWorldRotationAxes(bool enabled) {
        g_worldRotationAxes = enabled;
    }

    void SetSourceObjectOffeset(const glm::vec3& offset) {
        g_sourceObjectOffset = offset;
    }

    void SetVisible(bool visible) {
        g_visible = visible;
        if (g_visible) return;

        CancelInteraction();
        for (std::vector<GizmoRenderItem>& renderItems : g_renderItems) {
            renderItems.clear();
        }
    }

    const std::string GizmoFlagToString(const GizmoFlag& flag) {
        switch (flag) {
        case GizmoFlag::NONE: return "NONE";
        case GizmoFlag::TRANSLATE_X: return "TRANSLATE_X";
        case GizmoFlag::TRANSLATE_Y: return "TRANSLATE_Y";
        case GizmoFlag::TRANSLATE_Z: return "TRANSLATE_Z";
        case GizmoFlag::ROTATE_X: return "ROTATE_X";
        case GizmoFlag::ROTATE_Y: return "ROTATE_Y";
        case GizmoFlag::ROTATE_Z: return "ROTATE_Z";
        case GizmoFlag::SCALE_X: return "SCALE_X";
        case GizmoFlag::SCALE_Y: return "SCALE_Y";
        case GizmoFlag::SCALE_Z: return "SCALE_Z";
        default: return "UNKNOWN"; // Handle unexpected cases
        }
    }

    float GetGizmoScalingFactorByViewportIndex(int viewportIndex) {
        Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(viewportIndex);
        if (!viewport) return 0.0f;

        const Resolutions& resolutions = Config::GetResolutions();
        float desiredGizmoHeightPixels = 75.0f;

        int renderTargetWidth = resolutions.gBuffer.x;
        int renderTargetHeight = resolutions.gBuffer.y;
        float viewportWidth = viewport->GetSize().x * renderTargetWidth;
        float viewportHeight = viewport->GetSize().y * renderTargetHeight;

        if (viewport->IsOrthographic()) {
            float m_aspect = viewportWidth / viewportHeight;
            float left = -viewport->GetOrthoSize() * m_aspect;
            float right = viewport->GetOrthoSize() * m_aspect;
            float bottom = -viewport->GetOrthoSize();
            float top = viewport->GetOrthoSize();
            float worldHeight = top - bottom;
            float worldPerPixel = worldHeight / viewportHeight;
            float gizmoHeightInWorld = desiredGizmoHeightPixels * worldPerPixel;
            return gizmoHeightInWorld;
        }
        else {
            glm::mat4 viewMatrix = GetActiveEditorViewportViewMatrix(viewportIndex);
            glm::mat4 inverseViewMatrix = glm::inverse(viewMatrix);
            glm::vec3 camPos = glm::vec3(inverseViewMatrix[3]);
            float distance = glm::length(g_gizmoPosition - camPos);
            float fov = viewport->GetPerspectiveFOV(); // radians
            float worldHeightAtDist = 2.0f * distance * tanf(fov * 0.5f);
            float worldPerPixel = worldHeightAtDist / viewportHeight;
            return desiredGizmoHeightPixels * worldPerPixel;
        }
    }

    const glm::vec3 GetPosition() {
        return g_gizmoPosition;
    }

    const glm::vec3 GetRotation() {
        return g_gizmoRotationEuler;
    }

    const glm::quat GetRotationQuaternion() {
        return g_gizmoRotationQ;
    }

    const bool HasHover() {
        return g_gizmoHasHover;
    }

    bool UsesLocalAxes() {
        return g_localAxes;
    }

    bool UsesWorldRotationAxes() {
        return g_worldRotationAxes;
    }

    const GizmoAction GetAction() {
        return g_action;
    }

    const GizmoMode GetMode() {
        return g_mode;
    }
}
