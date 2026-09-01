#include "CoarseWorldBVHGeometry.h"

#include "Hell/Math/Transform.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/ResourceManagement/Types/Material.h"

#include "Unloved/Common/Constants.h"
#include "Unloved/Objects/House/Door.h"
#include "Unloved/Objects/House/Wall.h"
#include "Unloved/Objects/House/WorldPlane.h"
#include "Unloved/World/World.h"

#include <limits>
#include <utility>

namespace Unloved::CoarseWorldBVH {
    namespace {
        struct GeometryBounds {
            glm::vec3 min = glm::vec3(0.0f);
            glm::vec3 max = glm::vec3(0.0f);
            bool isBounded = false;
        };
    }

    constexpr float DOOR_PROXY_PADDING_POS_X = 0.01f;
    constexpr float DOOR_PROXY_PADDING_POS_Y = 0.03f;
    constexpr float DOOR_PROXY_PADDING_POS_Z = 0.02f;
    constexpr float DOOR_PROXY_PADDING_NEG_X = 0.08f;
    constexpr float DOOR_PROXY_PADDING_NEG_Y = 0.03f;
    constexpr float DOOR_PROXY_PADDING_NEG_Z = 0.02f;

    AABB CalculateDoorProxyWorldAABB(const glm::mat4& worldTransform) {
        const glm::vec3 localBoundsMin(-DOOR_DEPTH - DOOR_PROXY_PADDING_NEG_X, -DOOR_PROXY_PADDING_NEG_Y, -DOOR_WIDTH - DOOR_PROXY_PADDING_NEG_Z);
        const glm::vec3 localBoundsMax(DOOR_PROXY_PADDING_POS_X, DOOR_HEIGHT + DOOR_PROXY_PADDING_POS_Y, DOOR_PROXY_PADDING_POS_Z);
        glm::vec3 worldBoundsMin(std::numeric_limits<float>::max());
        glm::vec3 worldBoundsMax(std::numeric_limits<float>::lowest());

        for (uint32_t cornerIndex = 0; cornerIndex < 8; cornerIndex++) {
            const glm::vec3 localCorner((cornerIndex & 1) ? localBoundsMax.x : localBoundsMin.x, (cornerIndex & 2) ? localBoundsMax.y : localBoundsMin.y, (cornerIndex & 4) ? localBoundsMax.z : localBoundsMin.z);
            const glm::vec3 worldCorner = glm::vec3(worldTransform * glm::vec4(localCorner, 1.0f));
            worldBoundsMin = glm::min(worldBoundsMin, worldCorner);
            worldBoundsMax = glm::max(worldBoundsMax, worldCorner);
        }

        return AABB(worldBoundsMin, worldBoundsMax);
    }

    bool TriangleIntersectsBounds(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const GeometryBounds& bounds) {
        if (!bounds.isBounded) {
            return true;
        }

        const glm::vec3 triangleBoundsMin = glm::min(glm::min(p0, p1), p2);
        const glm::vec3 triangleBoundsMax = glm::max(glm::max(p0, p1), p2);

        return
            triangleBoundsMax.x >= bounds.min.x && triangleBoundsMin.x <= bounds.max.x &&
            triangleBoundsMax.y >= bounds.min.y && triangleBoundsMin.y <= bounds.max.y &&
            triangleBoundsMax.z >= bounds.min.z && triangleBoundsMin.z <= bounds.max.z;
    }

    void AppendSurfaceTriangle(std::vector<SurfaceTriangle>& triangles, const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec2& uv0, const glm::vec2& uv1, const glm::vec2& uv2, int baseColorTextureIndex, int rmaTextureIndex) {
        const glm::vec3 edge1 = p1 - p0;
        const glm::vec3 edge2 = p2 - p0;
        const glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

        SurfaceTriangle& triangle = triangles.emplace_back();
        triangle.v0 = p0;
        triangle.v1 = p1;
        triangle.v2 = p2;
        triangle.uv0 = uv0;
        triangle.uv1 = uv1;
        triangle.uv2 = uv2;
        triangle.normal = normal;
        triangle.baseColorTextureIndex = baseColorTextureIndex;
        triangle.rmaTextureIndex = rmaTextureIndex;
    }

