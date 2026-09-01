#include "SceneBvh.h"

#include "Hell/BVH/Types/MeshBvh.h"
#include "Hell/Common/Bit.h"
#include "Hell/Logging.h"
#include "Hell/Math/Math.h"

#include "bvh/v2/bbox.h"
#include "bvh/v2/bvh.h"
#include "bvh/v2/default_builder.h"
#include "bvh/v2/node.h"
#include "bvh/v2/thread_pool.h"
#include "bvh/v2/vec.h"

#include <algorithm>
#include <cmath>

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/matrix.hpp>

namespace {
    using MadmannVec3 = bvh::v2::Vec<float, 3>;
    using MadmannBBox = bvh::v2::BBox<float, 3>;
    using MadmannBvhNode = bvh::v2::Node<float, 3>;
    using MadmannBvh = bvh::v2::Bvh<MadmannBvhNode>;
    using MadmannBvhBuilder = bvh::v2::DefaultBuilder<MadmannBvhNode>;

    constexpr size_t GPU_TARGET_MAX_STACK_SIZE = 32;
    bvh::v2::ThreadPool g_threadPool;

    MadmannBvh BuildTopLevelBvh(const std::vector<MadmannBBox>& bboxes, const std::vector<MadmannVec3>& centers) {
        MadmannBvhBuilder::Config config;
        config.quality = MadmannBvhBuilder::Quality::High;
        return MadmannBvhBuilder::build(g_threadPool, bboxes, centers, config);
    }

    bool HasSameStructure(const PrimitiveInstance& a, const PrimitiveInstance& b) {
        return a.objectId == b.objectId &&
               a.meshBvhId == b.meshBvhId &&
               a.openableId == b.openableId &&
               a.customId == b.customId &&
               a.globalMeshIndex == b.globalMeshIndex &&
               a.localMeshNodeIndex == b.localMeshNodeIndex;
    }

    MadmannVec3 GlmVec3ToMadmannVec3(const glm::vec3& vec) {
        return MadmannVec3(vec.x, vec.y, vec.z);
    }

    glm::vec3 TransformNormalToWorldSpace(const GpuPrimitiveInstance& instance, const glm::vec3& localNormal) {
        const glm::mat3 normalMatrix = glm::transpose(glm::mat3(instance.inverseWorldTransform));
        return glm::normalize(normalMatrix * localNormal);
    }

    RayData ComputeRayData(const glm::vec3& rayOrigin, const glm::vec3& rayDir, float minDistance, float maxDistance) {
        RayData rayData;
        rayData.origin[0] = rayOrigin.x;
        rayData.origin[1] = rayOrigin.y;
        rayData.origin[2] = rayOrigin.z;
        rayData.dir[0] = rayDir.x;
        rayData.dir[1] = rayDir.y;
        rayData.dir[2] = rayDir.z;
        rayData.invDir[0] = 1.0f / rayDir.x;
        rayData.invDir[1] = 1.0f / rayDir.y;
        rayData.invDir[2] = 1.0f / rayDir.z;
        rayData.minDistance = minDistance;
        rayData.maxDistance = maxDistance;
        return rayData;
    }

    bool IntersectNode(const RayData& rayData, const glm::vec3& aabbBoundsMin, const glm::vec3& aabbBoundsMax, float& t) {
        const glm::vec3 t1(
            (aabbBoundsMin[0] - rayData.origin[0]) * rayData.invDir[0],
            (aabbBoundsMin[1] - rayData.origin[1]) * rayData.invDir[1],
            (aabbBoundsMin[2] - rayData.origin[2]) * rayData.invDir[2]);

        const glm::vec3 t2(
            (aabbBoundsMax[0] - rayData.origin[0]) * rayData.invDir[0],
            (aabbBoundsMax[1] - rayData.origin[1]) * rayData.invDir[1],
            (aabbBoundsMax[2] - rayData.origin[2]) * rayData.invDir[2]);

        const glm::vec3 tminVec = glm::min(t1, t2);
        const glm::vec3 tmaxVec = glm::max(t1, t2);

        const float tmin = std::max({ tminVec.x, tminVec.y, tminVec.z, rayData.minDistance });
        const float tmax = std::min({ tmaxVec.x, tmaxVec.y, tmaxVec.z, rayData.maxDistance });

        t = tmin;
        return tmin <= tmax;
    }

