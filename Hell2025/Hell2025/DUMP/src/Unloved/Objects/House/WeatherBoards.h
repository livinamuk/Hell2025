#pragma once
#include "glm/glm.hpp"

namespace Unloved {

struct WeatherBoards {
    glm::vec3 begin;
    glm::vec3 end;
    float height;
    float beginOffset = 0.0f;
    float endOffset = 0.0f;
};
}
