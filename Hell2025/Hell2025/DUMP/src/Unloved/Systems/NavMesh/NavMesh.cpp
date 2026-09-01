#include "NavMesh.h"

#include "Hell/Containers/SlotMap.h"
#include "Hell/Logging.h"
#include "Hell/Math/Math.h"

#include "Unloved/Debug/DebugDraw.h"
#include "Unloved/Render/Renderer.h"
#include "Legacy/Timer.hpp"

#include "Unloved/ObjectId.h"
#include "Unloved/Objects/House/Door.h"
#include "Unloved/Objects/House/Fireplace.h"
#include "Unloved/Objects/House/WorldPlane.h"
#include "Unloved/Objects/Interior/Piano.h"
#include "Unloved/Objects/Props/GenericObject.h"
#include "Unloved/Objects/Renderables/MeshNodes.h"
#include "Unloved/Session/Session.h"
#include "Unloved/World/World.h"

#include "clipper2/clipper.h"
#include "earcut/earcut.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>
#include <unordered_map>
#include <vector>

#define NAV_MESH_PROFILING 0

#if NAV_MESH_PROFILING
#define NAV_MESH_TIMER(name) Timer timer__(name)
#else
#define NAV_MESH_TIMER(name) ((void)0)
#endif

namespace mapbox {
    namespace util {
        template <std::size_t I>
        struct nth<I, glm::vec2> {
            static_assert(I < 2, "Index out of range for glm::vec2");

            inline static float get(const glm::vec2& t) {
                return (I == 0) ? t.x : t.y;
            }
        };
    }
}

namespace Unloved::NavMeshManager {

    struct LevelInfo {
        Clipper2Lib::PathsD staticPaths;
        Clipper2Lib::PathsD staticObstaclePaths;
        Clipper2Lib::PathsD dynamicObstaclePaths;

        Clipper2Lib::PathsD solutionStatic;
        Clipper2Lib::PathsD solutionDynamic;
        Clipper2Lib::PathsD solutionFinal;

        float y = 0.0f;
    };

    struct Portal {
        glm::vec3 left;
        glm::vec3 right;
    };

    struct EdgeValue {
        int triIndex;
        int edgeIndex; // 0, 1, or 2
    };

    struct EdgeRef {
        glm::vec3 v0;
        glm::vec3 v1;
        int triIndex;
        int edgeIndex;
    };

    struct EarcutPolygon {
        std::vector<std::vector<glm::vec2>> rings; // rings[0] = outer, rings[1..] = holes
    };

    struct NavMesh {
        NavMesh() = default;
        NavMesh(uint64_t id, float agentRadius);
        void Update();
        void Reset();
        void MarkStaticDirty() {
            m_staticPathsDirty = true;
            m_dynamicPathsDirty = true;
        }
        void MarkDynamicDirty() { m_dynamicPathsDirty = true; }
        void DrawTris();
        std::vector<glm::vec3> FindPath(const glm::vec3& start, const glm::vec3& dest);
        std::vector<glm::vec3> PullPath(const std::vector<glm::vec3>& path);

    private:
        uint64_t m_id = 0;
        bool m_staticPathsDirty = true;
        bool m_dynamicPathsDirty = true;
        float m_agentRadius = 0.0f;
        std::unordered_map<int, LevelInfo> m_levelInfo; // mapped to y height (multiplied by 1000 as integer)
        std::vector<EdgeRef> m_edgeList;                // Used for triangle neighbor building
        std::vector<NavTri> m_tris;                     // The actual nav mesh
        glm::vec2 m_boundsMin = glm::vec2(0);
        glm::vec2 m_boundsMax = glm::vec2(0);

        float m_gridCellSize = 2.0f;                    // Size of one square in the grid
        float m_invCellSize = 1.0f / m_gridCellSize;
        int m_gridWidth = 0;                            // How many cells wide (X axis)
        int m_gridHeight = 0;                           // How many cells tall (Z axis)
        glm::vec2 m_gridOrigin = glm::vec2(0);          // The World Space coordinate of the grid's bottom left corner
        std::vector<std::vector<int>> m_grid;           // Flat vector representing a 2D array

        // Permanent scratch memory
        std::vector<EarcutPolygon> m_tempPolys;
        std::vector<std::vector<glm::vec2>> m_tempEarcutInput;
        std::vector<glm::vec2> m_tempVerts2D;
        std::vector<uint32_t> m_tempIndices;

        void AddMeshNodeToPath(const MeshNode& meshNode, Clipper2Lib::PathsD LevelInfo::* member);
        void AddTri(const glm::vec3 a, const glm::vec3 b, const glm::vec3 c);
        void UpdateStaticPaths();
        void UpdateDynamicPaths();
        void UpdateNavMesh();

        void RemoveTJunctions();
        void BuildNeighbors();
        void BuildSpatialGrid();

        void DebugDrawAdjacentTris();
        int FindContainingTriIndex(const glm::vec3& p);
        int GetGridIndex(const glm::vec3& p);
        std::vector<glm::vec3> Funnel(const std::vector<Portal>& portals);
        std::vector<Portal> BuildPortalsFromCenters(const std::vector<glm::vec3>& path, const std::vector<int>& triIndices);
    };

    Hell::SlotMap<NavMesh> g_navMeshes;
    uint64_t g_testNaveMeshID = 0;

    glm::vec3 g_destination = glm::vec3(666.0f);

    // Util
    int CreatePathKeyFromHeightY(float y);
    void BuildEarcutPolygons(const Clipper2Lib::PathsD& paths, std::vector<EarcutPolygon>& outPolys);
    NavMesh* GetNavMeshById(int64_t id);
    std::vector<glm::vec2> ComputeConvexHull2D(std::vector<glm::vec2>& points);
    Clipper2Lib::PathD ConvertToClipperPath(const std::vector<glm::vec2>& points);
    Clipper2Lib::PathsD DeflatePaths(const Clipper2Lib::PathsD& paths, float agentRadius);

