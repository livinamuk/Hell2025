#include "Bvh.h"
#include "Input/Input.h"
#include "Renderer/Renderer.h"
#include "HellConstants.h"
#include "UniqueID.h"
#include "Util.h"
#include "Timer.hpp"

#include <iostream>
#include <algorithm>
#include <unordered_map>

#include "bvh/v2/bvh.h"
#include "bvh/v2/vec.h"
#include "bvh/v2/bbox.h"
#include "bvh/v2/ray.h"
#include "bvh/v2/node.h"
#include "bvh/v2/default_builder.h"
#include "bvh/v2/thread_pool.h"
#include "bvh/v2/stack.h"

using MadmannVec3 = bvh::v2::Vec<float, 3>;
using MadmannBBox = bvh::v2::BBox<float, 3>;
using MadmannBvhNode = bvh::v2::Node<float, 3>;
using MadmannBvh = bvh::v2::Bvh<MadmannBvhNode>;
using MadmannBvhBuilder = bvh::v2::DefaultBuilder<MadmannBvhNode>;

namespace Bvh::Gpu {
    std::unordered_map<uint64_t, MeshBvh> g_meshBvhs;
    std::unordered_map<uint64_t, SceneBvh> g_sceneBvhs;
    bvh::v2::ThreadPool g_threadPool;

    // Gpu data
    std::unordered_map<uint64_t, uint32_t> g_meshBvhRootNodeOffsetMapping; // Maps a flatterend MeshBvh's root node to its id
    std::vector<BvhNode> g_meshBvhsNodes;
    std::vector<float> g_triangleData;

    glm::vec3 BvhVec3ToGlmVec3(MadmannVec3 vec);
    MadmannVec3 GlmVec3ToBvhVec3(glm::vec3 vec);

    uint64_t CreateMeshBvhFromMeshBvh(MeshBvh& sourceMeshBvh) {
        uint64_t uniqueId = UniqueID::GetNextObjectId(ObjectType::UNDEFINED); // make me use another ID system !!!

        MeshBvh& targetMeshBvh = g_meshBvhs[uniqueId];
        targetMeshBvh.m_nodes.swap(sourceMeshBvh.m_nodes);
        targetMeshBvh.m_triangleData.swap(sourceMeshBvh.m_triangleData);
        return uniqueId;
    }

