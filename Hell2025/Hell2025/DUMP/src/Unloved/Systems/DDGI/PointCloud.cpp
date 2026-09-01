#include "PointCloud.h"

#include "Hell/Geometry/Geometry.h"
#include "Legacy/World/LegacyWorld.h"

#include "Unloved/Debug/DebugDraw.h"

namespace Unloved {

float RoundUp(float value, float spacing) { return std::ceil(value / spacing) * spacing; }
float RoundDown(float value, float spacing) { return std::floor(value / spacing) * spacing; }

void PointCloud::Create(const std::vector<Triangle>& triangles, const glm::vec3& volumeMinBounds, const glm::vec3& volumeMaxBounds, float pointCloudSpacing, float gridCellSize) {
    m_pointCloudSpacing = pointCloudSpacing;
    m_volumeMinBounds = volumeMinBounds;
    m_volumeMaxBounds = volumeMaxBounds;
    m_gridCellSize = gridCellSize;

    m_points.clear();
    m_textureInfo.clear();

    for (const Triangle& triangle : triangles) {
        // Choose the up vector based on the normal
        glm::vec3 up = (std::abs(triangle.normal.z) > 0.999f) ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(0.0f, 0.0f, 1.0f);

        // Calculate right and up vectors
        glm::vec3 right = glm::normalize(glm::cross(up, triangle.normal));
        up = glm::cross(triangle.normal, right);

        glm::vec2 v0_2d(glm::dot(right, triangle.v0), glm::dot(up, triangle.v0));
        glm::vec2 v1_2d(glm::dot(right, triangle.v1), glm::dot(up, triangle.v1));
        glm::vec2 v2_2d(glm::dot(right, triangle.v2), glm::dot(up, triangle.v2));

        // Determine the bounding box of the 2d triangle
        glm::vec2 min = glm::min(glm::min(v0_2d, v1_2d), v2_2d);
        glm::vec2 max = glm::max(glm::max(v0_2d, v1_2d), v2_2d);

        // Round min and max values
        min.x = RoundDown(min.x, pointCloudSpacing) - pointCloudSpacing * 0.5f;
        min.y = RoundDown(min.y, pointCloudSpacing) - pointCloudSpacing * 0.5f;
        max.x = RoundUp(max.x, pointCloudSpacing) + pointCloudSpacing * 0.5f;
        max.y = RoundUp(max.y, pointCloudSpacing) + pointCloudSpacing * 0.5f;

        float threshold = 0.05f;
        min.x += threshold;
        min.y += threshold;
        max.x -= threshold;
        max.y -= threshold;

        // Generate points within the volume bounding box
        for (float x = min.x; x <= max.x; x += pointCloudSpacing) {
            for (float y = min.y; y <= max.y; y += pointCloudSpacing) {
                glm::vec2 pt(x, y);
                if (Hell::Geometry::IsPointInTriangle2D(pt, v0_2d, v1_2d, v2_2d)) {
                    glm::vec3 pt3d = triangle.v0 + right * (pt.x - v0_2d.x) + up * (pt.y - v0_2d.y);

                    // Exclude points outside the volume bounds
                    if (pt3d.x < volumeMinBounds.x || pt3d.y < volumeMinBounds.y || pt3d.z < volumeMinBounds.z ||
                        pt3d.x > volumeMaxBounds.x || pt3d.y > volumeMaxBounds.y || pt3d.z > volumeMaxBounds.z) {
                        continue;
                    }

                    CloudPoint& cloudPoint = m_points.emplace_back();
                    cloudPoint.position = glm::vec4(pt3d, 0.0f);
                    cloudPoint.normal = glm::vec4(triangle.normal, 0.0f);

                    // Calculate uv via barycentrics
                    glm::vec3 bary = Hell::Geometry::Barycentric2D(pt, v0_2d, v1_2d, v2_2d);
                    glm::vec2 uv = bary.x * triangle.uv0 + bary.y * triangle.uv1 + bary.z * triangle.uv2;

                    CloudPointTextureInfo& cloudPointTextureInfo = m_textureInfo.emplace_back();
                    cloudPointTextureInfo.u = uv.x;
                    cloudPointTextureInfo.v = uv.y;
                    cloudPointTextureInfo.baseColorIndex = triangle.baseColorTextureIndex;
                    cloudPointTextureInfo.rmaIndex = triangle.rmaTextureIndex;
                }
            }
        }
    }

    glm::vec3 size = volumeMaxBounds - volumeMinBounds;

    m_gridDimensions = glm::ivec3(
        std::max(1, static_cast<int>(std::ceil(size.x / m_gridCellSize))),
        std::max(1, static_cast<int>(std::ceil(size.y / m_gridCellSize))),
        std::max(1, static_cast<int>(std::ceil(size.z / m_gridCellSize)))
    );

    struct PointRef {
        uint32_t cellIndex;
        uint32_t originalIndex;

        bool operator<(const PointRef& other) const {
            return cellIndex < other.cellIndex;
        }
    };

    std::vector<PointRef> pointRefs;
    pointRefs.reserve(m_points.size());

    for (uint32_t i = 0; i < m_points.size(); ++i) {
        glm::ivec3 cellCoords = GetCellCoords(glm::vec3(m_points[i].position));
        const uint32_t cellIndex = GetCellIndex(cellCoords.x, cellCoords.y, cellCoords.z);
        pointRefs.push_back({ cellIndex, i });
    }

    // Sort points so spatial cells are contiguous in memory
    std::sort(pointRefs.begin(), pointRefs.end());

    std::vector<CloudPoint> sortedPoints;
    std::vector<CloudPointTextureInfo> sortedTextureInfo;
    sortedPoints.reserve(m_points.size());
    sortedTextureInfo.reserve(m_textureInfo.size());

    uint32_t totalCells = m_gridDimensions.x * m_gridDimensions.y * m_gridDimensions.z;
    m_gridCellOffsets.assign(totalCells, 0);
    m_gridCellCounts.assign(totalCells, 0);
    m_gridCellDirtyFlags.assign(totalCells, 0);

    if (!pointRefs.empty()) {
        uint32_t currentCell = pointRefs[0].cellIndex;
        m_gridCellOffsets[currentCell] = 0;
        uint32_t countInCell = 0;

        for (uint32_t i = 0; i < pointRefs.size(); ++i) {
            uint32_t cell = pointRefs[i].cellIndex;
            uint32_t origIdx = pointRefs[i].originalIndex;

            sortedPoints.push_back(m_points[origIdx]);
            sortedTextureInfo.push_back(m_textureInfo[origIdx]);

            if (cell != currentCell) {
                m_gridCellCounts[currentCell] = countInCell;
                currentCell = cell;
                m_gridCellOffsets[currentCell] = i;
                countInCell = 1;
            }
            else {
                countInCell++;
            }
        }

        m_gridCellCounts[currentCell] = countInCell;
    }

    // Swapping in the newly sorted data
    m_points = std::move(sortedPoints);
    m_textureInfo = std::move(sortedTextureInfo);
}

void PointCloud::CleanUp() {
    m_points.clear();
    m_textureInfo.clear();
    m_gridCellOffsets.clear();
    m_gridCellCounts.clear();
    m_gridCellDirtyFlags.clear();
}

void PointCloud::Update() {
}

void PointCloud::DebugDrawGrid() const {
    if (m_gridCellCounts.empty() || m_points.empty()) {
        return;
    }

    // Iterate through every cell in the 3D grid
    for (int z = 0; z < m_gridDimensions.z; ++z) {
        for (int y = 0; y < m_gridDimensions.y; ++y) {
            for (int x = 0; x < m_gridDimensions.x; ++x) {

                const uint32_t cellIndex = GetCellIndex(x, y, z);
                const glm::vec3 cellMin = GetCellMin(x, y, z);
                const glm::vec3 cellMax = GetCellMax(x, y, z);
                uint32_t pointCount = m_gridCellCounts[cellIndex];

                AABB cellAABB(cellMin, cellMax);
                DebugDraw::DrawAABB(cellAABB, YELLOW);

                // Skip drawing points if this cell is empty
                if (pointCount == 0) continue;
                
                // Calculate the gradient color based on the cell's normalized position
                // Max prevents division by zero if a dimension is only 1 cell wide
                float r = (float)x / (float)std::max(1, m_gridDimensions.x - 1);
                float g = (float)y / (float)std::max(1, m_gridDimensions.y - 1);
                float b = (float)z / (float)std::max(1, m_gridDimensions.z - 1);

                glm::vec4 cellColor(r, g, b, 1.0f);

                // Draw all points belonging to this cell using the lookup table
                uint32_t offset = m_gridCellOffsets[cellIndex];

                for (uint32_t i = 0; i < pointCount; ++i) {
                    uint32_t pointIndex = offset + i;
                    DebugDraw::DrawPoint(m_points[pointIndex].position, cellColor);
                }
            }
        }
    }
}

void PointCloud::DirtyCellsInAABB(const glm::vec3& minBounds, const glm::vec3& maxBounds) {
    if (m_gridCellDirtyFlags.empty()) return;

    // Skip if outside the volume
    if (maxBounds.x < m_volumeMinBounds.x || minBounds.x > m_volumeMaxBounds.x ||
        maxBounds.y < m_volumeMinBounds.y || minBounds.y > m_volumeMaxBounds.y ||
        maxBounds.z < m_volumeMinBounds.z || minBounds.z > m_volumeMaxBounds.z) {
        return;
    }

    const glm::ivec3 minCellCoords = GetCellCoords(minBounds);
    const glm::ivec3 maxCellCoords = GetCellCoords(maxBounds);

    for (int z = minCellCoords.z; z <= maxCellCoords.z; ++z) {
        for (int y = minCellCoords.y; y <= maxCellCoords.y; ++y) {
            for (int x = minCellCoords.x; x <= maxCellCoords.x; ++x) {
                uint32_t index = GetCellIndex(x, y, z);
                m_gridCellDirtyFlags[index] = 1; // Mark as dirty
            }
        }
    }
}

void PointCloud::DirtyCellsInSphere(const glm::vec3& position, float radius) {
    const glm::vec3 minBounds = position - glm::vec3(radius);
    const glm::vec3 maxBounds = position + glm::vec3(radius);

    DirtyCellsInAABB(minBounds, maxBounds);
}

glm::ivec3 PointCloud::GetCellCoords(const glm::vec3& worldPos) const {
    glm::vec3 localPos = worldPos - m_volumeMinBounds;
    glm::ivec3 coords = glm::ivec3(localPos / m_gridCellSize);

    return glm::clamp(coords, glm::ivec3(0), m_gridDimensions - glm::ivec3(1));
}

glm::ivec3 PointCloud::GetCellCoords(int32_t index) const {
    int32_t area = m_gridDimensions.x * m_gridDimensions.y;
    int32_t z = index / area;
    int32_t remainder = index % area;
    int32_t y = remainder / m_gridDimensions.x;
    int32_t x = remainder % m_gridDimensions.x;

    return { x, y, z };
}

uint32_t PointCloud::GetCellIndex(int32_t cellX, int32_t cellY, int32_t cellZ) const {
    return cellX + cellY * m_gridDimensions.x + cellZ * m_gridDimensions.x * m_gridDimensions.y;
}

glm::vec3 PointCloud::GetCellMin(int32_t cellX, int32_t cellY, int32_t cellZ) const {
    return m_volumeMinBounds + glm::vec3(cellX, cellY, cellZ) * m_gridCellSize;
}

glm::vec3 PointCloud::GetCellMax(int32_t cellX, int32_t cellY, int32_t cellZ) const {
    const glm::vec3 cellMin = GetCellMin(cellX, cellY, cellZ);
    return glm::min(cellMin + glm::vec3(m_gridCellSize), m_volumeMaxBounds);
}



} // namespace Unloved