    bool IntersectTri(glm::vec3 rayOrigin, glm::vec3 rayDir, float minDistance, float maxDistance, glm::vec3 p0, glm::vec3 e1, glm::vec3 e2, float& t) {
        const glm::vec3 p = glm::cross(rayDir, e2);
        const float det = glm::dot(e1, p);

        if (std::abs(det) < 0.000001f) {
            return false;
        }

        const float invDet = 1.0f / det;
        const glm::vec3 s = rayOrigin - p0;
        const float u = glm::dot(s, p) * invDet;
        if (u < 0.0f || u > 1.0f) {
            return false;
        }

        const glm::vec3 q = glm::cross(s, e1);
        const float v = glm::dot(rayDir, q) * invDet;
        if (v < 0.0f || u + v > 1.0f) {
            return false;
        }

        t = glm::dot(e2, q) * invDet;
        return t >= minDistance && t < maxDistance;
    }

    void UnpackTriangle(const BVHTriangle& triangle, glm::vec3& p0, glm::vec3& e1, glm::vec3& e2, glm::vec3& normal) {
        p0 = glm::vec3(triangle.v0_and_e1x);
        e1 = glm::vec3(triangle.v0_and_e1x.w, triangle.e1yz_and_e2xy.x, triangle.e1yz_and_e2xy.y);
        e2 = glm::vec3(triangle.e1yz_and_e2xy.z, triangle.e1yz_and_e2xy.w, triangle.e2z_and_normal.x);
        normal = glm::vec3(triangle.e2z_and_normal.y, triangle.e2z_and_normal.z, triangle.e2z_and_normal.w);
    }

    BvhRayResult CreateMissResult(float maxDistance) {
        BvhRayResult rayResult;
        rayResult.hitFound = false;
        rayResult.distanceToHit = maxDistance;
        return rayResult;
    }

