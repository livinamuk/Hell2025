#include "OBB.h"
#include "Ray.h"

#include <glm/common.hpp>
#include <glm/matrix.hpp>
#include <glm/vec4.hpp>

OBB::OBB(const AABB& bounds, const glm::mat4& matrix) {
    m_localBounds = bounds;
    m_worldTransform = matrix;
    RecomputeCorners();
}

void OBB::SetTransform(const glm::mat4& matrix) {
    m_worldTransform = matrix;
    RecomputeCorners();
}

void OBB::SetLocalBounds(const AABB& bounds) {
    m_localBounds = bounds;
    RecomputeCorners();
}

glm::vec3 OBB::ClosestPoint(const glm::vec3& point) const {
    const glm::vec3 localPoint = glm::vec3(glm::inverse(m_worldTransform) * glm::vec4(point, 1.0f));
    const glm::vec3 closestLocalPoint = glm::clamp(localPoint, m_localBounds.GetBoundsMin(), m_localBounds.GetBoundsMax());
    return glm::vec3(m_worldTransform * glm::vec4(closestLocalPoint, 1.0f));
}

OBBRayResult OBB::Raycast(const glm::vec3& rayOrigin, const glm::vec3& rayDir, float maxDistance) const {
    OBBRayResult result;
    const Hell::Ray::Hit rayHit = Hell::Ray::IntersectAABB(rayOrigin, rayDir, maxDistance, m_localBounds, m_worldTransform);

    if (rayHit.hitFound) {
        result.hitFound = true;
        result.distanceToHit = rayHit.distanceToHit;
        result.hitPositionWorld = rayHit.hitPositionWorld;
        result.hitPositionLocal = rayHit.hitPositionLocal;
        result.hitNormalWorld = rayHit.hitNormalWorld;
        result.hitNormalLocal = rayHit.hitNormalLocal;
    }

    return result;
}

void OBB::RecomputeCorners() {
    m_corners.clear();
    m_corners.reserve(8);

    const glm::vec3& min = m_localBounds.GetBoundsMin();
    const glm::vec3& max = m_localBounds.GetBoundsMax();

    std::vector<glm::vec3> localPoints = {
        glm::vec3(min.x, min.y, min.z),
        glm::vec3(max.x, min.y, min.z),
        glm::vec3(min.x, max.y, min.z),
        glm::vec3(max.x, max.y, min.z),
        glm::vec3(min.x, min.y, max.z),
        glm::vec3(max.x, min.y, max.z),
        glm::vec3(min.x, max.y, max.z),
        glm::vec3(max.x, max.y, max.z)
    };

    for (int i = 0; i < 8; i++) {
        glm::vec4 worldP = m_worldTransform * glm::vec4(localPoints[i], 1.0f);
        m_corners.push_back(glm::vec3(worldP));
    }
}
