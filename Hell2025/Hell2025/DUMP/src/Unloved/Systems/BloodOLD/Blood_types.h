#pragma once

#include "glm/vec3.hpp"

struct BloodScreenSpaceDecalCreateInfo {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 direction = glm::vec3(0.0f);
    int type = 0;
};
