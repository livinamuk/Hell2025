#pragma once

#include "Hell/Math/GLM.h"
#include "Hell/Math/AABB.h"
#include "Hell/Math/LocalFrame.h"
#include "Hell/Math/OBB.h"
#include "Hell/Render/VertexAttributes.h"

#include <vector>

namespace Hell::DebugDraw {
    void BeginFrame();

    const std::vector<DebugVertex2D>& GetLines2D();
    const std::vector<DebugVertex3D>& GetLines3D();
    const std::vector<DebugVertex2D>& GetPoints2D();
    const std::vector<DebugVertex3D>& GetPoints3D();

    void DrawLine(const glm::vec3& begin, const glm::vec3& end, const glm::vec4& color, bool depthEnabled = false, int thickness = 1, int exclusiveViewportIndex = -1, int ignoredViewportIndex = -1);
    void DrawLine(const glm::vec3& begin, const glm::vec3& end, const glm::vec4& beginColor, const glm::vec4& endColor, bool depthEnabled = false, int thickness = 1, int exclusiveViewportIndex = -1, int ignoredViewportIndex = -1);
    void DrawLocalFrame(const glm::vec3& position, const LocalFrame& localFrame, float scale);
    void DrawLine2D(const glm::ivec2& begin, const glm::ivec2& end, const glm::vec4& color);
    void DrawPoint(const glm::vec3& position, const glm::vec4& color, bool depthEnabled = false, int exclusiveViewportIndex = -1);
    void DrawPoint2D(const glm::ivec2& position, const glm::vec4& color);
    void DrawAABB(const AABB& aabb, const glm::vec4& color);
    void DrawAABB(const AABB& aabb, const glm::vec4& color, const glm::mat4& worldTransform);
    void DrawOBB(const OBB& obb, const glm::vec4& color);
    void DrawSphere(const glm::vec3& position, float radius, const glm::vec4& color);
}