    BvhRayResult MeshAnyHit(const SceneBvh& sceneBvh, const GpuPrimitiveInstance& instance, glm::vec3 rayOrigin, glm::vec3 rayDir, float maxDistance) {
        BvhRayResult rayResult = CreateMissResult(maxDistance);

        if (sceneBvh.m_meshNodes.empty() || sceneBvh.m_triangles.empty() || instance.rootNodeIndex < 0) {
            return rayResult;
        }

        const float globalMinDistance = 0.001f;
        const glm::vec3 localOrigin = glm::vec3(instance.inverseWorldTransform * glm::vec4(rayOrigin, 1.0f));
        const glm::vec3 localEnd = glm::vec3(instance.inverseWorldTransform * glm::vec4(rayOrigin + rayDir * maxDistance, 1.0f));
        const glm::vec3 localDir = glm::normalize(localEnd - localOrigin);
        const float localMaxDistance = glm::length(localEnd - localOrigin);
        const float localMinDistance = globalMinDistance * localMaxDistance / maxDistance;

        RayData rayData = ComputeRayData(localOrigin, localDir, localMinDistance, localMaxDistance);

        uint32_t stack[GPU_TARGET_MAX_STACK_SIZE];
        size_t currentStackSize = 0;
        stack[currentStackSize++] = static_cast<uint32_t>(instance.rootNodeIndex);

        while (currentStackSize != 0) {
            const uint32_t nodeIndex = stack[--currentStackSize];
            if (nodeIndex >= sceneBvh.m_meshNodes.size()) {
                return rayResult;
            }

            const BvhNode& node = sceneBvh.m_meshNodes[nodeIndex];

            float t = 0.0f;
            if (!IntersectNode(rayData, node.boundsMin, node.boundsMax, t)) {
                continue;
            }

            if (node.primitiveCount > 0) {
                for (uint32_t i = 0; i < node.primitiveCount; ++i) {
                    const uint32_t floatOffset = node.firstChildOrPrimitive + i * 12;
                    const size_t triangleIndex = floatOffset / 12;
                    if (triangleIndex >= sceneBvh.m_triangles.size()) {
                        continue;
                    }

                    glm::vec3 p0, e1, e2, normal;
                    UnpackTriangle(sceneBvh.m_triangles[triangleIndex], p0, e1, e2, normal);

                    float localT = 0.0f;
                    if (!IntersectTri(localOrigin, localDir, localMinDistance, localMaxDistance, p0, e1, e2, localT)) {
                        continue;
                    }

                    const glm::vec3 hitPositionLocal = localOrigin + (localDir * localT);
                    const glm::vec3 hitPositionWorld = instance.worldTransform * glm::vec4(hitPositionLocal, 1.0f);

                    rayResult.hitFound = true;
                    rayResult.hitPosition = hitPositionWorld;
                    rayResult.distanceToHit = glm::length(hitPositionWorld - rayOrigin);
                    rayResult.primtiviveId = floatOffset;
                    rayResult.primitiveTransform = instance.worldTransform;
                    rayResult.nodeBoundsMin = node.boundsMin;
                    rayResult.nodeBoundsMax = node.boundsMax;
                    rayResult.openableId = instance.openableId;
                    rayResult.customId = instance.customId;
                    rayResult.globalMeshIndex = instance.globalMeshIndex;
                    rayResult.localMeshNodeIndex = instance.localMeshNodeIndex;
                    rayResult.hitNormal = TransformNormalToWorldSpace(instance, normal);
                    Hell::Bit::UnpackUint64(instance.objectIdLowerBit, instance.objectIdUpperBit, rayResult.objectId);
                    return rayResult;
                }
            }
            else {
                stack[currentStackSize++] = node.firstChildOrPrimitive + 0;
                stack[currentStackSize++] = node.firstChildOrPrimitive + 1;
            }
        }

        return rayResult;
    }

