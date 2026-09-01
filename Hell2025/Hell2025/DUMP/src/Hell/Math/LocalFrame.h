#pragma once

#include <glm/mat3x3.hpp>
#include <glm/vec3.hpp>

namespace Hell {

struct LocalFrame {
    glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 forward = glm::vec3(0.0f, 0.0f, 1.0f);

    LocalFrame() = default;

    explicit LocalFrame(const glm::vec3& forward)
        : forward(glm::normalize(forward)) {

        glm::vec3 referenceUp(0.0f, 1.0f, 0.0f);

        if (glm::abs(glm::dot(
            this->forward,
            glm::vec3(0.0f, 1.0f, 0.0f))) > 0.999f) {

            referenceUp = glm::vec3(1.0f, 0.0f, 0.0f);
        }

        right = glm::normalize(
            glm::cross(referenceUp, this->forward)
        );

        up = glm::normalize(
            glm::cross(this->forward, right)
        );
    }

    glm::mat3 ToRotationMatrix() const {
        return glm::mat3(right, up, forward);
    }
};

}