    // Math
    glm::vec2 ToXZ(const glm::vec3& p);
    glm::vec3 RoundVertex(const glm::vec3& v);
    bool EdgesMatch(const glm::vec3& a0, const glm::vec3& a1, const glm::vec3& b0, const glm::vec3& b1, float epsilon = 0.0001f);
    bool PointInNavTriXZ(const NavTri& tri, const glm::vec3& position);
    bool PointInPolygon2D(const glm::vec2& p, const std::vector<glm::vec2>& poly);
    bool PointOnSegmentXZ(const glm::vec3& p, const glm::vec3& a, const glm::vec3& b, float eps, float& tOut);
    float Cross2D(const glm::vec2& a, const glm::vec2& b);
    float TriArea2D(const glm::vec2& a, const glm::vec2& b, const glm::vec2& c);

    // Debug
    void DrawNavTri(const NavTri& navTri, const glm::vec4& color);

    uint64_t CreateNavMesh(float agentRadius);


    NavMesh::NavMesh(uint64_t id, float agentRadius) {
        m_id = id;
        m_agentRadius = agentRadius;
    }

    void NavMesh::Reset() {
        m_tris.clear();
        m_id = 0;
        m_agentRadius = 0;
    }

    void NavMesh::Update() {
        if (m_staticPathsDirty) UpdateStaticPaths();
        if (m_dynamicPathsDirty) UpdateDynamicPaths();

        if (m_staticPathsDirty || m_dynamicPathsDirty) UpdateNavMesh();

        m_staticPathsDirty = false;
        m_dynamicPathsDirty = false;
    }

    void NavMesh::UpdateStaticPaths() {
        NAV_MESH_TIMER("UpdateStaticPaths");

        // Clear all static paths at all levels
        for (auto& it : m_levelInfo) {
            int key = it.first;
            LevelInfo& levelInfo = it.second;
            levelInfo.staticPaths.clear();
            levelInfo.staticObstaclePaths.clear();
            levelInfo.solutionStatic.clear();
        }

        // Gather floors
        for (WorldPlane& worldPlane : Unloved::World::GetWorldPlanes()) {
            if (worldPlane.GetType() != WorldPlaneType::FLOOR) continue;

            // Use y height as key
            float height = worldPlane.GetWorldSpaceCenter().y;
            int key = CreatePathKeyFromHeightY(height);
            m_levelInfo[key].y = height;

            // Add nav mesh poly to clipper path
            const std::vector<glm::vec2>& navMeshPoly = worldPlane.GetNavMeshPoly();
            m_levelInfo[key].staticPaths.push_back(ConvertToClipperPath(navMeshPoly));
        }

        // Gather obstacles
        for (Piano& piano : Unloved::World::GetPianos()) {
            for (const MeshNode& meshNode : piano.GetMeshNodes().GetNodes()) {
                AddMeshNodeToPath(meshNode, &LevelInfo::staticObstaclePaths);
            }
        }
        for (Fireplace& fireplace : Unloved::World::GetFireplaces()) {
            for (const MeshNode& meshNode : fireplace.GetMeshNodes().GetNodes()) {
                if (meshNode.addToNavMesh) {
                    AddMeshNodeToPath(meshNode, &LevelInfo::staticObstaclePaths);
                }
            }
        }
        for (GenericObject& genericObject: Unloved::World::GetGenericObjects()) {
            for (const MeshNode& meshNode : genericObject.GetMeshNodes().GetNodes()) {
                if (meshNode.addToNavMesh) {
                    AddMeshNodeToPath(meshNode, &LevelInfo::staticObstaclePaths);
                }
            }
        }

        // Boolean ops + Triangulation per level
        for (auto& it : m_levelInfo) {
            int key = it.first;
            LevelInfo& levelInfo = it.second;
            Clipper2Lib::PathsD pathsUnion = Clipper2Lib::Union(levelInfo.staticPaths, Clipper2Lib::FillRule::NonZero);

            // Subtract any static obstacles at this level
            if (!levelInfo.staticObstaclePaths.empty()) {
                Clipper2Lib::PathsD obstacleUnion = Clipper2Lib::Union(levelInfo.staticObstaclePaths, Clipper2Lib::FillRule::NonZero);
                levelInfo.solutionStatic = Clipper2Lib::Difference(pathsUnion, obstacleUnion, Clipper2Lib::FillRule::NonZero);
            }
            else {
                levelInfo.solutionStatic = pathsUnion;
            }
        }
    }


    void NavMesh::UpdateDynamicPaths() {
        NAV_MESH_TIMER("UpdateDynamicPaths");

        // Clear all dynamic paths at all levels
        for (auto& it : m_levelInfo) {
            int key = it.first;
            LevelInfo& levelInfo = it.second;
            levelInfo.dynamicObstaclePaths.clear();
            levelInfo.solutionDynamic.clear();
        }

        // Cut doors out of the nav mesh
        for (Door& door : Unloved::World::GetDoors()) {
            for (const MeshNode& meshNode : door.GetMeshNodes().GetNodes()) {
                if (meshNode.addToNavMesh) {
                    AddMeshNodeToPath(meshNode, &LevelInfo::dynamicObstaclePaths);
                }
            }
        }

        // Boolean ops + Triangulation per level
        for (auto& it : m_levelInfo) {
            int key = it.first;
            LevelInfo& levelInfo = it.second;
            levelInfo.solutionDynamic = Clipper2Lib::Union(levelInfo.dynamicObstaclePaths, Clipper2Lib::FillRule::NonZero);
        }
    }

