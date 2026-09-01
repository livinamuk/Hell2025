#include "EditorPointSequences.h"

#include "EditorSelection.h"
#include "EditorVisibility.h"
#include "Unloved/EditorSession/Core/EditorViewports.h"

#include "Hell/Input.h"

#include "Unloved/Common/Constants.h"
#include "Unloved/Debug/DebugDraw.h"
#include "Unloved/EditorSession/Gizmo/Gizmo.h"
#include "Unloved/ObjectId.h"
#include "Unloved/Objects/Exterior/Fence.h"
#include "Unloved/Objects/Exterior/PowerPoleSet.h"
#include "Unloved/Objects/House/Wall.h"
#include "Unloved/Objects/House/WorldPlane.h"
#include "Unloved/Objects/House/PlanarQuadObject.h"
#include "Unloved/Objects/House/PointPairObject.h"
#include "Unloved/Objects/Props/Christmas/ChristmasLights.h"
#include "Unloved/Objects/Traversal/Ladder.h"
#include "Unloved/Systems/DDGI/DDGIVolume.h"
#include "Unloved/Viewport/Viewport.h"
#include "Unloved/Viewport/ViewportManager.h"
#include "Unloved/World/World.h"

#include <array>
#include <limits>
#include <vector>

namespace Unloved::EditorSession::PointSequences {
    namespace {
        constexpr float POINT_HIT_RADIUS = 8.0f;
        constexpr float EDGE_HIT_RADIUS = 5.0f;
        constexpr float MIN_DDGI_EXTENT = 0.1f;
        // Point zero moves the whole volume, the other eight resize it
        constexpr int32_t DDGI_POINT_COUNT = 9;

        constexpr std::array<std::array<int32_t, 2>, 12> DDGI_EDGES = {{
            { 0, 1 }, { 0, 2 }, { 0, 4 }, { 1, 3 }, { 1, 5 }, { 2, 3 },
            { 2, 6 }, { 3, 7 }, { 4, 5 }, { 4, 6 }, { 5, 7 }, { 6, 7 }
        }};

        struct DDGIBoundsPreview {
            uint64_t objectId = 0;
            int32_t pointIndex = -1;
            glm::vec3 startBoundsMin = glm::vec3(0.0f);
            glm::vec3 startBoundsMax = glm::vec3(0.0f);
            glm::vec3 boundsMin = glm::vec3(0.0f);
            glm::vec3 boundsMax = glm::vec3(0.0f);
        };

        uint64_t g_hoveredObjectId = 0;
        int32_t g_hoveredPointIndex = -1;
        PointHandleType g_hoveredHandleType = PointHandleType::ANCHOR;
        DDGIBoundsPreview g_ddgiBoundsPreview;

        glm::vec3 GetDDGICorner(const glm::vec3& boundsMin, const glm::vec3& boundsMax, int32_t cornerIndex) {
            return glm::vec3(cornerIndex & 1 ? boundsMax.x : boundsMin.x, cornerIndex & 2 ? boundsMax.y : boundsMin.y, cornerIndex & 4 ? boundsMax.z : boundsMin.z);
        }

        bool GetDDGIBounds(uint64_t objectId, glm::vec3& boundsMin, glm::vec3& boundsMax) {
            if (g_ddgiBoundsPreview.objectId == objectId) {
                boundsMin = g_ddgiBoundsPreview.boundsMin;
                boundsMax = g_ddgiBoundsPreview.boundsMax;
                return true;
            }

            DDGIVolume* volume = World::GetDDGIVolumeByObjectId(objectId);
            if (!volume) return false;
            boundsMin = volume->GetBoundsMin();
            boundsMax = volume->GetBoundsMax();
            return true;
        }

