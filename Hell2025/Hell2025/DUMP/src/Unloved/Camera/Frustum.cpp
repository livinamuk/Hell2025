#include "Frustum.h"

#include <array>
#include <cfloat>
#include <cmath>

namespace Unloved {

void Frustum::Update(const glm::mat4& projectionView) {
    // Left clipping plane
    m_planes[0].normal.x = projectionView[0][3] + projectionView[0][0];
    m_planes[0].normal.y = projectionView[1][3] + projectionView[1][0];
    m_planes[0].normal.z = projectionView[2][3] + projectionView[2][0];
    m_planes[0].offset = projectionView[3][3] + projectionView[3][0];

    // Right clipping plane
    m_planes[1].normal.x = projectionView[0][3] - projectionView[0][0];
    m_planes[1].normal.y = projectionView[1][3] - projectionView[1][0];
    m_planes[1].normal.z = projectionView[2][3] - projectionView[2][0];
    m_planes[1].offset = projectionView[3][3] - projectionView[3][0];

    // Top clipping plane
    m_planes[2].normal.x = projectionView[0][3] - projectionView[0][1];
    m_planes[2].normal.y = projectionView[1][3] - projectionView[1][1];
    m_planes[2].normal.z = projectionView[2][3] - projectionView[2][1];
    m_planes[2].offset = projectionView[3][3] - projectionView[3][1];

    // Bottom clipping plane
    m_planes[3].normal.x = projectionView[0][3] + projectionView[0][1];
    m_planes[3].normal.y = projectionView[1][3] + projectionView[1][1];
    m_planes[3].normal.z = projectionView[2][3] + projectionView[2][1];
    m_planes[3].offset = projectionView[3][3] + projectionView[3][1];

    // Near clipping plane
    m_planes[4].normal.x = projectionView[0][3] + projectionView[0][2];
    m_planes[4].normal.y = projectionView[1][3] + projectionView[1][2];
    m_planes[4].normal.z = projectionView[2][3] + projectionView[2][2];
    m_planes[4].offset = projectionView[3][3] + projectionView[3][2];

    // Far clipping plane
    m_planes[5].normal.x = projectionView[0][3] - projectionView[0][2];
    m_planes[5].normal.y = projectionView[1][3] - projectionView[1][2];
    m_planes[5].normal.z = projectionView[2][3] - projectionView[2][2];
    m_planes[5].offset = projectionView[3][3] - projectionView[3][2];

    // Normalize planes
    for (int i = 0; i < 6; i++) {
        float magnitude = glm::length(m_planes[i].normal);
        m_planes[i].normal /= magnitude;
        m_planes[i].offset /= magnitude;
    }

    m_corners[0] = IntersectPlanes(m_planes[0].normal, m_planes[0].offset, m_planes[2].normal, m_planes[2].offset, m_planes[4].normal, m_planes[4].offset); // Near bottom-left
    m_corners[1] = IntersectPlanes(m_planes[1].normal, m_planes[1].offset, m_planes[2].normal, m_planes[2].offset, m_planes[4].normal, m_planes[4].offset); // Near bottom-right
    m_corners[2] = IntersectPlanes(m_planes[0].normal, m_planes[0].offset, m_planes[3].normal, m_planes[3].offset, m_planes[4].normal, m_planes[4].offset); // Near top-left
    m_corners[3] = IntersectPlanes(m_planes[1].normal, m_planes[1].offset, m_planes[3].normal, m_planes[3].offset, m_planes[4].normal, m_planes[4].offset); // Near top-right  
    m_corners[4] = IntersectPlanes(m_planes[0].normal, m_planes[0].offset, m_planes[2].normal, m_planes[2].offset, m_planes[5].normal, m_planes[5].offset); // Far bottom-left
    m_corners[5] = IntersectPlanes(m_planes[1].normal, m_planes[1].offset, m_planes[2].normal, m_planes[2].offset, m_planes[5].normal, m_planes[5].offset); // Far bottom-right
    m_corners[6] = IntersectPlanes(m_planes[0].normal, m_planes[0].offset, m_planes[3].normal, m_planes[3].offset, m_planes[5].normal, m_planes[5].offset); // Far top-left
    m_corners[7] = IntersectPlanes(m_planes[1].normal, m_planes[1].offset, m_planes[3].normal, m_planes[3].offset, m_planes[5].normal, m_planes[5].offset); // Far top-right

    m_boundsMin = glm::vec3(FLT_MAX);
    m_boundsMax = glm::vec3(-FLT_MAX);

    for (int i = 0; i < 8; i++) {
        m_boundsMin = glm::min(m_boundsMin, m_corners[i]);
        m_boundsMax = glm::max(m_boundsMax, m_corners[i]);
    }
}

bool Frustum::IntersectsAABB(const AABB& aabb) {
    glm::vec3 aabbCorners[8] = {
        glm::vec3(aabb.GetBoundsMin().x, aabb.GetBoundsMin().y, aabb.GetBoundsMin().z), // Near-bottom-left
        glm::vec3(aabb.GetBoundsMax().x, aabb.GetBoundsMin().y, aabb.GetBoundsMin().z), // Near-bottom-right
        glm::vec3(aabb.GetBoundsMin().x, aabb.GetBoundsMax().y, aabb.GetBoundsMin().z), // Near-top-left
        glm::vec3(aabb.GetBoundsMax().x, aabb.GetBoundsMax().y, aabb.GetBoundsMin().z), // Near-top-right
        glm::vec3(aabb.GetBoundsMin().x, aabb.GetBoundsMin().y, aabb.GetBoundsMax().z), // Far-bottom-left
        glm::vec3(aabb.GetBoundsMax().x, aabb.GetBoundsMin().y, aabb.GetBoundsMax().z), // Far-bottom-right
        glm::vec3(aabb.GetBoundsMin().x, aabb.GetBoundsMax().y, aabb.GetBoundsMax().z), // Far-top-left
        glm::vec3(aabb.GetBoundsMax().x, aabb.GetBoundsMax().y, aabb.GetBoundsMax().z)  // Far-top-right
    };
    for (int i = 0; i < 6; ++i) {
        int pointsOutside = 0;
        for (int j = 0; j < 8; ++j) {
            if (SignedDistance(aabbCorners[j], m_planes[i]) < 0.0f) {
                pointsOutside++;
            }
        }
        if (pointsOutside == 8) {
            return false;
        }
    }
    return true;
}

bool Frustum::IntersectsAABBFast(const AABB& aabb) {
    for (int i = 0; i < 6; ++i) {
        glm::vec3 min_corner = glm::vec3(
            m_planes[i].normal.x > 0 ? aabb.GetBoundsMax().x : aabb.GetBoundsMin().x,
            m_planes[i].normal.y > 0 ? aabb.GetBoundsMax().y : aabb.GetBoundsMin().y,
            m_planes[i].normal.z > 0 ? aabb.GetBoundsMax().z : aabb.GetBoundsMin().z
        );
        if (SignedDistance(min_corner, m_planes[i]) <= 0.0f) {
            return false; 
        }
    }
    return true;
}

bool Frustum::IntersectsAABBFast(const RenderItem& renderItem) {
    for (int i = 0; i < 6; ++i) {
        glm::vec3 min_corner = glm::vec3(
            m_planes[i].normal.x > 0 ? renderItem.aabbMax.x : renderItem.aabbMin.x,
            m_planes[i].normal.y > 0 ? renderItem.aabbMax.y : renderItem.aabbMin.y,
            m_planes[i].normal.z > 0 ? renderItem.aabbMax.z : renderItem.aabbMin.z
        );
        if (SignedDistance(min_corner, m_planes[i]) <= 0.0f) {
            return false;
        }
    }
    return true;
}

//bool Frustum::IntersectsSphere(const Sphere& sphere) {
//    for (int i = 0; i < 6; ++i) {
//        float distance = SignedDistance(sphere.origin, m_planes[i]);
//        if (distance + sphere.radius < 0) {
//            return false;
//        }
//    }
//    return true;
//}

bool Frustum::IntersectsPoint(const glm::vec3 point) {
    bool insideFrustum = true;
    for (int i = 0; i < 6; i++) {
        float distance = SignedDistance(point, m_planes[i]);
        if (distance < 0) {
            return false;
        }
    }
    return true;
}

FrustumPlane Frustum::MakePlane(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2) {
    FrustumPlane plane;
    plane.normal = glm::normalize(glm::cross(p1 - p0, p2 - p0));
    plane.offset = -glm::dot(plane.normal, p0);
    return plane;
}

FrustumPlane Frustum::MakePlane(const glm::vec3& n, const glm::vec3& p) {
    FrustumPlane pl;
    pl.normal = glm::normalize(n);
    pl.offset = -glm::dot(pl.normal, p);
    return pl;
}

float Frustum::SignedDistance(const glm::vec3& point, const FrustumPlane& plane) const {
    return glm::dot(plane.normal, point) + plane.offset;
}

glm::vec3 Frustum::IntersectPlanes(const glm::vec3& n1, float d1, const glm::vec3& n2, float d2, const glm::vec3& n3, float d3) {
    glm::vec3 crossN2N3 = glm::cross(n2, n3);
    float denom = glm::dot(n1, crossN2N3);
    if (std::fabs(denom) < 1e-6f) {
        return glm::vec3(0.0f);
    }
    glm::vec3 result = -(d1 * crossN2N3 + d2 * glm::cross(n3, n1) + d3 * glm::cross(n1, n2)) / denom;
    return result;
}

glm::vec4 Frustum::GetPlane(int index) { 
    return glm::vec4(m_planes[index].normal, m_planes[index].offset);
}

}