    void NavMesh::UpdateNavMesh() {
        NAV_MESH_TIMER("UpdateNavMesh");

        constexpr auto fillRule = Clipper2Lib::FillRule::NonZero;
        m_boundsMin = glm::vec2(std::numeric_limits<float>::max());
        m_boundsMax = glm::vec2(std::numeric_limits<float>::lowest());
        std::size_t estimatedTris = 0;

        // Iterate each path level and add the tris to the nav mesh
        for (auto& it : m_levelInfo) {
            LevelInfo& levelInfo = it.second;

            // Skip levels with no static geometry at all
            if (levelInfo.solutionStatic.empty()) {
                levelInfo.solutionFinal.clear();
                continue;
            }

            // Pointer for the base shape. This will be static only or static minus dynamic
            const Clipper2Lib::PathsD* basePaths = nullptr;

            // Static minus dynamic
            if (!levelInfo.solutionDynamic.empty()) {
                levelInfo.solutionFinal = Clipper2Lib::Difference(levelInfo.solutionStatic, levelInfo.solutionDynamic, fillRule);
                basePaths = &levelInfo.solutionFinal;
            }
            // If there are no dynamics then use static directly
            else {
                basePaths = &levelInfo.solutionStatic;
            }

            if (basePaths->empty()) {
                levelInfo.solutionFinal.clear();
                continue;
            }

            // Deflate into final
            levelInfo.solutionFinal = DeflatePaths(*basePaths, m_agentRadius);

            // Accumulate estimate tris
            for (const auto& path : levelInfo.solutionFinal) {
                const std::size_t n = path.size();
                if (n >= 3) estimatedTris += n;
            }
        }

        // Build nav mesh tris with earcut
        m_tris.clear();
        m_tris.reserve(estimatedTris);

        for (auto& it : m_levelInfo) {
            LevelInfo& levelInfo = it.second;

            // Build outer hole polygons from Clipper output
            BuildEarcutPolygons(levelInfo.solutionFinal, m_tempPolys);

            for (const EarcutPolygon& poly : m_tempPolys) {
                if (poly.rings.empty()) continue;

                m_tempEarcutInput.clear();
                m_tempEarcutInput.reserve(poly.rings.size());

                // Precompute vertex count
                std::size_t totalVerts = 0;
                for (const auto& ring : poly.rings) {
                    totalVerts += ring.size();
                }
                m_tempVerts2D.clear();
                m_tempVerts2D.reserve(totalVerts);

                // Fill earcut input + flat vertices
                for (const auto& ring : poly.rings) {
                    auto& dstRing = m_tempEarcutInput.emplace_back();
                    dstRing.reserve(ring.size());

                    for (const auto& v : ring) {
                        dstRing.emplace_back(v.x, v.y);
                        m_tempVerts2D.push_back(v);
                    }
                }

                m_tempIndices = mapbox::earcut<uint32_t>(m_tempEarcutInput);
                for (size_t i = 0; i + 2 < m_tempIndices.size(); i += 3) {
                    const glm::vec2& v0 = m_tempVerts2D[m_tempIndices[i + 0]];
                    const glm::vec2& v1 = m_tempVerts2D[m_tempIndices[i + 1]];
                    const glm::vec2& v2 = m_tempVerts2D[m_tempIndices[i + 2]];
                    AddTri(glm::vec3(v0.x, levelInfo.y, v0.y), glm::vec3(v1.x, levelInfo.y, v1.y), glm::vec3(v2.x, levelInfo.y, v2.y));
                }
            }
        }

        // RemoveTJunctions();
        BuildNeighbors();
        BuildSpatialGrid();
    }

    void NavMesh::RemoveTJunctions() {
        // Post process the triangulated mesh: Split triangles when a vertex encroaches an edge.
        bool postProcess = false;
        if (postProcess) {
            float eps = 0.0001f;
            float aabbEps = 0.01f;
            float tOut = 0.0f; // unused, but a required param for PointOnSegmentXZ(..)
            bool changed = true;

            while (changed) {
                changed = false;

                for (size_t i = 0; i < m_tris.size(); i++) {
                    NavTri& triA = m_tris[i];

                    // Iterate over all vertices of triA
                    for (int vA = 0; vA < 3; vA++) {
                        glm::vec3 P = triA.v[vA];

                        // Iterate over all OTHER triangles
                        for (size_t j = 0; j < m_tris.size(); j++) {
                            if (i == j) continue;

                            NavTri& triB = m_tris[j];

                            // AABB reject in XZ with padding
                            if (P.x < triB.boundsMin.x - aabbEps ||
                                P.x > triB.boundsMax.x + aabbEps ||
                                P.z < triB.boundsMin.z - aabbEps ||
                                P.z > triB.boundsMax.z + aabbEps) {
                                continue;
                            }

                            // Iterate over all edges of triB
                            for (int eB = 0; eB < 3; eB++) {
                                glm::vec3 q0 = triB.v[eB];
                                glm::vec3 q1 = triB.v[(eB + 1) % 3];
                                glm::vec3 q2 = triB.v[(eB + 2) % 3]; // The opposite vertex

                                // Check if vertex P lies on the interior of edge q0 to q1
                                if (PointOnSegmentXZ(P, q0, q1, eps, tOut)) {

                                    // Remove the old encroached triangle triB
                                    m_tris.erase(m_tris.begin() + j);

                                    // Add the two new triangles
                                    AddTri(q0, P, q2);
                                    AddTri(P, q1, q2);

                                    changed = true;

                                    // Restart the loop to ensure you operate on a valid index set
                                    goto restart_loops;
                                }
                            }
                        }
                    }
                }
            restart_loops:; // Goto label
            }
        }
    }

    void NavMesh::BuildNeighbors() {
        const int triCount = (int)m_tris.size();
        if (triCount == 0) return;

        m_edgeList.clear();
        m_edgeList.reserve(triCount * 3);

        // Build edge list
        for (int i = 0; i < triCount; ++i) {
            const NavTri& tri = m_tris[i];

            for (int e = 0; e < 3; ++e) {
                glm::vec3 v0 = tri.v[e];
                glm::vec3 v1 = tri.v[(e + 1) % 3];

                // Canonical order in full 3D, this way edge keys match regardless of the vertex order
                if (v0.x > v1.x ||
                    (v0.x == v1.x && (v0.z > v1.z ||
                        (v0.z == v1.z && v0.y > v1.y)))) {
                    std::swap(v0, v1);
                }

                m_edgeList.push_back({ v0, v1, i, e });
            }
        }

        auto cmp = [](const EdgeRef& a, const EdgeRef& b) {
            if (a.v0.x != b.v0.x) return a.v0.x < b.v0.x;
            if (a.v0.z != b.v0.z) return a.v0.z < b.v0.z;
            if (a.v0.y != b.v0.y) return a.v0.y < b.v0.y;
            if (a.v1.x != b.v1.x) return a.v1.x < b.v1.x;
            if (a.v1.z != b.v1.z) return a.v1.z < b.v1.z;
            return a.v1.y < b.v1.y;
            };
        std::sort(m_edgeList.begin(), m_edgeList.end(), cmp);

        const size_t edgeCount = m_edgeList.size();
        size_t i = 0;
        while (i + 1 < edgeCount) {
            const EdgeRef& e0 = m_edgeList[i];
            const EdgeRef& e1 = m_edgeList[i + 1];

            bool equal =
                e0.v0.x == e1.v0.x && e0.v0.y == e1.v0.y && e0.v0.z == e1.v0.z &&
                e0.v1.x == e1.v1.x && e0.v1.y == e1.v1.y && e0.v1.z == e1.v1.z;

            if (equal) {
                m_tris[e0.triIndex].neighbor[e0.edgeIndex] = e1.triIndex;
                m_tris[e1.triIndex].neighbor[e1.edgeIndex] = e0.triIndex;
                i += 2;
            }
            else {
                ++i;
            }
        }
    }