        bool UpdateDDGIBoundsPreview(uint64_t objectId, int32_t pointIndex, const glm::vec3& position) {
            DDGIVolume* volume = World::GetDDGIVolumeByObjectId(objectId);
            if (!volume || pointIndex < 0 || pointIndex >= DDGI_POINT_COUNT) return false;

            if (g_ddgiBoundsPreview.objectId != objectId || g_ddgiBoundsPreview.pointIndex != pointIndex) {
                g_ddgiBoundsPreview.objectId = objectId;
                g_ddgiBoundsPreview.pointIndex = pointIndex;
                g_ddgiBoundsPreview.startBoundsMin = volume->GetBoundsMin();
                g_ddgiBoundsPreview.startBoundsMax = volume->GetBoundsMax();
            }

            if (pointIndex == 0) {
                const glm::vec3 startCenter = (g_ddgiBoundsPreview.startBoundsMin + g_ddgiBoundsPreview.startBoundsMax) * 0.5f;
                const glm::vec3 offset = position - startCenter;
                g_ddgiBoundsPreview.boundsMin = g_ddgiBoundsPreview.startBoundsMin + offset;
                g_ddgiBoundsPreview.boundsMax = g_ddgiBoundsPreview.startBoundsMax + offset;
                return true;
            }

            const int32_t cornerIndex = pointIndex - 1;
            const glm::vec3 opposite = GetDDGICorner(g_ddgiBoundsPreview.startBoundsMin, g_ddgiBoundsPreview.startBoundsMax, cornerIndex ^ 7);
            glm::vec3 corner = position;
            for (int32_t axis = 0; axis < 3; axis++) {
                const bool maximumSide = (cornerIndex & (1 << axis)) != 0;
                corner[axis] = maximumSide ? glm::max(corner[axis], opposite[axis] + MIN_DDGI_EXTENT) : glm::min(corner[axis], opposite[axis] - MIN_DDGI_EXTENT);
            }
            g_ddgiBoundsPreview.boundsMin = glm::min(corner, opposite);
            g_ddgiBoundsPreview.boundsMax = glm::max(corner, opposite);
            return true;
        }

        float DistanceSquaredToLine(const glm::vec2& point, const glm::vec2& start, const glm::vec2& end) {
            const glm::vec2 line = end - start;
            const float lineLengthSquared = glm::dot(line, line);
            if (lineLengthSquared <= 0.0001f) return glm::dot(point - start, point - start);
            const float t = glm::clamp(glm::dot(point - start, line) / lineLengthSquared, 0.0f, 1.0f);
            const glm::vec2 closestPoint = start + line * t;
            return glm::dot(point - closestPoint, point - closestPoint);
        }

        const std::vector<SequencePoint>* GetPoints(uint64_t objectId) {
            switch (GetObjectIdType(objectId)) {
                case ObjectType::CHRISTMAS_LIGHTS: {
                    ChristmasLightSet* christmasLights = World::GetChristmasLightsByObjectId(objectId);
                    return christmasLights ? &christmasLights->GetCreateInfo().sequencePoints : nullptr;
                }
                case ObjectType::FENCE: {
                    Fence* fence = World::GetFenceByObjectId(objectId);
                    return fence ? &fence->GetCreateInfo().sequencePoints : nullptr;
                }
                case ObjectType::POWER_POLE_SET: {
                    PowerPoleSet* powerPoleSet = World::GetPowerPoleSetByObjectId(objectId);
                    return powerPoleSet ? &powerPoleSet->GetCreateInfo().sequencePoints : nullptr;
                }
                case ObjectType::WALL: {
                    Wall* wall = World::GetWallByObjectId(objectId);
                    return wall ? &wall->GetCreateInfo().sequencePoints : nullptr;
                }
                default: return nullptr;
            }
        }

        bool HasControlHandle(uint64_t objectId) {
            return GetObjectIdType(objectId) == ObjectType::WALL;
        }

        bool IsClosedWall(uint64_t objectId, const std::vector<SequencePoint>& points) {
            return GetObjectIdType(objectId) == ObjectType::WALL && points.size() > 2 && glm::distance(points.front().position, points.back().position) < 0.05f;
        }

