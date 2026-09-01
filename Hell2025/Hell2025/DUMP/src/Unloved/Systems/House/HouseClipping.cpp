#include "HouseClipping.h"

#include "Hell/Geometry/Geometry.h"

#include "Unloved/Objects/House/Door.h"
#include "Unloved/Objects/House/Window.h"
#include "Unloved/World/World.h"
#include "Unloved/Systems/House/ClippingVolume.h"

#include <clipper2/clipper.h>
#include <earcut/earcut.hpp>
#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <algorithm>
#include <cmath>

namespace mapbox {
    namespace util {
        template <std::size_t I>
        struct nth<I, glm::vec2> {
            static_assert(I < 2, "Index out of range for glm::vec2");

            inline static float get(const glm::vec2& t) {
                return (I == 0) ? t.x : t.y;
            }
        };
    }
}

namespace Unloved::HouseClipping {

void RaycastClippingVolume(const ClippingVolume& clippingVolume, const glm::vec3& rayOrigin, const glm::vec3& rayDir, float maxDistance, ClipRayResult& result) {
    const OBBRayResult hit = clippingVolume.GetOBB().Raycast(rayOrigin, rayDir, maxDistance);

    // If no ray hit, or a hit was found greater or equal than the one already stored in ray result, then bail
    if (!hit.hitFound || hit.distanceToHit >= result.distanceToHit) {
        return;
    }

    result.hitFound = true;
    result.distanceToHit = hit.distanceToHit;
    result.hitPosition = hit.hitPositionWorld;
    result.hitNormal = hit.hitNormalWorld;
    result.objectId = clippingVolume.GetOwnerObjectId();
}

std::vector<const ClippingVolume*> GetClippingVolumes() {
    std::vector<const ClippingVolume*> clippingVolumes;

    for (const Door& door : World::GetDoors()) {
        clippingVolumes.push_back(&door.GetClippingVolume());
    }

    for (const Window& window : World::GetWindows()) {
        clippingVolumes.push_back(&window.GetClippingVolume());
    }

    return clippingVolumes;
}

ClipRayResult RaycastClippingVolumes(const glm::vec3& rayOrigin, const glm::vec3& rayDir, float maxDistance) {
    ClipRayResult result;

    // Update ray result with the closest hit against all door clipping volumes
    for (const Door& door : World::GetDoors()) {
        RaycastClippingVolume(door.GetClippingVolume(), rayOrigin, rayDir, maxDistance, result);
    }

    // Update ray result with the closest hit against all window clipping volumes
    for (const Window& window : World::GetWindows()) {
        RaycastClippingVolume(window.GetClippingVolume(), rayOrigin, rayDir, maxDistance, result);
    }

    return result;
}

}

