#include "AABB.h"

#include <algorithm>
#include <glm/common.hpp>
#include <glm/geometric.hpp>

AABB::AABB(const glm::vec3& min, const glm::vec3& max) {
    boundsMin = min;
    boundsMax = max;
    CalculateCenterAndExtents();
}

AABB::AABB(const std::vector<glm::vec3>& points) {
    for (const glm::vec3& point : points) {
        boundsMin = glm::min(boundsMin, point);
        boundsMax = glm::max(boundsMax, point);
    }

    if (!points.empty()) {
        CalculateCenterAndExtents();
    }
}

void AABB::Grow(AABB& b) {
    if (b.boundsMin.x != 1e30f && b.boundsMin.x != -1e30f) {
        Grow(b.boundsMin); Grow(b.boundsMax);
    }
    AABB::CalculateCenterAndExtents();
}
void AABB::Grow(const glm::vec3& p) {
    boundsMin = glm::vec3(std::min(boundsMin.x, p.x), std::min(boundsMin.y, p.y), std::min(boundsMin.z, p.z));
    boundsMax = glm::vec3(std::max(boundsMax.x, p.x), std::max(boundsMax.y, p.y), std::max(boundsMax.z, p.z));
    CalculateCenterAndExtents();
}
float AABB::Area() {
    glm::vec3 e = boundsMax - boundsMin; // box extent
    return 2.0f * (e.x * e.y + e.y * e.z + e.z * e.x);
}

void AABB::CalculateCenterAndExtents() {
    center = { (boundsMin.x + boundsMax.x) / 2, (boundsMin.y + boundsMax.y) / 2, (boundsMin.z + boundsMax.z) / 2 };
    extents = boundsMax - boundsMin;
}

bool AABB::ContainsPoint(const glm::vec3& point) const {
    return (point.x >= boundsMin.x && point.x <= boundsMax.x) &&
        (point.y >= boundsMin.y && point.y <= boundsMax.y) &&
        (point.z >= boundsMin.z && point.z <= boundsMax.z);
}

bool AABB::IntersectsSphere(const glm::vec3& sphereCenter, float radius) const {
    glm::vec3 closestPoint = glm::clamp(sphereCenter, boundsMin, boundsMax);
    glm::vec3 diff = closestPoint - sphereCenter;
    float distSq = glm::dot(diff, diff);
    return distSq <= (radius * radius);
}

bool AABB::IntersectsAABB(const AABB& other) const {
    return (boundsMin.x <= other.boundsMax.x && boundsMax.x >= other.boundsMin.x) &&
        (boundsMin.y <= other.boundsMax.y && boundsMax.y >= other.boundsMin.y) &&
        (boundsMin.z <= other.boundsMax.z && boundsMax.z >= other.boundsMin.z);
}

bool AABB::IntersectsAABB(const glm::vec3& otherBoundsMin, const glm::vec3& otherBoundsMax) const {
    return (boundsMin.x <= otherBoundsMax.x && boundsMax.x >= otherBoundsMin.x) &&
        (boundsMin.y <= otherBoundsMax.y && boundsMax.y >= otherBoundsMin.y) &&
        (boundsMin.z <= otherBoundsMax.z && boundsMax.z >= otherBoundsMin.z);
}

bool AABB::IntersectsAABB(const AABB& other, float threshold) const {
    glm::vec3 inflatedMinA = boundsMin - glm::vec3(threshold);
    glm::vec3 inflatedMaxA = boundsMax + glm::vec3(threshold);
    glm::vec3 inflatedMinB = other.boundsMin - glm::vec3(threshold);
    glm::vec3 inflatedMaxB = other.boundsMax + glm::vec3(threshold);

    return (inflatedMinA.x <= inflatedMaxB.x && inflatedMaxA.x >= inflatedMinB.x) &&
        (inflatedMinA.y <= inflatedMaxB.y && inflatedMaxA.y >= inflatedMinB.y) &&
        (inflatedMinA.z <= inflatedMaxB.z && inflatedMaxA.z >= inflatedMinB.z);
}

glm::vec3 AABB::NearestPointTo(const glm::vec3& worldPosition) const {
    return glm::clamp(worldPosition, boundsMin, boundsMax);
}
