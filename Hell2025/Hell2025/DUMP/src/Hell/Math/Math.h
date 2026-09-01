#pragma once

#include "Easing.h"
#include "GLM.h"
#include "Interpolation.h"
#include "Matrix.h"
#include "Range.h"
#include "Ray.h"
#include "Rotation.h"
#include "Hell/Common/Constants.h"
#include "Hell/Transform.h"

#include <cmath>
#include <numeric>
#include <stdexcept>
#include <vector>

namespace Hell::Math {

    inline float DegToRad(float degrees) {
        return degrees * (HELL_PI / 180.0f);
    }

    inline glm::vec3 MidPoint(const glm::vec3& a, const glm::vec3& b) {
        return (a + b) * 0.5f;
    }

    inline float DistSquared(const glm::vec3& a, const glm::vec3& b) {
        glm::vec3 d = a - b;
        return glm::dot(d, d);
    }

    inline float DistSquared2D(const glm::vec2& a, const glm::vec2& b) {
        glm::vec2 d = a - b;
        return glm::dot(d, d);
    }

    inline float ManhattanDistance(const glm::vec3& a, const glm::vec3& b) {
        return std::abs(a.x - b.x) + std::abs(a.y - b.y) + std::abs(a.z - b.z);
    }

    inline bool WithinDistance(const glm::ivec2& a, const glm::ivec2& b, float distance) {
        if (distance < 0.0f) return false;

        glm::vec2 diff = glm::vec2(a) - glm::vec2(b);
        return glm::dot(diff, diff) <= distance * distance;
    }

    inline void NormalizeWeights(std::vector<float>& weights) {
        float sum = std::accumulate(weights.begin(), weights.end(), 0.0f);
        if (sum == 0.0f) {
            throw std::invalid_argument("Sum of weights cannot be zero.");
        }

        for (float& weight : weights) {
            weight /= sum;
        }
    }

    inline float Sanitize(float value, float threshold = 0.002f) {
        if (std::abs(value) < threshold) return 0.0f;
        if (std::abs(value - 1.0f) < threshold) return 1.0f;
        if (std::abs(value + 1.0f) < threshold) return -1.0f;
        return value;
    }

    inline glm::vec3 Sanitize(const glm::vec3& value, float threshold = 0.002f) {
        return glm::vec3(Sanitize(value.x, threshold), Sanitize(value.y, threshold), Sanitize(value.z, threshold));
    }

    inline glm::quat Sanitize(const glm::quat& value, float threshold = 0.002f) {
        glm::quat result = value;
        result.x = Sanitize(value.x, threshold);
        result.y = Sanitize(value.y, threshold);
        result.z = Sanitize(value.z, threshold);
        result.w = Sanitize(value.w, threshold);
        return glm::normalize(result);
    }

    inline void Sanitize(glm::mat4& value, float threshold = 1e-5f) {
        for (int col = 0; col < 4; ++col) {
            for (int row = 0; row < 4; ++row) {
                value[col][row] = Sanitize(value[col][row], threshold);
            }
        }
    }

    inline bool IsNan(float value) {
        return glm::isnan(value);
    }

    inline bool IsNan(const glm::vec2& value) {
        return glm::isnan(value.x) || glm::isnan(value.y);
    }

    inline bool IsNan(const glm::vec3& value) {
        return glm::isnan(value.x) || glm::isnan(value.y) || glm::isnan(value.z);
    }

    inline bool IsNan(const glm::vec4& value) {
        return glm::isnan(value.x) || glm::isnan(value.y) || glm::isnan(value.z) || glm::isnan(value.w);
    }

    inline bool IsNan(const glm::mat4& matrix) {
        return glm::any(glm::isnan(matrix[0])) ||
            glm::any(glm::isnan(matrix[1])) ||
            glm::any(glm::isnan(matrix[2])) ||
            glm::any(glm::isnan(matrix[3]));
    }

    inline bool IsNaN(const glm::mat4& matrix) {
        return IsNan(matrix);
    }

    inline bool PointsEqual(const glm::vec3& a, const glm::vec3& b, float epsilon) {
        glm::vec3 d = a - b;
        return d.x * d.x + d.y * d.y + d.z * d.z <= epsilon * epsilon;
    }

    inline bool IsDegenerateXZ(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c) {
        // Check if any two points are the same
        if (PointsEqual(a, b, 0.0001f) ||
            PointsEqual(b, c, 0.0001f) ||
            PointsEqual(c, a, 0.0001f)) {
            return true;
        }

        // Check for near-zero area (only care about the XZ plane)
        glm::vec2 A(a.x, a.z), B(b.x, b.z), C(c.x, c.z);
        float area2 = (B.x - A.x) * (C.y - A.y) - (B.y - A.y) * (C.x - A.x);

        // Check if area is extremely small
        return std::fabs(area2) < 0.00001f;
    }

    inline glm::vec3 SnapVec3(const glm::vec3& v, int decimalPlaces) {
        static const float pow10[] = {
            1.0f,
            10.0f,
            100.0f,
            1000.0f,
            10000.0f,
            100000.0f
        };

        if (decimalPlaces < 0) decimalPlaces = 0;
        if (decimalPlaces > 5) decimalPlaces = 5;

        float scale = pow10[decimalPlaces];
        return glm::round(v * scale) / scale;
    }

    inline float TriArea2D(const glm::vec2& a, const glm::vec2& b, const glm::vec2& c) {
        return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    }

    inline bool NearlyEqual(const glm::mat4& a, const glm::mat4& b) {
        constexpr float absEps = 1e-8f;
        constexpr float relEps = 1e-5f;

        for (int c = 0; c < 4; ++c) {
            for (int r = 0; r < 4; ++r) {
                const float av = a[c][r];
                const float bv = b[c][r];
                const float diff = glm::abs(av - bv);
                const float largest = glm::max(glm::abs(av), glm::abs(bv));
                const float tolerance = glm::max(absEps, relEps * largest);

                if (diff > tolerance) {
                    return false;
                }
            }
        }

        return true;
    }

    inline bool NearlyEqual(const Hell::Transform& a, const Hell::Transform& b) {
        constexpr float kPosEps = 1e-4f;
        constexpr float kAngEps = 1e-3f;
        constexpr float kScaleEps = 1e-4f;

        return glm::all(glm::lessThanEqual(glm::abs(a.position - b.position), glm::vec3(kPosEps))) &&
            glm::all(glm::lessThanEqual(glm::abs(a.rotation - b.rotation), glm::vec3(kAngEps))) &&
            glm::all(glm::lessThanEqual(glm::abs(a.scale - b.scale), glm::vec3(kScaleEps)));
    }
}
