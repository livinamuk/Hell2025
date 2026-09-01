#include "ClippingVolume.h"

#include "Unloved/Debug/DebugDraw.h"

namespace Unloved {

namespace {
    constexpr int EDGE_INDICES[12][2] = {
        {0, 1}, {0, 2}, {0, 4},
        {1, 3}, {1, 5},
        {2, 3}, {2, 6},
        {3, 7},
        {4, 5}, {4, 6},
        {5, 7},
        {6, 7}
    };

    AABB BuildAABBFromPoints(const std::vector<glm::vec3>& points) {
        AABB aabb;

        for (const glm::vec3& point : points) {
            aabb.Grow(point);
        }

        return aabb;
    }
}

void ClippingVolume::Update(const Hell::Transform& transform) {
    m_transform = transform;
    m_modelMatrix = m_transform.to_mat4();
    m_obb = OBB(m_localAABB, m_modelMatrix);
    m_corners = m_obb.GetCorners();
    m_worldAABB = BuildAABBFromPoints(m_corners);
}

void ClippingVolume::SetOwner(ObjectType objectType, uint64_t objectId) {
    m_ownerObjectType = objectType;
    m_ownerObjectId = objectId;
}

void ClippingVolume::DrawDebugCorners(const glm::vec4& color) const {
    for (const glm::vec3& corner : m_corners) {
        DebugDraw::DrawPoint(corner, color);
    }
}

void ClippingVolume::DrawDebugEdges(const glm::vec4& color) const {
    for (int i = 0; i < 12; i++) {
        DebugDraw::DrawLine(m_corners[EDGE_INDICES[i][0]], m_corners[EDGE_INDICES[i][1]], color);
    }
}

}
