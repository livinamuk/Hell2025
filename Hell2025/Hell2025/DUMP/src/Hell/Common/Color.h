#pragma once

#include <glm/common.hpp>
#include <glm/vec3.hpp>

#include <cmath>

namespace Hell::Color {

    inline glm::vec3 Random(int seed) {
        float hue = std::sin(static_cast<float>(seed) * 17.17f) * 43758.5453f;
        hue = hue - std::floor(hue);

        const glm::vec3 hsvOffset = glm::vec3(0.0f, 2.0f / 3.0f, 1.0f / 3.0f);
        glm::vec3 rgb = glm::abs(glm::fract(glm::vec3(hue) + hsvOffset) * 6.0f - 3.0f) - 1.0f;
        rgb = glm::clamp(rgb, glm::vec3(0.0f), glm::vec3(1.0f));
        rgb = glm::mix(glm::vec3(1.0f), rgb, 0.85f);

        return rgb;
    }
}