    void AppendSurfaceTriangleIfInsideBounds(std::vector<SurfaceTriangle>& triangles, const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec2& uv0, const glm::vec2& uv1, const glm::vec2& uv2, int baseColorTextureIndex, int rmaTextureIndex, const GeometryBounds& bounds) {
        if (!TriangleIntersectsBounds(p0, p1, p2, bounds)) {
            return;
        }

        AppendSurfaceTriangle(triangles, p0, p1, p2, uv0, uv1, uv2, baseColorTextureIndex, rmaTextureIndex);
    }

    void CollectWorldPlaneSurfaceTriangles(std::vector<SurfaceTriangle>& triangles, const GeometryBounds& bounds) {
        for (WorldPlane& plane : Unloved::World::GetWorldPlanes()) {
            if (plane.GetParentDoorId() != 0) {
                continue;
            }

            const std::vector<Vertex>& planeVertices = plane.GetVertices();
            const std::vector<uint32_t>& planeIndices = plane.GetIndices();
            Material* material = plane.GetMaterial();
            const int baseColorTextureIndex = material ? material->m_basecolor : -1;
            const int rmaTextureIndex = material ? material->m_rma : -1;

            for (uint32_t i = 0; i + 2 < planeIndices.size(); i += 3) {
                const uint32_t idx0 = planeIndices[i + 0];
                const uint32_t idx1 = planeIndices[i + 1];
                const uint32_t idx2 = planeIndices[i + 2];

                if (idx0 >= planeVertices.size() || idx1 >= planeVertices.size() || idx2 >= planeVertices.size()) {
                    continue;
                }

                AppendSurfaceTriangleIfInsideBounds(triangles, planeVertices[idx0].position, planeVertices[idx1].position, planeVertices[idx2].position, planeVertices[idx0].uv, planeVertices[idx1].uv, planeVertices[idx2].uv, baseColorTextureIndex, rmaTextureIndex, bounds);
            }
        }
    }

    void CollectWallSurfaceTriangles(std::vector<SurfaceTriangle>& triangles, const GeometryBounds& bounds) {
        for (Wall& wall : Unloved::World::GetWalls()) {
            Material* material = wall.GetMaterial();
            const int baseColorTextureIndex = material ? material->m_basecolor : -1;
            const int rmaTextureIndex = material ? material->m_rma : -1;

            for (WallSegment& wallSegment : wall.GetWallSegments()) {
                const std::vector<Vertex>& wallVertices = wallSegment.GetVertices();
                const std::vector<uint32_t>& wallIndices = wallSegment.GetIndices();

                for (uint32_t i = 0; i + 2 < wallIndices.size(); i += 3) {
                    const uint32_t idx0 = wallIndices[i + 0];
                    const uint32_t idx1 = wallIndices[i + 1];
                    const uint32_t idx2 = wallIndices[i + 2];

                    if (idx0 >= wallVertices.size() || idx1 >= wallVertices.size() || idx2 >= wallVertices.size()) {
                        continue;
                    }

                    AppendSurfaceTriangleIfInsideBounds(triangles, wallVertices[idx0].position, wallVertices[idx1].position, wallVertices[idx2].position, wallVertices[idx0].uv, wallVertices[idx1].uv, wallVertices[idx2].uv, baseColorTextureIndex, rmaTextureIndex, bounds);
                }
            }
        }
    }

