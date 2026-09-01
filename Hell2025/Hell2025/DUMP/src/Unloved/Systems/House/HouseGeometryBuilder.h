#pragma once

#include "Hell/Render/VertexAttributes.h"
#include "Unloved/Common/PlanarQuad.h"

#include <cstdint>
#include <vector>

struct HouseGeometrySourceMesh {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

namespace Unloved::HouseGeometryBuilder {

    void Init();

    void CreateDownFacingPlane(const PlanarQuad& planarQuad, std::vector<Vertex>& verticesOut, std::vector<uint32_t>& indicesOut);
    void CreateUpFacingPlane(const PlanarQuad& planarQuad, std::vector<Vertex>& verticesOut, std::vector<uint32_t>& indicesOut);

    const HouseGeometrySourceMesh& GetCubeSourceMesh();
    const HouseGeometrySourceMesh& GetDeckingBoardsSourceMesh();
    const HouseGeometrySourceMesh& GetGutterSourceMesh();
    const HouseGeometrySourceMesh& GetGutterEndCapLeftSourceMesh();
    const HouseGeometrySourceMesh& GetGutterEndCapRightSourceMesh();
    const HouseGeometrySourceMesh& GetGutterFasciaSourceMesh();
    const HouseGeometrySourceMesh& GetRidgeCappingSourceMesh();
    const HouseGeometrySourceMesh& GetRoofingFlashingLeftSourceMesh();
    const HouseGeometrySourceMesh& GetRoofingFlashingRightSourceMesh();
    const HouseGeometrySourceMesh& GetRoofingIronSourceMesh();
}
