#pragma once

#include "Hell/Physics/Ragdoll/RagdollAsset.h"
#include "Hell/Render/VertexAttributes.h"

#include <vector>

struct RagdollShapeMeshData {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

namespace RagdollShapeMesh {
    RagdollShapeMeshData Create(const RagdollShape& shape);
}
