#include "Wall.h"
#include "Hell/Common/Bit.h"
#include "Hell/Math/Math.h"
#include "Unloved/Debug/DebugDraw.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Unloved/Systems/House/HouseClipping.h"
#include "Unloved/Systems/House/HouseBuilder.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Render/RendererUtil.h"
#include "Legacy/World/LegacyWorld.h"

#include "Hell/Logging.h"
#include "Unloved/Render/RendererConstants.h"

#include "Unloved/Common/CreateInfo.h"

#include <array>
#include <cmath>

namespace Unloved {

Wall::Wall(uint64_t id, const WallCreateInfo& createInfo, const SpawnOffset& spawnOffset) {
    m_objectId = id;
    m_createInfo = createInfo;
    m_spawnOffset = spawnOffset;
    m_createInfo.weatherBoardTextureBoardCount = glm::max(m_createInfo.weatherBoardTextureBoardCount, 1u);

    for (SequencePoint& sequencePoint : m_createInfo.sequencePoints) {
        sequencePoint.position += spawnOffset.translation;
    }

    UpdateSegmentsTrimsAndVertexData();
}

void Wall::UpdateSegmentsTrimsAndVertexData() {
    CleanUp();
    m_wallSegments.clear();

    m_materialIndex = Hell::ResourceManager::GetMaterialIndexByName(m_createInfo.materialName);
    m_ceilingTrimType = m_createInfo.ceilingTrimType;
    m_floorTrimType = m_createInfo.floorTrimType;

    for (size_t i = 0; i + 1 < GetPointCount(); i++) {
        const SequencePoint& pointA = m_createInfo.sequencePoints[i];
        const SequencePoint& pointB = m_createInfo.sequencePoints[i + 1];
        const SequencePoint& start = m_createInfo.useReversePointOrder ? pointB : pointA;
        const SequencePoint& end = m_createInfo.useReversePointOrder ? pointA : pointB;
        WallSegment& wallSegment = m_wallSegments.emplace_back();
        wallSegment.Init(start.position, end.position, start.customFloat, end.customFloat, m_objectId, m_spawnOffset);
    }

    // Calculate worldspace center
    m_worldSpaceCenter = glm::vec3(0.0f);
    if (!m_createInfo.sequencePoints.empty()) {
        for (const SequencePoint& sequencePoint : m_createInfo.sequencePoints) {
            m_worldSpaceCenter += sequencePoint.position;
        }
        m_worldSpaceCenter /= m_createInfo.sequencePoints.size();
    }

    // Create weather boards
    if (m_createInfo.wallType == WallType::WEATHER_BOARDS) {
        CreateCSGVertexData();
        RecreateWeatherBoardMesh();
    }
    // Create CSG geometry and trims
    else {
        CreateCSGVertexData();
        CreateTrims();
    }
}

void Wall::FlipFaces() {
    m_createInfo.useReversePointOrder = !m_createInfo.useReversePointOrder;
    UpdateSegmentsTrimsAndVertexData();
    HouseBuilder::MarkDirty();
}

void Wall::SetPosition(const glm::vec3& position) {
    UpdateWorldSpaceCenter(position);
}

void Wall::UpdateWorldSpaceCenter(glm::vec3 worldSpaceCenter) {
    glm::vec3 offset = worldSpaceCenter - m_worldSpaceCenter;
    for (SequencePoint& sequencePoint : m_createInfo.sequencePoints) {
        sequencePoint.position += offset;
    }
    UpdateSegmentsTrimsAndVertexData();
    HouseBuilder::MarkDirty();
}

bool Wall::AddPointToEnd(glm::vec3 point, bool supressWarning) {
    if (m_createInfo.sequencePoints.empty()) {
        SequencePoint& sequencePoint = m_createInfo.sequencePoints.emplace_back();
        sequencePoint.position = point;
        sequencePoint.customFloat = 2.4f;
        UpdateSegmentsTrimsAndVertexData();
        HouseBuilder::MarkDirty();
        return true;
    }

    SequencePoint& previousSequencePoint = m_createInfo.sequencePoints.back();
    const glm::vec3& previousPoint = previousSequencePoint.position;
    float threshold = 0.05f;
    if (glm::distance(point, previousPoint) < threshold) {
        Logging::Debug() << "Wall::AddPoint() failed: new point " << point << " is too close to previous point " << previousPoint << "\n";
        return false;
    }

    SequencePoint& sequencePoint = m_createInfo.sequencePoints.emplace_back();
    sequencePoint.position = point;
    sequencePoint.customFloat = previousSequencePoint.customFloat;
    UpdateSegmentsTrimsAndVertexData();
    HouseBuilder::MarkDirty();
    return true;
}

bool Wall::UpdatePointPosition(int pointIndex, glm::vec3 position, bool supressWarning) {
    if (pointIndex < 0 || pointIndex >= m_createInfo.sequencePoints.size()) {
        Logging::Debug() << "Wall::UpdatePointPosition() failed: point index " << pointIndex << " out of range of size " << m_createInfo.sequencePoints.size() << "\n";
        return false;
    }

    // Threshold check
    float threshold = 0.05f;
    const bool closed = m_createInfo.sequencePoints.size() > 2 && glm::distance(m_createInfo.sequencePoints.front().position, m_createInfo.sequencePoints.back().position) < threshold;
    if (pointIndex > 0) {
        const glm::vec3& previousPoint = m_createInfo.sequencePoints[pointIndex - 1].position;
        if (glm::distance(position, previousPoint) < threshold) {
            Logging::Debug() << "Wall::UpdatePointPosition() failed: new point " << position << " is too close to previous point " << previousPoint << "\n";
            return false;
        }
    }
    if (pointIndex < m_createInfo.sequencePoints.size() - 1) {
        const glm::vec3& nextPoint = m_createInfo.sequencePoints[pointIndex + 1].position;
        if (glm::distance(position, nextPoint) < threshold) {
            Logging::Debug() << "Wall::UpdatePointPosition() failed: new point " << position << " is too close to next point " << nextPoint << "\n";
            return false;
        }
    }
    if (closed && pointIndex == 0 && glm::distance(position, m_createInfo.sequencePoints[m_createInfo.sequencePoints.size() - 2].position) < threshold) return false;
    if (closed && pointIndex == m_createInfo.sequencePoints.size() - 1 && glm::distance(position, m_createInfo.sequencePoints[1].position) < threshold) return false;

    m_createInfo.sequencePoints[pointIndex].position = position;
    if (closed && pointIndex == 0) m_createInfo.sequencePoints.back().position = position;
    if (closed && pointIndex == m_createInfo.sequencePoints.size() - 1) m_createInfo.sequencePoints.front().position = position;
    UpdateSegmentsTrimsAndVertexData();
    HouseBuilder::MarkDirty();
    return true;
}

void Wall::UpdateSequencePoints(const std::vector<SequencePoint>& sequencePoints) {
    m_createInfo.sequencePoints = sequencePoints;
    UpdateSegmentsTrimsAndVertexData();
    HouseBuilder::MarkDirty();
}

void Wall::SetPointHeight(int pointIndex, float height) {
    if (pointIndex < 0 || pointIndex >= m_createInfo.sequencePoints.size()) return;
    const bool closed = m_createInfo.sequencePoints.size() > 2 && glm::distance(m_createInfo.sequencePoints.front().position, m_createInfo.sequencePoints.back().position) < 0.05f;
    m_createInfo.sequencePoints[pointIndex].customFloat = height;
    if (closed && pointIndex == 0) m_createInfo.sequencePoints.back().customFloat = height;
    if (closed && pointIndex == m_createInfo.sequencePoints.size() - 1) m_createInfo.sequencePoints.front().customFloat = height;
    UpdateSegmentsTrimsAndVertexData();
    HouseBuilder::MarkDirty();
}

void Wall::SetPointCustomBool(int pointIndex, bool value) {
    if (pointIndex < 0 || pointIndex >= m_createInfo.sequencePoints.size()) return;
    m_createInfo.sequencePoints[pointIndex].customBool = value;
    RecreateWeatherBoardMesh();
    HouseBuilder::MarkDirty();
}

void Wall::SetMaterial(const std::string& materialName) {
    const int32_t materialIndex = Hell::ResourceManager::GetMaterialIndexByName(materialName);
    if (materialIndex != -1) {
        m_createInfo.materialName = materialName;
        m_materialIndex = materialIndex;
        UpdateSegmentsTrimsAndVertexData();
        HouseBuilder::MarkDirty();
    }
}

void Wall::SetWeatherBoardMaterial(const std::string& materialName, uint32_t boardCount, uint32_t startIndex, uint32_t endIndex, float textureOffsetU, float textureOffsetV) {
    const int32_t materialIndex = Hell::ResourceManager::GetMaterialIndexByName(materialName);
    if (materialIndex == -1) return;

    m_createInfo.materialName = materialName;
    m_createInfo.weatherBoardTextureBoardCount = glm::max(boardCount, 1u);
    m_createInfo.weatherBoardStartIndex = startIndex;
    m_createInfo.weatherBoardEndIndex = endIndex;
    m_createInfo.textureOffsetU = textureOffsetU;
    m_createInfo.textureOffsetV = textureOffsetV;
    m_materialIndex = materialIndex;
    UpdateSegmentsTrimsAndVertexData();
    HouseBuilder::MarkDirty();
}

void Wall::SetWeatherBoardStopMaterial(const std::string& materialName) {
    if (Hell::ResourceManager::GetMaterialIndexByName(materialName) == -1) return;
    m_createInfo.weatherBoardStopMaterialName = materialName;
    RecreateWeatherBoardMesh();
    HouseBuilder::MarkDirty();
}

void Wall::SetWallType(WallType wallType) {
    m_createInfo.wallType = wallType;
    UpdateSegmentsTrimsAndVertexData();
    HouseBuilder::MarkDirty();
}

void Wall::SetWeatherBoardTextureBoardCount(uint32_t value) {
    m_createInfo.weatherBoardTextureBoardCount = glm::max(value, 1u);
    UpdateSegmentsTrimsAndVertexData();
    HouseBuilder::MarkDirty();
}

void Wall::SetWeatherBoardStartIndex(uint32_t value) {
    m_createInfo.weatherBoardStartIndex = value;
    UpdateSegmentsTrimsAndVertexData();
    HouseBuilder::MarkDirty();
}

void Wall::SetWeatherBoardEndIndex(uint32_t value) {
    m_createInfo.weatherBoardEndIndex = value;
    UpdateSegmentsTrimsAndVertexData();
    HouseBuilder::MarkDirty();
}

Material* Wall::GetMaterial() {
    return Hell::ResourceManager::GetMaterialByIndex(m_materialIndex);
}

void Wall::SetTextureScale(float value) {
    m_createInfo.textureScale = value;
    UpdateSegmentsTrimsAndVertexData();
    HouseBuilder::MarkDirty();
}

void Wall::SetTextureOffsetU(float value) {
    m_createInfo.textureOffsetU = value;
    UpdateSegmentsTrimsAndVertexData();
    HouseBuilder::MarkDirty();
}

void Wall::SetTextureOffsetV(float value) {
    m_createInfo.textureOffsetV = value;
    UpdateSegmentsTrimsAndVertexData();
    HouseBuilder::MarkDirty();
}

void Wall::SetRoughnessFactor(float value) {
    m_createInfo.roughnessFactor = glm::clamp(value, 0.0f, 10.0f);
    for (RenderItem& renderItem : m_weatherBoardstopRenderItems) renderItem.roughnessFactor = m_createInfo.roughnessFactor;
    HouseBuilder::MarkDirty();
}

void Wall::SetMetallicFactor(float value) {
    m_createInfo.metallicFactor = glm::clamp(value, 0.0f, 10.0f);
    for (RenderItem& renderItem : m_weatherBoardstopRenderItems) renderItem.metallicFactor = m_createInfo.metallicFactor;
    HouseBuilder::MarkDirty();
}

void Wall::SetFloorTrimType(TrimType trimType) {
    m_createInfo.floorTrimType = trimType;
    UpdateSegmentsTrimsAndVertexData();
    HouseBuilder::MarkDirty();
}
void Wall::SetCeilingTrimType(TrimType trimType) {
    m_createInfo.ceilingTrimType = trimType;
    UpdateSegmentsTrimsAndVertexData();
    HouseBuilder::MarkDirty();
}

const glm::vec3& Wall::GetPointByIndex(int pointIndex) {
    static glm::vec3 invalid = glm::vec3(0.0f);

    if (pointIndex < 0 || pointIndex >= m_createInfo.sequencePoints.size()) {
        Logging::Error() << "Wall::GetPointByIndex() failed: point index " << pointIndex << " out of range of size " << m_createInfo.sequencePoints.size() << "\n";
        return invalid;
    }
    return m_createInfo.sequencePoints[pointIndex].position;
}

float Wall::GetPointHeightByIndex(int pointIndex) const {
    if (pointIndex < 0 || pointIndex >= m_createInfo.sequencePoints.size()) return 0.0f;
    return m_createInfo.sequencePoints[pointIndex].customFloat;
}

void Wall::CleanUp() {
    CleanUpWeatherBoardMesh();

    for (WallSegment& wallSegment : m_wallSegments) {
        wallSegment.CleanUp();
    }
}

void Wall::CreateTrims() {
    m_trims.clear();
	return;

    // Ceiling
    if (m_ceilingTrimType != TrimType::NONE) {
        for (int i = 0; i < (int)m_createInfo.sequencePoints.size() - 1; i++) {
            const SequencePoint& start = m_createInfo.sequencePoints[i];
            const SequencePoint& end = m_createInfo.sequencePoints[i + 1];

            Hell::Transform t;
            t.position = start.position;
            t.position.y += start.customFloat;
            t.rotation.y = Hell::Math::YawBetweenPoints(start.position, end.position);
            t.scale.x = glm::distance(start.position, end.position);

            Trim& trim = m_trims.emplace_back();
            trim.Init(t, "TrimCeiling", "Trims");
        }
    }

    // Floor
    if (m_floorTrimType != TrimType::NONE) {
        for (int i = 0; i < (int)m_createInfo.sequencePoints.size() - 1; i++) {
            const glm::vec3& start = m_createInfo.sequencePoints[i].position;
            const glm::vec3& end = m_createInfo.sequencePoints[i + 1].position;

            glm::vec3 rayOrigin = start;
            glm::vec3 rayDir = glm::normalize(end - start);
            const float segmentLength = glm::distance(start, end);
            float remaining = segmentLength;
            const float eps = 1e-3f;

            while (remaining > eps) {
                ClipRayResult rayResult = HouseClipping::RaycastClippingVolumes(rayOrigin, rayDir, remaining);
                if (!rayResult.hitFound) break;

                // Only add a trim up to a NEAR face (entering the cube)
                if (glm::dot(rayResult.hitNormal, rayDir) < 0.0f) {
                    Hell::Transform t;
                    t.position = rayOrigin;
                    t.rotation.y = Hell::Math::YawBetweenPoints(start, end);
                    t.scale.x = rayResult.distanceToHit;
                    if (t.scale.x > eps) {
                        Trim& trim = m_trims.emplace_back();
                        trim.Init(t, "TrimFloor", "Trims");
                    }
                }

                float advance = rayResult.distanceToHit + eps; // step through face
                rayOrigin += rayDir * advance;
                remaining -= advance;
            }

            if (remaining > eps) {
                Hell::Transform t;
                t.position = rayOrigin;
                t.rotation.y = Hell::Math::YawBetweenPoints(rayOrigin, end);
                t.scale.x = remaining;
                Trim& trim = m_trims.emplace_back();
                trim.Init(t, "TrimFloor", "Trims");
            }
        }
    }
}

void Wall::CreateCSGVertexData() {
    const std::vector<const ClippingVolume*> clippingVolumes = HouseClipping::GetClippingVolumes();

    for (WallSegment& wallSegment : m_wallSegments) {
        wallSegment.CreateVertexData(clippingVolumes, m_createInfo.textureOffsetU, m_createInfo.textureOffsetV, m_createInfo.textureScale);
    }
}

void Wall::SubmitRenderItems() {
    Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("Procedural");

    // If this wall is exterior, then don't render the CSG geometry, or any trims if you accidentally set it to have trims
    if (m_createInfo.wallType == WallType::WEATHER_BOARDS) {


        for (size_t i = 0; i < m_weatherBoardSegmentMeshIds.size(); i++) {
            const uint32_t meshId = m_weatherBoardSegmentMeshIds[i];
            Mesh* mesh = meshBuffer.GetMeshById(meshId);
            if (!mesh) continue;

            Material* material = Hell::ResourceManager::GetMaterialByIndex(m_materialIndex);
            if (!material) continue;

            RenderItem renderItem;
            renderItem.materialIndex = m_materialIndex;
            renderItem.roughnessFactor = m_createInfo.roughnessFactor;
            renderItem.metallicFactor = m_createInfo.metallicFactor;
            renderItem.modelMatrix = glm::mat4(1.0f);
            renderItem.inverseModelMatrix = glm::mat4(1.0f);
            renderItem.prevModelMatrix = glm::mat4(1.0f);
            renderItem.aabbMin = glm::vec4(mesh->aabbMin, 0.0f);
            renderItem.aabbMax = glm::vec4(mesh->aabbMax, 0.0f);
            renderItem.meshId = meshId;
            renderItem.vertexCount = mesh->vertexCount;
            renderItem.indexCount = mesh->indexCount;
            renderItem.baseVertex = mesh->baseVertex;
            renderItem.baseIndex = mesh->baseIndex;
            renderItem.shadowFlags |= (SHADOW_FLAG_POINT_LIGHT | SHADOW_FLAG_CSM);
            if (i < m_wallSegments.size()) Hell::Bit::PackUint64(m_wallSegments[i].GetObjectId(), renderItem.objectIdLowerBit, renderItem.objectIdUpperBit);

            RenderDataManager::SubmitRenderItemProcedural(renderItem);
        }

        return;
    }

    for (WallSegment& wallSegment : m_wallSegments) {
        Mesh* mesh = meshBuffer.GetMeshById(wallSegment.GetMeshId());
        if (!mesh) return;

        Material* material = Hell::ResourceManager::GetMaterialByIndex(m_materialIndex);
        if (!material) return;

	    RenderItem renderItem;
        renderItem.materialIndex = m_materialIndex;
        renderItem.roughnessFactor = m_createInfo.roughnessFactor;
        renderItem.metallicFactor = m_createInfo.metallicFactor;
		renderItem.modelMatrix = glm::mat4(1.0f);
        renderItem.inverseModelMatrix = glm::mat4(1.0f);
        renderItem.prevModelMatrix = glm::mat4(1.0f);
		renderItem.aabbMin = glm::vec4(mesh->aabbMin, 0.0f);
		renderItem.aabbMax = glm::vec4(mesh->aabbMax, 0.0f);
        renderItem.meshId = wallSegment.GetMeshId();
        renderItem.vertexCount = mesh->vertexCount;
        renderItem.indexCount = mesh->indexCount;
        renderItem.baseVertex = mesh->baseVertex;
        renderItem.baseIndex = mesh->baseIndex;
        renderItem.shadowFlags |= (SHADOW_FLAG_POINT_LIGHT | SHADOW_FLAG_CSM);
        Hell::Bit::PackUint64(wallSegment.GetObjectId(), renderItem.objectIdLowerBit, renderItem.objectIdUpperBit);

		RenderDataManager::SubmitRenderItemProcedural(renderItem);
    }

    for (Trim& trim : m_trims) {
        trim.SubmitRenderItem();
    }
}

void Wall::DrawSegmentVertices(glm::vec4 color) {
    for (WallSegment& wallSegment : m_wallSegments) {
        const glm::vec3& p1 = wallSegment.GetStart();
        const glm::vec3& p2 = wallSegment.GetEnd();
        glm::vec3 p3 = wallSegment.GetStart() + glm::vec3(0.0f, wallSegment.GetStartHeight(), 0.0f);
        glm::vec3 p4 = wallSegment.GetEnd() + glm::vec3(0.0f, wallSegment.GetEndHeight(), 0.0f);
        DebugDraw::DrawPoint(p1, color);
        DebugDraw::DrawPoint(p2, color);
        DebugDraw::DrawPoint(p3, color);
        DebugDraw::DrawPoint(p4, color);
    }
}

void Wall::DrawSegmentLines(glm::vec4 color) {
    for (WallSegment& wallSegment : m_wallSegments) {
        const glm::vec3& p1 = wallSegment.GetStart();
        const glm::vec3& p2 = wallSegment.GetEnd();
        glm::vec3 p3 = wallSegment.GetStart() + glm::vec3(0.0f, wallSegment.GetStartHeight(), 0.0f);
        glm::vec3 p4 = wallSegment.GetEnd() + glm::vec3(0.0f, wallSegment.GetEndHeight(), 0.0f);
        DebugDraw::DrawLine(p1, p2, color);
        DebugDraw::DrawLine(p3, p4, color);
        DebugDraw::DrawLine(p1, p3, color);
        DebugDraw::DrawLine(p2, p4, color);

        glm::vec3 midPoint = Hell::Math::MidPoint(wallSegment.GetStart(), wallSegment.GetEnd());
        glm::vec3 normal = wallSegment.GetNormal();
        glm::vec3 projectedMidPoint = midPoint + (normal * 0.2f);
        DebugDraw::DrawLine(midPoint, projectedMidPoint, color);
    }
}

Vertex InterpolateVertex(const Vertex& a, const Vertex& b, float t) {
    Vertex vertex;
    vertex.position = glm::mix(a.position, b.position, t);
    vertex.normal = glm::normalize(glm::mix(a.normal, b.normal, t));
    vertex.uv = glm::mix(a.uv, b.uv, t);
    vertex.tangent = glm::normalize(glm::mix(a.tangent, b.tangent, t));
    return vertex;
}

void AddBoard(const glm::vec3& origin, const glm::vec3& boardDir, uint32_t textureRow, uint32_t textureBoardCount, float textureOffsetU, float textureOffsetV, float boardWidth, const WallSegment& wallSegment, std::vector<Vertex>& verticesOut, std::vector<uint32_t>& indicesOut) {
    const std::vector<Vertex>& sourceVertices = HouseBuilder::GetWeatherBoardVertices();
    const std::vector<uint32_t>& sourceIndices = HouseBuilder::GetWeatherBoardIndices();
    if (sourceVertices.empty() || sourceIndices.empty()) return;

    std::vector<Vertex> boardVertices;
    boardVertices.reserve(sourceVertices.size());

    // Calculate rotation matrix
    glm::vec3 zAxis = glm::normalize(boardDir);
    glm::vec3 xAxis = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), zAxis));
    glm::vec3 yAxis = glm::cross(zAxis, xAxis);
    glm::mat3 rotationMatrix = glm::mat3(xAxis, yAxis, zAxis);

    for (Vertex vertex : sourceVertices) {

        // If this vertex is on the right of the board then shift it to the desired width and update uvs
        bool isRightEdge = vertex.uv.x > 0.5f;
        if (isRightEdge) {
            vertex.position.z *= boardWidth;
            vertex.uv.x = boardWidth * 0.25f; // 0.25 because the texture is 4 meter wide in world space
        }

        vertex.position = rotationMatrix * vertex.position;
        vertex.position += origin;
        
        vertex.normal = glm::normalize(rotationMatrix * vertex.normal);

        // Remap the model UV into the selected texture row
        vertex.uv.y = 1.0f - (textureRow + vertex.uv.y) / textureBoardCount;
        vertex.uv.x += textureOffsetU;
        vertex.uv.y += textureOffsetV;

        boardVertices.push_back(vertex);
    }

    glm::vec3 wallStart = wallSegment.GetStart();
    glm::vec3 wallDirection = wallSegment.GetEnd() - wallStart;
    wallDirection.y = 0.0f;
    const float wallLength = glm::length(wallDirection);
    if (wallLength <= 0.0f) return;
    wallDirection /= wallLength;

    const float wallStartTop = wallSegment.GetStart().y + wallSegment.GetStartHeight();
    const float wallEndTop = wallSegment.GetEnd().y + wallSegment.GetEndHeight();
    auto GetClipDistance = [&](const Vertex& vertex) { return glm::mix(wallStartTop, wallEndTop, glm::dot(vertex.position - wallStart, wallDirection) / wallLength) - vertex.position.y; };

    // Cut the source triangles against the sloped wall top
    for (size_t i = 0; i + 2 < sourceIndices.size(); i += 3) {
        const std::array<Vertex, 3> triangle = { boardVertices[sourceIndices[i]], boardVertices[sourceIndices[i + 1]], boardVertices[sourceIndices[i + 2]] };
        std::array<Vertex, 4> clipped;
        int clippedCount = 0;
        Vertex previous = triangle.back();
        float previousDistance = GetClipDistance(previous);
        bool previousInside = previousDistance >= 0.0f;

        for (const Vertex& current : triangle) {
            const float currentDistance = GetClipDistance(current);
            const bool currentInside = currentDistance >= 0.0f;
            if (currentInside != previousInside) clipped[clippedCount++] = InterpolateVertex(previous, current, previousDistance / (previousDistance - currentDistance));
            if (currentInside) clipped[clippedCount++] = current;
            previous = current;
            previousDistance = currentDistance;
            previousInside = currentInside;
        }

        if (clippedCount < 3) continue;
        const uint32_t baseVertex = verticesOut.size();
        for (int j = 0; j < clippedCount; j++) verticesOut.push_back(clipped[j]);
        for (int j = 1; j + 1 < clippedCount; j++) {
            indicesOut.push_back(baseVertex);
            indicesOut.push_back(baseVertex + j);
            indicesOut.push_back(baseVertex + j + 1);
        }
    }
}