    void CollectDoorGapSurfaceTriangles(std::vector<SurfaceTriangle>& triangles, const GeometryBounds& bounds) {
        Material* material = Hell::ResourceManager::GetMaterialByName("Ceiling2");
        const int baseColorTextureIndex = material ? material->m_basecolor : -1;
        const int rmaTextureIndex = material ? material->m_rma : -1;

        for (Door& door : Unloved::World::GetDoors()) {
            Hell::Transform transform;
            transform.position = door.GetPosition();
            transform.rotation = door.GetRotation();
            const glm::mat4 modelMatrix = transform.to_mat4();

            const float padding = 0.02f;
            const float halfP = padding * 0.5f;
            const float halfD = DOOR_WIDTH * 0.5f + halfP;
            const float h = DOOR_HEIGHT + halfP;
            const float halfW = 0.05f;

            glm::vec3 p[8];
            p[0] = glm::vec3(halfW, 0.0f, halfD);
            p[1] = glm::vec3(-halfW, 0.0f, halfD);
            p[2] = glm::vec3(-halfW, h, halfD);
            p[3] = glm::vec3(halfW, h, halfD);
            p[4] = glm::vec3(halfW, 0.0f, -halfD);
            p[5] = glm::vec3(-halfW, 0.0f, -halfD);
            p[6] = glm::vec3(-halfW, h, -halfD);
            p[7] = glm::vec3(halfW, h, -halfD);

            for (int i = 0; i < 8; ++i) {
                p[i] = glm::vec3(modelMatrix * glm::vec4(p[i], 1.0f));
            }

            const glm::vec2 uv0 = glm::vec2(0.0f, 0.0f);
            const glm::vec2 uv1 = glm::vec2(1.0f, 0.0f);
            const glm::vec2 uv2 = glm::vec2(1.0f, 1.0f);
            const glm::vec2 uv3 = glm::vec2(0.0f, 1.0f);

            AppendSurfaceTriangleIfInsideBounds(triangles, p[0], p[1], p[2], uv0, uv1, uv2, baseColorTextureIndex, rmaTextureIndex, bounds);
            AppendSurfaceTriangleIfInsideBounds(triangles, p[2], p[3], p[0], uv2, uv3, uv0, baseColorTextureIndex, rmaTextureIndex, bounds);

            AppendSurfaceTriangleIfInsideBounds(triangles, p[5], p[4], p[7], uv0, uv1, uv2, baseColorTextureIndex, rmaTextureIndex, bounds);
            AppendSurfaceTriangleIfInsideBounds(triangles, p[7], p[6], p[5], uv2, uv3, uv0, baseColorTextureIndex, rmaTextureIndex, bounds);

            AppendSurfaceTriangleIfInsideBounds(triangles, p[3], p[2], p[6], uv0, uv1, uv2, baseColorTextureIndex, rmaTextureIndex, bounds);
            AppendSurfaceTriangleIfInsideBounds(triangles, p[6], p[7], p[3], uv2, uv3, uv0, baseColorTextureIndex, rmaTextureIndex, bounds);

            AppendSurfaceTriangleIfInsideBounds(triangles, p[1], p[0], p[4], uv0, uv1, uv2, baseColorTextureIndex, rmaTextureIndex, bounds);
            AppendSurfaceTriangleIfInsideBounds(triangles, p[4], p[5], p[1], uv2, uv3, uv0, baseColorTextureIndex, rmaTextureIndex, bounds);
        }
    }

    void CollectHouseSurfaceTrianglesInternal(const GeometryBounds& bounds, std::vector<SurfaceTriangle>& triangles) {
        triangles.clear();

        CollectWorldPlaneSurfaceTriangles(triangles, bounds);
        CollectWallSurfaceTriangles(triangles, bounds);
        CollectDoorGapSurfaceTriangles(triangles, bounds);
    }

    void CollectHouseSurfaceTriangles(const glm::vec3& boundsMin, const glm::vec3& boundsMax, std::vector<SurfaceTriangle>& triangles) {
        CollectHouseSurfaceTrianglesInternal({ boundsMin, boundsMax, true }, triangles);
    }

