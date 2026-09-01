#pragma once

#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>

namespace Hell::Math {

    glm::mat4 RemoveScaleAndShear(const glm::mat4& matrix);
    glm::dmat4 RemoveScaleAndShear(const glm::dmat4& matrix);
    glm::quat ExtractRotation(const glm::mat4& matrix);
    void SetRotationPreserveTranslation(glm::mat4& matrix, const glm::quat& rotation);
}
