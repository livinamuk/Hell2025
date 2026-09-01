#pragma once

#include "Hell/Math/AABB.h"
#include "Hell/Math/OBB.h"
#include "Hell/Math/Transform.h"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <cstdint>
#include <vector>

namespace Unloved {

struct BlockingVolume {
    void Update(const Hell::Transform& transform);
    void SetOwner(uint64_t objectId);
    void DrawDebugCorners(const glm::vec4& color) const;
    void DrawDebugEdges(const glm::vec4& color) const;
    OBBRayResult Raycast(const glm::vec3& rayOrigin, const glm::vec3& rayDir, float maxDistance) const;

    const Hell::Transform& GetTransform() const       { return m_transform; }
    const AABB& GetLocalAABB() const                  { return m_localAABB; }
    const AABB& GetWorldAABB() const                  { return m_worldAABB; }
    const OBB& GetOBB() const                         { return m_obb; }
    const glm::mat4& GetModelMatrix() const           { return m_modelMatrix; }
    const std::vector<glm::vec3>& GetCorners() const  { return m_corners; }
    const uint64_t GetOwnerObjectId() const           { return m_ownerObjectId; }

private:
    Hell::Transform m_transform;
    AABB m_localAABB = AABB(glm::vec3(-0.5f), glm::vec3(0.5f));
    AABB m_worldAABB;
    OBB m_obb;
    glm::mat4 m_modelMatrix = glm::mat4(1.0f);
    std::vector<glm::vec3> m_corners;
    uint64_t m_ownerObjectId = 0;
};

}
