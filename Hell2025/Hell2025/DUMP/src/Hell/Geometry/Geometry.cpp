#include "Geometry.h"

#include <algorithm>
#include <cmath>

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>

namespace {
    float Cross2D(const glm::vec2& origin, const glm::vec2& a, const glm::vec2& b) {
        return (a.x - origin.x) * (b.y - origin.y) - (a.y - origin.y) * (b.x - origin.x);
    }
}

namespace Hell::Geometry {

glm::vec2 CalculateUV(const glm::vec3& vertexPosition, const glm::vec3& vertexNormal) {
    glm::vec2 uv;

    glm::vec3 absNormal = glm::abs(vertexNormal);

    if (absNormal.x > absNormal.y && absNormal.x > absNormal.z) {
        uv.y = vertexPosition.y / absNormal.x;
        uv.x = vertexPosition.z / absNormal.x;
        uv.y = 1.0f - uv.y;

        if (vertexNormal.x > 0) {
            uv.x = 1.0f - uv.x;
        }
    }
    else if (absNormal.y > absNormal.x && absNormal.y > absNormal.z) {
        uv.x = vertexPosition.x / absNormal.y;
        uv.y = vertexPosition.z / absNormal.y;
        uv.y = 1.0f - uv.y;

        if (vertexNormal.y < 0) {
            uv.x = 1.0f - uv.x;
        }
    }
    else {
        uv.x = vertexPosition.x / absNormal.z;
        uv.y = vertexPosition.y / absNormal.z;
        uv.y = 1.0f - uv.y;

        if (vertexNormal.z < 0) {
            uv.x = 1.0f - uv.x;
        }
    }

    return uv;
}

glm::vec2 CalculateUV(const glm::vec3& vertexPosition, const glm::vec3& origin, const glm::vec3& uAxis, const glm::vec3& vAxis) {
    const glm::vec3 localPosition = vertexPosition - origin;
    return glm::vec2(glm::dot(localPosition, glm::normalize(uAxis)), glm::dot(localPosition, glm::normalize(vAxis)));
}

void SetNormalsAndTangentsFromVertices(Vertex& vert0, Vertex& vert1, Vertex& vert2) {
    glm::vec3& v0 = vert0.position;
    glm::vec3& v1 = vert1.position;
    glm::vec3& v2 = vert2.position;
    glm::vec2& uv0 = vert0.uv;
    glm::vec2& uv1 = vert1.uv;
    glm::vec2& uv2 = vert2.uv;

    glm::vec3 deltaPos1 = v1 - v0;
    glm::vec3 deltaPos2 = v2 - v0;
    glm::vec2 deltaUV1 = uv1 - uv0;
    glm::vec2 deltaUV2 = uv2 - uv0;
    float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
    glm::vec3 tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
    glm::vec3 normal = glm::normalize(glm::cross(deltaPos1, deltaPos2));
    vert0.normal = normal;
    vert1.normal = normal;
    vert2.normal = normal;
    vert0.tangent = tangent;
    vert1.tangent = tangent;
    vert2.tangent = tangent;
}

glm::vec3 ComputeFaceNormal(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2) {
    glm::vec3 e1 = p1 - p0;
    glm::vec3 e2 = p2 - p0;
    return glm::normalize(glm::cross(e1, e2));
}

glm::vec3 Barycentric2D(const glm::vec2& point, const glm::vec2& v0, const glm::vec2& v1, const glm::vec2& v2) {
    glm::vec2 edge0 = v1 - v0;
    glm::vec2 edge1 = v2 - v0;
    glm::vec2 edgeTarget = point - v0;

    float d00 = glm::dot(edge0, edge0);
    float d01 = glm::dot(edge0, edge1);
    float d11 = glm::dot(edge1, edge1);
    float d20 = glm::dot(edgeTarget, edge0);
    float d21 = glm::dot(edgeTarget, edge1);
    float denom = d00 * d11 - d01 * d01;

    float v = (d11 * d20 - d01 * d21) / denom;
    float w = (d00 * d21 - d01 * d20) / denom;
    float u = 1.0f - v - w;

    return glm::vec3(u, v, w);
}

bool IsPointInTriangle2D(const glm::vec2& point, const glm::vec2& v0, const glm::vec2& v1, const glm::vec2& v2) {
    glm::vec2 v0v1 = v1 - v0;
    glm::vec2 v0v2 = v2 - v0;
    glm::vec2 v0Point = point - v0;

    float denom = v0v1.x * v0v2.y - v0v1.y * v0v2.x;
    if (denom == 0.0f) return false;

    float invDenom = 1.0f / denom;
    float v = (v0Point.x * v0v2.y - v0Point.y * v0v2.x) * invDenom;
    float w = (v0v1.x * v0Point.y - v0v1.y * v0Point.x) * invDenom;
    float u = 1.0f - v - w;

    return u >= 0.0f && v >= 0.0f && w >= 0.0f;
}

float DistancePointToSegmentSquared2D(const glm::vec2& point, const glm::vec2& segmentStart, const glm::vec2& segmentEnd) {
    const glm::vec2 segment = segmentEnd - segmentStart;
    const float segmentLengthSquared = glm::dot(segment, segment);
    if (segmentLengthSquared == 0.0f) return glm::dot(point - segmentStart, point - segmentStart);

    const float t = std::clamp(glm::dot(point - segmentStart, segment) / segmentLengthSquared, 0.0f, 1.0f);
    const glm::vec2 closestPoint = segmentStart + t * segment;
    const glm::vec2 distance = point - closestPoint;
    return glm::dot(distance, distance);
}

bool PointWithinLineSegment2D(const glm::vec2& point, const glm::vec2& segmentStart, const glm::vec2& segmentEnd, float threshold) {
    const float distanceSquared = DistancePointToSegmentSquared2D(point, segmentStart, segmentEnd);
    const float thresholdSquared = threshold * threshold;
    return distanceSquared <= thresholdSquared;
}

glm::vec2 ComputeCentroid2D(const std::vector<glm::vec2>& points) {
    if (points.empty()) {
        return glm::vec2(0.0f);
    }

    glm::vec2 centroid(0.0f);
    for (const glm::vec2& point : points) {
        centroid += point;
    }

    return centroid / static_cast<float>(points.size());
}

std::vector<glm::vec2> SortConvexHullPoints2D(std::vector<glm::vec2>& points) {
    glm::vec2 centroid = ComputeCentroid2D(points);

    std::sort(points.begin(), points.end(), [&](const glm::vec2& a, const glm::vec2& b) {
        float angleA = atan2(a.y - centroid.y, a.x - centroid.x);
        float angleB = atan2(b.y - centroid.y, b.x - centroid.x);
        return angleA < angleB;
    });

    return points;
}

std::vector<glm::vec2> ComputeConvexHull2D(std::vector<glm::vec2> points) {
    if (points.size() <= 3) return points;

    std::sort(points.begin(), points.end(), [](const glm::vec2& a, const glm::vec2& b) {
        return a.x < b.x || (a.x == b.x && a.y < b.y);
    });

    std::vector<glm::vec2> hull;

    for (const glm::vec2& p : points) {
        while (hull.size() >= 2 && Cross2D(hull[hull.size() - 2], hull[hull.size() - 1], p) <= 0.0f) {
            hull.pop_back();
        }
        hull.push_back(p);
    }

    size_t lowerSize = hull.size();
    for (int i = static_cast<int>(points.size()) - 1; i >= 0; --i) {
        while (hull.size() > lowerSize && Cross2D(hull[hull.size() - 2], hull[hull.size() - 1], points[i]) <= 0.0f) {
            hull.pop_back();
        }
        hull.push_back(points[i]);
    }

    hull.pop_back();
    return hull;
}

std::vector<glm::vec3> GenerateOrientedCirclePoints(const glm::vec3& center, const glm::vec3& forward, float radius, int pointCount) {
    std::vector<glm::vec3> points;
    if (pointCount <= 0) return points;

    points.reserve(pointCount);

    glm::vec3 normalizedForward = glm::normalize(forward);
    glm::vec3 arbitrary = glm::abs(normalizedForward.x) < 0.9f ? glm::vec3(1, 0, 0) : glm::vec3(0, 1, 0);
    glm::vec3 right = glm::normalize(glm::cross(normalizedForward, arbitrary));
    glm::vec3 up = glm::normalize(glm::cross(right, normalizedForward));

    for (int i = 0; i < pointCount; ++i) {
        float angle = (2.0f * glm::pi<float>() * i) / pointCount;
        glm::vec3 offset = radius * (std::cos(angle) * right + std::sin(angle) * up);
        points.push_back(center + offset);
    }

    return points;
}

std::vector<uint32_t> GenerateSequentialIndices(int vertexCount) {
    std::vector<uint32_t> indices(vertexCount);
    for (int i = 0; i < vertexCount; ++i) {
        indices[i] = static_cast<uint32_t>(i);
    }
    return indices;
}

}
