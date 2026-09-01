#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif

#include "Transform.h"
#include "LocalFrame.h"

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/vec4.hpp>

namespace Hell {

    Transform::Transform(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale) {
        this->position = position;
        this->rotation = rotation;
        this->scale = scale;
    }

    glm::mat4 Transform::ToMat4() const {
        glm::mat4 m = glm::translate(glm::mat4(1), position);
        m *= glm::mat4_cast(glm::quat(rotation));
        m = glm::scale(m, scale);
        return m;
    }

    QuatTransform::QuatTransform(glm::mat4 matrix) {
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(matrix, scale, rotation, translation, skew, perspective);
    }

    QuatTransform::QuatTransform(const glm::vec3& position, const LocalFrame& localFrame, const glm::vec3& scale)
        : translation(position)
        , rotation(glm::normalize(glm::quat_cast(localFrame.ToRotationMatrix())))
        , scale(scale) {
    }

    glm::mat4 QuatTransform::ToMat4() const {
        glm::mat4 m = glm::translate(glm::mat4(1), translation);
        m *= glm::mat4_cast(rotation);
        m = glm::scale(m, scale);
        return m;
    }

    glm::vec3 QuatTransform::Forward() const {
        return glm::normalize(rotation * glm::vec3(0.0f, 0.0f, 1.0f));
    }
}