    uint64_t CreateMeshBvhFromVertexData(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices) {
        //Timer timer("CreateBvhFromVertices() " + std::to_string(indices.size()) + " indices");

        uint64_t uniqueId = UniqueID::GetNextObjectId(ObjectType::UNDEFINED); // make me use another ID system !!!
        MeshBvh& meshBvh = g_meshBvhs[uniqueId];

        // Validate index count
        if (indices.size() % 3 != 0) {
            std::cout << "BVH::CreateMeshBvhFromVertexData() failed: index count " << indices.size() << " must be a multiple of 3\n";
        }

        size_t triangleCount = indices.size() / 3;
        std::vector<MadmannBBox> bboxes(triangleCount);
        std::vector<MadmannVec3> centers(triangleCount);

        for (int i = 0; i < triangleCount; ++i) {
            // Access the vertices for the current triangle
            const size_t indexOffset = i * 3;

            const size_t vertexIndex0 = indices[indexOffset];
            const size_t vertexIndex1 = indices[indexOffset + 1];
            const size_t vertexIndex2 = indices[indexOffset + 2];

            const glm::vec3& p0 = vertices[vertexIndex0].position;
            const glm::vec3& p1 = vertices[vertexIndex1].position;
            const glm::vec3& p2 = vertices[vertexIndex2].position;

            // Calculate bounding box and center
            glm::vec3 min_p = glm::min(glm::min(p0, p1), p2);
            glm::vec3 max_p = glm::max(glm::max(p0, p1), p2);
            glm::vec3 center_p = (p0 + p1 + p2) / 3.0f;

            bboxes[i] = MadmannBBox(MadmannVec3(min_p.x, min_p.y, min_p.z), MadmannVec3(max_p.x, max_p.y, max_p.z));
            centers[i] = MadmannVec3(center_p.x, center_p.y, center_p.z);
        }

        MadmannBvhBuilder::Config config;
        config.quality = MadmannBvhBuilder::Quality::High;

        //MadmannBvh bvh = MadmannBvhBuilder::build(g_threadPool, bboxes, centers, config);
        MadmannBvh bvh = MadmannBvhBuilder::build(bboxes, centers, config);

        // Create our node array
        int nodeCount = bvh.nodes.size();
        meshBvh.m_nodes.resize(nodeCount);

        // .. by iterating the bvh created by the library and extracting the relevant data
        for (int i = 0; i < nodeCount; i++) {
            const MadmannBvhNode& mmNode = bvh.nodes[i];
            BvhNode& node = meshBvh.m_nodes[i];
            node.boundsMin = glm::vec3(mmNode.bounds[0], mmNode.bounds[2], mmNode.bounds[4]);
            node.boundsMax = glm::vec3(mmNode.bounds[1], mmNode.bounds[3], mmNode.bounds[5]);
            node.primitiveCount = mmNode.index.value & ((1u << MadmannBvhNode::prim_count_bits) - 1);
            node.firstChildOrPrimitive = mmNode.index.value >> MadmannBvhNode::prim_count_bits;
        }

        // Create the re-ordered triangle array
        int floatCount = indices.size() / 3 * 3 * 4;
        meshBvh.m_triangleData.reserve(floatCount);

        // .. by walking the bvh and re-order triangles so that leaf node tris are adjacent, and updating the node indices
        std::vector<uint32_t> stack;
        const size_t GPU_TARGET_MAX_STACK_SIZE = 32;
        stack.reserve(GPU_TARGET_MAX_STACK_SIZE);

        if (!meshBvh.m_nodes.empty()) { // Push the root
            stack.push_back(0);
        }

        size_t largestStackSize = 0;

        while (!stack.empty()) {
            // Track the maximum size the stack reaches
            largestStackSize = std::max(largestStackSize, stack.size());

            // Get the index from the back of the vector top of stack
            uint32_t currentNodeIndex = stack.back();
            stack.pop_back();

            // Get the actual node
            if (currentNodeIndex >= meshBvh.m_nodes.size()) {
                std::cout << "Error: Invalid node index " << currentNodeIndex << " popped from stack!\n";
                continue;
            }
            BvhNode& node = meshBvh.m_nodes[currentNodeIndex];

            // Iterate primitives
            if (node.primitiveCount > 0) {
                uint32_t newPrimitiveFloatIndex = static_cast<uint32_t>(meshBvh.m_triangleData.size());

                for (uint32_t i = 0; i < node.primitiveCount; i++) {
                    const size_t originalPrimitiveId = node.firstChildOrPrimitive + i;
                    const size_t triIndex = bvh.prim_ids[originalPrimitiveId];
                    const size_t indexOffset = triIndex * 3;

                    // Check index offset bounds
                    if (indexOffset + 2 >= indices.size()) {
                        std::cout << "Triangle index " << triIndex << " is out of range (max: " << (indices.size() / 3) - 1 << ")\n";
                        return uniqueId;
                    }

                    // Retrieve triangle vertex indices
                    int vertexIndex0 = indices[indexOffset];
                    int vertexIndex1 = indices[indexOffset + 1];
                    int vertexIndex2 = indices[indexOffset + 2];

                    // Check vertex index bounds
                    if (vertexIndex0 >= vertices.size()) {
                        std::cout << "vertexIndex0 " << vertexIndex0 << " out of range of vertex buffer size " << vertices.size() << "\n";
                        return uniqueId;
                    }
                    if (vertexIndex1 >= vertices.size()) {
                        std::cout << "vertexIndex1 " << vertexIndex1 << " out of range of vertex buffer size " << vertices.size() << "\n";
                        return uniqueId;
                    }
                    if (vertexIndex2 >= vertices.size()) {
                        std::cout << "vertexIndex2 " << vertexIndex2 << " out of range of vertex buffer size " << vertices.size() << "\n";
                        return uniqueId;
                    }

                    // Fetch triangle vertices
                    const glm::vec3& p0 = vertices[vertexIndex0].position;
                    const glm::vec3& p1 = vertices[vertexIndex1].position;
                    const glm::vec3& p2 = vertices[vertexIndex2].position;

                    // Precompute edges
                    glm::vec3 e1 = p0 - p1;
                    glm::vec3 e2 = p2 - p0;

                    // Precompute normal
                    glm::vec3 normal = cross(e1, e2);

                    // Store p0
                    meshBvh.m_triangleData.push_back(p0.x);
                    meshBvh.m_triangleData.push_back(p0.y);
                    meshBvh.m_triangleData.push_back(p0.z);

                    // Store e1
                    meshBvh.m_triangleData.push_back(e1.x);
                    meshBvh.m_triangleData.push_back(e1.y);
                    meshBvh.m_triangleData.push_back(e1.z);

                    // Store e2
                    meshBvh.m_triangleData.push_back(e2.x);
                    meshBvh.m_triangleData.push_back(e2.y);
                    meshBvh.m_triangleData.push_back(e2.z);

                    // Store normal
                    meshBvh.m_triangleData.push_back(normal.x);
                    meshBvh.m_triangleData.push_back(normal.y);
                    meshBvh.m_triangleData.push_back(normal.z);
                }

                // Update node primitive pointer to the start of the reordered data
                // This index points to the first float (p0.x) of the first triangle in the leaf
                node.firstChildOrPrimitive = newPrimitiveFloatIndex;
            }

            // The node is internal, so recurse
            else {
                stack.push_back(node.firstChildOrPrimitive + 0);
                stack.push_back(node.firstChildOrPrimitive + 1);
            }
        }

        // Check if the current size exceeds the target limit BEFORE popping
        if (largestStackSize >= GPU_TARGET_MAX_STACK_SIZE) {
            std::cout << "\n";
            std::cout << "Warning: GPU BVH traversal stack size " << largestStackSize;
            std::cout << " exceeded target GPU limit " << GPU_TARGET_MAX_STACK_SIZE << " ";
            std::cout << "(" << indices.size() << " indices)\n";
            std::cout << "\n";
        }

        return uniqueId;
    }