    void NavMesh::AddMeshNodeToPath(const MeshNode& meshNode, Clipper2Lib::PathsD LevelInfo::* member) {
        // Inflate slightly to ensure the OBB intersects the floor plane even if it's resting perfectly on top
        float threshold = 0.05f;

        // Only process nodes marked for collision
        // if (!meshNode.aabbCollision) continue;

        const OBB& obb = meshNode.worldSpaceObb;
        const AABB& localBounds = obb.GetLocalBounds();

        // Inflate Local Bounds
        glm::vec3 min = localBounds.GetBoundsMin() - glm::vec3(threshold);
        glm::vec3 max = localBounds.GetBoundsMax() + glm::vec3(threshold);

        glm::vec3 localCorners[8] = {
            { min.x, min.y, min.z }, { max.x, min.y, min.z },
            { min.x, max.y, min.z }, { max.x, max.y, min.z },
            { min.x, min.y, max.z }, { max.x, min.y, max.z },
            { min.x, max.y, max.z }, { max.x, max.y, max.z }
        };

        // Transform to World Space & Find Vertical Extents
        glm::vec3 worldCorners[8];
        float minWorldY = std::numeric_limits<float>::max();
        float maxWorldY = std::numeric_limits<float>::lowest();

        const glm::mat4& M = obb.GetWorldTransform();
        std::vector<glm::vec2> xzPoints;
        xzPoints.reserve(8);

        for (int i = 0; i < 8; ++i) {
            worldCorners[i] = glm::vec3(M * glm::vec4(localCorners[i], 1.0f));

            if (worldCorners[i].y < minWorldY) minWorldY = worldCorners[i].y;
            if (worldCorners[i].y > maxWorldY) maxWorldY = worldCorners[i].y;

            // Collect the 2D projected points
            xzPoints.emplace_back(worldCorners[i].x, worldCorners[i].z);
        }

        // Compute the single, closed obstacle boundary
        std::vector<glm::vec2> obstacleBoundary = ComputeConvexHull2D(xzPoints);
        if (obstacleBoundary.size() < 3) return;

        // Add to Relevant Floors
        const float yBuffer = 0.05f;

        for (auto& pair : m_levelInfo) {
            LevelInfo& levelInfo = pair.second;
            float floorY = levelInfo.y;

            // Does the object span across this floor's height?
            if (floorY >= (minWorldY - yBuffer) && floorY <= (maxWorldY + yBuffer)) {
                (levelInfo.*member).push_back(ConvertToClipperPath(obstacleBoundary));
            }
        }
    }

    void NavMesh::AddTri(const glm::vec3 a, const glm::vec3 b, const glm::vec3 c) {
        NavTri tri;
        tri.v[0] = RoundVertex(a);
        tri.v[1] = RoundVertex(b);
        tri.v[2] = RoundVertex(c);

        const glm::vec3& A = tri.v[0];
        const glm::vec3& B = tri.v[1];
        const glm::vec3& C = tri.v[2];

        float area2 = (B.x - A.x) * (C.z - A.z) - (B.z - A.z) * (C.x - A.x);

        // Reject degenerate tris (in XZ)
        if (std::fabs(area2) < 0.00001f) return;

        // Enforce winding order
        if (area2 < 0.0f) std::swap(tri.v[1], tri.v[2]);

        // Center
        tri.center = (tri.v[0] + tri.v[1] + tri.v[2]) * (1.0f / 3.0f);

        // Compute tri AABB
        tri.boundsMin = tri.v[0];
        tri.boundsMax = tri.v[0];
        for (int i = 1; i < 3; ++i) {
            const glm::vec3& p = tri.v[i];
            tri.boundsMin.x = std::min(tri.boundsMin.x, p.x);
            tri.boundsMin.y = std::min(tri.boundsMin.y, p.y);
            tri.boundsMin.z = std::min(tri.boundsMin.z, p.z);
            tri.boundsMax.x = std::max(tri.boundsMax.x, p.x);
            tri.boundsMax.y = std::max(tri.boundsMax.y, p.y);
            tri.boundsMax.z = std::max(tri.boundsMax.z, p.z);
        }

        // Reset neighbours
        tri.neighbor[0] = -1;
        tri.neighbor[1] = -1;
        tri.neighbor[2] = -1;

        // Update nav mesh AABB
        m_boundsMin.x = std::min(m_boundsMin.x, tri.boundsMin.x);
        m_boundsMin.y = std::min(m_boundsMin.y, tri.boundsMin.z);
        m_boundsMax.x = std::max(m_boundsMax.x, tri.boundsMax.x);
        m_boundsMax.y = std::max(m_boundsMax.y, tri.boundsMax.z);

        m_tris.push_back(tri);
    }