        bool UpdatePoints(uint64_t objectId, const std::vector<SequencePoint>& points) {
            switch (GetObjectIdType(objectId)) {
                case ObjectType::CHRISTMAS_LIGHTS: {
                    ChristmasLightSet* christmasLights = World::GetChristmasLightsByObjectId(objectId);
                    if (!christmasLights) return false;
                    christmasLights->UpdateSequencePoints(points);
                    return true;
                }
                case ObjectType::FENCE: {
                    Fence* fence = World::GetFenceByObjectId(objectId);
                    if (!fence) return false;
                    fence->UpdateSequencePoints(points);
                    return true;
                }
                case ObjectType::POWER_POLE_SET: {
                    PowerPoleSet* powerPoleSet = World::GetPowerPoleSetByObjectId(objectId);
                    if (!powerPoleSet) return false;
                    powerPoleSet->UpdateSequencePoints(points);
                    return true;
                }
                case ObjectType::WALL: {
                    Wall* wall = World::GetWallByObjectId(objectId);
                    if (!wall) return false;
                    wall->UpdateSequencePoints(points);
                    return true;
                }
                default: return false;
            }
        }

        bool ProjectPoint(Viewport& viewport, uint32_t viewportIndex, const glm::vec3& position, glm::vec2& screenPosition, float& depth) {
            const glm::mat4& viewMatrix = Viewports::GetViewMatrix(viewportIndex);
            const glm::vec4 viewPosition = viewMatrix * glm::vec4(position, 1.0f);
            const glm::vec4 clipPosition = viewport.GetProjectionMatrix() * viewPosition;
            if (glm::abs(clipPosition.w) < 0.0001f || (!viewport.IsOrthographic() && clipPosition.w <= 0.0f)) return false;

            const glm::vec3 ndc = glm::vec3(clipPosition) / clipPosition.w;
            const float width = static_cast<float>(viewport.GetRightPixel() - viewport.GetLeftPixel());
            const float height = static_cast<float>(viewport.GetBottomPixel() - viewport.GetTopPixel());
            screenPosition.x = (ndc.x * 0.5f + 0.5f) * width;
            screenPosition.y = (0.5f - ndc.y * 0.5f) * height;
            depth = glm::abs(viewPosition.z);
            return true;
        }
    }

