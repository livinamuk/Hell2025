#pragma once

#include "Unloved/Common/Types.h"

#include <vector>

namespace Unloved {

struct Triangle {
    glm::vec3 v0;
    glm::vec3 v1;
    glm::vec3 v2;
    glm::vec2 uv0;
    glm::vec2 uv1;
    glm::vec2 uv2;
    glm::vec3 normal;
    int baseColorTextureIndex;
    int rmaTextureIndex;
};

struct CloudPoint {
    glm::vec4 position = glm::vec4(0);
    glm::vec4 normal = glm::vec4(0);
    glm::vec4 directLighting = glm::vec4(0);
    glm::vec4 baseColor = glm::vec4(1.0);
};

struct CloudPointTextureInfo {
    float u;
    float v;
    int baseColorIndex;
    int rmaIndex;
};

struct PointCloud {
    void Create(const std::vector<Triangle>& triangles, const glm::vec3& volumeMinBounds, const glm::vec3& volumeMaxBounds, float pointCloudSpacing, float gridCellSize);
    void CleanUp();
    void Update();
    void DebugDrawGrid() const;
    void DirtyCellsInSphere(const glm::vec3& position, float radius);
    void DirtyCellsInAABB(const glm::vec3& minBounds, const glm::vec3& maxBounds);
    void ClearDirtyFlags();

    uint32_t GetCellIndex(int32_t cellX, int32_t cellY, int32_t cellZ) const;
    glm::vec3 GetCellMin(int32_t cellX, int32_t cellY, int32_t cellZ) const;
    glm::vec3 GetCellMax(int32_t cellX, int32_t cellY, int32_t cellZ) const;
    glm::ivec3 GetCellCoords(const glm::vec3& worldPos) const;
    glm::ivec3 GetCellCoords(int32_t index) const;

    const std::vector<CloudPoint>& GetPoints() const                 { return m_points; }
    const std::vector<CloudPointTextureInfo>& GetTextureInfo() const { return m_textureInfo; }
    const std::vector<uint32_t>& GetGridCellOffsets() const          { return m_gridCellOffsets; }
    const std::vector<uint32_t>& GetGridCellCounts() const           { return m_gridCellCounts; }
    const std::vector<uint32_t>& GetGridCellDirtyFlags() const       { return m_gridCellDirtyFlags; }
    uint32_t GetPointCount() const                                   { return (uint32_t)m_points.size(); }
    uint32_t GetGridCellCount() const                                { return (uint32_t)m_gridCellOffsets.size(); }
    glm::ivec3 GetGridDimensions() const                             { return m_gridDimensions; }
    float GetGridCellSize() const                                    { return m_gridCellSize; }

private:
    std::vector<CloudPoint> m_points;
    std::vector<CloudPointTextureInfo> m_textureInfo;
    std::vector<uint32_t> m_gridCellOffsets;
    std::vector<uint32_t> m_gridCellCounts;
    std::vector<uint32_t> m_gridCellDirtyFlags;
    glm::vec3 m_volumeMinBounds {};
    glm::vec3 m_volumeMaxBounds {};
    glm::ivec3 m_gridDimensions {};
    float m_pointCloudSpacing {};
    float m_gridCellSize {};
};

} // namespace Unloved