    void NavMesh::BuildSpatialGrid() {
        if (m_tris.empty()) return;

        // Add a small buffer padding to ensure points on the exact edge don't cause OOB errors
        m_gridOrigin = m_boundsMin - glm::vec2(0.1f);
        glm::vec2 endPos = m_boundsMax + glm::vec2(0.1f);

        float widthWorld = endPos.x - m_gridOrigin.x;
        float heightWorld = endPos.y - m_gridOrigin.y;

        m_gridWidth = (int)std::ceil(widthWorld * m_invCellSize);
        m_gridHeight = (int)std::ceil(heightWorld * m_invCellSize);

        // Resize Grid
        int numCells = m_gridWidth * m_gridHeight;
        if (m_grid.size() != numCells) {
            m_grid.resize(numCells);
        }

        // Clear lists
        for (auto& cell : m_grid) {
            cell.clear();
        }

        // Populate Grid
        for (int i = 0; i < (int)m_tris.size(); ++i) {
            const NavTri& tri = m_tris[i];

            int minX = (int)((tri.boundsMin.x - m_gridOrigin.x) * m_invCellSize);
            int minZ = (int)((tri.boundsMin.z - m_gridOrigin.y) * m_invCellSize);
            int maxX = (int)((tri.boundsMax.x - m_gridOrigin.x) * m_invCellSize);
            int maxZ = (int)((tri.boundsMax.z - m_gridOrigin.y) * m_invCellSize);

            minX = std::max(0, minX);
            minZ = std::max(0, minZ);
            maxX = std::min(m_gridWidth - 1, maxX);
            maxZ = std::min(m_gridHeight - 1, maxZ);

            for (int z = minZ; z <= maxZ; ++z) {
                for (int x = minX; x <= maxX; ++x) {
                    int cellIndex = z * m_gridWidth + x;
                    m_grid[cellIndex].push_back(i);
                }
            }
        }
    }

    int NavMesh::FindContainingTriIndex(const glm::vec3& p) {
        int cellIndex = GetGridIndex(p);
        if (cellIndex == -1) return -1;

        // Only look at the specific triangles inside this one cell
        const std::vector<int>& candidateIndices = m_grid[cellIndex];

        for (int triIndex : candidateIndices) {
            if (PointInNavTriXZ(m_tris[triIndex], p)) {
                return triIndex;
            }
        }

        return -1;
    }

    int NavMesh::GetGridIndex(const glm::vec3& p) {
        if (m_grid.empty()) return -1;

        // World space to grid space
        int x = (int)((p.x - m_gridOrigin.x) / m_gridCellSize);
        int z = (int)((p.z - m_gridOrigin.y) / m_gridCellSize);

        // Bounds check
        if (x < 0 || x >= m_gridWidth || z < 0 || z >= m_gridHeight) {
            return -1;
        }

        // Flatten 2D coordinates (x, z) into a 1D index
        return z * m_gridWidth + x;
    }

    std::vector<glm::vec3> NavMesh::FindPath(const glm::vec3& start, const glm::vec3& dest) {
        NAV_MESH_TIMER("Find path");

        std::vector<glm::vec3> result;
        if (m_tris.empty()) return result;

        int startTri = FindContainingTriIndex(start);
        int endTri = FindContainingTriIndex(dest);

        if (startTri == -1 || endTri == -1) return result;

        if (startTri == endTri) {
            glm::vec3 s = start;
            glm::vec3 d = dest;
            float h = m_tris[startTri].center.y;
            s.y = h;
            d.y = h;
            result.push_back(s);
            result.push_back(d);
            return result;
        }

        const int triCount = (int)m_tris.size();
        std::vector<float> gScore(triCount, std::numeric_limits<float>::max());
        std::vector<float> fScore(triCount, std::numeric_limits<float>::max());
        std::vector<int> cameFrom(triCount, -1);
        std::vector<bool> inOpen(triCount, false);
        std::vector<bool> inClosed(triCount, false);
        std::vector<int> openSet;

        auto Heuristic = [&](int triIndex) {
            const glm::vec3& c = m_tris[triIndex].center;
            glm::vec2 d(c.x - dest.x, c.z - dest.z);
            return glm::length(d);
            };

        gScore[startTri] = 0.0f;
        fScore[startTri] = Heuristic(startTri);
        openSet.push_back(startTri);
        inOpen[startTri] = true;

        bool found = false;

        while (!openSet.empty()) {
            int current = -1;
            float bestF = std::numeric_limits<float>::max();
            int bestIndex = -1;

            for (int i = 0; i < (int)openSet.size(); ++i) {
                int triIndex = openSet[i];
                if (fScore[triIndex] < bestF) {
                    bestF = fScore[triIndex];
                    current = triIndex;
                    bestIndex = i;
                }
            }

            if (current == -1) break;
            if (current == endTri) {
                found = true;
                break;
            }

            openSet[bestIndex] = openSet.back();
            openSet.pop_back();
            inOpen[current] = false;
            inClosed[current] = true;

            const glm::vec3& currentCenter = m_tris[current].center;

            for (int e = 0; e < 3; ++e) {
                int neighbor = m_tris[current].neighbor[e];
                if (neighbor < 0) continue;
                if (inClosed[neighbor]) continue;

                const glm::vec3& neighborCenter = m_tris[neighbor].center;
                glm::vec2 delta(neighborCenter.x - currentCenter.x, neighborCenter.z - currentCenter.z);
                float stepCost = glm::length(delta);
                float tentativeG = gScore[current] + stepCost;

                if (!inOpen[neighbor]) {
                    openSet.push_back(neighbor);
                    inOpen[neighbor] = true;
                }
                else if (tentativeG >= gScore[neighbor]) {
                    continue;
                }

                cameFrom[neighbor] = current;
                gScore[neighbor] = tentativeG;
                fScore[neighbor] = tentativeG + Heuristic(neighbor);
            }
        }

        if (!found) return result;

        std::vector<int> triPath;
        int current = endTri;
        triPath.push_back(current);
        while (current != startTri) {
            current = cameFrom[current];
            if (current == -1) {
                result.clear();
                return result;
            }
            triPath.push_back(current);
        }
        std::reverse(triPath.begin(), triPath.end());

        glm::vec3 s = start;
        s.y = m_tris[startTri].GetHeightAtXZ(start);
        glm::vec3 d = dest;
        d.y = m_tris[endTri].GetHeightAtXZ(dest);

        result.push_back(s);

        if (triPath.size() > 2) {
            for (size_t i = 1; i + 1 < triPath.size(); ++i) {
                int triIndex = triPath[i];
                result.push_back(m_tris[triIndex].center);
            }
        }

        result.push_back(d);
        return result;
    }

