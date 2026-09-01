#pragma once
#include <glm/vec3.hpp>

#include <limits>
#include <vector>

struct AABB {
    AABB() = default;
    AABB(const glm::vec3& min, const glm::vec3& max);
    explicit AABB(const std::vector<glm::vec3>& points);
    void Grow(AABB& b);
    void Grow(const glm::vec3& p);
    float Area();
    bool IntersectsSphere(const glm::vec3& sphereCenter, float radius) const;
    bool IntersectsAABB(const AABB& other) const;
    bool IntersectsAABB(const AABB& other, float threshold) const;
    bool IntersectsAABB(const glm::vec3& otherBoundsMin, const glm::vec3& otherBoundsMax) const;
    bool ContainsPoint(const glm::vec3& point) const;
    glm::vec3 NearestPointTo(const glm::vec3& worldPos) const;

    const glm::vec3& GetCenter()         const { return center; }
    const glm::vec3& GetBoundsMin()      const { return boundsMin; }
    const glm::vec3& GetBoundsMax()      const { return boundsMax; }
    const glm::vec3& GetExtents()        const { return extents; }

private:
    void CalculateCenterAndExtents();
    glm::vec3 extents = glm::vec3(0);
    glm::vec3 center = glm::vec3(0);
    glm::vec3 boundsMin = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 boundsMax = glm::vec3(std::numeric_limits<float>::lowest());
};