    bool UpdateInput(bool allowInput) {
        g_hoveredObjectId = 0;
        g_hoveredPointIndex = -1;
        g_hoveredHandleType = PointHandleType::ANCHOR;
        if (!allowInput || Viewports::IsFlyMode() || Gizmo::GetAction() == GizmoAction::DRAGGING) return false;

        const int32_t viewportIndex = Viewports::GetHoveredViewportIndex();
        if (viewportIndex < 0) return false;

        Viewport* viewport = ViewportManager::GetViewportByIndex(viewportIndex);
        if (!viewport) return false;

        const glm::vec2 mousePosition = viewport->GetLocalMouseCoords();

        // Check point handles
        if (Selection::GetMode() == EditorSelectionMode::VERTEX) {
            const uint64_t objectId = Selection::GetSelectedObjectId();
            const int32_t pointCount = GetPointCount(objectId);
            float closestDistanceSquared = POINT_HIT_RADIUS * POINT_HIT_RADIUS;
            float closestDepth = std::numeric_limits<float>::max();

            const int32_t handleCount = HasControlHandle(objectId) ? 2 : 1;
            for (int32_t pointIndex = 0; pointIndex < pointCount; pointIndex++) {
                for (int32_t handleIndex = 0; handleIndex < handleCount; handleIndex++) {
                    const PointHandleType handleType = static_cast<PointHandleType>(handleIndex);
                    glm::vec3 pointPosition;
                    glm::vec2 screenPosition;
                    float depth = 0.0f;
                    if (!GetPointPosition(objectId, pointIndex, handleType, pointPosition) || !ProjectPoint(*viewport, viewportIndex, pointPosition, screenPosition, depth)) continue;

                    const glm::vec2 pointToMouse = mousePosition - screenPosition;
                    const float distanceSquared = glm::dot(pointToMouse, pointToMouse);
                    if (distanceSquared > closestDistanceSquared || (distanceSquared == closestDistanceSquared && depth >= closestDepth)) continue;

                    closestDistanceSquared = distanceSquared;
                    closestDepth = depth;
                    g_hoveredObjectId = objectId;
                    g_hoveredPointIndex = pointIndex;
                    g_hoveredHandleType = handleType;
                }
            }

            if (g_hoveredPointIndex >= 0) {
                if (!Hell::Input::LeftMousePressed() || Gizmo::HasHover()) return false;
                Selection::SelectPoint(objectId, g_hoveredPointIndex, g_hoveredHandleType);
                return true;
            }
        }

        // Check DDGI bounds
        float closestDistanceSquared = EDGE_HIT_RADIUS * EDGE_HIT_RADIUS;
        float closestDepth = std::numeric_limits<float>::max();
        for (DDGIVolume& volume : World::GetDDGIVolumes()) {
            const uint64_t objectId = volume.GetObjectId();
            if (Visibility::ShouldHide(objectId)) continue;

            glm::vec3 boundsMin;
            glm::vec3 boundsMax;
            if (!GetDDGIBounds(objectId, boundsMin, boundsMax)) continue;

            for (const std::array<int32_t, 2>& edge : DDGI_EDGES) {
                glm::vec2 start;
                glm::vec2 end;
                float startDepth = 0.0f;
                float endDepth = 0.0f;
                if (!ProjectPoint(*viewport, viewportIndex, GetDDGICorner(boundsMin, boundsMax, edge[0]), start, startDepth)) continue;
                if (!ProjectPoint(*viewport, viewportIndex, GetDDGICorner(boundsMin, boundsMax, edge[1]), end, endDepth)) continue;

                const float distanceSquared = DistanceSquaredToLine(mousePosition, start, end);
                const float depth = (startDepth + endDepth) * 0.5f;
                if (distanceSquared > closestDistanceSquared || (distanceSquared == closestDistanceSquared && depth >= closestDepth)) continue;
                closestDistanceSquared = distanceSquared;
                closestDepth = depth;
                g_hoveredObjectId = objectId;
            }
        }

        if (g_hoveredObjectId == 0 || !Hell::Input::LeftMousePressed() || Gizmo::HasHover()) return false;
        Selection::SelectObject(g_hoveredObjectId);
        return true;
    }