    BvhRayResult MeshClosestHit(const SceneBvh& sceneBvh, const GpuPrimitiveInstance& instance, glm::vec3 rayOrigin, glm::vec3 rayDir, float maxDistance) {
        BvhRayResult closestRayResult = CreateMissResult(maxDistance);

        if (sceneBvh.m_meshNodes.empty() || sceneBvh.m_triangles.empty() || instance.rootNodeIndex < 0) {
            return closestRayResult;
        }

        const float globalMinDistance = 0.001f;
        const glm::vec3 localOrigin = glm::vec3(instance.inverseWorldTransform * glm::vec4(rayOrigin, 1.0f));
        const glm::vec3 localEnd = glm::vec3(instance.inverseWorldTransform * glm::vec4(rayOrigin + rayDir * maxDistance, 1.0f));
        const glm::vec3 localDir = glm::normalize(localEnd - localOrigin);
        const float localMaxDistance = glm::length(localEnd - localOrigin);
        const float localMinDistance = globalMinDistance * localMaxDistance / maxDistance;

        RayData rayData = ComputeRayData(localOrigin, localDir, localMinDistance, localMaxDistance);

        uint32_t stack[GPU_TARGET_MAX_STACK_SIZE];
        size_t currentStackSize = 0;
        stack[currentStackSize++] = static_cast<uint32_t>(instance.rootNodeIndex);

        while (currentStackSize != 0) {
            const uint32_t nodeIndex = stack[--currentStackSize];
            if (nodeIndex >= sceneBvh.m_meshNodes.size()) {
                return closestRayResult;
            }

            const BvhNode& node = sceneBvh.m_meshNodes[nodeIndex];

            float t = 0.0f;
            if (!IntersectNode(rayData, node.boundsMin, node.boundsMax, t)) {
                continue;
            }

            if (node.primitiveCount > 0) {
                for (uint32_t i = 0; i < node.primitiveCount; ++i) {
                    const uint32_t floatOffset = node.firstChildOrPrimitive + i * 12;
                    const size_t triangleIndex = floatOffset / 12;
                    if (triangleIndex >= sceneBvh.m_triangles.size()) {
                        continue;
                    }

                    glm::vec3 p0, e1, e2, normal;
                    UnpackTriangle(sceneBvh.m_triangles[triangleIndex], p0, e1, e2, normal);

                    float localT = 0.0f;
                    if (!IntersectTri(localOrigin, localDir, rayData.minDistance, rayData.maxDistance, p0, e1, e2, localT)) {
                        continue;
                    }

                    const glm::vec3 hitPositionLocal = localOrigin + (localDir * localT);
                    const glm::vec3 hitPositionWorld = instance.worldTransform * glm::vec4(hitPositionLocal, 1.0f);
                    const float distanceToHit = glm::length(hitPositionWorld - rayOrigin);

                    if (distanceToHit < closestRayResult.distanceToHit) {
                        closestRayResult.hitFound = true;
                        closestRayResult.hitPosition = hitPositionWorld;
                        closestRayResult.distanceToHit = distanceToHit;
                        closestRayResult.primtiviveId = floatOffset;
                        closestRayResult.primitiveTransform = instance.worldTransform;
                        closestRayResult.nodeBoundsMin = node.boundsMin;
                        closestRayResult.nodeBoundsMax = node.boundsMax;
                        closestRayResult.openableId = instance.openableId;
                        closestRayResult.customId = instance.customId;
                        closestRayResult.globalMeshIndex = instance.globalMeshIndex;
                        closestRayResult.localMeshNodeIndex = instance.localMeshNodeIndex;
                        closestRayResult.hitNormal = TransformNormalToWorldSpace(instance, normal);
                        Hell::Bit::UnpackUint64(instance.objectIdLowerBit, instance.objectIdUpperBit, closestRayResult.objectId);

                        rayData.maxDistance = localT;
                    }
                }
            }
            else {
                stack[currentStackSize++] = node.firstChildOrPrimitive + 0;
                stack[currentStackSize++] = node.firstChildOrPrimitive + 1;
            }
        }

        return closestRayResult;
    }
}

bool SceneBvh::SetMeshBvhs(const std::vector<SceneBvhMeshInput>& meshBvhs) {
    m_nodes.clear();
    m_instances.clear();
    m_gpuInstances.clear();
    m_leafInstanceIndices.clear();
    m_refitNodeOrder.clear();

    m_meshBvhIds.clear();
    m_meshRootNodeOffsets.clear();
    m_meshNodes.clear();
    m_triangles.clear();

    return AddMeshBvhs(meshBvhs);
}

bool SceneBvh::AddMeshBvh(uint64_t meshBvhId, const MeshBvh* meshBvh) {
    if (HasMeshBvh(meshBvhId)) {
        return true;
    }

    if (!meshBvh || meshBvh->m_nodes.empty()) {
        Logging::Error() << "SceneBvh::AddMeshBvh(..) failed: invalid mesh BVH id " << meshBvhId << "\n";
        return false;
    }

    const uint32_t rootNodeOffset = static_cast<uint32_t>(m_meshNodes.size());
    const uint32_t baseTriangleFloatOffset = static_cast<uint32_t>(m_triangles.size() * 12);

    m_meshRootNodeOffsets[meshBvhId] = rootNodeOffset;
    m_meshBvhIds.push_back(meshBvhId);
    m_meshNodes.reserve(m_meshNodes.size() + meshBvh->m_nodes.size());
    m_triangles.reserve(m_triangles.size() + meshBvh->m_triangles.size());

    for (const BvhNode& sourceNode : meshBvh->m_nodes) {
        BvhNode& appendedNode = m_meshNodes.emplace_back(sourceNode);

        if (appendedNode.primitiveCount > 0) {
            appendedNode.firstChildOrPrimitive += baseTriangleFloatOffset;
        }
        else {
            appendedNode.firstChildOrPrimitive += rootNodeOffset;
        }
    }

    m_triangles.insert(m_triangles.end(), meshBvh->m_triangles.begin(), meshBvh->m_triangles.end());

    return true;
}

