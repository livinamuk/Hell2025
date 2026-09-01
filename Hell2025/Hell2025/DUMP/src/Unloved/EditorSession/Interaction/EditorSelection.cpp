#include "EditorSelection.h"

#include "Unloved/EditorSession/UI/EditorInputElements.h"
#include "Unloved/EditorSession/EditorObjectOptions.h"
#include "EditorPointSequences.h"
#include "Unloved/EditorSession/Core/EditorViewports.h"
#include "EditorVisibility.h"

#include "Hell/Audio.h"
#include "Hell/Input.h"
#include "Hell/Physics/Physics.h"
#include "Unloved/Common/Constants.h"
#include "Unloved/EditorSession/Gizmo/Gizmo.h"
#include "Unloved/ObjectId.h"
#include "Unloved/Objects/House/Wall.h"
#include "Unloved/Objects/House/WorldPlane.h"
#include "Unloved/Objects/Renderables/SkinnedGameObject.h"
#include "Unloved/Systems/WorldBVH/WorldBVH.h"
#include "Unloved/World/World.h"

#include <limits>

namespace Unloved::EditorSession::Selection {
    namespace {
        constexpr float MAX_PICK_DISTANCE = 2000.0f;
        constexpr bool CULL_BACK_FACING = true;

        struct EditorPickResult {
            uint64_t objectId = 0;
            float distanceToHit = std::numeric_limits<float>::max();
        };

        uint64_t g_hoveredObjectId = 0;
        uint64_t g_selectedObjectId = 0;
        EditorSelectionMode g_mode = EditorSelectionMode::OBJECT;
        int32_t g_selectedPointIndex = -1;
        PointSequences::PointHandleType g_selectedPointHandleType = PointSequences::PointHandleType::ANCHOR;
        int32_t g_selectedWallSegmentIndex = -1;
        bool g_workspaceSelected = false;
        bool g_gizmoWasDragging = false;

        EditorPickResult PickClosestObject(uint32_t viewportIndex) {
            const glm::vec3& rayOrigin = Viewports::GetMouseRayOrigin(viewportIndex);
            const glm::vec3& rayDirection = Viewports::GetMouseRayDirection(viewportIndex);
            const BvhRayResult bvhResult = Unloved::WorldBVH::ClosestHit(rayOrigin, rayDirection, MAX_PICK_DISTANCE);
            const PhysXRayResult physXResult = Hell::Physics::CastPhysXRay(rayOrigin, rayDirection, MAX_PICK_DISTANCE, CULL_BACK_FACING);
            EditorPickResult result;

            if (bvhResult.hitFound) {
                result.objectId = bvhResult.objectId;
                result.distanceToHit = bvhResult.distanceToHit;
            }
            if (physXResult.hitFound && !Visibility::ShouldHide(physXResult.userData.objectId) && physXResult.distanceToHit < result.distanceToHit) {
                result.objectId = physXResult.userData.objectId;
                result.distanceToHit = physXResult.distanceToHit;
            }
            return result;
        }

        uint64_t ResolveSelectableObjectId(uint64_t objectId) {
            if (Unloved::GetObjectIdType(objectId) == ObjectType::SKINNED_GAME_OBJECT) {
                SkinnedGameObject* skinnedGameObject = World::GetSkinnedGameObjectByObjectId(objectId);
                return skinnedGameObject ? skinnedGameObject->GetOwnerObjectId() : 0;
            }
            if (Unloved::GetObjectIdType(objectId) == ObjectType::WORLD_PLANE) {
                WorldPlane* worldPlane = World::GetWorldPlaneByObjectId(objectId);
                if (worldPlane && worldPlane->GetParentDoorId() != 0) return worldPlane->GetParentDoorId();
            }
            if (Unloved::GetObjectIdType(objectId) != ObjectType::WALL_SEGMENT) return objectId;

            Wall* wall = Unloved::World::GetWallByWallSegmentObjectId(objectId);
            return wall ? wall->GetObjectId() : 0;
        }

        bool ObjectUsesRotation(uint64_t objectId) {
            switch (Unloved::GetObjectIdType(objectId)) {
                case ObjectType::SKINNED_GAME_OBJECT:
                case ObjectType::CHRISTMAS_LIGHTS:
                case ObjectType::LADDER_DISMOUNT:
                case ObjectType::SHARK:
                case ObjectType::WALL:
                    return false;
                default:
                    return true;
            }
        }

        bool SupportsObjectMode(uint64_t objectId) {
            const EditorObjectMode mode = ObjectOptions::GetEditorMode(objectId);
            return mode == EditorObjectMode::OBJECT || mode == EditorObjectMode::VERTEX_AND_OBJECT;
        }

