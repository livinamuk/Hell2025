#pragma once

#include "Hell/Render/VertexAttributes.h"
#include "Unloved/Systems/House/ClippingVolume.h"

#include <glm/vec3.hpp>

#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace Unloved::HouseBuilder {

    void Init();
    void MarkDirty();
    void RebuildAll();
    void RebuildIfDirty();

    bool IsDirty();

    void RecreateAllProceduralWallMesh();
    void RecreateAllProcedularWorldPlaneMesh();
    void RecreateAllWeatherBoards();
    void RecreateAllWallTrims();
    void RecreateAllHangingLightCords();
    void RemoveAllWeatherBoards();

    void ResetPictureFrameImageList();
    void TakeLargePictureFrameMaterial(const std::string& name);
    const std::vector<std::string> GetLargePictureFrameMaterialNames();
    std::string GetNextRandomLargePictureFrameMaterial();

    const std::vector<Vertex>& GetWeatherBoardVertices();
    const std::vector<uint32_t>& GetWeatherBoardIndices();

}