bool SceneBvh::AddMeshBvhs(const std::vector<SceneBvhMeshInput>& meshBvhs) {
    for (const SceneBvhMeshInput& input : meshBvhs) {
        if (!AddMeshBvh(input.meshBvhId, input.meshBvh)) {
            return false;
        }
    }

    return true;
}

bool SceneBvh::CanRefitInstances(const std::vector<PrimitiveInstance>& instances) const {
    if (instances.size() != m_instances.size()) {
        return false;
    }

    if (instances.empty()) {
        return m_nodes.empty();
    }

    if (m_nodes.empty() ||
        m_gpuInstances.size() != instances.size() ||
        m_leafInstanceIndices.size() != instances.size() ||
        m_refitNodeOrder.size() != m_nodes.size()) {
        return false;
    }

    for (size_t i = 0; i < instances.size(); i++) {
        if (!HasSameStructure(m_instances[i], instances[i])) {
            return false;
        }
    }

    return true;
}

bool SceneBvh::BuildRefitNodeOrder() {
    struct NodeVisit {
        uint32_t nodeIndex;
        bool childrenVisited;
    };

    m_refitNodeOrder.clear();
    if (m_nodes.empty()) {
        return true;
    }

    std::vector<NodeVisit> stack;
    stack.reserve(GPU_TARGET_MAX_STACK_SIZE * 2);
    stack.push_back({ 0, false });

    while (!stack.empty()) {
        const NodeVisit visit = stack.back();
        stack.pop_back();

        if (visit.nodeIndex >= m_nodes.size()) {
            Logging::Error() << "SceneBvh::BuildRefitNodeOrder() failed: invalid node index " << visit.nodeIndex << "\n";
            return false;
        }

        const BvhNode& node = m_nodes[visit.nodeIndex];
        if (node.primitiveCount > 0 || visit.childrenVisited) {
            m_refitNodeOrder.push_back(visit.nodeIndex);
            continue;
        }

        const uint32_t leftChildIndex = node.firstChildOrPrimitive;
        const uint32_t rightChildIndex = leftChildIndex + 1;
        if (rightChildIndex >= m_nodes.size()) {
            Logging::Error() << "SceneBvh::BuildRefitNodeOrder() failed: invalid child node index " << rightChildIndex << "\n";
            return false;
        }

        stack.push_back({ visit.nodeIndex, true });
        stack.push_back({ rightChildIndex, false });
        stack.push_back({ leftChildIndex, false });
    }

    return m_refitNodeOrder.size() == m_nodes.size();
}

bool SceneBvh::UpdateInstances(const std::vector<PrimitiveInstance>& instances) {
    if (RefitInstances(instances)) {
        return true;
    }

    return RebuildInstances(instances);
}

