#pragma once

#include "Hell/Math/AABB.h"
#include "Unloved/Render/RendererTypes.h"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <string>
#include <vector>

namespace Unloved::RendererUtil {
    RenderItem CreateAssetGeometryRenderItem(const std::string& modelName, const std::string& meshName, const glm::mat4& worldMatrix, const int32_t materialIndex, const uint64_t objectId);
    RenderItem CreateProceduralGeometryRenderItem(uint32_t meshId, int32_t materialIndex, const uint64_t objectId);

    void UpdateRenderItemAABB(RenderItem& renderItem);
    AABB ComputeWorldAABB(glm::vec3& localAabbMin, glm::vec3& localAabbMax, glm::mat4& modelMatrix);
    glm::mat4 GetLightSpaceMatrix(const glm::mat4& viewMatrix, glm::vec3 lightDir, const float viewportWidth, const float viewportHeight, const float fov, const float nearPlane, const float farPlane);
    std::vector<glm::vec4> GetFrustumCornersWorldSpace(const glm::mat4& projectionMatrix, const glm::mat4& viewMatrix);
    std::vector<glm::mat4> GetLightProjectionViews(const glm::mat4& viewMatrix, glm::vec3 lightDir, std::vector<float>& shadowCascadeLevels, const float viewportWidth, const float viewportHeight, const float fov);
}
