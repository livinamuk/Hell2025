#include "LineMath.h"

#include <cmath>
#include <glm/gtc/constants.hpp>

namespace Hell::LineMath {
    glm::vec3 CollisionLine::GetNormal() {
        glm::vec2 direction = glm::vec2(p2.x, p2.z) - glm::vec2(p1.x, p1.z);
        glm::vec2 normal = glm::normalize(glm::vec2(-direction.y, direction.x));
        return glm::vec3(normal.x, 0, normal.y);
    }

    glm::vec3 GetLineNormal(const glm::vec3& p1, const glm::vec3& p2) {
        glm::vec2 direction = glm::vec2(p2.x, p2.z) - glm::vec2(p1.x, p1.z);
        glm::vec2 normal = glm::normalize(glm::vec2(-direction.y, direction.x));
        return glm::vec3(normal.x, 0, normal.y);
    }

    glm::vec3 GetLineMidPoint(const glm::vec3& p1, const glm::vec3& p2) {
        return (p1 + p2) * 0.5f;
    }

    bool IsPointOnOtherSideOfLine(const glm::vec3& lineStart, const glm::vec3& lineEnd, const glm::vec3& lineNormal, const glm::vec3& point) {
        glm::vec3 pointDirection = point - lineStart;
        return glm::dot(pointDirection, lineNormal) < 0;
    }

    glm::vec3 ClosestPointOnLine(const glm::vec3& lineStart, const glm::vec3& lineEnd, const glm::vec3& testPoint) {
        glm::vec3 lineDirection = lineEnd - lineStart;
        float lineLengthSquared = glm::dot(lineDirection, lineDirection);
        if (lineLengthSquared == 0.0f) {
            return lineStart;
        }
        float t = glm::dot(testPoint - lineStart, lineDirection) / lineLengthSquared;
        t = glm::clamp(t, 0.0f, 1.0f);
        return lineStart + t * lineDirection;
    }

    bool LineIntersectsLine(const glm::vec2& p1, const glm::vec2& p2, const glm::vec2& q1, const glm::vec2& q2) {
        auto cross = [](const glm::vec2& a, const glm::vec2& b) -> float {
            return a.x * b.y - a.y * b.x;
        };
        glm::vec2 r = p2 - p1;
        glm::vec2 s = q2 - q1;
        float rxs = cross(r, s);
        glm::vec2 qp = q1 - p1;
        float qpxr = cross(qp, r);
        if (glm::abs(rxs) < 1e-6f) {
            return false;
        }
        float t = cross(qp, s) / rxs;
        float u = qpxr / rxs;
        return (t >= 0.0f && t <= 1.0f && u >= 0.0f && u <= 1.0f);
    }

    bool LineIntersectsLine(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& q1, const glm::vec3& q2) {
        return LineIntersectsLine(glm::vec2(p1.x, p1.z), glm::vec2(p2.x, p2.z), glm::vec2(q1.x, q1.z), glm::vec2(q2.x, q2.z));
    }

    std::vector<glm::vec3> GenerateCirclePoints(const glm::vec3& origin, float radius, int pointCount) {
        std::vector<glm::vec3> points;
        points.reserve(pointCount);
        for (int i = 0; i < pointCount; ++i) {
            float angle = 2.0f * glm::pi<float>() * i / pointCount;
            float x = origin.x + radius * std::cos(angle);
            float z = origin.z + radius * std::sin(angle);
            points.emplace_back(x, origin.y, z);
        }
        return points;
    }

    bool CheckSphereLineIntersection(const glm::vec3& sphereCenter, float sphereRadius, const glm::vec3& linePoint1, const glm::vec3& linePoint2) {
        glm::vec3 lineDir = linePoint2 - linePoint1;
        glm::vec3 lineToSphere = sphereCenter - linePoint1;
        float t = glm::dot(lineToSphere, lineDir) / glm::dot(lineDir, lineDir);
        t = glm::clamp(t, 0.0f, 1.0f);
        glm::vec3 closestPoint = linePoint1 + t * lineDir;
        float distanceSquared = glm::dot(closestPoint - sphereCenter, closestPoint - sphereCenter);
        return distanceSquared <= sphereRadius * sphereRadius;
    }
}
