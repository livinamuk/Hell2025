#pragma once

#include "Hell/Render/VertexAttributes.h"

#include <cstdint>
#include <vector>

namespace Hell::PrimitiveMesh {
    std::vector<Vertex> GenerateSphereVertices(float radius, int segments);
    std::vector<Vertex> GenerateSphereVertices(float radius, int latitudeSegments, int longitudeSegments);
    std::vector<Vertex> GenerateCapsuleVertices(float radius, float cylinderLength, int hemisphereRings, int segments);
    std::vector<Vertex> GenerateRingVertices(float sphereRadius, float ringThickness, int ringSegments, int thicknessSegments);
    std::vector<Vertex> GenerateConeVertices(float radius, float height, int segments);
    std::vector<Vertex> GenerateCylinderVertices(float radius, float height, int subdivisions);
    std::vector<Vertex> GenerateCubeVertices();
    std::vector<uint32_t> GenerateRingIndices(int segments, int thicknessSegments);
    std::vector<uint32_t> GenerateSphereIndices(int segments);
    std::vector<uint32_t> GenerateSphereIndices(int latitudeSegments, int longitudeSegments);
    std::vector<uint32_t> GenerateCapsuleIndices(int hemisphereRings, int segments);
    std::vector<uint32_t> GenerateConeIndices(int segments);
    std::vector<uint32_t> GenerateCylinderIndices(int subdivisions);
    std::vector<uint32_t> GenerateCubeIndices();
}