    HouseGeometry BuildHouseGeometryInternal(const GeometryBounds& bounds) {
        HouseGeometry geometry;
        CollectHouseSurfaceTrianglesInternal(bounds, geometry.surfaceTriangles);

        geometry.vertices.reserve(geometry.surfaceTriangles.size() * 3);
        geometry.indices.reserve(geometry.surfaceTriangles.size() * 3);

        for (const SurfaceTriangle& triangle : geometry.surfaceTriangles) {
            const uint32_t baseIndex = static_cast<uint32_t>(geometry.vertices.size());

            Vertex v0 = {};
            Vertex v1 = {};
            Vertex v2 = {};
            v0.position = triangle.v0;
            v1.position = triangle.v1;
            v2.position = triangle.v2;
            v0.normal = triangle.normal;
            v1.normal = triangle.normal;
            v2.normal = triangle.normal;
            v0.uv = triangle.uv0;
            v1.uv = triangle.uv1;
            v2.uv = triangle.uv2;

            geometry.vertices.push_back(v0);
            geometry.vertices.push_back(v1);
            geometry.vertices.push_back(v2);

            geometry.indices.push_back(baseIndex + 0);
            geometry.indices.push_back(baseIndex + 1);
            geometry.indices.push_back(baseIndex + 2);
        }

        return geometry;
    }

    HouseGeometry BuildHouseGeometry(const glm::vec3& boundsMin, const glm::vec3& boundsMax) {
        return BuildHouseGeometryInternal({ boundsMin, boundsMax, true });
    }