        bool SupportsVertexMode(uint64_t objectId) {
            const EditorObjectMode mode = ObjectOptions::GetEditorMode(objectId);
            return mode == EditorObjectMode::VERTEX || mode == EditorObjectMode::VERTEX_AND_OBJECT;
        }

        void UpdateSelectedObjectFromGizmo() {
            if (g_selectedObjectId == 0) return;
            if (g_selectedWallSegmentIndex >= 0) return;

            if (g_selectedPointIndex >= 0) {
                if (Gizmo::GetMode() != GizmoMode::TRANSLATE) return;

                glm::vec3 pointPosition;
                if (!PointSequences::GetPointPosition(g_selectedObjectId, g_selectedPointIndex, g_selectedPointHandleType, pointPosition) || pointPosition == Gizmo::GetPosition()) return;
                if (!PointSequences::SetPointPosition(g_selectedObjectId, g_selectedPointIndex, g_selectedPointHandleType, Gizmo::GetPosition())) {
                    Gizmo::SetPosition(pointPosition);
                }
                else if (PointSequences::GetPointPosition(g_selectedObjectId, g_selectedPointIndex, g_selectedPointHandleType, pointPosition)) {
                    Gizmo::SetPosition(pointPosition);
                }
                return;
            }

            if (Gizmo::GetMode() == GizmoMode::TRANSLATE) {
                Unloved::World::SetPositionById(g_selectedObjectId, Gizmo::GetPosition());
            }
            if (Gizmo::GetMode() == GizmoMode::ROTATE) {
                Unloved::World::SetRotationById(g_selectedObjectId, Gizmo::GetRotation());
            }
        }
    }

    void Reset() {
        PointSequences::CancelInteraction();
        g_hoveredObjectId = 0;
        g_selectedObjectId = 0;
        g_mode = EditorSelectionMode::OBJECT;
        g_selectedPointIndex = -1;
        g_selectedPointHandleType = PointSequences::PointHandleType::ANCHOR;
        g_selectedWallSegmentIndex = -1;
        g_workspaceSelected = false;
        g_gizmoWasDragging = false;
        Gizmo::SetLocalAxes(false);
        Gizmo::SetWorldRotationAxes(false);
    }

    void Update(bool allowInput) {
        // Sync the selection from the gizmo
        const bool gizmoDragging = Gizmo::GetAction() == GizmoAction::DRAGGING;
        if (gizmoDragging || g_gizmoWasDragging) {
            UpdateSelectedObjectFromGizmo();
        }
        if (!gizmoDragging && g_gizmoWasDragging) {
            PointSequences::CommitInteraction();
        }
        g_gizmoWasDragging = gizmoDragging;
        g_hoveredObjectId = 0;

        if (!allowInput || Viewports::IsFlyMode() || gizmoDragging) return;

        // Pick from the hovered viewport
        const int32_t viewportIndex = Viewports::GetHoveredViewportIndex();
        if (viewportIndex < 0) return;

        g_hoveredObjectId = ResolveSelectableObjectId(PickClosestObject(static_cast<uint32_t>(viewportIndex)).objectId);
        if (!Hell::Input::LeftMousePressed() || Gizmo::HasHover()) return;

        if (g_hoveredObjectId == 0) {
            ClearSelection();
        }
        else {
            SelectObject(g_hoveredObjectId);
        }
    }

    void SelectWorkspace() {
        PointSequences::CancelInteraction();
        InputElements::Reset();
        g_selectedObjectId = 0;
        g_mode = EditorSelectionMode::OBJECT;
        g_selectedPointIndex = -1;
        g_selectedPointHandleType = PointSequences::PointHandleType::ANCHOR;
        g_selectedWallSegmentIndex = -1;
        g_workspaceSelected = true;
        g_gizmoWasDragging = false;
        Gizmo::CancelInteraction();
        Gizmo::SetLocalAxes(false);
        Gizmo::SetWorldRotationAxes(false);
        Hell::Audio::PlayAudio(AUDIO_SELECT, 1.0f);
    }

