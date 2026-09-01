#pragma once

#include "Hell/Math/GLM.h"

#include <vector>

namespace Hell::LineMath {
    struct CollisionLine {
        glm::vec3 p1;
        glm::vec3 p2;
        glm::vec3 GetNormal();
    };

    glm::vec3 GetLineNormal(const glm::vec3& p1, const glm::vec3& p2);
    glm::vec3 GetLineMidPoint(const glm::vec3& p1, const glm::vec3& p2);
    bool IsPointOnOtherSideOfLine(const glm::vec3& lineStart, const glm::vec3& lineEnd, const glm::vec3& lineNormal, const glm::vec3& point);
    glm::vec3 ClosestPointOnLine(const glm::vec3& lineStart, const glm::vec3& lineEnd, const glm::vec3& testPoint);
    bool LineIntersectsLine(const glm::vec2& p1, const glm::vec2& p2, const glm::vec2& q1, const glm::vec2& q2);
    bool LineIntersectsLine(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& q1, const glm::vec3& q2);
    std::vector<glm::vec3> GenerateCirclePoints(const glm::vec3& origin, float radius, int pointCount);
    bool CheckSphereLineIntersection(const glm::vec3& sphereCenter, float sphereRadius, const glm::vec3& linePoint1, const glm::vec3& linePoint2);
}