    void BuildHouseMesh(const glm::vec3& boundsMin, const glm::vec3& boundsMax, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {
        HouseGeometry geometry = BuildHouseGeometry(boundsMin, boundsMax);
        vertices = std::move(geometry.vertices);
        indices = std::move(geometry.indices);
    }

    void BuildHouseMesh(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {
        HouseGeometry geometry = BuildHouseGeometryInternal({});
        vertices = std::move(geometry.vertices);
        indices = std::move(geometry.indices);
    }

    void CollectDoorProxyInstancesInternal(const GeometryBounds& bounds, std::vector<DoorProxyInstance>& instances) {
        instances.clear();

        auto& doors = Unloved::World::GetDoors();
        instances.reserve(doors.size());

        for (Door& door : doors) {
            MeshNode* meshNode = door.GetMeshNodes().GetMeshNodeByMeshName("Door_Hinges");
            if (!meshNode) continue;

            glm::mat4 worldTransform = meshNode->worldMatrix;
            worldTransform[3][1] = door.GetDoorModelMatrix()[3][1];
            const AABB doorAabb = CalculateDoorProxyWorldAABB(worldTransform);
            if (bounds.isBounded && !doorAabb.IntersectsAABB(bounds.min, bounds.max)) continue;

            DoorProxyInstance& instance = instances.emplace_back();
            instance.objectId = door.GetObjectId();
            instance.worldAabb = doorAabb;
            instance.worldTransform = worldTransform;
        }
    }

    std::vector<DoorProxyInstance> CollectDoorProxyInstances(const glm::vec3& boundsMin, const glm::vec3& boundsMax) {
        std::vector<DoorProxyInstance> instances;
        CollectDoorProxyInstancesInternal({ boundsMin, boundsMax, true }, instances);
        return instances;
    }

    void CollectDoorProxyInstances(std::vector<DoorProxyInstance>& instances) {
        CollectDoorProxyInstancesInternal({}, instances);
    }

    void BuildDoorProxyMesh(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {
        const float w = DOOR_DEPTH;
        const float h = DOOR_HEIGHT;
        const float d = DOOR_WIDTH;

        const glm::vec3 p0 = glm::vec3(0 + DOOR_PROXY_PADDING_POS_X, 0 - DOOR_PROXY_PADDING_NEG_Y, 0 + DOOR_PROXY_PADDING_POS_Z);
        const glm::vec3 p1 = glm::vec3(-w - DOOR_PROXY_PADDING_NEG_X, 0 - DOOR_PROXY_PADDING_NEG_Y, 0 + DOOR_PROXY_PADDING_POS_Z);
        const glm::vec3 p2 = glm::vec3(-w - DOOR_PROXY_PADDING_NEG_X, h + DOOR_PROXY_PADDING_POS_Y, 0 + DOOR_PROXY_PADDING_POS_Z);
        const glm::vec3 p3 = glm::vec3(0 + DOOR_PROXY_PADDING_POS_X, h + DOOR_PROXY_PADDING_POS_Y, 0 + DOOR_PROXY_PADDING_POS_Z);
        const glm::vec3 p4 = glm::vec3(0 + DOOR_PROXY_PADDING_POS_X, 0 - DOOR_PROXY_PADDING_NEG_Y, -d - DOOR_PROXY_PADDING_NEG_Z);
        const glm::vec3 p5 = glm::vec3(-w - DOOR_PROXY_PADDING_NEG_X, 0 - DOOR_PROXY_PADDING_NEG_Y, -d - DOOR_PROXY_PADDING_NEG_Z);
        const glm::vec3 p6 = glm::vec3(-w - DOOR_PROXY_PADDING_NEG_X, h + DOOR_PROXY_PADDING_POS_Y, -d - DOOR_PROXY_PADDING_NEG_Z);
        const glm::vec3 p7 = glm::vec3(0 + DOOR_PROXY_PADDING_POS_X, h + DOOR_PROXY_PADDING_POS_Y, -d - DOOR_PROXY_PADDING_NEG_Z);

        vertices.clear();
        vertices.reserve(24);

        vertices.emplace_back(Vertex(p0, glm::vec3(0, 0, 1)));
        vertices.emplace_back(Vertex(p3, glm::vec3(0, 0, 1)));
        vertices.emplace_back(Vertex(p2, glm::vec3(0, 0, 1)));
        vertices.emplace_back(Vertex(p1, glm::vec3(0, 0, 1)));

        vertices.emplace_back(Vertex(p5, glm::vec3(0, 0, -1)));
        vertices.emplace_back(Vertex(p6, glm::vec3(0, 0, -1)));
        vertices.emplace_back(Vertex(p7, glm::vec3(0, 0, -1)));
        vertices.emplace_back(Vertex(p4, glm::vec3(0, 0, -1)));

        vertices.emplace_back(Vertex(p1, glm::vec3(-1, 0, 0)));
        vertices.emplace_back(Vertex(p2, glm::vec3(-1, 0, 0)));
        vertices.emplace_back(Vertex(p6, glm::vec3(-1, 0, 0)));
        vertices.emplace_back(Vertex(p5, glm::vec3(-1, 0, 0)));

        vertices.emplace_back(Vertex(p4, glm::vec3(1, 0, 0)));
        vertices.emplace_back(Vertex(p7, glm::vec3(1, 0, 0)));
        vertices.emplace_back(Vertex(p3, glm::vec3(1, 0, 0)));
        vertices.emplace_back(Vertex(p0, glm::vec3(1, 0, 0)));

        vertices.emplace_back(Vertex(p3, glm::vec3(0, 1, 0)));
        vertices.emplace_back(Vertex(p7, glm::vec3(0, 1, 0)));
        vertices.emplace_back(Vertex(p6, glm::vec3(0, 1, 0)));
        vertices.emplace_back(Vertex(p2, glm::vec3(0, 1, 0)));

        vertices.emplace_back(Vertex(p1, glm::vec3(0, -1, 0)));
        vertices.emplace_back(Vertex(p5, glm::vec3(0, -1, 0)));
        vertices.emplace_back(Vertex(p4, glm::vec3(0, -1, 0)));
        vertices.emplace_back(Vertex(p0, glm::vec3(0, -1, 0)));

        indices = {
            0, 1, 2, 2, 3, 0,
            4, 5, 6, 6, 7, 4,
            8, 9, 10, 10, 11, 8,
            12, 13, 14, 14, 15, 12,
            16, 17, 18, 18, 19, 16,
            20, 21, 22, 22, 23, 20
        };
    }

}