    std::vector<Portal> NavMesh::BuildPortalsFromCenters(const std::vector<glm::vec3>& path, const std::vector<int>& triIndices) {
        std::vector<Portal> portals;

        if (path.size() < 2 || path.size() != triIndices.size())
            return portals;

        portals.reserve(path.size() + 2);

        // Insert degen portal for start
        portals.push_back({ path[0], path[0] });

        const float epsSq = 1e-6f;

        for (size_t i = 0; i + 1 < path.size(); ++i) {
            const NavTri& a = m_tris[triIndices[i + 0]];
            const NavTri& b = m_tris[triIndices[i + 1]];

            glm::vec3 e0(0.0f);
            glm::vec3 e1(0.0f);
            bool foundEdge = false;

            for (int ea = 0; ea < 3 && !foundEdge; ++ea) {
                glm::vec3 a0 = a.v[ea];
                glm::vec3 a1 = a.v[(ea + 1) % 3];

                for (int eb = 0; eb < 3 && !foundEdge; ++eb) {
                    glm::vec3 b0 = b.v[eb];
                    glm::vec3 b1 = b.v[(eb + 1) % 3];

                    bool same01 = Hell::Math::DistSquared(a0, b0) < epsSq && Hell::Math::DistSquared(a1, b1) < epsSq;
                    bool same10 = Hell::Math::DistSquared(a0, b1) < epsSq && Hell::Math::DistSquared(a1, b0) < epsSq;

                    if (same01 || same10) {
                        e0 = a0;
                        e1 = a1;
                        foundEdge = true;
                    }
                }
            }

            if (!foundEdge)
                continue;

            glm::vec3 centerCurr3 = path[i + 0];
            glm::vec3 centerNext3 = path[i + 1];

            glm::vec2 centerCurr = ToXZ(centerCurr3);
            glm::vec2 centerNext = ToXZ(centerNext3);

            glm::vec2 dir = centerNext - centerCurr;
            float lenSq = glm::dot(dir, dir);
            if (lenSq < 1e-8f)
                continue;
            dir /= std::sqrt(lenSq);

            glm::vec2 p0 = ToXZ(e0) - centerCurr;
            glm::vec2 p1 = ToXZ(e1) - centerCurr;

            float c0 = Cross2D(dir, p0);
            float c1 = Cross2D(dir, p1);

            Portal portal;

            if (c0 > 0.0f && c1 < 0.0f) {
                portal.left = e1;
                portal.right = e0;
            }
            else if (c1 > 0.0f && c0 < 0.0f) {
                portal.left = e0;
                portal.right = e1;
            }
            else {
                if (c0 >= c1) {
                    portal.left = e1;
                    portal.right = e0;
                }
                else {
                    portal.left = e0;
                    portal.right = e1;
                }
            }

            portals.push_back(portal);
        }

        // Insert degen portal for destination
        portals.push_back({ path[path.size() - 1], path[path.size() - 1] });

        return portals;
    }

    std::vector<glm::vec3> NavMesh::PullPath(const std::vector<glm::vec3>& path) {
        NAV_MESH_TIMER("PullPath");

        if (path.size() < 2 || m_tris.empty()) {
            return path;
        }

        // Rebuild triangle indices from raw path points
        std::vector<int> triIndices;
        for (const glm::vec3& p : path) {
            triIndices.push_back(FindContainingTriIndex(p));
        }

        // Build portals
        std::vector<Portal> portals = BuildPortalsFromCenters(path, triIndices);


        std::vector<glm::vec3> smooth = Funnel(portals);

        return smooth;
    }

    std::vector<glm::vec3> NavMesh::Funnel(const std::vector<Portal>& portals) {
        std::vector<glm::vec3> result;
        const int nportals = (int)portals.size();
        if (nportals == 0) return result;

        result.reserve(nportals + 1);

        glm::vec2 apex2 = ToXZ(portals[0].left);
        glm::vec2 left2 = apex2;
        glm::vec2 right2 = ToXZ(portals[0].right);
        int apexIndex = 0;
        int leftIndex = 0;
        int rightIndex = 0;

        result.push_back(portals[0].left); // start

        const float eq = 0.001f * 0.001f;

        for (int i = 1; i < nportals; ++i) {
            glm::vec2 newLeft2 = ToXZ(portals[i].left);
            glm::vec2 newRight2 = ToXZ(portals[i].right);

            // Right
            if (TriArea2D(apex2, right2, newRight2) <= 0.0f) {
                if (Hell::Math::DistSquared2D(apex2, right2) < eq ||
                    TriArea2D(apex2, left2, newRight2) > 0.0f) {
                    right2 = newRight2;
                    rightIndex = i;
                }
                else {
                    // Right over left: commit left
                    result.push_back(portals[leftIndex].left);

                    apex2 = ToXZ(portals[leftIndex].left);
                    apexIndex = leftIndex;

                    left2 = apex2;
                    right2 = apex2;
                    leftIndex = apexIndex;
                    rightIndex = apexIndex;

                    i = apexIndex;
                    continue;
                }
            }

            // Left
            if (TriArea2D(apex2, left2, newLeft2) >= 0.0f) {
                if (Hell::Math::DistSquared2D(apex2, left2) < eq ||
                    TriArea2D(apex2, right2, newLeft2) < 0.0f) {
                    left2 = newLeft2;
                    leftIndex = i;
                }
                else {
                    // Left over right: commit right
                    result.push_back(portals[rightIndex].right);

                    apex2 = ToXZ(portals[rightIndex].right);
                    apexIndex = rightIndex;

                    left2 = apex2;
                    right2 = apex2;
                    leftIndex = apexIndex;
                    rightIndex = apexIndex;

                    i = apexIndex;
                    continue;
                }
            }
        }

        // Append goal (last portal is [end,end])
        if (result.empty() ||
            result.back().x != portals.back().left.x ||
            result.back().y != portals.back().left.y ||
            result.back().z != portals.back().left.z) {
            result.push_back(portals.back().left);
        }

        return result;
    }

