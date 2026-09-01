#pragma once
#include "Unloved/Camera/Frustum.h"

#include "Hell/Debug/DebugDraw.h"
#include "Hell/Math/AABB.h"
#include "Hell/Math/OBB.h"

#include <vector>

struct BvhRayResult;
struct BVHTriangle;
struct MeshBvh;
struct SceneBvh;

namespace DebugDraw {
    void BeginFrame();
    void UploadVertexData();

    void DrawLine(const glm::vec3& begin, const glm::vec3& end, const glm::vec4& color, bool depthEnabled = false, int thickness = 1, int exclusiveViewportIndex = -1, int ignoredViewportIndex = -1);
    void DrawLine(const glm::vec3& begin, const glm::vec3& end, const glm::vec4& beginColor, const glm::vec4& endColor, bool depthEnabled = false, int thickness = 1, int exclusiveViewportIndex = -1, int ignoredViewportIndex = -1);
    void DrawLocalFrame(const glm::vec3& position, const Hell::LocalFrame& localFrame, float scale);
    void DrawLine2D(const glm::ivec2& begin, const glm::ivec2& end, const glm::vec4& color);
    void DrawPoint(const glm::vec3& position, const glm::vec4& color, bool depthEnabled = false, int exclusiveViewportIndex = -1);
    void DrawPoint2D(const glm::ivec2& position, const glm::vec4& color);
    void DrawAABB(const AABB& aabb, const glm::vec4& color);
    void DrawAABB(const AABB& aabb, const glm::vec4& color, const glm::mat4& worldTransform);
    void DrawOBB(const OBB& obb, const glm::vec4& color);
    void DrawSphere(const glm::vec3& position, float radius, const glm::vec4& color);
    void DrawFrustum(const Unloved::Frustum& frustum, const glm::vec4& color);
    void DrawMeshBvhTriangles(const MeshBvh& meshBvh, const glm::vec4& color, const glm::mat4& worldTransform);
    void DrawMeshBvhNodes(const MeshBvh& meshBvh, const glm::vec4& color, const glm::mat4& worldTransform);
    void DrawSceneBvhNodes(const SceneBvh& sceneBvh, const glm::vec4& color);
    void DrawBvhRayResultTriangle(const BvhRayResult& rayResult, const SceneBvh& sceneBvh, const glm::vec4& color);
    void DrawBvhRayResultTriangle(const BvhRayResult& rayResult, const std::vector<BVHTriangle>& triangles, const glm::vec4& color);
    void DrawBvhRayResultNode(const BvhRayResult& rayResult, const glm::vec4& color);
}
