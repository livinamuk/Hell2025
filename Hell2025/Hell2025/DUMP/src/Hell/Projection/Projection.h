#pragma once

#include <cmath>

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace Hell::Projection {

    inline glm::mat4 ReverseZPerspective(float fovYRadians, float aspect, float zNear) {
        float f = 1.0f / std::tan(fovYRadians / 2.0f);

        glm::mat4 projection(0.0f);
        projection[0][0] = f / aspect;
        projection[1][1] = f;
        projection[2][2] = 0.0f;
        projection[2][3] = -1.0f;
        projection[3][2] = zNear;

        return projection;
    }

    inline glm::ivec2 WorldToScreen(const glm::vec3& worldPos, const glm::mat4& viewProjection, int screenWidth, int screenHeight, bool flipY = false) {
        glm::vec4 clipCoords = viewProjection * glm::vec4(worldPos, 1.0f);
        glm::vec3 ndcCoords = glm::vec3(clipCoords) / clipCoords.w;

        glm::ivec2 screenCoords;
        screenCoords.x = static_cast<int>((ndcCoords.x + 1.0f) * 0.5f * screenWidth);
        screenCoords.y = flipY
            ? static_cast<int>(screenHeight - (ndcCoords.y + 1.0f) * 0.5f * screenHeight)
            : static_cast<int>((1.0f - ndcCoords.y) * 0.5f * screenHeight);

        return screenCoords;
    }

    inline glm::mat4 Oblique(const glm::mat4& projection, const glm::mat4& view, const glm::vec4& plane) {
        glm::mat4 obliqueProjection = projection;
        glm::vec4 viewPlane = glm::transpose(glm::inverse(view)) * plane;

        glm::vec4 q;
        q.x = (glm::sign(viewPlane.x) + projection[2][0]) / projection[0][0];
        q.y = (glm::sign(viewPlane.y) + projection[2][1]) / projection[1][1];
        q.z = -1.0f;
        q.w = (1.0f + projection[2][2]) / projection[3][2];

        glm::vec4 c = viewPlane * (2.0f / glm::dot(viewPlane, q));

        obliqueProjection[0][2] = c.x;
        obliqueProjection[1][2] = c.y;
        obliqueProjection[2][2] = c.z + 1.0f;
        obliqueProjection[3][2] = c.w;

        return obliqueProjection;
    }
}
