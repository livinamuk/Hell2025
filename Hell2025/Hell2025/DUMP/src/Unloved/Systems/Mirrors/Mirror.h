#pragma once
#include "Unloved/Camera/Frustum.h"

struct Mirror {
    Mirror() = default;
    Mirror(uint64_t id, uint64_t parentId, uint32_t meshNodeIndex, uint32_t globalMeshIndex);
    Mirror(const Mirror&) = delete;
    Mirror& operator=(const Mirror&) = delete;
    Mirror(Mirror&&) noexcept = default;
    Mirror& operator=(Mirror&&) noexcept = default;
    ~Mirror() = default;

    void Update(const glm::mat4& worldMatrix);
    void DebugDraw();

    Unloved::Frustum* GetFrustum(int viewportIndex);

    bool IsFacingViewportCamera(int viewportIndex) const;
    const glm::vec4& GetClipPlane(int viewportIndex) const;
    const glm::vec3& GetReflectVector(int viewportIndex) const;
    const glm::mat4& GetProjecionMatrix(int viewportIndex) const;
    const glm::mat4& GetViewMatrix(int viewportIndex) const;

    uint64_t GetObjectId() const            { return m_objectId; }
    uint64_t GetParentId() const            { return m_parentId; }
    uint64_t GetMeshNodeIndex() const       { return m_meshNodeIndex; }
    uint32_t GetGlobalMeshIndex() const     { return m_globalMeshIndex; }
    const AABB& GetLocalAABB() const        { return m_localAabb; }
    const glm::mat4& GetWorldMatrix() const { return m_worldMatrix; }
    const glm::vec3& GetWorldCenter() const { return m_worldCenter; }
    const glm::vec3& GetWorldNormal() const { return m_worldNormal; }

    uint64_t m_objectId = 0;
    uint64_t m_parentId = 0;
    uint64_t m_meshNodeIndex = 0;
    uint32_t m_globalMeshIndex = 0;
    bool m_facingViewportCamera[4] = {};
    float m_farDistance = 5.0f;
    AABB m_localAabb;
    Unloved::Frustum m_frustums[4] = {};
    glm::mat4 m_projectionMatrices[4] = {};
    glm::mat4 m_viewMatrices[4] = {};
    glm::vec3 m_reflectVectors[4] = {};
    glm::vec4 m_clipPlanes[4] = {};
    glm::mat4 m_worldMatrix = glm::mat4(1.0f);
    glm::vec3 m_localCenter = glm::vec3(0.0f);
    glm::vec3 m_worldCenter = glm::vec3(0.0f);
    glm::vec3 m_localNormal = glm::vec3(0.0f);
    glm::vec3 m_worldNormal = glm::vec3(0.0f);
    std::vector<glm::vec3> m_localCorners;
    std::vector<glm::vec3> m_worldCorners;
};