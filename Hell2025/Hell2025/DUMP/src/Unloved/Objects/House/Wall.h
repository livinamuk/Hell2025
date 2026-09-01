#pragma once
#include "Trim.h"
#include "WallSegment.h"

#include "Hell/ResourceManagement/Types/Material.h"

#include "Unloved/Common/CreateInfo.h"
#include "Unloved/Render/RendererTypes.h"

#include <glm/glm.hpp>

#include <vector>

namespace Unloved {

struct Wall {
    Wall() = default;
    Wall(uint64_t id, const WallCreateInfo& createInfo, const SpawnOffset& spawnOffset);
    Wall(const Wall&) = delete;
    Wall& operator=(const Wall&) = delete;
    Wall(Wall&&) noexcept = default;
    Wall& operator=(Wall&&) noexcept = default;
    ~Wall() = default;

    void CleanUp();
    void UpdateSegmentsTrimsAndVertexData();
    void SetPosition(const glm::vec3& position);
    void UpdateWorldSpaceCenter(glm::vec3 worldSpaceCenter);
    void SubmitRenderItems();
    void CreateTrims();
    void DrawSegmentVertices(glm::vec4 color);
    void DrawSegmentLines(glm::vec4 color);
    void FlipFaces();
    bool AddPointToEnd(glm::vec3 point, bool supressWarning = true);
    bool UpdatePointPosition(int pointIndex, glm::vec3 position, bool supressWarning = true);
    void UpdateSequencePoints(const std::vector<SequencePoint>& sequencePoints);
    void SetPointHeight(int pointIndex, float height);
    void SetPointCustomBool(int pointIndex, bool value);

    void SetCeilingTrimType(TrimType trimType);
    void SetFloorTrimType(TrimType trimType);
    void SetTextureScale(float value);
    void SetTextureOffsetU(float value);
    void SetTextureOffsetV(float value);
    void SetRoughnessFactor(float value);
    void SetMetallicFactor(float value);
    void SetMaterial(const std::string& materialName);
    void SetWeatherBoardMaterial(const std::string& materialName, uint32_t boardCount, uint32_t startIndex, uint32_t endIndex, float textureOffsetU, float textureOffsetV);
    void SetWeatherBoardStopMaterial(const std::string& materialName);
    void SetWallType(WallType wallType);
    void SetWeatherBoardTextureBoardCount(uint32_t value);
    void SetWeatherBoardStartIndex(uint32_t value);
    void SetWeatherBoardEndIndex(uint32_t value);

    void RecreateWeatherBoardMesh();
    void CleanUpWeatherBoardMesh();

    const glm::vec3& GetPointByIndex(int pointIndex); 
    float GetPointHeightByIndex(int pointIndex) const;

    bool IsWeatherBoards()                                                  { return m_createInfo.wallType == WallType::WEATHER_BOARDS; }
    const WallType GetWallType() const                                      { return m_createInfo.wallType; }
    const size_t GetPointCount() const                                      { return m_createInfo.sequencePoints.size(); }
    const glm::vec3& GetWorldSpaceCenter() const                            { return m_worldSpaceCenter; }
    Material* GetMaterial();
    int32_t GetMaterialIndex() const                                        { return m_materialIndex; }
    const std::vector<RenderItem>& GetWeatherBoardstopRenderItems()         { return m_weatherBoardstopRenderItems; }
    std::vector<WallSegment>& GetWallSegments()                             { return m_wallSegments; }
    const uint64_t GetObjectId() const                                      { return m_objectId; }
    const WallCreateInfo& GetCreateInfo() const                             { return m_createInfo; }
    const std::string& GetEditorName() const                                { return m_createInfo.editorName; }

private:
    uint64_t m_objectId = 0;
    int32_t m_materialIndex = -1;
    TrimType m_ceilingTrimType = TrimType::NONE;
    TrimType m_floorTrimType = TrimType::NONE;
    glm::vec3 m_worldSpaceCenter = glm::vec3(0.0f);
    std::vector<RenderItem> m_weatherBoardstopRenderItems;
    std::vector<WallSegment> m_wallSegments;
    std::vector<Trim> m_trims;
    WallCreateInfo m_createInfo;
    SpawnOffset m_spawnOffset;
    std::vector<uint32_t> m_weatherBoardSegmentMeshIds;

    void CreateCSGVertexData();
};
}
