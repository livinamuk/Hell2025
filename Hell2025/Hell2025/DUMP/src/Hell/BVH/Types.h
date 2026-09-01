#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <cstddef>
#include <cstdint>
#include <limits>

#pragma pack(push, 1)
struct BvhNode {
    glm::vec3 boundsMin;
    uint32_t firstChildOrPrimitive;
    glm::vec3 boundsMax;
    uint32_t primitiveCount;
};
#pragma pack(pop)

struct BVHTriangle {
    glm::vec4 v0_and_e1x;     // p0.xyz, e1.x
    glm::vec4 e1yz_and_e2xy;  // e1.yz, e2.xy
    glm::vec4 e2z_and_normal; // e2.z, normal.xyz
};

static_assert(sizeof(BVHTriangle) == sizeof(float) * 12);
static_assert(offsetof(BVHTriangle, v0_and_e1x) == 0);
static_assert(offsetof(BVHTriangle, e1yz_and_e2xy) == sizeof(float) * 4);
static_assert(offsetof(BVHTriangle, e2z_and_normal) == sizeof(float) * 8);

struct RayData {
    float origin[3];
    float dir[3];
    float invDir[3];
    float paddedInvDir[3];
    float minDistance = 0;
    float maxDistance = 0;
    int octant[3];
};

struct PrimitiveInstance {
    uint64_t objectId;
    uint64_t meshBvhId;
    glm::vec3 worldAabbBoundsMin;
    glm::vec3 worldAabbBoundsMax;
    glm::vec3 worldAabbCenter;
    glm::mat4 worldTransform;
    glm::mat4 inverseWorldTransform;
    uint32_t openableId;
    uint32_t customId;
    uint32_t globalMeshIndex;
    uint32_t localMeshNodeIndex;
};

struct GpuPrimitiveInstance {
    glm::mat4 worldTransform;
    glm::mat4 inverseWorldTransform;

    int32_t rootNodeIndex;
    uint32_t objectIdLowerBit;
    uint32_t objectIdUpperBit;
    uint32_t openableId;

    uint32_t globalMeshIndex;
    uint32_t customId;
    uint32_t localMeshNodeIndex;
    uint32_t padding2;
};

struct BvhRayResult {
    bool hitFound = false;
    size_t primtiviveId = 0;
    uint64_t objectId = 0;
    uint32_t openableId = 0;
    uint32_t customId = 0;
    uint32_t globalMeshIndex = 0;
    uint32_t localMeshNodeIndex = 0;
    float distanceToHit = std::numeric_limits<float>::max();
    glm::vec3 hitPosition = glm::vec3(0);
    glm::vec3 hitNormal = glm::vec3(0.0f);
    glm::mat4 primitiveTransform = glm::mat4(1.0f);
    glm::vec3 nodeBoundsMin = glm::vec3(0.0f);
    glm::vec3 nodeBoundsMax = glm::vec3(0.0f);
};

#include "Hell/BVH/Types/MeshBvh.h"
#include "Hell/BVH/Types/SceneBvh.h"
