#include "RendererUtil.h"

#include "Hell/Logging.h"
#include "Hell/ResourceManagement/ResourceManager.h"

#include "Unloved/Config/Config.h"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/matrix.hpp>

#include <algorithm>
#include <limits>

namespace Unloved::RendererUtil {

RenderItem CreateAssetGeometryRenderItem(const std::string& modelName, const std::string& meshName, const glm::mat4& worldMatrix, const int32_t materialIndex, const uint64_t objectId) {
    RenderItem renderItem;

    uint32_t meshId = Hell::ResourceManager::GetModelMeshIdByName(modelName, meshName);
    Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("AssetGeometry");

    Mesh* mesh = meshBuffer.GetMeshById(meshId);

    if (!mesh) {
        Logging::Error() << "RendererUtil::CreateAssetGeometryRenderItem(..) mesh name '" << meshName << "' not found in AssetGeomtry mesh buffer\n";
        return renderItem;
    }

    if (materialIndex < 0) {
        Logging::Error() << "RendererUtil::CreateAssetGeometryRenderItem(..) called with invalid materialIndex\n";
        return renderItem;
    }

    // Mesh Id
    renderItem.meshId = meshId;

    // Vertex info
    renderItem.vertexCount = mesh->vertexCount;
    renderItem.indexCount = mesh->indexCount;
    renderItem.baseVertex = mesh->baseVertex;
    renderItem.baseIndex = mesh->baseIndex;

    // Matrices
    renderItem.modelMatrix = worldMatrix;
    renderItem.inverseModelMatrix = glm::inverse(renderItem.modelMatrix);
    renderItem.prevModelMatrix = worldMatrix;

    // Material
    renderItem.materialIndex = materialIndex;

    // Object ID
    Hell::Bit::PackUint64(objectId, renderItem.objectIdLowerBit, renderItem.objectIdUpperBit);

    // Emissive
    int32_t localMeshNodeIndex = 0;
    renderItem.emissiveR = 0.0f;
    renderItem.emissiveG = 0.0f;
    renderItem.emissiveB = 0.0f;

    // Tint
    renderItem.tintColorR = 1.0f;
    renderItem.tintColorG = 1.0f;
    renderItem.tintColorB = 1.0f;

    // Blending mode
    renderItem.blendingMode = static_cast<uint32_t>(BlendingMode::DEFAULT);

    // World AABB
    const glm::vec3 localMin = mesh->aabbMin;
    const glm::vec3 localMax = mesh->aabbMax;
    const glm::vec3 localCenter = 0.5f * (localMin + localMax);
    const glm::vec3 localExtents = 0.5f * (localMax - localMin);
    const glm::vec3 worldCenter = glm::vec3(worldMatrix * glm::vec4(localCenter, 1.0f));
    const glm::vec3 col0 = glm::vec3(worldMatrix[0]);
    const glm::vec3 col1 = glm::vec3(worldMatrix[1]);
    const glm::vec3 col2 = glm::vec3(worldMatrix[2]);
    const glm::vec3 worldExtents = glm::abs(col0) * localExtents.x + glm::abs(col1) * localExtents.y + glm::abs(col2) * localExtents.z;

    renderItem.aabbMin = glm::vec4(worldCenter - worldExtents, 0.0f);
    renderItem.aabbMax = glm::vec4(worldCenter + worldExtents, 0.0f);

    // TODO: Add function params to set these
    renderItem.miscFlags = 0;
    renderItem.shadowFlags = 0;
    renderItem.vulkanFlags = 0;

    // Unused or not applicable, but explicitly set reasonable defaults
    renderItem.baseVertexWeight = 0;
    renderItem.baseSkinningTransformIndex = 0;
    renderItem.woundMaskTextureIndex = -1;
    renderItem.exclusiveViewportIndex = -1;
    renderItem.ignoredViewportIndex = -1;
    renderItem.customId = 0;
    renderItem.openableId = 0;
    renderItem.woundMaterialIndex = -1;
    renderItem.shadowMeshId = 0;

    return renderItem;
}

RenderItem CreateProceduralGeometryRenderItem(uint32_t meshId, int32_t materialIndex, const uint64_t objectId) {
    RenderItem renderItem;

    Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("Procedural");
    Mesh* mesh = meshBuffer.GetMeshById(meshId);

    if (!mesh) {
        Logging::Error() << "RendererUtil::CreateProceduralGeometryRenderItem() mesh id '" << meshId << "' not found\n";
        return renderItem;
    }

    if (materialIndex < 0) {
        Logging::Error() << "RendererUtil::CreateProceduralGeometryRenderItem() called with invalid material index\n";
        return renderItem;
    }

    renderItem.meshId = meshId;
    renderItem.vertexCount = mesh->vertexCount;
    renderItem.indexCount = mesh->indexCount;
    renderItem.baseVertex = mesh->baseVertex;
    renderItem.baseIndex = mesh->baseIndex;
    renderItem.materialIndex = materialIndex;
    renderItem.localMeshNodeIndex = -1;
    renderItem.aabbMin = glm::vec4(mesh->aabbMin, 0.0f);
    renderItem.aabbMax = glm::vec4(mesh->aabbMax, 0.0f);

    Hell::Bit::PackUint64(objectId, renderItem.objectIdLowerBit, renderItem.objectIdUpperBit);

    return renderItem;
}

void UpdateRenderItemAABB(RenderItem& renderItem) {
    Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(renderItem.meshId);
    if (!mesh) return;

    const glm::vec3 localCenter = 0.5f * (mesh->aabbMin + mesh->aabbMax);
    const glm::vec3 localExtents = 0.5f * (mesh->aabbMax - mesh->aabbMin);
    const glm::vec3 worldCenter = glm::vec3(renderItem.modelMatrix * glm::vec4(localCenter, 1.0f));
    const glm::vec3 worldExtents = glm::abs(glm::vec3(renderItem.modelMatrix[0])) * localExtents.x + glm::abs(glm::vec3(renderItem.modelMatrix[1])) * localExtents.y + glm::abs(glm::vec3(renderItem.modelMatrix[2])) * localExtents.z;

    renderItem.aabbMin = glm::vec4(worldCenter - worldExtents, 0.0f);
    renderItem.aabbMax = glm::vec4(worldCenter + worldExtents, 0.0f);
}

AABB ComputeWorldAABB(glm::vec3& localAabbMin, glm::vec3& localAabbMax, glm::mat4& modelMatrix) {
    glm::vec3 worldAabbMin = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 worldAabbMax = glm::vec3(-std::numeric_limits<float>::max());

    std::vector<glm::vec3> corners = {
        modelMatrix * glm::vec4(localAabbMin.x, localAabbMax.y, localAabbMax.z, 1.0f),
        modelMatrix * glm::vec4(localAabbMax.x, localAabbMax.y, localAabbMax.z, 1.0f),
        modelMatrix * glm::vec4(localAabbMin.x, localAabbMin.y, localAabbMax.z, 1.0f),
        modelMatrix * glm::vec4(localAabbMax.x, localAabbMin.y, localAabbMax.z, 1.0f),
        modelMatrix * glm::vec4(localAabbMin.x, localAabbMax.y, localAabbMin.z, 1.0f),
        modelMatrix * glm::vec4(localAabbMax.x, localAabbMax.y, localAabbMin.z, 1.0f),
        modelMatrix * glm::vec4(localAabbMin.x, localAabbMin.y, localAabbMin.z, 1.0f),
        modelMatrix * glm::vec4(localAabbMax.x, localAabbMin.y, localAabbMin.z, 1.0f)
    };

    for (glm::vec3& corner : corners) {
        worldAabbMin = glm::min(worldAabbMin, corner);
        worldAabbMax = glm::max(worldAabbMax, corner);
    }

    return AABB(worldAabbMin, worldAabbMax);
}

std::vector<glm::vec4> GetFrustumCornersWorldSpace(const glm::mat4& projectionMatrix, const glm::mat4& viewMatrix) {
    const auto inv = glm::inverse(projectionMatrix * viewMatrix);

    std::vector<glm::vec4> frustumCorners;
    for (unsigned int x = 0; x < 2; ++x) {
        for (unsigned int y = 0; y < 2; ++y) {
            for (unsigned int z = 0; z < 2; ++z) {
                const glm::vec4 pt = inv * glm::vec4(2.0f * x - 1.0f, 2.0f * y - 1.0f, 2.0f * z - 1.0f, 1.0f);
                frustumCorners.push_back(pt / pt.w);
            }
        }
    }
    return frustumCorners;
}

glm::mat4 GetLightSpaceMatrix(const glm::mat4& viewMatrix, glm::vec3 lightDir, const float viewportWidth, const float viewportHeight, const float fov, const float nearPlane, const float farPlane) {
    const auto proj = glm::perspective(fov, viewportWidth / viewportHeight, nearPlane, farPlane);
    const auto corners = GetFrustumCornersWorldSpace(proj, viewMatrix);

    glm::vec3 center = glm::vec3(0, 0, 0);
    for (const glm::vec3& v : corners) {
        center += glm::vec3(v);
    }
    center /= corners.size();

    const glm::mat4 lightView = glm::lookAt(center + lightDir, center, glm::vec3(0.0f, 1.0f, 0.0f));

    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float minY = std::numeric_limits<float>::max();
    float maxY = std::numeric_limits<float>::lowest();
    float minZ = std::numeric_limits<float>::max();
    float maxZ = std::numeric_limits<float>::lowest();
    for (const glm::vec4& v : corners) {
        const glm::vec4 trf = lightView * v;
        minX = std::min(minX, trf.x);
        maxX = std::max(maxX, trf.x);
        minY = std::min(minY, trf.y);
        maxY = std::max(maxY, trf.y);
        minZ = std::min(minZ, trf.z);
        maxZ = std::max(maxZ, trf.z);
    }

    constexpr float zMult = 10.0f;
    if (minZ < 0) {
        minZ *= zMult;
    }
    else {
        minZ /= zMult;
    }
    if (maxZ < 0) {
        maxZ /= zMult;
    }
    else {
        maxZ *= zMult;
    }

    const glm::mat4 lightProjection = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);
    return lightProjection * lightView;
}

std::vector<glm::mat4> GetLightProjectionViews(const glm::mat4& viewMatrix, glm::vec3 lightDir, std::vector<float>& shadowCascadeLevels, const float viewportWidth, const float viewportHeight, const float fov) {
    std::vector<glm::mat4> ret;

    for (size_t i = 0; i < shadowCascadeLevels.size() + 1; ++i) {
        if (i == 0) {
            ret.push_back(GetLightSpaceMatrix(viewMatrix, lightDir, viewportWidth, viewportHeight, fov, Config::GetNearPlane(), shadowCascadeLevels[i]));
        }
        else if (i < shadowCascadeLevels.size()) {
            ret.push_back(GetLightSpaceMatrix(viewMatrix, lightDir, viewportWidth, viewportHeight, fov, shadowCascadeLevels[i - 1], shadowCascadeLevels[i]));
        }
        else {
            ret.push_back(GetLightSpaceMatrix(viewMatrix, lightDir, viewportWidth, viewportHeight, fov, shadowCascadeLevels[i - 1], Config::GetFarPlane()));
        }
    }
    return ret;
}
}
