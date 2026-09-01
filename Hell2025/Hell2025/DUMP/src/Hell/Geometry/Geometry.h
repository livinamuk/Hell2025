#pragma once

#include "Hell/Render/VertexAttributes.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <cstdint>
#include <vector>

namespace Hell::Geometry {
    glm::vec2 CalculateUV(const glm::vec3& vertexPosition, const glm::vec3& vertexNormal);
    glm::vec2 CalculateUV(const glm::vec3& vertexPosition, const glm::vec3& origin, const glm::vec3& uAxis, const glm::vec3& vAxis);
    void SetNormalsAndTangentsFromVertices(Vertex& vert0, Vertex& vert1, Vertex& vert2);
    glm::vec3 ComputeFaceNormal(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2);
    glm::vec3 Barycentric2D(const glm::vec2& point, const glm::vec2& v0, const glm::vec2& v1, const glm::vec2& v2);
    bool IsPointInTriangle2D(const glm::vec2& point, const glm::vec2& v0, const glm::vec2& v1, const glm::vec2& v2);
    float DistancePointToSegmentSquared2D(const glm::vec2& point, const glm::vec2& segmentStart, const glm::vec2& segmentEnd);
    bool PointWithinLineSegment2D(const glm::vec2& point, const glm::vec2& segmentStart, const glm::vec2& segmentEnd, float threshold);
    glm::vec2 ComputeCentroid2D(const std::vector<glm::vec2>& points);
    std::vector<glm::vec2> SortConvexHullPoints2D(std::vector<glm::vec2>& points);
    std::vector<glm::vec2> ComputeConvexHull2D(std::vector<glm::vec2> points);
    std::vector<glm::vec3> GenerateOrientedCirclePoints(const glm::vec3& center, const glm::vec3& forward, float radius, int pointCount);
    std::vector<uint32_t> GenerateSequentialIndices(int vertexCount);
}
