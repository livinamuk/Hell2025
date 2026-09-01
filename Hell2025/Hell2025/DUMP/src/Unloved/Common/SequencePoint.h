#pragma once

#include <glm/vec3.hpp>

namespace Unloved {

struct SequencePoint {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 normal = glm::vec3(0.0f, 1.0f, 0.0f);
    float customFloat = 0.0f;
    bool customBool = true;
};

}
