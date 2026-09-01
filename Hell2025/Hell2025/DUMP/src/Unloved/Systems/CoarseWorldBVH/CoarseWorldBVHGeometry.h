#pragma once

#include "Hell/Math/AABB.h"
#include "Hell/Render/VertexAttributes.h"

#include <glm/glm.hpp>
#include <cstdint>
#include <vector>

namespace Unloved::CoarseWorldBVH {

    struct DoorProxyInstance {
        uint64_t objectId = 0;
        AABB worldAabb;
        glm::mat4 worldTransform = glm::mat4(1.0f);
    };

    struct SurfaceTriangle {
        glm::vec3 v0 = glm::vec3(0.0f);
        glm::vec3 v1 = glm::vec3(0.0f);
        glm::vec3 v2 = glm::vec3(0.0f);
        glm::vec2 uv0 = glm::vec2(0.0f);
        glm::vec2 uv1 = glm::vec2(0.0f);
        glm::vec2 uv2 = glm::vec2(0.0f);
        glm::vec3 normal = glm::vec3(0.0f);
        int baseColorTextureIndex = -1;
        int rmaTextureIndex = -1;
    };

    struct HouseGeometry {
        std::vector<SurfaceTriangle> surfaceTriangles;
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
    };

    void CollectHouseSurfaceTriangles(const glm::vec3& boundsMin, const glm::vec3& boundsMax, std::vector<SurfaceTriangle>& triangles);
    HouseGeometry BuildHouseGeometry(const glm::vec3& boundsMin, const glm::vec3& boundsMax);
    void BuildHouseMesh(const glm::vec3& boundsMin, const glm::vec3& boundsMax, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);
    void BuildHouseMesh(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);

    std::vector<DoorProxyInstance> CollectDoorProxyInstances(const glm::vec3& boundsMin, const glm::vec3& boundsMax);
    void CollectDoorProxyInstances(std::vector<DoorProxyInstance>& instances);
    void BuildDoorProxyMesh(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);
}