bool SceneBvh::RefitInstances(const std::vector<PrimitiveInstance>& instances) {
    if (!CanRefitInstances(instances)) {
        return false;
    }

    if (instances.empty()) {
        m_instances.clear();
        return true;
    }

    for (size_t gpuInstanceIndex = 0; gpuInstanceIndex < m_gpuInstances.size(); gpuInstanceIndex++) {
        const uint32_t instanceIndex = m_leafInstanceIndices[gpuInstanceIndex];
        if (instanceIndex >= instances.size()) {
            Logging::Error() << "SceneBvh::RefitInstances(..) failed: invalid instance index " << instanceIndex << "\n";
            return false;
        }

        const PrimitiveInstance& instance = instances[instanceIndex];
        auto rootNodeOffsetIt = m_meshRootNodeOffsets.find(instance.meshBvhId);
        if (rootNodeOffsetIt == m_meshRootNodeOffsets.end()) {
            Logging::Error() << "SceneBvh::RefitInstances(..) failed: missing root node offset for mesh BVH id " << instance.meshBvhId << "\n";
            return false;
        }

        GpuPrimitiveInstance& gpuInstance = m_gpuInstances[gpuInstanceIndex];
        gpuInstance.worldTransform = instance.worldTransform;
        gpuInstance.inverseWorldTransform = instance.inverseWorldTransform;
        gpuInstance.rootNodeIndex = static_cast<int32_t>(rootNodeOffsetIt->second);
        gpuInstance.openableId = instance.openableId;
        gpuInstance.customId = instance.customId;
        gpuInstance.globalMeshIndex = instance.globalMeshIndex;
        gpuInstance.localMeshNodeIndex = instance.localMeshNodeIndex;
        gpuInstance.padding2 = 0;
        Hell::Bit::PackUint64(instance.objectId, gpuInstance.objectIdLowerBit, gpuInstance.objectIdUpperBit);
    }

    for (uint32_t nodeIndex : m_refitNodeOrder) {
        BvhNode& node = m_nodes[nodeIndex];

        if (node.primitiveCount > 0) {
            const uint32_t firstPrimitive = node.firstChildOrPrimitive;
            const uint32_t primitiveEnd = firstPrimitive + node.primitiveCount;
            if (primitiveEnd > m_leafInstanceIndices.size()) {
                Logging::Error() << "SceneBvh::RefitInstances(..) failed: invalid leaf primitive range\n";
                return false;
            }

            const uint32_t firstInstanceIndex = m_leafInstanceIndices[firstPrimitive];
            if (firstInstanceIndex >= instances.size()) {
                Logging::Error() << "SceneBvh::RefitInstances(..) failed: invalid leaf instance index " << firstInstanceIndex << "\n";
                return false;
            }

            node.boundsMin = instances[firstInstanceIndex].worldAabbBoundsMin;
            node.boundsMax = instances[firstInstanceIndex].worldAabbBoundsMax;

            for (uint32_t primitiveIndex = firstPrimitive + 1; primitiveIndex < primitiveEnd; primitiveIndex++) {
                const uint32_t instanceIndex = m_leafInstanceIndices[primitiveIndex];
                if (instanceIndex >= instances.size()) {
                    Logging::Error() << "SceneBvh::RefitInstances(..) failed: invalid leaf instance index " << instanceIndex << "\n";
                    return false;
                }

                node.boundsMin = glm::min(node.boundsMin, instances[instanceIndex].worldAabbBoundsMin);
                node.boundsMax = glm::max(node.boundsMax, instances[instanceIndex].worldAabbBoundsMax);
            }
        }
        else {
            const uint32_t leftChildIndex = node.firstChildOrPrimitive;
            const uint32_t rightChildIndex = leftChildIndex + 1;
            if (rightChildIndex >= m_nodes.size()) {
                Logging::Error() << "SceneBvh::RefitInstances(..) failed: invalid child node index " << rightChildIndex << "\n";
                return false;
            }

            node.boundsMin = glm::min(m_nodes[leftChildIndex].boundsMin, m_nodes[rightChildIndex].boundsMin);
            node.boundsMax = glm::max(m_nodes[leftChildIndex].boundsMax, m_nodes[rightChildIndex].boundsMax);
        }
    }

    m_instances = instances;
    return true;
}