namespace Unloved::HouseClipping {

double ComputeSignedArea(const std::vector<glm::vec2>& points);
std::vector<glm::vec2> ProjectWallSegmentTo2D(WallSegment& wallSegment);
std::vector<glm::vec2> ProjectClippingVolumeSliceTo2D(const ClippingVolume& clippingVolume, const WallSegment& refWallSegment);
std::vector<glm::vec2> ProjectVolumeCornersSliceTo2D(const std::vector<glm::vec3>& volumeCorners, const WallSegment& refWallSegment);
std::vector<glm::vec2> FlattenEarcutInput(const std::vector<std::vector<glm::vec2>>& earcutInput);
std::vector<std::vector<glm::vec2>> ConvertClipperToEarcut(const Clipper2Lib::PathsD& solution);
std::vector<Vertex> ProjectBackTo3D(const std::vector<glm::vec2>& vertices2D, const WallSegment& refWallSegment);
Clipper2Lib::PathD ConvertToClipperPath(const std::vector<glm::vec2>& points);

void SubtractClippingVolumesFromWallSegment(WallSegment& wallSegment, const std::vector<const ClippingVolume*>& clippingVolumes, std::vector<Vertex>& verticesOut, std::vector<uint32_t>& indicesOut) {
    const AABB& wallAABB = wallSegment.GetAABB();

    // Create wall path
    std::vector<glm::vec2> projectedWall = ProjectWallSegmentTo2D(wallSegment);
    Clipper2Lib::PathsD wallPath = { ConvertToClipperPath(projectedWall) };

    // Create clipping volume paths
    Clipper2Lib::PathsD clippingPaths;
    for (const ClippingVolume* clippingVolume : clippingVolumes) {
        if (!clippingVolume) continue;
        if (!wallAABB.IntersectsAABB(clippingVolume->GetWorldAABB())) continue;

        std::vector<glm::vec2> projectedVolume = ProjectClippingVolumeSliceTo2D(*clippingVolume, wallSegment);
        if (projectedVolume.size() < 3) continue;

        Clipper2Lib::PathD clipPath = ConvertToClipperPath(projectedVolume);
        clippingPaths.push_back(clipPath);
    }

    // Perform Boolean Subtraction
    Clipper2Lib::PathsD solution = Clipper2Lib::Difference(wallPath, clippingPaths, Clipper2Lib::FillRule::NonZero);

    // Triangulate the result
    std::vector<std::vector<glm::vec2>> earcutInput = ConvertClipperToEarcut(solution);
    std::vector<glm::vec2> vertices2D = FlattenEarcutInput(earcutInput);
    verticesOut = ProjectBackTo3D(vertices2D, wallSegment);
    indicesOut = mapbox::earcut<uint32_t>(earcutInput);
}

std::vector<glm::vec2> ProjectWallSegmentTo2D(WallSegment& wallSegment) {
    glm::vec3 uDir = glm::normalize(wallSegment.GetEnd() - wallSegment.GetStart());
    glm::vec3 vDir = glm::vec3(0, 1, 0);
    glm::vec3 origin = wallSegment.GetStart();
    std::vector<glm::vec2> projectedPoints;
    for (const glm::vec3& corner : wallSegment.GetCorners()) {
        glm::vec3 local = corner - origin;
        projectedPoints.emplace_back(glm::dot(local, uDir), glm::dot(local, vDir));
    }
    return projectedPoints;
}

std::vector<glm::vec2> ProjectClippingVolumeSliceTo2D(const ClippingVolume& clippingVolume, const WallSegment& refWallSegment) {
    return ProjectVolumeCornersSliceTo2D(clippingVolume.GetCorners(), refWallSegment);
}

std::vector<glm::vec2> ProjectVolumeCornersSliceTo2D(const std::vector<glm::vec3>& volumeCorners, const WallSegment& refWallSegment) {
    if (volumeCorners.size() < 8) return {};

    const int edgeIndices[12][2] = {
        {0, 1}, {1, 3}, {3, 2}, {2, 0},
        {4, 5}, {5, 7}, {7, 6}, {6, 4},
        {0, 4}, {1, 5}, {2, 6}, {3, 7}
    };

    glm::vec3 uDir = glm::normalize(refWallSegment.GetEnd() - refWallSegment.GetStart());
    glm::vec3 vDir = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 wallNormal = glm::normalize(glm::cross(uDir, vDir));
    glm::vec3 planeOrigin = refWallSegment.GetStart();

    std::vector<glm::vec3> intersectionPoints;
    const float tolerance = 0.0001f;
    for (int i = 0; i < 12; ++i) {
        int idx0 = edgeIndices[i][0], idx1 = edgeIndices[i][1];
        const glm::vec3& p0 = volumeCorners[idx0], & p1 = volumeCorners[idx1];

        float d0 = glm::dot(p0 - planeOrigin, wallNormal);
        float d1 = glm::dot(p1 - planeOrigin, wallNormal);

        if (fabs(d0) < tolerance) intersectionPoints.push_back(p0);
        if (fabs(d1) < tolerance) intersectionPoints.push_back(p1);

        if (d0 * d1 < 0.0f) {
            float t = d0 / (d0 - d1);
            intersectionPoints.push_back(p0 + t * (p1 - p0));
        }
    }

    const float dupEpsilon = 0.001f;
    std::vector<glm::vec3> uniquePoints;
    for (const auto& pt : intersectionPoints) {
        if (std::none_of(uniquePoints.begin(), uniquePoints.end(), [&](const glm::vec3& upt) { return glm::length(pt - upt) < dupEpsilon; })) {
            uniquePoints.push_back(pt);
        }
    }

    if (uniquePoints.empty()) return {};

    std::vector<glm::vec2> projectedPoints;
    projectedPoints.reserve(uniquePoints.size());
    for (const glm::vec3& pt : uniquePoints) {
        glm::vec3 offset = pt - refWallSegment.GetStart();
        projectedPoints.emplace_back(glm::dot(offset, uDir), glm::dot(offset, vDir));
    }

    std::vector<glm::vec2> convexHull = Hell::Geometry::ComputeConvexHull2D(projectedPoints);
    return Hell::Geometry::SortConvexHullPoints2D(convexHull);
}

std::vector<Vertex> ProjectBackTo3D(const std::vector<glm::vec2>& vertices2D, const WallSegment& refWallSegment) {
    glm::vec3 uDir = glm::normalize(refWallSegment.GetEnd() - refWallSegment.GetStart());
    glm::vec3 vDir = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 wDir = glm::normalize(glm::cross(uDir, vDir));
    glm::vec3 origin = refWallSegment.GetStart();

    std::vector<Vertex> vertices;
    vertices.reserve(vertices2D.size());

    for (const glm::vec2& pt : vertices2D) {
        glm::vec3 pos = origin + (pt.x * uDir) + (pt.y * vDir);
        pos -= glm::dot(pos - origin, wDir) * wDir;

        Vertex vertex;
        vertex.position = pos;
        vertices.push_back(vertex);
    }
    return vertices;
}

double ComputeSignedArea(const std::vector<glm::vec2>& points) {
    double area = 0.0;
    size_t n = points.size();
    for (size_t i = 0; i < n; ++i) {
        size_t j = (i + 1) % n;
        area += (points[j].x - points[i].x) * (points[j].y + points[i].y);
    }
    return area * 0.5;
}

std::vector<glm::vec2> FlattenEarcutInput(const std::vector<std::vector<glm::vec2>>& earcutInput) {
    std::vector<glm::vec2> finalVertices;
    for (const auto& polygon : earcutInput) {
        finalVertices.insert(finalVertices.end(), polygon.begin(), polygon.end());
    }
    return finalVertices;
}

std::vector<std::vector<glm::vec2>> ConvertClipperToEarcut(const Clipper2Lib::PathsD& solution) {
    std::vector<std::vector<glm::vec2>> earcutInput;
    std::vector<uint32_t> holeIndices;

    size_t totalVertices = 0;
    for (size_t i = 0; i < solution.size(); ++i) {
        std::vector<glm::vec2> polygon;
        for (const auto& p : solution[i]) {
            polygon.emplace_back(p.x, p.y);
        }

        if (!polygon.empty()) {
            if (ComputeSignedArea(polygon) < 0) {
                holeIndices.push_back(totalVertices);
            }
            totalVertices += polygon.size();
            earcutInput.push_back(std::move(polygon));
        }
    }

    return earcutInput;
}

Clipper2Lib::PathD ConvertToClipperPath(const std::vector<glm::vec2>& points) {
    Clipper2Lib::PathD path;
    for (const auto& pt : points) {
        path.push_back(Clipper2Lib::PointD(pt.x, pt.y));
    }
    return path;
}
}