    uint64_t CreateNewSceneBvh() {
        uint64_t uniqueId = UniqueID::GetNextObjectId(ObjectType::UNDEFINED); // make me use another ID system !!!
        SceneBvh& sceneBvh = g_sceneBvhs[uniqueId];
        return uniqueId;
    }

    void UpdateSceneBvh(uint64_t bvhId, std::vector<PrimitiveInstance>& instances) {
        //Timer timer("UpdateSceneBvh() " + std::to_string(instances.size()) + " instances");

        // Early out if bvhId is invalid
        if (!SceneBvhExists(bvhId)) return;

        SceneBvh& sceneBvh = g_sceneBvhs[bvhId];

        // Clear last frames data
        sceneBvh.m_gpuInstances.clear();
        sceneBvh.m_nodes.clear();

        if (instances.empty()) return;

        std::vector<MadmannBBox> bboxes(instances.size());
        std::vector<MadmannVec3> centers(instances.size());

        for (int i = 0; i < instances.size(); i++) {
            PrimitiveInstance& instance = instances[i];
            MadmannVec3 aabbMin = GlmVec3ToBvhVec3(instance.worldAabbBoundsMin);
            MadmannVec3 aabbMax = GlmVec3ToBvhVec3(instance.worldAabbBoundsMax);
            MadmannVec3 center = GlmVec3ToBvhVec3(instance.worldAabbCenter);
            bboxes[i] = MadmannBBox(aabbMin, aabbMax);
            centers[i] = center;
        }

        typename bvh::v2::DefaultBuilder<MadmannBvhNode>::Config config;
        config.quality = bvh::v2::DefaultBuilder<MadmannBvhNode>::Quality::High;
        MadmannBvh bvh = bvh::v2::DefaultBuilder<MadmannBvhNode>::build(g_threadPool, bboxes, centers, config);

        int nodeCount = bvh.nodes.size();
        sceneBvh.m_nodes.resize(nodeCount);

        for (int i = 0; i < nodeCount; i++) {
            const MadmannBvhNode& mmNode = bvh.nodes[i];
            BvhNode& node = sceneBvh.m_nodes[i];
            node.boundsMin = glm::vec3(mmNode.bounds[0], mmNode.bounds[2], mmNode.bounds[4]);
            node.boundsMax = glm::vec3(mmNode.bounds[1], mmNode.bounds[3], mmNode.bounds[5]);
            node.primitiveCount = mmNode.index.value & ((1u << MadmannBvhNode::prim_count_bits) - 1);
            node.firstChildOrPrimitive = mmNode.index.value >> MadmannBvhNode::prim_count_bits;
        }

        sceneBvh.m_gpuInstances.reserve(instances.size());

        // Walk the scene BVH and create the entity instance array, specifically ordering them such that leaf node instances are adjacent
        std::vector<uint32_t> stack;
        stack.reserve(std::max<size_t>(MAX_BVH_STACK_SIZE, sceneBvh.m_nodes.size()));
        stack.push_back(0);

        while (!stack.empty()) {
            uint32_t currentIndex = stack.back();
            stack.pop_back();
            BvhNode& node = sceneBvh.m_nodes[currentIndex];

            // If this node is a leaf...
            if (node.primitiveCount > 0) {
                int newPrimitiveIndex = static_cast<int>(sceneBvh.m_gpuInstances.size());

                // Loop over each primitive instance in the leaf
                for (uint32_t i = 0; i < node.primitiveCount; i++) {
                    int primitiveId = node.firstChildOrPrimitive + i;
                    int instanceIndex = bvh.prim_ids[primitiveId];

                    const PrimitiveInstance& instance = instances[instanceIndex];

                    // Create the GPU primitive instance
                    GpuPrimitiveInstance& gpuInstance = sceneBvh.m_gpuInstances.emplace_back();
                    auto rootNodeOffsetIt = g_meshBvhRootNodeOffsetMapping.find(instance.meshBvhId);
                    if (rootNodeOffsetIt != g_meshBvhRootNodeOffsetMapping.end()) {
                        gpuInstance.rootNodeIndex = rootNodeOffsetIt->second;
                    }
                    else {
                        std::cout << "Warning: missing mesh BVH root mapping for meshBvhId " << instance.meshBvhId << "\n";
                        gpuInstance.rootNodeIndex = 0;
                    }
                    gpuInstance.worldTransform = instance.worldTransform;
                    gpuInstance.inverseWorldTransform = glm::inverse(gpuInstance.worldTransform);
                    gpuInstance.openableId = instance.openableId;
                    gpuInstance.customId = instance.customId;
                    gpuInstance.globalMeshIndex = instance.globalMeshIndex;
                    Util::PackUint64(instance.objectId, gpuInstance.objectIdLowerBit, gpuInstance.objectIdUpperBit);
                }
                // Update the leaf node's pointer so it now points into the new, contiguous instance array
                node.firstChildOrPrimitive = newPrimitiveIndex;
            }
            else {
                stack.push_back(node.firstChildOrPrimitive);
                stack.push_back(node.firstChildOrPrimitive + 1);
            }
        }
    }

