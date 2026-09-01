#include "Wire.h"

#include "Hell/Common/Bit.h"
#include "Hell/Curve/Curve.h"
#include "Hell/Geometry/Geometry.h"
#include "Hell/Render/VertexAttributes.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Common/Constants.h"
#include "Unloved/Debug/DebugDraw.h"
#include "Unloved/Render/RendererConstants.h"
#include "Unloved/Render/RendererTypes.h"

#include <algorithm>
#include <cmath>

namespace Unloved {

Wire::Wire(uint64_t id, const WireCreateInfo& createInfo) {
    m_objectId = id;
    Init(createInfo);
}

void Wire::Init(const WireCreateInfo& createInfo) {
    CleanUp();

    m_createInfo = createInfo;

    if (m_createInfo.sequencePoints.size() < 2) {
        return;
    }

    const float minSpacing = 0.01f;
    float spacing = m_createInfo.spacing;
    if (spacing <= 0.0f) {
        spacing = 0.25f;
    }
    else if (spacing < minSpacing) {
        spacing = minSpacing;
    }

    const float minSpan = 0.0001f;
    m_segmentPoints.clear();

    for (size_t i = 1; i < m_createInfo.sequencePoints.size(); i++) {
        const SequencePoint& begin = m_createInfo.sequencePoints[i - 1];
        const SequencePoint& end = m_createInfo.sequencePoints[i];
        const float span = glm::distance(begin.position, end.position);

        if (span < minSpan) {
            continue;
        }

        const int segmentCount = std::max(1, (int)std::ceil(span / spacing));
        const int numSagPoints = segmentCount + 1;
        std::vector<glm::vec3> sagPoints = Hell::Curve::GenerateSagPoints(begin.position, end.position, numSagPoints, end.customFloat);

        if (sagPoints.size() < 2) {
            sagPoints.clear();
            sagPoints.push_back(begin.position);
            sagPoints.push_back(end.position);
        }

        if (!m_segmentPoints.empty() && !sagPoints.empty()) {
            sagPoints.erase(sagPoints.begin());
        }

        m_segmentPoints.insert(m_segmentPoints.end(), sagPoints.begin(), sagPoints.end());
    }

    if (m_segmentPoints.size() < 2) {
        return;
    }
    
    const int ringPointCount = 12;

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    for (int j = 0; j < m_segmentPoints.size() - 1; j++) {
        glm::vec3& p0 = m_segmentPoints[j];
        glm::vec3& p1 = m_segmentPoints[j + 1];
        glm::vec3 forward = p0 - p1;

        const std::vector<glm::vec3>& circle1 = Hell::Geometry::GenerateOrientedCirclePoints(p0, forward, m_createInfo.radius, ringPointCount);
        const std::vector<glm::vec3>& circle2 = Hell::Geometry::GenerateOrientedCirclePoints(p1, forward, m_createInfo.radius, ringPointCount);

        size_t pointCount = circle1.size();
        glm::vec3 center1(0.0f), center2(0.0f);
        for (const auto& p : circle1) center1 += p;
        for (const auto& p : circle2) center2 += p;
        center1 /= (float)pointCount;
        center2 /= (float)pointCount;

        for (size_t i = 0; i < pointCount; ++i) {
            size_t next = (i + 1) % pointCount;
            // Positions
            glm::vec3 pos1 = circle1[i];
            glm::vec3 pos2 = circle1[next];
            glm::vec3 pos3 = circle2[i];
            glm::vec3 pos4 = circle2[next];
            // Normals
            glm::vec3 normal1 = glm::normalize(pos1 - center1);
            glm::vec3 normal2 = glm::normalize(pos2 - center1);
            glm::vec3 normal3 = glm::normalize(pos3 - center2);
            glm::vec3 normal4 = glm::normalize(pos4 - center2);
            // UVs
            glm::vec2 uv1(i / (float)pointCount, 0.0f);
            glm::vec2 uv2((next) / (float)pointCount, 0.0f);
            glm::vec2 uv3(i / (float)pointCount, 1.0f);
            glm::vec2 uv4((next) / (float)pointCount, 1.0f);
            // Tangents
            glm::vec3 edge1 = pos2 - pos1;
            glm::vec3 edge2 = pos3 - pos1;
            glm::vec2 deltaUV1 = uv2 - uv1;
            glm::vec2 deltaUV2 = uv3 - uv1;
            float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
            glm::vec3 tangent1;
            tangent1.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
            tangent1.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
            tangent1.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
            tangent1 = glm::normalize(tangent1);
            edge1 = pos4 - pos2;
            edge2 = pos3 - pos2;
            deltaUV1 = uv4 - uv2;
            deltaUV2 = uv3 - uv2;
            glm::vec3 tangent2;
            tangent2.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
            tangent2.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
            tangent2.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
            tangent2 = glm::normalize(tangent2);
            // Populate vectors
            uint32_t idx1 = (uint32_t)vertices.size();
            vertices.emplace_back(pos1, normal1, uv1, tangent1);
            vertices.emplace_back(pos2, normal2, uv2, tangent1);
            vertices.emplace_back(pos3, normal3, uv3, tangent2);
            vertices.emplace_back(pos4, normal4, uv4, tangent2);
            indices.push_back(idx1);
            indices.push_back(idx1 + 1);
            indices.push_back(idx1 + 2);
            indices.push_back(idx1 + 1);
            indices.push_back(idx1 + 3);
            indices.push_back(idx1 + 2);
        }
    }

    Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("Procedural");
    m_meshId = meshBuffer.AddMesh(vertices, indices, "Wire");
}

void Wire::CleanUp() {
    if (m_meshId != 0) {
        Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("Procedural");
        meshBuffer.RemoveMesh(m_meshId);
        m_meshId = 0;
    }

    m_segmentPoints.clear();
}

void Wire::SubmitRenderItem() {
    Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("Procedural");

    Mesh* mesh = meshBuffer.GetMeshById(m_meshId);
    if (!mesh) return;

    const int32_t materialIndex = Hell::ResourceManager::GetMaterialIndexByName("Black");
    Material* material = Hell::ResourceManager::GetMaterialByIndex(materialIndex);
    if (!material) return;

    RenderItem renderItem;
    renderItem.materialIndex = materialIndex;
    renderItem.modelMatrix = glm::mat4(1.0f);
    renderItem.inverseModelMatrix = glm::mat4(1.0f);
    renderItem.aabbMin = glm::vec4(mesh->aabbMin, 0.0f);
    renderItem.aabbMax = glm::vec4(mesh->aabbMax, 0.0f);
    renderItem.meshId = m_meshId;
    renderItem.vertexCount = mesh->vertexCount;
    renderItem.indexCount = mesh->indexCount;
    renderItem.baseVertex = mesh->baseVertex;
    renderItem.baseIndex = mesh->baseIndex;
    renderItem.shadowFlags = SHADOW_FLAG_NONE;

    const uint64_t renderObjectId = m_createInfo.parentObjectId != 0 ? m_createInfo.parentObjectId : m_objectId;
    Hell::Bit::PackUint64(renderObjectId, renderItem.objectIdLowerBit, renderItem.objectIdUpperBit);

    RenderDataManager::SubmitRenderItemProcedural(renderItem);
}

void Wire::Update() {
    for (glm::vec3& point : m_segmentPoints) {
        DebugDraw::DrawPoint(point, OUTLINE_COLOR);
    }
}
}