    void Draw() {
        // Draw DDGI bounds
        for (DDGIVolume& volume : World::GetDDGIVolumes()) {
            const uint64_t objectId = volume.GetObjectId();
            if (Visibility::ShouldHide(objectId)) continue;

            glm::vec3 boundsMin;
            glm::vec3 boundsMax;
            if (!GetDDGIBounds(objectId, boundsMin, boundsMax)) continue;

            const bool selected = Selection::GetSelectedObjectId() == objectId;
            const bool hovered = g_hoveredObjectId == objectId && g_hoveredPointIndex < 0;

            const glm::vec4 color = hovered ? YELLOW : selected ? WHITE : GRID_COLOR;

            for (const std::array<int32_t, 2>& edge : DDGI_EDGES) {
                DebugDraw::DrawLine(GetDDGICorner(boundsMin, boundsMax, edge[0]), GetDDGICorner(boundsMin, boundsMax, edge[1]), color);
            }
        }

        // Draw the selected sequence
        if (Selection::GetMode() != EditorSelectionMode::VERTEX) return;
        const uint64_t objectId = Selection::GetSelectedObjectId();
        const int32_t pointCount = GetPointCount(objectId);

        const std::vector<SequencePoint>* points = GetPoints(objectId);
        WorldPlane* worldPlane = GetObjectIdType(objectId) == ObjectType::WORLD_PLANE ? World::GetWorldPlaneByObjectId(objectId) : nullptr;
        PlanarQuadObject* planarQuadObject = GetObjectIdType(objectId) == ObjectType::PLANAR_QUAD_OBJECT ? World::GetPlanarQuadObjectByObjectId(objectId) : nullptr;
        PointPairObject* pointPairObject = GetObjectIdType(objectId) == ObjectType::POINT_PAIR_OBJECT ? World::GetPointPairObjectByObjectId(objectId) : nullptr;
        Ladder* ladder = GetObjectIdType(objectId) == ObjectType::LADDER ? World::GetLadderByObjectId(objectId) : nullptr;
        if (worldPlane) {
            worldPlane->DrawEdges(WHITE);
        }
        else if (planarQuadObject) {
            DebugDraw::DrawLine(planarQuadObject->GetPositionP0(), planarQuadObject->GetPositionP1(), WHITE);
            DebugDraw::DrawLine(planarQuadObject->GetPositionP1(), planarQuadObject->GetPositionP2(), WHITE);
            DebugDraw::DrawLine(planarQuadObject->GetPositionP2(), planarQuadObject->GetPositionP3(), WHITE);
            DebugDraw::DrawLine(planarQuadObject->GetPositionP3(), planarQuadObject->GetPositionP0(), WHITE);
        }
        else if (pointPairObject) {
            DebugDraw::DrawLine(pointPairObject->GetPositionP0(), pointPairObject->GetPositionP1(), WHITE);
        }
        else if (ladder) {
            DebugDraw::DrawLine(ladder->GetBottomPoint(), ladder->GetTopPoint(), WHITE);
        }
        else if (points) {
            for (size_t pointIndex = 1; pointIndex < points->size(); pointIndex++) {
                DebugDraw::DrawLine((*points)[pointIndex - 1].position, (*points)[pointIndex].position, WHITE);
            }

            if (GetObjectIdType(objectId) == ObjectType::WALL) {
                for (size_t pointIndex = 1; pointIndex < points->size(); pointIndex++) {
                    const glm::vec3 previousTop = (*points)[pointIndex - 1].position + glm::vec3(0.0f, (*points)[pointIndex - 1].customFloat, 0.0f);
                    const glm::vec3 currentTop = (*points)[pointIndex].position + glm::vec3(0.0f, (*points)[pointIndex].customFloat, 0.0f);
                    DebugDraw::DrawLine(previousTop, currentTop, WHITE);
                }
                for (int32_t pointIndex = 0; pointIndex < pointCount; pointIndex++) {
                    const SequencePoint& point = (*points)[pointIndex];
                    DebugDraw::DrawLine(point.position, point.position + glm::vec3(0.0f, point.customFloat, 0.0f), WHITE);
                }
            }
        }

        const int32_t handleCount = HasControlHandle(objectId) ? 2 : 1;
        for (int32_t pointIndex = 0; pointIndex < pointCount; pointIndex++) {
            for (int32_t handleIndex = 0; handleIndex < handleCount; handleIndex++) {
                const PointHandleType handleType = static_cast<PointHandleType>(handleIndex);
                glm::vec3 position;
                if (!GetPointPosition(objectId, pointIndex, handleType, position)) continue;

                const bool hovered = g_hoveredObjectId == objectId && pointIndex == g_hoveredPointIndex && handleType == g_hoveredHandleType;
                DebugDraw::DrawPoint(position, hovered ? YELLOW : OUTLINE_COLOR);
            }
        }
    }

    void CancelInteraction() {
        g_ddgiBoundsPreview = {};
    }