bool SceneBvh::RebuildInstances(const std::vector<PrimitiveInstance>& instances) {
    for (const PrimitiveInstance& instance : instances) {
        if (!HasMeshBvh(instance.meshBvhId)) {
            Logging::Error() << "SceneBvh::UpdateInstances(..) failed: mesh BVH id " << instance.meshBvhId << " was not included in the scene\n";
            return false;
        }
    }

    m_instances = instances;
    m_gpuInstances.clear();
    m_leafInstanceIndices.clear();
    m_nodes.clear();
    m_refitNodeOrder.clear();

    if (instances.empty()) {
        return true;
    }

    std::vector<MadmannBBox> bboxes(instances.size());
    std::vector<MadmannVec3> centers(instances.size());

    for (size_t i = 0; i < instances.size(); ++i) {
        const PrimitiveInstance& instance = instances[i];
        bboxes[i] = MadmannBBox(
            GlmVec3ToMadmannVec3(instance.worldAabbBoundsMin),
            GlmVec3ToMadmannVec3(instance.worldAabbBoundsMax));
        centers[i] = GlmVec3ToMadmannVec3(instance.worldAabbCenter);
    }

    MadmannBvh bvh = BuildTopLevelBvh(bboxes, centers);

    m_nodes.resize(bvh.nodes.size());

    for (size_t i = 0; i < bvh.nodes.size(); ++i) {
        const MadmannBvhNode& sourceNode = bvh.nodes[i];
        BvhNode& targetNode = m_nodes[i];

        targetNode.boundsMin = glm::vec3(sourceNode.bounds[0], sourceNode.bounds[2], sourceNode.bounds[4]);
        targetNode.boundsMax = glm::vec3(sourceNode.bounds[1], sourceNode.bounds[3], sourceNode.bounds[5]);
        targetNode.primitiveCount = sourceNode.index.value & ((1u << MadmannBvhNode::prim_count_bits) - 1);
        targetNode.firstChildOrPrimitive = sourceNode.index.value >> MadmannBvhNode::prim_count_bits;
    }

    std::vector<uint32_t> stack;
    stack.reserve(GPU_TARGET_MAX_STACK_SIZE);

    if (!m_nodes.empty()) {
        stack.push_back(0);
    }

    m_gpuInstances.reserve(instances.size());
    m_leafInstanceIndices.reserve(instances.size());

    while (!stack.empty()) {
        const uint32_t currentNodeIndex = stack.back();
        stack.pop_back();

        if (currentNodeIndex >= m_nodes.size()) {
            Logging::Error() << "SceneBvh::UpdateInstances(..) failed: invalid scene node index " << currentNodeIndex << "\n";
            return false;
        }

        BvhNode& node = m_nodes[currentNodeIndex];

        if (node.primitiveCount > 0) {
            const uint32_t newPrimitiveIndex = static_cast<uint32_t>(m_gpuInstances.size());

            for (uint32_t i = 0; i < node.primitiveCount; ++i) {
                const uint32_t primitiveId = node.firstChildOrPrimitive + i;
                const uint32_t instanceIndex = static_cast<uint32_t>(bvh.prim_ids[primitiveId]);

                if (instanceIndex >= instances.size()) {
                    Logging::Error() << "SceneBvh::UpdateInstances(..) failed: invalid instance index " << instanceIndex << "\n";
                    return false;
                }

                const PrimitiveInstance& instance = instances[instanceIndex];
                auto rootNodeOffsetIt = m_meshRootNodeOffsets.find(instance.meshBvhId);

                if (rootNodeOffsetIt == m_meshRootNodeOffsets.end()) {
                    Logging::Error() << "SceneBvh::UpdateInstances(..) failed: missing root node offset for mesh BVH id " << instance.meshBvhId << "\n";
                    return false;
                }

                GpuPrimitiveInstance& gpuInstance = m_gpuInstances.emplace_back();
                gpuInstance.worldTransform = instance.worldTransform;
                gpuInstance.inverseWorldTransform = instance.inverseWorldTransform;
                gpuInstance.rootNodeIndex = static_cast<int32_t>(rootNodeOffsetIt->second);
                gpuInstance.openableId = instance.openableId;
                gpuInstance.customId = instance.customId;
                gpuInstance.globalMeshIndex = instance.globalMeshIndex;
                gpuInstance.localMeshNodeIndex = instance.localMeshNodeIndex;
                gpuInstance.padding2 = 0;
                Hell::Bit::PackUint64(instance.objectId, gpuInstance.objectIdLowerBit, gpuInstance.objectIdUpperBit);

                m_leafInstanceIndices.push_back(instanceIndex);
            }

            node.firstChildOrPrimitive = newPrimitiveIndex;
        }
        else {
            stack.push_back(node.firstChildOrPrimitive + 0);
            stack.push_back(node.firstChildOrPrimitive + 1);
        }
    }

    return BuildRefitNodeOrder();
}

