#pragma once

#include "Hell/Math/Interpolation.h"

#include <algorithm>
#include <vector>

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

namespace Hell::Curve {

    inline glm::vec3 BezierEval(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, float t) {
        float u = 1.0f - t;
        float uu = u * u;
        float tt = t * t;
        return u * uu * p0 + 3.0f * uu * t * p1 + 3.0f * u * tt * p2 + tt * t * p3;
    }

    inline std::vector<glm::vec3> SampleBezierPath(const std::vector<glm::vec3>& controlPoints, float spacing) {
        std::vector<glm::vec3> result;
        if (controlPoints.size() < 2) return result;
        if (spacing <= 0.0f) return result;

        struct Segment { glm::vec3 b0, b1, b2, b3; };
        std::vector<Segment> segments;
        segments.reserve(std::max<int>(1, static_cast<int>(controlPoints.size()) - 1));

        for (int i = 0; i < static_cast<int>(controlPoints.size()) - 1; ++i) {
            const glm::vec3& pMinus1 = (i > 0) ? controlPoints[i - 1] : controlPoints[i];
            const glm::vec3& p0 = controlPoints[i];
            const glm::vec3& p1 = controlPoints[i + 1];
            const glm::vec3& p2 = (i + 2 < static_cast<int>(controlPoints.size())) ? controlPoints[i + 2] : controlPoints[i + 1];

            glm::vec3 b0 = p0;
            glm::vec3 b3 = p1;
            glm::vec3 b1 = p0 + (p1 - pMinus1) / 6.0f;
            glm::vec3 b2 = p1 - (p2 - p0) / 6.0f;

            segments.push_back({ b0, b1, b2, b3 });
        }

        std::vector<glm::vec3> dense;
        dense.reserve(segments.size() * 64 + 1);

        constexpr int samplesPerSegment = 64;
        for (size_t s = 0; s < segments.size(); ++s) {
            const Segment& segment = segments[s];
            for (int i = 0; i < samplesPerSegment; ++i) {
                float t = static_cast<float>(i) / static_cast<float>(samplesPerSegment);
                dense.push_back(BezierEval(segment.b0, segment.b1, segment.b2, segment.b3, t));
            }
        }
        dense.push_back(segments.back().b3);

        if (dense.empty()) return result;

        result.push_back(dense.front());
        float nextDist = spacing;
        float accum = 0.0f;

        for (size_t i = 1; i < dense.size(); ++i) {
            const glm::vec3& a = dense[i - 1];
            const glm::vec3& b = dense[i];
            accum += glm::length(b - a);

            while (accum >= nextDist) {
                float over = accum - nextDist;
                float segmentLength = glm::length(b - a);
                float t = segmentLength > 0.0f ? 1.0f - (over / segmentLength) : 0.0f;
                glm::vec3 point = a + (b - a) * t;
                if (glm::length(point - result.back()) > 1e-5f) result.push_back(point);
                nextDist += spacing;
            }
        }

        if (glm::length(result.back() - dense.back()) > 0.01f) {
            result.push_back(dense.back());
        }

        return result;
    }

    inline std::vector<glm::vec3> GenerateSagPoints(const glm::vec3& start, const glm::vec3& end, int numPoints, float sagAmount) {
        std::vector<glm::vec3> points;
        if (numPoints <= 0) return points;
        if (numPoints == 1) {
            points.push_back(start);
            return points;
        }

        float totalDistanceX = end.x - start.x;
        float totalDistanceZ = end.z - start.z;

        for (int i = 0; i < numPoints; ++i) {
            float t = static_cast<float>(i) / (numPoints - 1);
            float x = start.x + t * totalDistanceX;
            float z = start.z + t * totalDistanceZ;
            float y = Hell::Math::ChristmasLerp(start.y, end.y, t);
            float sag = sagAmount * (4.0f * (t - 0.5f) * (t - 0.5f) - 1.0f);
            y += sag;
            points.push_back(glm::vec3(x, y, z));
        }

        return points;
    }
}
