#include "Matrix.h"

#include <glm/common.hpp>
#include <glm/exponential.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat3x3.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <limits>

namespace {

    template<typename T>
    constexpr T MinimumLengthSquared() {
        return std::numeric_limits<T>::epsilon() * std::numeric_limits<T>::epsilon();
    }

    template<typename T>
    glm::vec<3, T> NormalizeOr(const glm::vec<3, T>& value, const glm::vec<3, T>& fallback) {
        const T lengthSquared = glm::dot(value, value);
        return lengthSquared > MinimumLengthSquared<T>() ? value / glm::sqrt(lengthSquared) : fallback;
    }

    template<typename T>
    glm::mat<4, 4, T> RemoveScaleAndShearImpl(const glm::mat<4, 4, T>& matrix) {
        const glm::vec<3, T> unitX(T(1), T(0), T(0));
        const glm::vec<3, T> unitY(T(0), T(1), T(0));
        const glm::vec<3, T> unitZ(T(0), T(0), T(1));

        const glm::vec<3, T> x = NormalizeOr(glm::vec<3, T>(matrix[0]), unitX);

        glm::vec<3, T> yCandidate(matrix[1]);
        yCandidate -= x * glm::dot(x, yCandidate);
        if (glm::dot(yCandidate, yCandidate) <= MinimumLengthSquared<T>()) {
            const glm::vec<3, T> source = glm::abs(x.y) < T(0.9) ? unitY : unitZ;
            yCandidate = source - x * glm::dot(x, source);
        }
        const glm::vec<3, T> y = NormalizeOr(yCandidate, unitY);
        const glm::vec<3, T> z = NormalizeOr(glm::cross(x, y), unitZ);

        glm::mat<4, 4, T> result(T(1));
        result[0] = glm::vec<4, T>(x, T(0));
        result[1] = glm::vec<4, T>(y, T(0));
        result[2] = glm::vec<4, T>(z, T(0));
        result[3] = matrix[3];
        return result;
    }
}

namespace Hell::Math {

    glm::mat4 RemoveScaleAndShear(const glm::mat4& matrix) {
        return RemoveScaleAndShearImpl(matrix);
    }

    glm::dmat4 RemoveScaleAndShear(const glm::dmat4& matrix) {
        return RemoveScaleAndShearImpl(matrix);
    }

    glm::quat ExtractRotation(const glm::mat4& matrix) {
        const glm::mat4 rigidMatrix = RemoveScaleAndShear(matrix);
        return glm::normalize(glm::quat_cast(glm::mat3(rigidMatrix)));
    }

    void SetRotationPreserveTranslation(glm::mat4& matrix, const glm::quat& rotation) {
        const glm::vec4 translation = matrix[3];
        const float lengthSquared = glm::dot(rotation, rotation);
        const glm::quat normalizedRotation = lengthSquared > std::numeric_limits<float>::epsilon()
            ? rotation / glm::sqrt(lengthSquared)
            : glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

        matrix = glm::mat4_cast(normalizedRotation);
        matrix[3] = translation;
    }
}
