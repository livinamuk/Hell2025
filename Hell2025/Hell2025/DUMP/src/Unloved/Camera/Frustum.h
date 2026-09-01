#pragma once

#include "Hell/Math/AABB.h"
#include "Hell/Math/GLM.h"

#include "Unloved/Render/RendererTypes.h"

#include <vector>

namespace Unloved {

struct FrustumPlane {
    glm::vec3 normal;
    float offset;
};

struct Frustum {
    Frustum() = default;

    void Update(const glm::mat4& projectionView);
    bool IntersectsAABB(const AABB& aabb);
    bool IntersectsAABBFast(const AABB& aabb);
    bool IntersectsAABBFast(const RenderItem& renderItem);
    bool IntersectsPoint(const glm::vec3 point);

    const glm::vec3& GetBoundsMin() const       { return m_boundsMin; }
    const glm::vec3& GetBoundsMax() const       { return m_boundsMax; }
    const glm::vec3& GetCorner(int index) const { return m_corners[index]; } // sketchy: allows out of bounds access
    glm::vec4 GetPlane(int index);

private:
    float SignedDistance(const glm::vec3& point, const FrustumPlane& plane) const;
    static FrustumPlane MakePlane(const glm::vec3& n, const glm::vec3& p);
    static FrustumPlane MakePlane(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3);
    static glm::vec3 IntersectPlanes(const glm::vec3& n1, float d1, const glm::vec3& n2, float d2, const glm::vec3& n3, float d3);

    FrustumPlane m_planes[6] = {};
    glm::vec3 m_corners[8] = {};
    glm::vec3 m_boundsMin = glm::vec3(0);
    glm::vec3 m_boundsMax = glm::vec3(0);
};

}
