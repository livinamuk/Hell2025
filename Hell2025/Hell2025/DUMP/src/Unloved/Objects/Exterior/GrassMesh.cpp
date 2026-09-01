#include "GrassMesh.h"
#include <random>
#include <glm/gtc/matrix_transform.hpp>
#include "Hell/Logging.h"

void GrassMesh::Create() {

    glm::vec3 offset = glm::vec3(0, 0, 0);

    float spacing = 1.0f / GRASS_SIZE;
    for (int x = 0; x < GRASS_SIZE; x++) {
        for (int z = 0; z < GRASS_SIZE; z++) {
            m_bladePoints[x][z] = glm::vec3(x * spacing, 0, z * spacing);
        }
    }

    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;

    // Grass blade parameters
    float bladeWidth = 0.075f;
    float bladeHeight = 0.15f;
    int segmentCount = 2;
    float bottomWidth = 0.01f;
    float topWidth = 0.0025f;
    float bendStrength = 0.1f;
    float baseJitterStrength = 0.25f;

    std::default_random_engine generator;
    std::uniform_real_distribution<float> randomBend(-1.0f, 1.0f);
    std::uniform_real_distribution<float> randomAngle(0.0f, glm::two_pi<float>());
    std::uniform_real_distribution<float> randomJitter(-baseJitterStrength, baseJitterStrength);

    for (int x = 0; x < GRASS_SIZE; x++) {
        for (int z = 0; z < GRASS_SIZE; z++) {
            glm::vec3 basePos = m_bladePoints[x][z] + offset;
            basePos.x += randomJitter(generator);
            basePos.y += randomJitter(generator) * 0.5;
            basePos.z += randomJitter(generator);

            glm::vec3 bendDirection(
                randomBend(generator) * bendStrength,
                0.0f,
                randomBend(generator) * bendStrength
            );

            float rotationAngle = randomAngle(generator);
            glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), rotationAngle, glm::vec3(0, 1, 0));

            glm::vec3 prevOffset(0.0f); 

            // Front facing blade
            int frontOffset = vertices.size();
            std::vector<glm::vec3> positions;

            for (int s = 0; s <= segmentCount; s++) {
                float t = s / (float)segmentCount;
                float y = t * bladeHeight;
                float width = bottomWidth * (1.0f - t) + topWidth * t;

                prevOffset += bendDirection * t;

                glm::vec2 uvY = glm::vec2(0, t);

                glm::vec3 leftPos = glm::vec3(basePos.x - width, basePos.y + y, basePos.z) + prevOffset;
                glm::vec3 rightPos = glm::vec3(basePos.x + width, basePos.y + y, basePos.z) + prevOffset;

                leftPos = glm::vec3(rotationMatrix * glm::vec4(leftPos - basePos, 1.0f)) + basePos;
                rightPos = glm::vec3(rotationMatrix * glm::vec4(rightPos - basePos, 1.0f)) + basePos;

                positions.push_back(leftPos);
                positions.push_back(rightPos);

                Vertex& v1 = vertices.emplace_back();
                v1.position = leftPos;
                v1.uv = uvY;

                Vertex& v2 = vertices.emplace_back();
                v2.position = rightPos;
                v2.uv = uvY;
            }

            // Front facing blade normals
            for (int s = 0; s < segmentCount; s++) {
                glm::vec3 edge1 = positions[s * 2 + 2] - positions[s * 2];
                glm::vec3 edge2 = positions[s * 2 + 3] - positions[s * 2 + 2];
                glm::vec3 normal = glm::normalize(glm::cross(edge2, edge1));

                vertices[frontOffset + s * 2].normal = normal;
                vertices[frontOffset + s * 2 + 1].normal = normal;
                vertices[frontOffset + s * 2 + 2].normal = normal;
                vertices[frontOffset + s * 2 + 3].normal = normal;
            }

            // Back facing blade
            int backOffset = vertices.size();
            prevOffset = glm::vec3(0.0f);
            positions.clear();

            for (int s = 0; s <= segmentCount; s++) {
                float t = s / (float)segmentCount;
                float y = t * bladeHeight;
                float width = bottomWidth * (1.0f - t) + topWidth * t;

                prevOffset += bendDirection * t;

                glm::vec2 uvY = glm::vec2(1, t);

                glm::vec3 rightPos = glm::vec3(basePos.x + width, basePos.y + y, basePos.z) + prevOffset;
                glm::vec3 leftPos = glm::vec3(basePos.x - width, basePos.y + y, basePos.z) + prevOffset;

                rightPos = glm::vec3(rotationMatrix * glm::vec4(rightPos - basePos, 1.0f)) + basePos;
                leftPos = glm::vec3(rotationMatrix * glm::vec4(leftPos - basePos, 1.0f)) + basePos;

                positions.push_back(rightPos);
                positions.push_back(leftPos);

                Vertex& v3 = vertices.emplace_back();
                v3.position = rightPos;
                v3.uv = uvY;

                Vertex& v4 = vertices.emplace_back();
                v4.position = leftPos;
                v4.uv = uvY;
            }

            // Back facing blade normals
            for (int s = 0; s < segmentCount; s++) {
                glm::vec3 edge1 = positions[s * 2 + 2] - positions[s * 2];
                glm::vec3 edge2 = positions[s * 2 + 3] - positions[s * 2 + 2];
                glm::vec3 normal = glm::normalize(glm::cross(edge2, edge1));

                vertices[backOffset + s * 2].normal = normal;
                vertices[backOffset + s * 2 + 1].normal = normal;
                vertices[backOffset + s * 2 + 2].normal = normal;
                vertices[backOffset + s * 2 + 3].normal = normal;
            }

            // Corrected Index Order for Triangle Strip**
            for (int s = 0; s <= segmentCount; s++) {
                indices.push_back(frontOffset + s * 2);
                indices.push_back(frontOffset + s * 2 + 1);
            }

            // Insert degenerate vertices
            indices.push_back(frontOffset + segmentCount * 2 + 1);
            indices.push_back(backOffset + 0);

            for (int s = 0; s <= segmentCount; s++) {
                indices.push_back(backOffset + s * 2);
                indices.push_back(backOffset + s * 2 + 1);
            }

            // Insert final degenerate triangle
            if (x < GRASS_SIZE - 1 || z < GRASS_SIZE - 1) {
                indices.push_back(backOffset + segmentCount * 2 + 1);
                indices.push_back(backOffset + segmentCount * 2 + 2);
            }
        }
    }

    glMesh.UpdateBuffers(vertices, indices);

    Logging::Error() << "Grass indices: " << indices.size() << "\n";
}