    void SelectObject(uint64_t objectId) {
        objectId = ResolveSelectableObjectId(objectId);
        if (objectId == 0) {
            ClearSelection();
            return;
        }

        PointSequences::CancelInteraction();
        const ObjectType objectType = GetObjectIdType(objectId);
        if (ObjectOptions::GetEditorMode(objectId) == EditorObjectMode::VERTEX) {
            SelectPoint(objectId, 0, PointSequences::PointHandleType::ANCHOR);
            return;
        }
        if (g_mode == EditorSelectionMode::VERTEX && SupportsVertexMode(objectId) && PointSequences::GetPointCount(objectId) > 0) {
            SelectPoint(objectId, 0, PointSequences::PointHandleType::ANCHOR);
            return;
        }

        InputElements::Reset();
        g_selectedObjectId = objectId;
        g_mode = EditorSelectionMode::OBJECT;
        g_selectedPointIndex = -1;
        g_selectedPointHandleType = PointSequences::PointHandleType::ANCHOR;
        g_selectedWallSegmentIndex = -1;
        g_workspaceSelected = false;
        Gizmo::SetPosition(Unloved::World::GetPositionById(objectId));
        Gizmo::SetRotation(ObjectUsesRotation(objectId) ? Unloved::World::GetRotationById(objectId) : glm::vec3(0.0f));
        Gizmo::SetLocalAxes(false);
        Gizmo::SetWorldRotationAxes(objectType == ObjectType::WORLD_PLANE || objectType == ObjectType::PLANAR_QUAD_OBJECT || objectType == ObjectType::POINT_PAIR_OBJECT);
        Gizmo::SetSourceObjectOffeset(glm::vec3(0.0f));
        Hell::Audio::PlayAudio(AUDIO_SELECT, 1.0f);
    }

    void SelectPoint(uint64_t objectId, int32_t pointIndex) {
        SelectPoint(objectId, pointIndex, PointSequences::PointHandleType::ANCHOR);
    }

    void SelectPoint(uint64_t objectId, int32_t pointIndex, PointSequences::PointHandleType handleType) {
        glm::vec3 position;
        if (!SupportsVertexMode(objectId) || !PointSequences::GetPointPosition(objectId, pointIndex, handleType, position)) {
            ClearSelection();
            return;
        }

        InputElements::Reset();
        g_selectedObjectId = objectId;
        g_mode = EditorSelectionMode::VERTEX;
        g_selectedPointIndex = pointIndex;
        g_selectedPointHandleType = handleType;
        g_selectedWallSegmentIndex = -1;
        g_workspaceSelected = false;
        Gizmo::SetPosition(position);
        const ObjectType objectType = GetObjectIdType(objectId);
        const bool localAxes = objectType == ObjectType::WORLD_PLANE || objectType == ObjectType::PLANAR_QUAD_OBJECT || objectType == ObjectType::POINT_PAIR_OBJECT || objectType == ObjectType::LADDER;
        Gizmo::SetRotation(localAxes ? Unloved::World::GetRotationById(objectId) : glm::vec3(0.0f));
        Gizmo::SetLocalAxes(localAxes);
        Gizmo::SetWorldRotationAxes(false);
        Gizmo::SetSourceObjectOffeset(glm::vec3(0.0f));
        Hell::Audio::PlayAudio(AUDIO_SELECT, 1.0f);
    }

    void SelectWallSegment(uint64_t objectId, int32_t segmentIndex) {
        Wall* wall = World::GetWallByObjectId(objectId);
        if (!wall || segmentIndex < 0 || segmentIndex >= static_cast<int32_t>(wall->GetWallSegments().size())) {
            ClearSelection();
            return;
        }

        const WallSegment& wallSegment = wall->GetWallSegments()[segmentIndex];
        InputElements::Reset();
        g_selectedObjectId = objectId;
        g_selectedPointIndex = -1;
        g_selectedPointHandleType = PointSequences::PointHandleType::ANCHOR;
        g_selectedWallSegmentIndex = segmentIndex;
        g_workspaceSelected = false;
        Gizmo::SetPosition((wallSegment.GetStart() + wallSegment.GetEnd()) * 0.5f);
        Gizmo::SetRotation(glm::vec3(0.0f));
        Gizmo::SetLocalAxes(false);
        Gizmo::SetWorldRotationAxes(false);
        Gizmo::SetSourceObjectOffeset(glm::vec3(0.0f));
        Hell::Audio::PlayAudio(AUDIO_SELECT, 1.0f);
    }

    void SetMode(EditorSelectionMode mode) {
        if (mode == EditorSelectionMode::OBJECT && !SupportsObjectMode(g_selectedObjectId)) {
            mode = EditorSelectionMode::VERTEX;
        }
        if (mode == EditorSelectionMode::VERTEX && (!SupportsVertexMode(g_selectedObjectId) || PointSequences::GetPointCount(g_selectedObjectId) == 0)) {
            mode = EditorSelectionMode::OBJECT;
        }
        if (mode == EditorSelectionMode::VERTEX) {
            if (g_mode == mode && g_selectedPointIndex >= 0) return;
            SelectPoint(g_selectedObjectId, 0, PointSequences::PointHandleType::ANCHOR);
            return;
        }
        if (g_mode != mode || g_selectedPointIndex >= 0) {
            g_mode = EditorSelectionMode::OBJECT;
            SelectObject(g_selectedObjectId);
            return;
        }
        g_mode = mode;
    }