void Wall::CleanUpWeatherBoardMesh() {
    Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("Procedural");

    // Clear any old mesh segments
    for (uint32_t meshId : m_weatherBoardSegmentMeshIds) {
        meshBuffer.RemoveMesh(meshId);
    }

    m_weatherBoardSegmentMeshIds.clear();
}

void Wall::RecreateWeatherBoardMesh() {
    Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("Procedural");

    CleanUpWeatherBoardMesh();
    m_weatherBoardstopRenderItems.clear();

    if (m_createInfo.wallType != WallType::WEATHER_BOARDS) return;

    Model* model = Hell::ResourceManager::GetModelByName("WeatherBoard_Stop");

    if (!model) {
        Logging::Error() << "Wall::CreateWeatherBoards() failed to load model 'WeatherBoard_Stop'";
        return;
    }

    float individualBoardHeight = 0.13f;
    const uint32_t textureBoardCount = glm::max(m_createInfo.weatherBoardTextureBoardCount, 1u);
    const uint32_t startIndex = glm::min(m_createInfo.weatherBoardStartIndex, textureBoardCount - 1);
    const uint32_t endIndex = glm::clamp(m_createInfo.weatherBoardEndIndex, startIndex, textureBoardCount - 1);
    const uint32_t boardIndexCount = endIndex - startIndex + 1;
    const int32_t stopMaterialIndex = Hell::ResourceManager::GetMaterialIndexByName(m_createInfo.weatherBoardStopMaterialName);

    // Each point owns the segment that follows it
    for (size_t i = 0; i < m_wallSegments.size(); i++) {
        if (stopMaterialIndex == -1 || !m_createInfo.sequencePoints[i].customBool) continue;
        WallSegment& wallSegment = m_wallSegments[i];
        glm::vec3 start = wallSegment.GetStart();
        glm::vec3 end = wallSegment.GetEnd();

        Hell::Transform transform;
        transform.position = start;
        transform.scale.y = glm::max(0.0f, wallSegment.GetStartHeight());
        transform.rotation.y = Hell::Math::YawBetweenPoints(start, end);

        RenderItem& renderItem = m_weatherBoardstopRenderItems.emplace_back();
        renderItem.modelMatrix = transform.to_mat4();
        renderItem.inverseModelMatrix = glm::inverse(renderItem.modelMatrix);
        renderItem.prevModelMatrix = renderItem.modelMatrix;
        renderItem.meshId = model->GetMeshIndices()[0];
        renderItem.materialIndex = stopMaterialIndex;
        renderItem.roughnessFactor = m_createInfo.roughnessFactor;
        renderItem.metallicFactor = m_createInfo.metallicFactor;

        Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("AssetGeometry");
        if (Mesh* mesh = meshBuffer.GetMeshById(renderItem.meshId)) {
            renderItem.baseIndex = mesh->baseIndex;
            renderItem.baseVertex = mesh->baseVertex;
            renderItem.vertexCount = mesh->vertexCount;
            renderItem.indexCount = mesh->indexCount;
        }

        renderItem.shadowFlags |= (SHADOW_FLAG_POINT_LIGHT | SHADOW_FLAG_CSM);

        RendererUtil::UpdateRenderItemAABB(renderItem);
        Hell::Bit::PackUint64(wallSegment.GetObjectId(), renderItem.objectIdLowerBit, renderItem.objectIdUpperBit);
    }

    // Create new mesh segments
    for (WallSegment& wallSegment : m_wallSegments) {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        const float startHeight = glm::max(0.0f, wallSegment.GetStartHeight());
        const float endHeight = glm::max(0.0f, wallSegment.GetEndHeight());
        const int weatherBoardCount = (int)std::ceil(glm::max(startHeight, endHeight) / individualBoardHeight);

        for (int i = 0; i < weatherBoardCount; i++) {
            glm::vec3 start = wallSegment.GetStart();
            glm::vec3 end = wallSegment.GetEnd();
            const uint32_t textureRow = startIndex + static_cast<uint32_t>(i) % boardIndexCount;

            start.y += individualBoardHeight * i;
            end.y += individualBoardHeight * i;

            glm::vec3 rayOrigin = start;
            glm::vec3 rayDir = glm::normalize(end - start);
            const float segLen = glm::distance(start, end);
            float remaining = segLen;
            const float eps = 1e-3f;

            while (remaining > eps) {
                ClipRayResult rayResult = HouseClipping::RaycastClippingVolumes(rayOrigin, rayDir, remaining);
                if (!rayResult.hitFound) break;

                if (glm::dot(rayResult.hitNormal, rayDir) < 0.0f && rayResult.distanceToHit > eps) {
                    glm::vec3 localStart = rayOrigin;
                    glm::vec3 localEnd = rayOrigin + (rayDir * rayResult.distanceToHit);
                    float boardWidth = glm::distance(localStart, localEnd);

                    AddBoard(rayOrigin, rayDir, textureRow, textureBoardCount, m_createInfo.textureOffsetU, m_createInfo.textureOffsetV, boardWidth, wallSegment, vertices, indices);
                }

                float advance = rayResult.distanceToHit + eps;
                rayOrigin += rayDir * advance;
                remaining -= advance;
            }

            if (remaining > eps) {
                float boardWidth = glm::distance(rayOrigin, end);
                AddBoard(rayOrigin, rayDir, textureRow, textureBoardCount, m_createInfo.textureOffsetU, m_createInfo.textureOffsetV, boardWidth, wallSegment, vertices, indices);
            }
        }

        uint32_t meshId = meshBuffer.AddMesh(vertices, indices, "Weatherboards");
        m_weatherBoardSegmentMeshIds.emplace_back(meshId);
    }
}
}
