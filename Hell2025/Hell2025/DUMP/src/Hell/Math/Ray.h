#pragma once

#include "AABB.h"

#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace Hell::Ray {

    struct Hit {
        bool hitFound = false;
        float distanceToHit = std::numeric_limits<float>::max();
        glm::vec3 hitPositionWorld = glm::vec3(0.0f);
        glm::vec3 hitPositionLocal = glm::vec3(0.0f);
        glm::vec3 hitNormalWorld = glm::vec3(0.0f);
        glm::vec3 hitNormalLocal = glm::vec3(0.0f);
    };

    inline std::vector<glm::vec3> GenerateSphereDirections(int count) {
        std::vector<glm::vec3> directions;
        directions.reserve(count);

        const float phi = glm::pi<float>() * (3.0f - std::sqrt(5.0f));

        for (int i = 0; i < count; ++i) {
            float y = 1.0f - (i / float(count - 1)) * 2.0f;
            float radius = std::sqrt(1.0f - y * y);
            float theta = phi * i;

            float x = std::cos(theta) * radius;
            float z = std::sin(theta) * radius;

            directions.push_back(glm::vec3(x, y, z));
        }

        return directions;
    }

    inline bool IntersectTriangle(const glm::vec3& rayOrigin, const glm::vec3& rayDir, const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, float& distanceToHit) {
        constexpr float EPSILON = 1e-8f;

        const glm::vec3 edge1 = v1 - v0;
        const glm::vec3 edge2 = v2 - v0;
        const glm::vec3 h = glm::cross(rayDir, edge2);
        const float a = glm::dot(edge1, h);

        if (std::fabs(a) < EPSILON) {
            return false;
        }

        const float f = 1.0f / a;
        const glm::vec3 s = rayOrigin - v0;
        const float u = f * glm::dot(s, h);

        if (u < 0.0f || u > 1.0f) {
            return false;
        }

        const glm::vec3 q = glm::cross(s, edge1);
        const float v = f * glm::dot(rayDir, q);

        if (v < 0.0f || u + v > 1.0f) {
            return false;
        }

        distanceToHit = f * glm::dot(edge2, q);
        return distanceToHit > EPSILON;
    }

    inline bool IntersectSphere(const glm::vec3& rayOrigin, const glm::vec3& rayDir, const glm::vec3& spherePosition, float sphereRadius) {
        const glm::dvec3 oc = glm::dvec3(rayOrigin) - glm::dvec3(spherePosition);
        const double b = glm::dot(oc, glm::dvec3(rayDir));
        const double c = glm::dot(oc, oc) - static_cast<double>(sphereRadius) * sphereRadius;
        const double discriminant = b * b - c;

        if (discriminant < 0.0) {
            return false;
        }

        return (-b + std::sqrt(discriminant)) >= 0.0;
    }

    inline bool IntersectRayPlane(
        const glm::vec3& rayOrigin,
        const glm::vec3& rayDirection,
        const glm::vec3& planeOrigin,
        const glm::vec3& planeNormal,
        glm::vec3& intersection,
        float epsilon = 1e-6f
    ) {
        const float denominator = glm::dot(rayDirection, planeNormal);
        if (std::abs(denominator) <= epsilon) {
            return false;
        }

        const float rayParameter = glm::dot(planeOrigin - rayOrigin, planeNormal) / denominator;
        if (rayParameter < 0.0f) {
            return false;
        }

        intersection = rayOrigin + rayDirection * rayParameter;
        return true;
    }

    inline float DistanceSquaredFromRay(
        const glm::vec3& rayOrigin,
        const glm::vec3& rayDirection,
        const glm::vec3& point,
        float maxDistance = std::numeric_limits<float>::max()
    ) {
        const float directionLengthSquared = glm::dot(rayDirection, rayDirection);
        if (directionLengthSquared <= std::numeric_limits<float>::epsilon()) {
            return std::numeric_limits<float>::max();
        }

        const float rayParameter = glm::dot(point - rayOrigin, rayDirection) / directionLengthSquared;
        const float distanceAlongRay = rayParameter * std::sqrt(directionLengthSquared);
        if (distanceAlongRay < 0.0f || distanceAlongRay > maxDistance) {
            return maxDistance < std::sqrt(std::numeric_limits<float>::max())
                ? maxDistance * maxDistance
                : std::numeric_limits<float>::max();
        }

        const glm::vec3 closestPoint = rayOrigin + rayDirection * rayParameter;
        const glm::vec3 offset = point - closestPoint;
        return glm::dot(offset, offset);
    }

    inline Hit IntersectAABB(const glm::vec3& rayOrigin, const glm::vec3& rayDir, float maxDistance, const AABB& aabb, const glm::mat4& worldTransform) {
        constexpr float MIN_RAY_DISTANCE = 0.001f;
        constexpr float RAY_EPSILON = 1e-8f;

        Hit result;

        if (maxDistance <= MIN_RAY_DISTANCE) {
            return result;
        }

        const float determinant = glm::determinant(worldTransform);
        if (!std::isfinite(determinant) || std::abs(determinant) < RAY_EPSILON) {
            return result;
        }

        const glm::mat4 inverseWorldTransform = glm::inverse(worldTransform);
        const glm::mat3 normalMatrix = glm::transpose(glm::mat3(inverseWorldTransform));
        const glm::vec3 localOrigin = glm::vec3(inverseWorldTransform * glm::vec4(rayOrigin, 1.0f));
        const glm::vec3 localDir = glm::vec3(inverseWorldTransform * glm::vec4(rayDir, 0.0f));

        if (glm::dot(localDir, localDir) < RAY_EPSILON) {
            return result;
        }

        const glm::vec3& boundsMin = aabb.GetBoundsMin();
        const glm::vec3& boundsMax = aabb.GetBoundsMax();
        glm::vec3 enterNormal = glm::vec3(0.0f);
        glm::vec3 exitNormal = glm::vec3(0.0f);
        float tEnter = -std::numeric_limits<float>::max();
        float tExit = maxDistance;

        for (int axis = 0; axis < 3; axis++) {
            if (std::abs(localDir[axis]) < RAY_EPSILON) {
                if (localOrigin[axis] < boundsMin[axis] || localOrigin[axis] > boundsMax[axis]) {
                    return result;
                }
                continue;
            }

            const float inverseDir = 1.0f / localDir[axis];
            float t0 = (boundsMin[axis] - localOrigin[axis]) * inverseDir;
            float t1 = (boundsMax[axis] - localOrigin[axis]) * inverseDir;
            float nearNormalSign = -1.0f;
            float farNormalSign = 1.0f;

            if (t0 > t1) {
                std::swap(t0, t1);
                nearNormalSign = 1.0f;
                farNormalSign = -1.0f;
            }

            if (t0 > tEnter) {
                tEnter = t0;
                enterNormal = glm::vec3(0.0f);
                enterNormal[axis] = nearNormalSign;
            }

            if (t1 < tExit) {
                tExit = t1;
                exitNormal = glm::vec3(0.0f);
                exitNormal[axis] = farNormalSign;
            }

            if (tEnter > tExit) {
                return result;
            }
        }

        if (tExit < MIN_RAY_DISTANCE) {
            return result;
        }

        result.distanceToHit = (tEnter >= MIN_RAY_DISTANCE) ? tEnter : tExit;
        result.hitFound = true;
        result.hitPositionLocal = localOrigin + (localDir * result.distanceToHit);
        result.hitPositionWorld = worldTransform * glm::vec4(result.hitPositionLocal, 1.0f);
        result.hitNormalLocal = (tEnter >= MIN_RAY_DISTANCE) ? enterNormal : exitNormal;

        if (glm::dot(result.hitNormalLocal, result.hitNormalLocal) > RAY_EPSILON) {
            result.hitNormalWorld = glm::normalize(normalMatrix * result.hitNormalLocal);
        }

        return result;
    }
}
