#pragma once

#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace Hell {

    struct LocalFrame;

    struct Transform {
        glm::vec3 position = glm::vec3(0);
        glm::vec3 rotation = glm::vec3(0);
        glm::vec3 scale = glm::vec3(1);

        Transform() = default;
        explicit Transform(const glm::vec3& position, const glm::vec3& rotation = glm::vec3(0.0f), const glm::vec3& scale = glm::vec3(1.0f));

        glm::mat4 ToMat4() const;
        glm::mat4 to_mat4() const { return ToMat4(); }
    };

    struct QuatTransform {
        QuatTransform() = default;
        QuatTransform(const glm::vec3& position, const LocalFrame& localFrame, const glm::vec3& scale = glm::vec3(1.0f));
        explicit QuatTransform(glm::mat4 matrix);

        glm::mat4 ToMat4() const;
        glm::mat4 to_mat4() const { return ToMat4(); }
        glm::vec3 Forward() const;
        glm::vec3 to_forward_vector() const { return Forward(); }

        glm::vec3 translation = glm::vec3(0);
        glm::quat rotation = glm::quat(1, 0, 0, 0);
        glm::vec3 scale = glm::vec3(1);
    };
}