    void NavMesh::DebugDrawAdjacentTris() {
        glm::vec3 viewPos = Unloved::Session::GetLocalPlayerByViewportIndex(0)->GetCameraPosition();
        for (NavTri& tri : m_tris) {
            if (PointInNavTriXZ(tri, viewPos)) {

                if (tri.neighbor[0] != -1) {
                    DrawNavTri(m_tris[tri.neighbor[0]], BLUE);
                }
                if (tri.neighbor[1] != -1) {
                    DrawNavTri(m_tris[tri.neighbor[1]], BLUE);
                }
                if (tri.neighbor[2] != -1) {
                    DrawNavTri(m_tris[tri.neighbor[2]], BLUE);
                }

                DrawNavTri(tri, GREEN);
            }
        }
    }

    void NavMesh::DrawTris() {
        for (NavTri& navTri : m_tris) {
            DrawNavTri(navTri, GREEN);
        }
    }


    void Init() {
        Logging::Init() << "Initialized the NavMesh";

        g_testNaveMeshID = CreateNavMesh(0.1f);
    }

    void Update() {
        for (NavMesh& navMesh : g_navMeshes) {
            navMesh.Update();
        }

        if (!Renderer::GetCurrentRendererSettings().debugDrawNavMesh) {
            return;
        }

        for (NavMesh& navMesh : g_navMeshes) {
            navMesh.DrawTris();
        }
    }

    void MarkStaticDirty() {
        for (NavMesh& navMesh : g_navMeshes) {
            navMesh.MarkStaticDirty();
        }
    }

    void MarkDynamicDirty() {
        for (NavMesh& navMesh : g_navMeshes) {
            navMesh.MarkDynamicDirty();
        }
    }

