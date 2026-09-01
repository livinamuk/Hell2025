#pragma once

#include <cmath>

#include <glm/common.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

namespace Hell::Math {

    inline float ChristmasLerp(float start, float end, float t) {
        return start + t * (end - start);
    }

    inline float InterpTo(float current, float target, float deltaTime, float interpSpeed) {
        if (interpSpeed <= 0.0f) {
            return target;
        }

        const float distance = target - current;
        if (distance * distance < 9.99999993922529e-9f) {
            return target;
        }

        return current + distance * glm::clamp(deltaTime * interpSpeed, 0.0f, 1.0f);
    }

    inline glm::vec3 InterpTo(const glm::vec3& current, const glm::vec3& target, float deltaTime, float interpSpeed) {
        glm::vec3 result;
        result.x = InterpTo(current.x, target.x, deltaTime, interpSpeed);
        result.y = InterpTo(current.y, target.y, deltaTime, interpSpeed);
        result.z = InterpTo(current.z, target.z, deltaTime, interpSpeed);
        return result;
    }

    inline glm::quat Slerp(const glm::quat& start, const glm::quat& end, float factor) {
        float cosom = start.x * end.x + start.y * end.y + start.z * end.z + start.w * end.w;
        glm::quat adjustedEnd = end;

        if (cosom < 0.0f) {
            cosom = -cosom;
            adjustedEnd.x = -adjustedEnd.x;
            adjustedEnd.y = -adjustedEnd.y;
            adjustedEnd.z = -adjustedEnd.z;
            adjustedEnd.w = -adjustedEnd.w;
        }

        float sclp;
        float sclq;
        if ((1.0f - cosom) > 0.0001f) {
            float omega = std::acos(cosom);
            float sinom = std::sin(omega);
            sclp = std::sin((1.0f - factor) * omega) / sinom;
            sclq = std::sin(factor * omega) / sinom;
        }
        else {
            sclp = 1.0f - factor;
            sclq = factor;
        }

        glm::quat result;
        result.x = sclp * start.x + sclq * adjustedEnd.x;
        result.y = sclp * start.y + sclq * adjustedEnd.y;
        result.z = sclp * start.z + sclq * adjustedEnd.z;
        result.w = sclp * start.w + sclq * adjustedEnd.w;
        return result;
    }
}