    bool AddPoint() {
        if (g_mode != EditorSelectionMode::VERTEX || g_selectedObjectId == 0) return false;

        const int32_t segmentStartIndex = g_selectedPointIndex >= 0 ? g_selectedPointIndex : g_selectedWallSegmentIndex;
        if (segmentStartIndex < 0) return false;

        const PointSequences::PointHandleType handleType = g_selectedPointIndex >= 0 ? g_selectedPointHandleType : PointSequences::PointHandleType::ANCHOR;
        const int32_t pointIndex = PointSequences::InsertPoint(g_selectedObjectId, segmentStartIndex);
        if (pointIndex < 0) return false;

        SelectPoint(g_selectedObjectId, pointIndex, handleType);
        return true;
    }

    bool DeleteSelected() {
        if (g_selectedObjectId == 0) return false;
        if (g_selectedWallSegmentIndex >= 0) return false;
        if (g_mode == EditorSelectionMode::VERTEX && g_selectedPointIndex < 0) return false;

        if (g_selectedPointIndex >= 0) {
            if (!PointSequences::RemovePoint(g_selectedObjectId, g_selectedPointIndex)) {
                if (ObjectOptions::GetEditorMode(g_selectedObjectId) != EditorObjectMode::VERTEX || !World::RemoveObjectById(g_selectedObjectId)) return false;
            }
        }
        else if (!World::RemoveObjectById(g_selectedObjectId)) {
            return false;
        }

        ClearSelection();
        return true;
    }

    void ClearSelection() {
        PointSequences::CancelInteraction();
        g_selectedObjectId = 0;
        g_mode = EditorSelectionMode::OBJECT;
        g_selectedPointIndex = -1;
        g_selectedPointHandleType = PointSequences::PointHandleType::ANCHOR;
        g_selectedWallSegmentIndex = -1;
        g_workspaceSelected = false;
        g_gizmoWasDragging = false;
        Gizmo::CancelInteraction();
        Gizmo::SetLocalAxes(false);
        Gizmo::SetWorldRotationAxes(false);
    }

    uint64_t GetHoveredObjectId() {
        return g_hoveredObjectId;
    }

    uint64_t GetSelectedObjectId() {
        return g_selectedObjectId;
    }

    EditorSelectionMode GetMode() {
        return g_mode;
    }

    int32_t GetSelectedPointIndex() {
        return g_selectedPointIndex;
    }

    PointSequences::PointHandleType GetSelectedPointHandleType() {
        return g_selectedPointHandleType;
    }

    int32_t GetSelectedWallSegmentIndex() {
        return g_selectedWallSegmentIndex;
    }

    bool HasSelectedPoint() {
        return g_selectedObjectId != 0 && g_selectedPointIndex >= 0;
    }

    bool HasSelectedWallSegment() {
        return g_selectedObjectId != 0 && g_selectedWallSegmentIndex >= 0;
    }

    bool HasWorkspaceSelection() {
        return g_workspaceSelected;
    }

    bool HasObjectSelection() {
        return g_selectedObjectId != 0;
    }

    bool HasSelection() {
        return g_workspaceSelected || g_selectedObjectId != 0;
    }

    bool ShouldOutlineObject(uint64_t objectId) {
        if (g_mode == EditorSelectionMode::VERTEX) return false;
        if (g_selectedObjectId == 0 || objectId == 0) return false;

        if (g_selectedWallSegmentIndex >= 0) {
            Wall* wall = World::GetWallByObjectId(g_selectedObjectId);
            if (!wall || g_selectedWallSegmentIndex >= static_cast<int32_t>(wall->GetWallSegments().size())) return false;
            return wall->GetWallSegments()[g_selectedWallSegmentIndex].GetObjectId() == objectId;
        }

        if (objectId == g_selectedObjectId) return true;
        if (Unloved::GetObjectIdType(g_selectedObjectId) != ObjectType::WALL) return false;
        if (Unloved::GetObjectIdType(objectId) != ObjectType::WALL_SEGMENT) return false;

        Wall* wall = World::GetWallByObjectId(g_selectedObjectId);
        if (!wall) return false;
        for (const WallSegment& wallSegment : wall->GetWallSegments()) {
            if (wallSegment.GetObjectId() == objectId) return true;
        }
        return false;
    }
}