    uint64_t CreateNavMesh(float agentRadius) {
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::NAV_MESH);
        g_navMeshes.emplace_with_id(id, id, agentRadius);
        return id;
    }

    std::vector<glm::vec3> FindPath(const glm::vec3& start, const glm::vec3& dest) {
        static std::vector<glm::vec3> emptyPath;

        if (NavMesh* navMesh = GetNavMeshById(g_testNaveMeshID)) {
            std::vector<glm::vec3> rawPath = navMesh->FindPath(start, dest);
            std::vector<glm::vec3> pulledPath = navMesh->PullPath(rawPath);
            return pulledPath;
        }
        else {
            return emptyPath;
        }
    }

    int CreatePathKeyFromHeightY(float y) {
        return (int)std::round(y * 1000.0f);
    }

    NavMesh* GetNavMeshById(int64_t id) {
        return g_navMeshes.get(id);
    }

    Clipper2Lib::PathsD DeflatePaths(const Clipper2Lib::PathsD& paths, float agentRadius) {
        float delta = -static_cast<double>(agentRadius);
        Clipper2Lib::JoinType jointType = Clipper2Lib::JoinType::Miter;
        Clipper2Lib::EndType polygon = Clipper2Lib::EndType::Polygon;
        double miterLimit = 2.0f;
        int precision = 2;
        double arcTolerance = 0.0f;
        return Clipper2Lib::InflatePaths(paths, delta, jointType, polygon, miterLimit, precision, arcTolerance);
    }

    Clipper2Lib::PathD ConvertToClipperPath(const std::vector<glm::vec2>& points) {
        Clipper2Lib::PathD path;
        for (const auto& pt : points) {
            path.push_back(Clipper2Lib::PointD(pt.x, pt.y));
        }
        return path;
    }

    void DrawPath(std::vector<glm::vec3>& path, const glm::vec4& color) {
		if (path.size() >= 2) {
			for (int i = 0; i < path.size() - 1; i++) {
				DebugDraw::DrawLine(path[i], path[i + 1], color);
                DebugDraw::DrawPoint(path[i], color);
            }
            DebugDraw::DrawPoint(path[path.size()-1], color);
		}
    }

    glm::vec3 RoundVertex(const glm::vec3& v) {
        return glm::round(v * 100.0f) / 100.0f;
    }

    bool PointInPolygon2D(const glm::vec2& p, const std::vector<glm::vec2>& poly) {
        bool inside = false;
        int n = (int)poly.size();
        for (int i = 0, j = n - 1; i < n; j = i++) {
            const glm::vec2& a = poly[i];
            const glm::vec2& b = poly[j];

            bool intersect = ((a.y > p.y) != (b.y > p.y)) && (p.x < (b.x - a.x) * (p.y - a.y) / ((b.y - a.y) + 1e-12f) + a.x);

            if (intersect) inside = !inside;
        }
        return inside;
    }

    // Build outer + hole groups from Clipper PathsD
    void BuildEarcutPolygons(const Clipper2Lib::PathsD& paths, std::vector<EarcutPolygon>& outPolys) {
        outPolys.clear();
        if (paths.empty()) return;

        struct RingInfo {
            std::vector<glm::vec2> pts;
            double area;
            glm::vec2 boundsMin;
            glm::vec2 boundsMax;
        };

        struct OuterInfo {
            int polyIndex;
            glm::vec2 boundsMin;
            glm::vec2 boundsMax;
        };

        static std::vector<RingInfo> rings;   // NOT THREAD SAFE!!!
        static std::vector<OuterInfo> outers; // NOT THREAD SAFE!!!

        rings.clear();
        outers.clear();
        rings.reserve(paths.size());
        outers.reserve(paths.size());

        double sumPos = 0.0;
        double sumNeg = 0.0;

        for (const auto& path : paths) {
            if (path.size() < 3) continue;

            RingInfo r;
            r.pts.reserve(path.size());

            glm::vec2 bmin(std::numeric_limits<float>::max());
            glm::vec2 bmax(std::numeric_limits<float>::lowest());

            for (const auto& p : path) {
                glm::vec2 v((float)p.x, (float)p.y);
                r.pts.push_back(v);

                bmin.x = std::min(bmin.x, v.x);
                bmin.y = std::min(bmin.y, v.y);
                bmax.x = std::max(bmax.x, v.x);
                bmax.y = std::max(bmax.y, v.y);
            }

            r.boundsMin = bmin;
            r.boundsMax = bmax;

            r.area = Clipper2Lib::Area(path);
            if (r.area > 0.0) sumPos += r.area;
            else             sumNeg -= r.area;

            rings.push_back(std::move(r));
        }

        if (rings.empty()) return;

        bool outerPositive = (sumPos >= sumNeg);

        // Create one EarcutPolygon per outer ring
        for (size_t i = 0; i < rings.size(); ++i) {
            const RingInfo& r = rings[i];
            bool isOuter = outerPositive ? (r.area > 0.0) : (r.area < 0.0);
            if (!isOuter) continue;

            EarcutPolygon poly;
            poly.rings.push_back(r.pts);
            outPolys.push_back(std::move(poly));

            OuterInfo oi;
            oi.polyIndex = (int)outPolys.size() - 1;
            oi.boundsMin = r.boundsMin;
            oi.boundsMax = r.boundsMax;
            outers.push_back(oi);
        }

        if (outPolys.empty()) return;

        // Assign holes to containing outer
        for (const RingInfo& r : rings) {
            bool isOuter = outerPositive ? (r.area > 0.0) : (r.area < 0.0);
            if (isOuter) continue;
            if (r.pts.empty()) continue;

            glm::vec2 testPoint = r.pts[0];

            for (const OuterInfo& oi : outers) {
                const glm::vec2& omin = oi.boundsMin;
                const glm::vec2& omax = oi.boundsMax;

                if (testPoint.x < omin.x || testPoint.x > omax.x ||
                    testPoint.y < omin.y || testPoint.y > omax.y) {
                    continue;
                }

                if (PointInPolygon2D(testPoint, outPolys[oi.polyIndex].rings[0])) {
                    outPolys[oi.polyIndex].rings.push_back(r.pts);
                    break;
                }
            }
        }
    }

    std::vector<glm::vec2> ComputeConvexHull2D(std::vector<glm::vec2>& points) {
        size_t n = points.size();
        if (n <= 3) return points;

        // Sort points by X, then Y
        std::sort(points.begin(), points.end(), [](const glm::vec2& a, const glm::vec2& b) {
            if (a.x != b.x) return a.x < b.x;
            return a.y < b.y;
            });

        std::vector<glm::vec2> hull;
        hull.reserve(n * 2);

        // Build lower hull
        for (size_t i = 0; i < n; ++i) {
            // While the last two points and the new point form a clockwise turn (area < 0)
            while (hull.size() >= 2 && Hell::Math::TriArea2D(hull[hull.size() - 2], hull.back(), points[i]) <= 0.0f) {
                hull.pop_back();
            }
            hull.push_back(points[i]);
        }

        // Build upper hull
        size_t t = hull.size() + 1;
        for (size_t i = n - 1; i > 0; --i) {
            // While the last two points and the new point form a clockwise turn (area < 0)
            while (hull.size() >= t && Hell::Math::TriArea2D(hull[hull.size() - 2], hull.back(), points[i - 1]) <= 0.0f) {
                hull.pop_back();
            }
            hull.push_back(points[i - 1]);
        }

        // Remove duplicate point (first point added twice)
        if (hull.size() > 1) {
            hull.pop_back();
        }

        return hull;
    }

    bool EdgesMatch(const glm::vec3& a0, const glm::vec3& a1, const glm::vec3& b0, const glm::vec3& b1, float epsilon) {
        bool sameDir = Hell::Math::PointsEqual(a0, b0, epsilon) && Hell::Math::PointsEqual(a1, b1, epsilon);
        bool oppoDir = Hell::Math::PointsEqual(a0, b1, epsilon) && Hell::Math::PointsEqual(a1, b0, epsilon);
        return sameDir || oppoDir;
    }

	bool PointOnSegmentXZ(const glm::vec3& p, const glm::vec3& a, const glm::vec3& b, float eps, float& tOut) {
		glm::vec2 A(a.x, a.z);
		glm::vec2 B(b.x, b.z);
		glm::vec2 P(p.x, p.z);

		glm::vec2 AB = B - A;
		glm::vec2 AP = P - A;

		float abLen2 = glm::dot(AB, AB);
		if (abLen2 < eps * eps) return false; // degenerate edge

		float cross = AB.x * AP.y - AB.y * AP.x;
		float dist = std::fabs(cross) / std::sqrt(abLen2);
		if (dist > eps) return false; // too far from the line

		// Project AP onto AB to get param t along the segment
		float t = glm::dot(AP, AB) / abLen2;

		// Require it to be strictly inside the segment (not just touching endpoints)
		if (t <= eps || t >= 1.0f - eps) return false;

		tOut = t;
		return true;
	}

    void DrawNavTri(const NavTri& navTri, const glm::vec4& color) {
        DebugDraw::DrawLine(navTri.v[0], navTri.v[1], color);
        DebugDraw::DrawLine(navTri.v[1], navTri.v[2], color);
        DebugDraw::DrawLine(navTri.v[2], navTri.v[0], color);
    }

    bool PointInNavTriXZ(const NavTri& tri, const glm::vec3& position) {
        glm::vec2 p(position.x, position.z);

        glm::vec2 a(tri.v[0].x, tri.v[0].z);
        glm::vec2 b(tri.v[1].x, tri.v[1].z);
        glm::vec2 c(tri.v[2].x, tri.v[2].z);

        glm::vec2 v0 = b - a;
        glm::vec2 v1 = c - a;
        glm::vec2 v2 = p - a;

        float d00 = glm::dot(v0, v0);
        float d01 = glm::dot(v0, v1);
        float d11 = glm::dot(v1, v1);
        float d20 = glm::dot(v2, v0);
        float d21 = glm::dot(v2, v1);

        float denom = d00 * d11 - d01 * d01;
        if (denom == 0.0f) return false;

        float invDenom = 1.0f / denom;
        float v = (d11 * d20 - d01 * d21) * invDenom;
        float w = (d00 * d21 - d01 * d20) * invDenom;
        float u = 1.0f - v - w;

        const float eps = 0.0001f;
        return u >= -eps && v >= -eps && w >= -eps;
    }

    glm::vec2 ToXZ(const glm::vec3& p) {
        return glm::vec2(p.x, p.z);
    }

    float TriArea2D(const glm::vec2& a, const glm::vec2& b, const glm::vec2& c) {
        return (b.x - a.x) * (c.y - a.y) - (c.x - a.x) * (b.y - a.y);
    }

    float Cross2D(const glm::vec2& a, const glm::vec2& b) {
        return a.x * b.y - a.y * b.x;
    }
}

