#pragma once

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <cstdint>
#include <limits>
#include <vector>

namespace Unloved::NavMeshManager {
	struct NavTri {
        glm::vec3 v[3];
        int32_t neighbor[3]; // Index of neighbor across each edge, or -1
        uint64_t doorId = 0; // equals non-zero if this tri belongs to a door
        glm::vec3 boundsMin = glm::vec3(std::numeric_limits<float>::max());
        glm::vec3 boundsMax = glm::vec3(-std::numeric_limits<float>::max());
        glm::vec3 center = glm::vec3(0.0f);

        float GetHeightAtXZ(const glm::vec3& p) const {
            glm::vec2 P(p.x, p.z);
            glm::vec2 A(v[0].x, v[0].z);
            glm::vec2 B(v[1].x, v[1].z);
            glm::vec2 C(v[2].x, v[2].z);

            glm::vec2 v0 = B - A;
            glm::vec2 v1 = C - A;
            glm::vec2 v2 = P - A;

            float d00 = glm::dot(v0, v0);
            float d01 = glm::dot(v0, v1);
            float d11 = glm::dot(v1, v1);
            float d20 = glm::dot(v2, v0);
            float d21 = glm::dot(v2, v1);

            float denom = d00 * d11 - d01 * d01;
            if (denom == 0.0f) return center.y;

            float invDenom = 1.0f / denom;
            float b = (d11 * d20 - d01 * d21) * invDenom;
            float c = (d00 * d21 - d01 * d20) * invDenom;
            float a = 1.0f - b - c;

            return a * v[0].y + b * v[1].y + c * v[2].y;
        }
	};

    void Init();
    void Update();
    void MarkStaticDirty();
    void MarkDynamicDirty();
    void DrawPath(std::vector<glm::vec3>& path, const glm::vec4& color);

    std::vector<glm::vec3> FindPath(const glm::vec3& start, const glm::vec3& dest);
}
