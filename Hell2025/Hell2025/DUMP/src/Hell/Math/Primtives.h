#pragma once

#include <glm/vec3.hpp>

#include <cstdint>
#include <vector>

namespace Hell::Primtiives {

    inline std::vector<glm::vec3> CreateCubeVertices(float size) {
        std::vector<glm::vec3> vertices;
        vertices.push_back(glm::vec3(-size, size, size));	// Top
        vertices.push_back(glm::vec3(-size, size, -size));
        vertices.push_back(glm::vec3(size, size, -size));
        vertices.push_back(glm::vec3(size, size, size));
        vertices.push_back(glm::vec3(-size, -size, size));	// Bottom
        vertices.push_back(glm::vec3(-size, -size, -size));
        vertices.push_back(glm::vec3(size, -size, -size));
        vertices.push_back(glm::vec3(size, -size, size));
        vertices.push_back(glm::vec3(-size, size, size));	// Z front
        vertices.push_back(glm::vec3(-size, -size, size));
        vertices.push_back(glm::vec3(size, -size, size));
        vertices.push_back(glm::vec3(size, size, size));
        vertices.push_back(glm::vec3(-size, size, -size));	// Z back
        vertices.push_back(glm::vec3(-size, -size, -size));
        vertices.push_back(glm::vec3(size, -size, -size));
        vertices.push_back(glm::vec3(size, size, -size));
        vertices.push_back(glm::vec3(size, size, -size));	// X front
        vertices.push_back(glm::vec3(size, -size, -size));
        vertices.push_back(glm::vec3(size, -size, size));
        vertices.push_back(glm::vec3(size, size, size));
        vertices.push_back(glm::vec3(-size, size, -size));	// X back
        vertices.push_back(glm::vec3(-size, -size, -size));
        vertices.push_back(glm::vec3(-size, -size, size));
        vertices.push_back(glm::vec3(-size, size, size));
        return vertices;
    }

    inline std::vector<uint32_t> CreateCubeIndices() {
        return { 0, 1, 3, 1, 2, 3, 7, 5, 4, 7, 6, 5, 11, 9, 8, 11, 10, 9, 12, 13, 15, 13, 14, 15, 16, 17, 19, 17, 18, 19, 23, 21, 20, 23, 22, 21 };
    }

}