#include "MeshBvh.h"

#include "Hell/Logging.h"
#include "Hell/Render/VertexAttributes.h"

#include "bvh/v2/bbox.h"
#include "bvh/v2/bvh.h"
#include "bvh/v2/default_builder.h"
#include "bvh/v2/node.h"
#include "bvh/v2/vec.h"

#include <algorithm>

#include <glm/common.hpp>
#include <glm/geometric.hpp>

namespace {
    using MadmannVec3 = bvh::v2::Vec<float, 3>;
    using MadmannBBox = bvh::v2::BBox<float, 3>;
    using MadmannBvhNode = bvh::v2::Node<float, 3>;
    using MadmannBvh = bvh::v2::Bvh<MadmannBvhNode>;
    using MadmannBvhBuilder = bvh::v2::DefaultBuilder<MadmannBvhNode>;

    constexpr size_t GPU_TARGET_MAX_STACK_SIZE = 32;
}

namespace Hell::Bvh {

    MeshBvh BuildMeshBvh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices) {
        MeshBvh meshBvh;

        if (indices.size() % 3 != 0) {
            Logging::Fatal() << "Hell::Bvh::BuildMeshBvh(..) failed: index count " << indices.size() << " must be a multiple of 3\n";
        }

        const size_t triangleCount = indices.size() / 3;
        std::vector<MadmannBBox> bboxes(triangleCount);
        std::vector<MadmannVec3> centers(triangleCount);

        for (size_t i = 0; i < triangleCount; ++i) {
            const size_t indexOffset = i * 3;
            const size_t vertexIndex0 = indices[indexOffset + 0];
            const size_t vertexIndex1 = indices[indexOffset + 1];
            const size_t vertexIndex2 = indices[indexOffset + 2];

            const glm::vec3& p0 = vertices[vertexIndex0].position;
            const glm::vec3& p1 = vertices[vertexIndex1].position;
            const glm::vec3& p2 = vertices[vertexIndex2].position;

            const glm::vec3 boundsMin = glm::min(glm::min(p0, p1), p2);
            const glm::vec3 boundsMax = glm::max(glm::max(p0, p1), p2);
            const glm::vec3 center = (p0 + p1 + p2) / 3.0f;

            bboxes[i] = MadmannBBox(
                MadmannVec3(boundsMin.x, boundsMin.y, boundsMin.z),
                MadmannVec3(boundsMax.x, boundsMax.y, boundsMax.z));
            centers[i] = MadmannVec3(center.x, center.y, center.z);
        }

        MadmannBvhBuilder::Config config;
        config.quality = MadmannBvhBuilder::Quality::High;
        MadmannBvh bvh = MadmannBvhBuilder::build(bboxes, centers, config);

        meshBvh.m_nodes.resize(bvh.nodes.size());

        for (size_t i = 0; i < bvh.nodes.size(); ++i) {
            const MadmannBvhNode& sourceNode = bvh.nodes[i];
            BvhNode& targetNode = meshBvh.m_nodes[i];

            targetNode.boundsMin = glm::vec3(sourceNode.bounds[0], sourceNode.bounds[2], sourceNode.bounds[4]);
            targetNode.boundsMax = glm::vec3(sourceNode.bounds[1], sourceNode.bounds[3], sourceNode.bounds[5]);
            targetNode.primitiveCount = sourceNode.index.value & ((1u << MadmannBvhNode::prim_count_bits) - 1);
            targetNode.firstChildOrPrimitive = sourceNode.index.value >> MadmannBvhNode::prim_count_bits;
        }

        meshBvh.m_triangles.reserve(triangleCount);

        std::vector<uint32_t> stack;
        stack.reserve(GPU_TARGET_MAX_STACK_SIZE);

        if (!meshBvh.m_nodes.empty()) {
            stack.push_back(0);
        }

        size_t largestStackSize = 0;

        while (!stack.empty()) {
            largestStackSize = std::max(largestStackSize, stack.size());

            const uint32_t currentNodeIndex = stack.back();
            stack.pop_back();

            if (currentNodeIndex >= meshBvh.m_nodes.size()) {
                Logging::Fatal() << "Hell::Bvh::BuildMeshBvh(..) failed: invalid node index " << currentNodeIndex << "\n";
                continue;
            }

            BvhNode& node = meshBvh.m_nodes[currentNodeIndex];

            if (node.primitiveCount > 0) {
                const uint32_t newPrimitiveFloatIndex = static_cast<uint32_t>(meshBvh.m_triangles.size() * 12);

                for (uint32_t i = 0; i < node.primitiveCount; ++i) {
                    const size_t originalPrimitiveId = node.firstChildOrPrimitive + i;
                    const size_t triangleIndex = bvh.prim_ids[originalPrimitiveId];
                    const size_t indexOffset = triangleIndex * 3;

                    if (indexOffset + 2 >= indices.size()) {
                        Logging::Fatal() << "Hell::Bvh::BuildMeshBvh(..) failed: triangle index " << triangleIndex << " is out of range\n";
                        return meshBvh;
                    }

                    const uint32_t vertexIndex0 = indices[indexOffset + 0];
                    const uint32_t vertexIndex1 = indices[indexOffset + 1];
                    const uint32_t vertexIndex2 = indices[indexOffset + 2];

                    if (vertexIndex0 >= vertices.size() ||
                        vertexIndex1 >= vertices.size() ||
                        vertexIndex2 >= vertices.size()) {
                        Logging::Fatal() << "Hell::Bvh::BuildMeshBvh(..) failed: vertex index is out of range\n";
                        return meshBvh;
                    }

                    const glm::vec3& p0 = vertices[vertexIndex0].position;
                    const glm::vec3& p1 = vertices[vertexIndex1].position;
                    const glm::vec3& p2 = vertices[vertexIndex2].position;

                    const glm::vec3 e1 = p1 - p0;
                    const glm::vec3 e2 = p2 - p0;
                    const glm::vec3 normal = glm::cross(e1, e2);

                    BVHTriangle& triangle = meshBvh.m_triangles.emplace_back();
                    triangle.v0_and_e1x = glm::vec4(p0, e1.x);
                    triangle.e1yz_and_e2xy = glm::vec4(e1.y, e1.z, e2.x, e2.y);
                    triangle.e2z_and_normal = glm::vec4(e2.z, normal.x, normal.y, normal.z);
                }

                node.firstChildOrPrimitive = newPrimitiveFloatIndex;
            }
            else {
                stack.push_back(node.firstChildOrPrimitive + 0);
                stack.push_back(node.firstChildOrPrimitive + 1);
            }
        }

        if (largestStackSize >= GPU_TARGET_MAX_STACK_SIZE) {
            Logging::Warning()
                << "Hell::Bvh::BuildMeshBvh(..) generated a traversal stack size of "
                << largestStackSize
                << ", exceeding the target GPU limit of "
                << GPU_TARGET_MAX_STACK_SIZE
                << " (" << indices.size() << " indices)\n";
        }

        return meshBvh;
    }

}