    void FlatternMeshBvhNodes() {
        g_meshBvhRootNodeOffsetMapping.clear();
        g_meshBvhsNodes.clear();
        g_triangleData.clear();

        // Preallocate memory
        uint32_t totalNodeCount = 0;
        uint32_t totalTriangleDataSize = 0;
        for (auto it = g_meshBvhs.begin(); it != g_meshBvhs.end(); ++it) {
            MeshBvh& meshBvh = it->second;
            totalNodeCount += (uint32_t)meshBvh.m_nodes.size();
            totalTriangleDataSize += (uint32_t)meshBvh.m_triangleData.size();
        }
        g_meshBvhsNodes.reserve(totalNodeCount);
        g_triangleData.reserve(totalTriangleDataSize);

        uint32_t rootNodeOffset = 0;
        uint32_t baseTriangleOffset = 0;

        // Iterate each mesh bvh, and store its nodes and triangle data in the global arrays
        for (auto it = g_meshBvhs.begin(); it != g_meshBvhs.end(); ++it) {
            uint64_t bvhId = it->first;
            MeshBvh& meshBvh = it->second;

            // Store the root node and triangle offsets
            g_meshBvhRootNodeOffsetMapping[bvhId] = rootNodeOffset;

            // Append this mesh's nodes to the global vector
            for (BvhNode& node : meshBvh.m_nodes) {

                // Copy the node
                BvhNode& appendedNode = g_meshBvhsNodes.emplace_back(node);

                // If the node is a leaf, add the base triangle offset
                if (appendedNode.primitiveCount > 0) {
                    appendedNode.firstChildOrPrimitive += baseTriangleOffset;
                }
                // If it's not a leaf, then add the root node offset
                else {
                    appendedNode.firstChildOrPrimitive += rootNodeOffset;
                }
            }

            // Copy the triangle data from this mesh into the global vector
            g_triangleData.insert(g_triangleData.end(), meshBvh.m_triangleData.begin(), meshBvh.m_triangleData.end());

            // Increment offsets
            rootNodeOffset += (uint32_t)meshBvh.m_nodes.size();
            baseTriangleOffset += (uint32_t)meshBvh.m_triangleData.size();
        }
    }

