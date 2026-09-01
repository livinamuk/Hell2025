#pragma once

#include "Hell/BVH/Types.h"

#include <cstdint>
#include <vector>

struct Vertex;

struct MeshBvh {
    std::vector<BvhNode> m_nodes;
    std::vector<BVHTriangle> m_triangles;
};

namespace Hell::Bvh {
    MeshBvh BuildMeshBvh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
}