    void CommitInteraction() {
        if (g_ddgiBoundsPreview.objectId == 0) return;

        // DDGI rebuilds are expensive so only change it when dragging stops
        DDGIVolume* volume = World::GetDDGIVolumeByObjectId(g_ddgiBoundsPreview.objectId);
        if (volume) {
            const glm::vec3 origin = (g_ddgiBoundsPreview.boundsMin + g_ddgiBoundsPreview.boundsMax) * 0.5f;
            const glm::vec3 extents = g_ddgiBoundsPreview.boundsMax - g_ddgiBoundsPreview.boundsMin;
            volume->SetOrigin(origin);
            volume->SetExtents(extents);
        }
        g_ddgiBoundsPreview = {};
    }

    int32_t GetPointCount(uint64_t objectId) {
        if (GetObjectIdType(objectId) == ObjectType::DDGI_VOLUME) return World::GetDDGIVolumeByObjectId(objectId) ? DDGI_POINT_COUNT : 0;
        if (GetObjectIdType(objectId) == ObjectType::LADDER) return World::GetLadderByObjectId(objectId) ? 2 : 0;
        if (GetObjectIdType(objectId) == ObjectType::PLANAR_QUAD_OBJECT) return World::GetPlanarQuadObjectByObjectId(objectId) ? 4 : 0;
        if (GetObjectIdType(objectId) == ObjectType::POINT_PAIR_OBJECT) return World::GetPointPairObjectByObjectId(objectId) ? 2 : 0;
        if (GetObjectIdType(objectId) == ObjectType::WORLD_PLANE) {
            WorldPlane* worldPlane = World::GetWorldPlaneByObjectId(objectId);
            return worldPlane && worldPlane->GetPlanarQuad().IsValid() ? 4 : 0;
        }

        const std::vector<SequencePoint>* points = GetPoints(objectId);
        if (!points) return 0;
        if (IsClosedWall(objectId, *points)) return static_cast<int32_t>(points->size()) - 1;
        return static_cast<int32_t>(points->size());
    }

    int32_t InsertPoint(uint64_t objectId, int32_t segmentStartIndex) {
        const std::vector<SequencePoint>* currentPoints = GetPoints(objectId);
        if (!currentPoints || segmentStartIndex < 0 || segmentStartIndex + 1 >= static_cast<int32_t>(currentPoints->size())) return -1;

        const SequencePoint& start = (*currentPoints)[segmentStartIndex];
        const SequencePoint& end = (*currentPoints)[segmentStartIndex + 1];
        SequencePoint point = start;
        const glm::vec3 normal = start.normal + end.normal;
        point.position = (start.position + end.position) * 0.5f;
        point.normal = glm::dot(normal, normal) > 0.000001f ? glm::normalize(normal) : start.normal;
        point.customFloat = (start.customFloat + end.customFloat) * 0.5f;

        std::vector<SequencePoint> points = *currentPoints;
        const int32_t pointIndex = segmentStartIndex + 1;
        points.insert(points.begin() + pointIndex, point);
        return UpdatePoints(objectId, points) ? pointIndex : -1;
    }

    bool RemovePoint(uint64_t objectId, int32_t pointIndex) {
        const std::vector<SequencePoint>* currentPoints = GetPoints(objectId);
        if (!currentPoints || pointIndex < 0 || pointIndex >= GetPointCount(objectId)) return false;

        const bool closedWall = IsClosedWall(objectId, *currentPoints);
        const int32_t minimumPointCount = closedWall ? 3 : 2;
        if (GetPointCount(objectId) <= minimumPointCount) return false;

        std::vector<SequencePoint> points = *currentPoints;
        if (closedWall) {
            points.pop_back();
        }
        points.erase(points.begin() + pointIndex);
        if (closedWall) {
            points.push_back(points.front());
        }
        return UpdatePoints(objectId, points);
    }