bool SceneBvh::HasMeshBvh(uint64_t meshBvhId) const {
    return m_meshRootNodeOffsets.find(meshBvhId) != m_meshRootNodeOffsets.end();
}

BvhRayResult SceneBvh::AnyHit(glm::vec3 rayOrigin, glm::vec3 rayDir, float maxDistance) const {
    BvhRayResult rayResult = CreateMissResult(maxDistance);

    if (Hell::Math::IsNan(rayOrigin) || Hell::Math::IsNan(rayDir) || m_nodes.empty() || m_gpuInstances.empty()) {
        return rayResult;
    }

    RayData rayData = ComputeRayData(rayOrigin, rayDir, 0.0001f, maxDistance);

    uint32_t stack[GPU_TARGET_MAX_STACK_SIZE];
    size_t currentStackSize = 0;
    stack[currentStackSize++] = 0;

    while (currentStackSize != 0) {
        const uint32_t nodeIndex = stack[--currentStackSize];
        if (nodeIndex >= m_nodes.size()) {
            return rayResult;
        }

        const BvhNode& node = m_nodes[nodeIndex];

        float t = 0.0f;
        if (!IntersectNode(rayData, node.boundsMin, node.boundsMax, t)) {
            continue;
        }

        if (node.primitiveCount > 0) {
            for (uint32_t i = 0; i < node.primitiveCount; ++i) {
                const uint32_t instanceIndex = node.firstChildOrPrimitive + i;
                if (instanceIndex >= m_gpuInstances.size()) {
                    continue;
                }

                BvhRayResult localRayResult = MeshAnyHit(*this, m_gpuInstances[instanceIndex], rayOrigin, rayDir, maxDistance);
                if (localRayResult.hitFound) {
                    return localRayResult;
                }
            }
        }
        else {
            stack[currentStackSize++] = node.firstChildOrPrimitive + 0;
            stack[currentStackSize++] = node.firstChildOrPrimitive + 1;
        }
    }

    return rayResult;
}

BvhRayResult SceneBvh::ClosestHit(glm::vec3 rayOrigin, glm::vec3 rayDir, float maxDistance) const {
    BvhRayResult rayResult = CreateMissResult(maxDistance);

    if (Hell::Math::IsNan(rayOrigin) || Hell::Math::IsNan(rayDir) || m_nodes.empty() || m_gpuInstances.empty()) {
        return rayResult;
    }

    RayData rayData = ComputeRayData(rayOrigin, rayDir, 0.0001f, maxDistance);

    uint32_t stack[GPU_TARGET_MAX_STACK_SIZE];
    size_t currentStackSize = 0;
    stack[currentStackSize++] = 0;

    while (currentStackSize != 0) {
        const uint32_t nodeIndex = stack[--currentStackSize];
        if (nodeIndex >= m_nodes.size()) {
            return rayResult;
        }

        const BvhNode& node = m_nodes[nodeIndex];

        float t = 0.0f;
        if (!IntersectNode(rayData, node.boundsMin, node.boundsMax, t)) {
            continue;
        }

        if (t >= rayData.maxDistance) {
            continue;
        }

        if (node.primitiveCount > 0) {
            for (uint32_t i = 0; i < node.primitiveCount; ++i) {
                const uint32_t instanceIndex = node.firstChildOrPrimitive + i;
                if (instanceIndex >= m_gpuInstances.size()) {
                    continue;
                }

                BvhRayResult localRayResult = MeshClosestHit(*this, m_gpuInstances[instanceIndex], rayOrigin, rayDir, rayResult.distanceToHit);
                if (localRayResult.hitFound && localRayResult.distanceToHit < rayResult.distanceToHit) {
                    rayResult = localRayResult;
                    rayData.maxDistance = rayResult.distanceToHit;
                }
            }
        }
        else {
            stack[currentStackSize++] = node.firstChildOrPrimitive + 0;
            stack[currentStackSize++] = node.firstChildOrPrimitive + 1;
        }
    }

    return rayResult;
}
