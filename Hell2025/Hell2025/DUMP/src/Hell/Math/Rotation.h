#pragma once

#ifndef GLM_ENABLE_EXPERIMENTAL
    #define GLM_ENABLE_EXPERIMENTAL
#endif

#include <cmath>

#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace Hell::Math {

    inline float WrapRadians(float angle) {
        return std::remainder(angle, glm::two_pi<float>());
    }

    inline glm::quat EulerXYZToQuaternion(const glm::vec3& radians) {
        const glm::quat rotateX = glm::angleAxis(radians.x, glm::vec3(1.0f, 0.0f, 0.0f));
        const glm::quat rotateY = glm::angleAxis(radians.y, glm::vec3(0.0f, 1.0f, 0.0f));
        const glm::quat rotateZ = glm::angleAxis(radians.z, glm::vec3(0.0f, 0.0f, 1.0f));
        return glm::normalize(rotateZ * rotateY * rotateX);
    }

    inline glm::vec3 QuaternionToEulerXYZ(const glm::quat& rotation) {
        return glm::eulerAngles(glm::normalize(rotation));
    }

    inline glm::vec3 NearestEulerEquivalent(const glm::quat& rotation, const glm::vec3& reference) {
        glm::vec3 primary = QuaternionToEulerXYZ(rotation);
        glm::vec3 secondary(
            primary.x + glm::pi<float>(),
            glm::pi<float>() - primary.y,
            primary.z + glm::pi<float>()
        );

        for (int axisIndex = 0; axisIndex < 3; axisIndex++) {
            primary[axisIndex] = reference[axisIndex] + WrapRadians(primary[axisIndex] - reference[axisIndex]);
            secondary[axisIndex] = reference[axisIndex] + WrapRadians(secondary[axisIndex] - reference[axisIndex]);
        }

        const glm::vec3 primaryDelta = primary - reference;
        const glm::vec3 secondaryDelta = secondary - reference;
        return glm::dot(primaryDelta, primaryDelta) <= glm::dot(secondaryDelta, secondaryDelta) ? primary : secondary;
    }

    inline glm::quat RotationFromTo(const glm::vec3& from, const glm::vec3& to) {
        constexpr float MIN_DIRECTION_LENGTH_SQUARED = 1e-12f;
        const float fromLengthSquared = glm::dot(from, from);
        const float toLengthSquared = glm::dot(to, to);
        if (fromLengthSquared <= MIN_DIRECTION_LENGTH_SQUARED || toLengthSquared <= MIN_DIRECTION_LENGTH_SQUARED) {
            return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        }

        const glm::vec3 normalizedFrom = from / std::sqrt(fromLengthSquared);
        const glm::vec3 normalizedTo = to / std::sqrt(toLengthSquared);
        const float cosine = glm::clamp(glm::dot(normalizedFrom, normalizedTo), -1.0f, 1.0f);

        if (cosine > 1.0f - 1e-6f) {
            return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        }

        if (cosine < -1.0f + 1e-6f) {
            const glm::vec3 referenceAxis = glm::abs(normalizedFrom.y) < 0.9f
                ? glm::vec3(0.0f, 1.0f, 0.0f)
                : glm::vec3(0.0f, 0.0f, 1.0f);
            return glm::angleAxis(glm::pi<float>(), glm::normalize(glm::cross(normalizedFrom, referenceAxis)));
        }

        const glm::vec3 axis = glm::cross(normalizedFrom, normalizedTo);
        return glm::normalize(glm::quat(1.0f + cosine, axis.x, axis.y, axis.z));
    }

    inline glm::vec3 EulerRotationFromNormal(const glm::vec3& normal, const glm::vec3& forward = glm::vec3(0.0f, 0.0f, 1.0f)) {
        glm::vec3 normalizedNormal = glm::normalize(normal);
        glm::quat q = glm::rotation(forward, normalizedNormal);
        glm::vec3 euler = glm::eulerAngles(q);
        euler.z = 0.0f;
        return euler;
    }

    inline float YawBetweenPoints(const glm::vec3& a, const glm::vec3& b) {
        float deltaX = b.x - a.x;
        float deltaZ = b.z - a.z;
        float thetaRadians = std::atan2(deltaZ, deltaX);
        return -thetaRadians;
    }

    inline glm::mat4 RotationMatrixFromForward(const glm::vec3& forward, const glm::vec3& worldForward, const glm::vec3& worldUp) {
        (void)worldUp;
        glm::vec3 normalizedForward = glm::normalize(forward);
        glm::vec3 normalizedWorldForward = glm::normalize(worldForward);
        glm::quat q = glm::rotation(normalizedWorldForward, normalizedForward);
        return glm::mat4_cast(q);
    }
}