    bool GetPointPosition(uint64_t objectId, int32_t pointIndex, PointHandleType handleType, glm::vec3& position) {
        if (GetObjectIdType(objectId) == ObjectType::DDGI_VOLUME) {
            glm::vec3 boundsMin;
            glm::vec3 boundsMax;
            if (handleType != PointHandleType::ANCHOR || pointIndex < 0 || pointIndex >= DDGI_POINT_COUNT || !GetDDGIBounds(objectId, boundsMin, boundsMax)) return false;
            position = pointIndex == 0 ? (boundsMin + boundsMax) * 0.5f : GetDDGICorner(boundsMin, boundsMax, pointIndex - 1);
            return true;
        }

        if (GetObjectIdType(objectId) == ObjectType::WORLD_PLANE) {
            WorldPlane* worldPlane = World::GetWorldPlaneByObjectId(objectId);
            if (!worldPlane || handleType != PointHandleType::ANCHOR || pointIndex < 0 || pointIndex >= 4) return false;

            switch (pointIndex) {
                case 0: position = worldPlane->GetPlanarQuad().GetPositionP0(); break;
                case 1: position = worldPlane->GetPlanarQuad().GetPositionP1(); break;
                case 2: position = worldPlane->GetPlanarQuad().GetPositionP2(); break;
                case 3: position = worldPlane->GetPlanarQuad().GetPositionP3(); break;
            }
            return true;
        }

        if (GetObjectIdType(objectId) == ObjectType::PLANAR_QUAD_OBJECT) {
            PlanarQuadObject* planarQuadObject = World::GetPlanarQuadObjectByObjectId(objectId);
            if (!planarQuadObject || handleType != PointHandleType::ANCHOR || pointIndex < 0 || pointIndex >= 4) return false;
            switch (pointIndex) {
                case 0: position = planarQuadObject->GetPositionP0(); break;
                case 1: position = planarQuadObject->GetPositionP1(); break;
                case 2: position = planarQuadObject->GetPositionP2(); break;
                case 3: position = planarQuadObject->GetPositionP3(); break;
            }
            return true;
        }

        if (GetObjectIdType(objectId) == ObjectType::POINT_PAIR_OBJECT) {
            PointPairObject* pointPairObject = World::GetPointPairObjectByObjectId(objectId);
            if (!pointPairObject || handleType != PointHandleType::ANCHOR || pointIndex < 0 || pointIndex >= 2) return false;
            position = pointIndex == 0 ? pointPairObject->GetPositionP0() : pointPairObject->GetPositionP1();
            return true;
        }

        if (GetObjectIdType(objectId) == ObjectType::LADDER) {
            Ladder* ladder = World::GetLadderByObjectId(objectId);
            if (!ladder || handleType != PointHandleType::ANCHOR || pointIndex < 0 || pointIndex >= 2) return false;
            position = pointIndex == 0 ? ladder->GetBottomPoint() : ladder->GetTopPoint();
            return true;
        }

        const std::vector<SequencePoint>* points = GetPoints(objectId);
        if (!points || pointIndex < 0 || pointIndex >= static_cast<int32_t>(points->size())) return false;

        const SequencePoint& point = (*points)[pointIndex];
        position = point.position;
        if (handleType == PointHandleType::CONTROL) {
            if (!HasControlHandle(objectId)) return false;
            position.y += point.customFloat;
        }
        return true;
    }

