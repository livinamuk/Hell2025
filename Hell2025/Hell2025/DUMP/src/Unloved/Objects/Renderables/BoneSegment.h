#pragma once

#include <glm/vec3.hpp>
#include <string>

namespace Unloved {

struct BoneSegment {
    std::string boneName;
    glm::vec3 start = glm::vec3(0.0f);
    glm::vec3 end = glm::vec3(0.0f);
};

}