    glm::vec3 BvhVec3ToGlmVec3(MadmannVec3 vec) {
        return { vec.values[0], vec.values[1], vec.values[2] };
    }

    MadmannVec3 GlmVec3ToBvhVec3(glm::vec3 vec) {
        return { vec.x, vec.y, vec.z };
    }

    bool MeshBvhExists(uint64_t bvhId) {
        return (g_meshBvhs.find(bvhId) != g_meshBvhs.end());
    }

    bool SceneBvhExists(uint64_t bvhId) {
        return (g_sceneBvhs.find(bvhId) != g_sceneBvhs.end());
    }

    void DestroySceneBvh(uint64_t bvhId) {
        auto it = g_sceneBvhs.find(bvhId);
        if (it != g_sceneBvhs.end()) {
            g_sceneBvhs.erase(it);
        }
    }

    void DestroyMeshBvh(uint64_t bvhId) {
        auto it = g_meshBvhs.find(bvhId);
        if (it != g_meshBvhs.end()) {
            g_meshBvhs.erase(it);
        }
    }

    SceneBvh* GetSceneBvhById(uint64_t bvhId) {
        if (!SceneBvhExists(bvhId)) return nullptr;
        return &g_sceneBvhs[bvhId];
    }
    
    MeshBvh* GetMeshBvhById(uint64_t bvhId) {
        if (!MeshBvhExists(bvhId)) return nullptr;
        return &g_meshBvhs[bvhId];
    }

    const std::vector<BvhNode>& GetMeshGpuBvhNodes() {
        return g_meshBvhsNodes;
    }

    //const std::vector<BvhNode>& GetSceneGpuBvhNodes() {
    //    return g_sceneBvhsNodes;
    //}

    const std::vector<float>& GetTriangleData() {
        return g_triangleData;
    }

    const std::vector<GpuPrimitiveInstance>& GetGpuEntityInstances(uint64_t sceneBvhId) {
        static std::vector<GpuPrimitiveInstance> empty;

        if (!SceneBvhExists(sceneBvhId)) return empty;
        return g_sceneBvhs[sceneBvhId].m_gpuInstances;
    }
}