    bool SetPointPosition(uint64_t objectId, int32_t pointIndex, PointHandleType handleType, const glm::vec3& position) {
        if (GetObjectIdType(objectId) == ObjectType::DDGI_VOLUME) {
            if (handleType != PointHandleType::ANCHOR || !UpdateDDGIBoundsPreview(objectId, pointIndex, position)) return false;
            if (Gizmo::GetAction() != GizmoAction::DRAGGING) {
                CommitInteraction();
            }
            return true;
        }

        switch (GetObjectIdType(objectId)) {
            case ObjectType::CHRISTMAS_LIGHTS: {
                if (handleType != PointHandleType::ANCHOR) return false;
                ChristmasLightSet* christmasLights = World::GetChristmasLightsByObjectId(objectId);
                if (!christmasLights || pointIndex < 0 || pointIndex >= static_cast<int32_t>(christmasLights->GetCreateInfo().sequencePoints.size())) return false;

                std::vector<SequencePoint> points = christmasLights->GetCreateInfo().sequencePoints;
                points[pointIndex].position = position;
                christmasLights->UpdateSequencePoints(points);
                return true;
            }
            case ObjectType::FENCE: {
                if (handleType != PointHandleType::ANCHOR) return false;
                Fence* fence = World::GetFenceByObjectId(objectId);
                if (!fence || pointIndex < 0 || pointIndex >= static_cast<int32_t>(fence->GetCreateInfo().sequencePoints.size())) return false;

                std::vector<SequencePoint> points = fence->GetCreateInfo().sequencePoints;
                points[pointIndex].position = position;
                fence->UpdateSequencePoints(points);
                return true;
            }
            case ObjectType::POWER_POLE_SET: {
                if (handleType != PointHandleType::ANCHOR) return false;
                PowerPoleSet* powerPoleSet = World::GetPowerPoleSetByObjectId(objectId);
                if (!powerPoleSet || pointIndex < 0 || pointIndex >= static_cast<int32_t>(powerPoleSet->GetCreateInfo().sequencePoints.size())) return false;

                std::vector<SequencePoint> points = powerPoleSet->GetCreateInfo().sequencePoints;
                points[pointIndex].position = position;
                powerPoleSet->UpdateSequencePoints(points);
                return true;
            }
            case ObjectType::PLANAR_QUAD_OBJECT: {
                if (handleType != PointHandleType::ANCHOR) return false;
                PlanarQuadObject* planarQuadObject = World::GetPlanarQuadObjectByObjectId(objectId);
                return planarQuadObject && pointIndex >= 0 && planarQuadObject->SetPointPosition(pointIndex, position);
            }
            case ObjectType::POINT_PAIR_OBJECT: {
                if (handleType != PointHandleType::ANCHOR) return false;
                PointPairObject* pointPairObject = World::GetPointPairObjectByObjectId(objectId);
                return pointPairObject && pointIndex >= 0 && pointPairObject->SetPointPosition(pointIndex, position);
            }
            case ObjectType::LADDER: {
                if (handleType != PointHandleType::ANCHOR) return false;
                Ladder* ladder = World::GetLadderByObjectId(objectId);
                return ladder && pointIndex >= 0 && ladder->SetEditableAxisSpanPointPosition(pointIndex, position);
            }
            case ObjectType::WALL: {
                Wall* wall = World::GetWallByObjectId(objectId);
                if (!wall || pointIndex < 0 || pointIndex >= static_cast<int32_t>(wall->GetCreateInfo().sequencePoints.size())) return false;

                const SequencePoint& point = wall->GetCreateInfo().sequencePoints[pointIndex];
                if (handleType == PointHandleType::ANCHOR) return wall->UpdatePointPosition(pointIndex, position);

                const float height = position.y - point.position.y;
                if (height < 0.0f) return false;

                const glm::vec3 basePosition = glm::vec3(position.x, point.position.y, position.z);
                if (basePosition != point.position && !wall->UpdatePointPosition(pointIndex, basePosition)) return false;
                if (height != point.customFloat) {
                    wall->SetPointHeight(pointIndex, height);
                }
                return true;
            }
            case ObjectType::WORLD_PLANE: {
                if (handleType != PointHandleType::ANCHOR) return false;
                WorldPlane* worldPlane = World::GetWorldPlaneByObjectId(objectId);
                return worldPlane && pointIndex >= 0 && worldPlane->SetPointPosition(pointIndex, position);
            }
            default: return false;
        }
    }
}
