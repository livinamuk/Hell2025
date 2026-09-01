#include "PrimitiveMesh.h"

#include <algorithm>
#include <cmath>
#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>

namespace Hell::PrimitiveMesh {

std::vector<Vertex> GenerateRingVertices(float sphereRadius, float ringThickness, int segments, int thicknessSegments) {
    std::vector<Vertex> vertices;
    for (int i = 0; i < segments; ++i) {
        float angle = glm::two_pi<float>() * i / segments;
        glm::vec3 ringCenter = glm::vec3(
            sphereRadius * std::cos(angle),
            sphereRadius * std::sin(angle),
            0.0f
        );

        for (int j = 0; j < thicknessSegments; ++j) {
            float thicknessAngle = glm::two_pi<float>() * j / thicknessSegments;
            glm::vec3 offset = glm::vec3(
                ringThickness * std::cos(thicknessAngle) * std::cos(angle),
                ringThickness * std::cos(thicknessAngle) * std::sin(angle),
                ringThickness * std::sin(thicknessAngle)
            );

            Vertex& vertex = vertices.emplace_back();
            vertex.position = ringCenter + offset;
            vertex.normal = glm::normalize(offset);
            vertex.tangent = glm::normalize(glm::vec3(-std::sin(angle), std::cos(angle), 0.0f));
        }
    }

    return vertices;
}

std::vector<uint32_t> GenerateRingIndices(int segments, int thicknessSegments) {
    std::vector<uint32_t> indices;
    for (int i = 0; i < segments; ++i) {
        for (int j = 0; j < thicknessSegments; ++j) {
            int nextI = (i + 1) % segments;
            int nextJ = (j + 1) % thicknessSegments;
            uint32_t v0 = i * thicknessSegments + j;
            uint32_t v1 = nextI * thicknessSegments + j;
            uint32_t v2 = i * thicknessSegments + nextJ;
            uint32_t v3 = nextI * thicknessSegments + nextJ;

            indices.push_back(v0);
            indices.push_back(v1);
            indices.push_back(v2);
            indices.push_back(v2);
            indices.push_back(v1);
            indices.push_back(v3);
        }
    }

    return indices;
}

std::vector<Vertex> GenerateSphereVertices(float radius, int latitudeSegments, int longitudeSegments) {
    std::vector<Vertex> vertices;
    latitudeSegments = std::max(latitudeSegments, 2);
    longitudeSegments = std::max(longitudeSegments, 3);
    vertices.reserve((latitudeSegments + 1) * (longitudeSegments + 1));

    for (int latitude = 0; latitude <= latitudeSegments; ++latitude) {
        const float v = static_cast<float>(latitude) / static_cast<float>(latitudeSegments);
        const float phi = v * glm::pi<float>();
        const float sinPhi = std::sin(phi);
        const float cosPhi = std::cos(phi);

        for (int longitude = 0; longitude <= longitudeSegments; ++longitude) {
            const float u = static_cast<float>(longitude) / static_cast<float>(longitudeSegments);
            const float theta = u * glm::two_pi<float>();
            const float sinTheta = std::sin(theta);
            const float cosTheta = std::cos(theta);
            const glm::vec3 normal(sinPhi * cosTheta, cosPhi, sinPhi * sinTheta);

            Vertex& vertex = vertices.emplace_back();
            vertex.position = normal * radius;
            vertex.normal = normal;
            vertex.uv = glm::vec2(u, v);
            vertex.tangent = glm::vec3(-sinTheta, 0.0f, cosTheta);
        }
    }

    return vertices;
}

std::vector<Vertex> GenerateSphereVertices(float radius, int segments) {
    segments = std::max(segments, 4);
    return GenerateSphereVertices(radius, segments, segments);
}

std::vector<uint32_t> GenerateSphereIndices(int latitudeSegments, int longitudeSegments) {
    std::vector<uint32_t> indices;
    latitudeSegments = std::max(latitudeSegments, 2);
    longitudeSegments = std::max(longitudeSegments, 3);
    indices.reserve(latitudeSegments * longitudeSegments * 6);

    const int verticesPerRow = longitudeSegments + 1;
    for (int latitude = 0; latitude < latitudeSegments; ++latitude) {
        for (int longitude = 0; longitude < longitudeSegments; ++longitude) {
            const uint32_t v0 = latitude * verticesPerRow + longitude;
            const uint32_t v1 = (latitude + 1) * verticesPerRow + longitude;
            const uint32_t v2 = v0 + 1;
            const uint32_t v3 = v1 + 1;

            indices.push_back(v2);
            indices.push_back(v1);
            indices.push_back(v0);
            indices.push_back(v3);
            indices.push_back(v1);
            indices.push_back(v2);
        }
    }

    return indices;
}

std::vector<uint32_t> GenerateSphereIndices(int segments) {
    segments = std::max(segments, 4);
    return GenerateSphereIndices(segments, segments);
}

std::vector<Vertex> GenerateCapsuleVertices(float radius, float cylinderLength, int hemisphereRings, int segments) {
    std::vector<Vertex> vertices;
    radius = std::max(radius, 0.0f);
    cylinderLength = std::max(cylinderLength, 0.0f);
    hemisphereRings = std::max(hemisphereRings, 1);
    segments = std::max(segments, 3);

    const int profileRows = (hemisphereRings + 1) * 2;
    const int verticesPerRow = segments + 1;
    const float halfLength = cylinderLength * 0.5f;
    vertices.reserve(profileRows * verticesPerRow);

    for (int profileIndex = 0; profileIndex < profileRows; ++profileIndex) {
        const bool leftHemisphere = profileIndex <= hemisphereRings;
        const int hemisphereIndex = leftHemisphere ? profileIndex : profileIndex - hemisphereRings - 1;
        const float hemisphereT = static_cast<float>(hemisphereIndex) / static_cast<float>(hemisphereRings);
        const float profileAngle = leftHemisphere
            ? -glm::half_pi<float>() + hemisphereT * glm::half_pi<float>()
            : hemisphereT * glm::half_pi<float>();
        const float axialNormal = std::sin(profileAngle);
        const float radialNormal = std::cos(profileAngle);
        const float hemisphereCenter = leftHemisphere ? -halfLength : halfLength;
        const float axialPosition = hemisphereCenter + axialNormal * radius;
        const float radialPosition = radialNormal * radius;
        const float v = static_cast<float>(profileIndex) / static_cast<float>(profileRows - 1);

        for (int segment = 0; segment <= segments; ++segment) {
            const float u = static_cast<float>(segment) / static_cast<float>(segments);
            const float angle = u * glm::two_pi<float>();
            const float sinAngle = std::sin(angle);
            const float cosAngle = std::cos(angle);

            Vertex& vertex = vertices.emplace_back();
            vertex.position = glm::vec3(axialPosition, radialPosition * sinAngle, radialPosition * cosAngle);
            vertex.normal = glm::vec3(axialNormal, radialNormal * sinAngle, radialNormal * cosAngle);
            vertex.uv = glm::vec2(u, v);
            vertex.tangent = glm::vec3(0.0f, cosAngle, -sinAngle);
        }
    }

    return vertices;
}

std::vector<uint32_t> GenerateCapsuleIndices(int hemisphereRings, int segments) {
    std::vector<uint32_t> indices;
    hemisphereRings = std::max(hemisphereRings, 1);
    segments = std::max(segments, 3);

    const int profileRows = (hemisphereRings + 1) * 2;
    const int verticesPerRow = segments + 1;
    indices.reserve((profileRows - 1) * segments * 6);

    for (int profileIndex = 0; profileIndex < profileRows - 1; ++profileIndex) {
        for (int segment = 0; segment < segments; ++segment) {
            const uint32_t bottomLeft = profileIndex * verticesPerRow + segment;
            const uint32_t bottomRight = bottomLeft + 1;
            const uint32_t topLeft = bottomLeft + verticesPerRow;
            const uint32_t topRight = topLeft + 1;

            indices.push_back(bottomLeft);
            indices.push_back(topLeft);
            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);
            indices.push_back(bottomRight);
        }
    }

    return indices;
}

std::vector<Vertex> GenerateConeVertices(float radius, float height, int segments) {
    std::vector<Vertex> vertices;
    segments = std::max(segments, 3);

    Vertex& apex = vertices.emplace_back();
    apex.position = glm::vec3(0.0f, height, 0.0f);
    apex.normal = glm::normalize(glm::vec3(0.0f, height, 0.0f));
    apex.tangent = glm::vec3(1.0f, 0.0f, 0.0f);

    float angleStep = glm::two_pi<float>() / segments;
    for (int i = 0; i < segments; ++i) {
        float angle = i * angleStep;
        glm::vec3 position(radius * std::cos(angle), 0.0f, radius * std::sin(angle));
        glm::vec3 normal = glm::normalize(glm::vec3(position.x, height / 2.0f, position.z));
        glm::vec3 tangent = glm::normalize(glm::vec3(-std::sin(angle), 0.0f, std::cos(angle)));

        Vertex& baseVertex = vertices.emplace_back();
        baseVertex.position = position;
        baseVertex.normal = normal;
        baseVertex.tangent = tangent;
    }

    Vertex& baseCenter = vertices.emplace_back();
    baseCenter.position = glm::vec3(0.0f, 0.0f, 0.0f);
    baseCenter.normal = glm::vec3(0.0f, -1.0f, 0.0f);
    baseCenter.tangent = glm::vec3(1.0f, 0.0f, 0.0f);

    return vertices;
}

std::vector<uint32_t> GenerateConeIndices(int segments) {
    std::vector<uint32_t> indices;

    for (int i = 0; i < segments; ++i) {
        uint32_t apexIndex = 0;
        uint32_t baseIndex1 = i + 1;
        uint32_t baseIndex2 = (i + 1) % segments + 1;

        indices.push_back(baseIndex2);
        indices.push_back(baseIndex1);
        indices.push_back(apexIndex);
    }

    uint32_t centerIndex = segments + 1;
    for (int i = 0; i < segments; ++i) {
        uint32_t baseIndex1 = i + 1;
        uint32_t baseIndex2 = (i + 1) % segments + 1;

        indices.push_back(baseIndex1);
        indices.push_back(baseIndex2);
        indices.push_back(centerIndex);
    }

    return indices;
}

std::vector<Vertex> GenerateCylinderVertices(float radius, float height, int subdivisions) {
    std::vector<Vertex> vertices;
    const float angleStep = glm::two_pi<float>() / subdivisions;

    vertices.push_back(Vertex(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
    for (int i = 0; i <= subdivisions; ++i) {
        float angle = i * angleStep;
        float x = radius * std::cos(angle);
        float z = radius * std::sin(angle);
        vertices.push_back(Vertex(glm::vec3(x, 0.0f, z), glm::vec3(0.0f, -1.0f, 0.0f)));
    }

    vertices.push_back(Vertex(glm::vec3(0.0f, height, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)));
    for (int i = 0; i <= subdivisions; ++i) {
        float angle = i * angleStep;
        float x = radius * std::cos(angle);
        float z = radius * std::sin(angle);
        vertices.push_back(Vertex(glm::vec3(x, height, z), glm::vec3(0.0f, 1.0f, 0.0f)));
    }

    for (int i = 0; i <= subdivisions; ++i) {
        float angle = i * angleStep;
        float x = radius * std::cos(angle);
        float z = radius * std::sin(angle);
        glm::vec3 normal = glm::normalize(glm::vec3(std::cos(angle), 0.0f, std::sin(angle)));
        vertices.push_back(Vertex(glm::vec3(x, 0.0f, z), normal));
        vertices.push_back(Vertex(glm::vec3(x, height, z), normal));
    }

    return vertices;
}

std::vector<uint32_t> GenerateCylinderIndices(int subdivisions) {
    std::vector<uint32_t> indices;

    for (int i = 1; i <= subdivisions; ++i) {
        indices.push_back(0);
        indices.push_back(i);
        indices.push_back(i % subdivisions + 1);
    }

    int topCenterIndex = subdivisions + 2;
    for (int i = 1; i <= subdivisions; ++i) {
        indices.push_back(topCenterIndex);
        indices.push_back(topCenterIndex + i);
        indices.push_back(topCenterIndex + (i % subdivisions) + 1);
    }

    int sideStartIndex = (subdivisions + 2) * 2;
    for (int i = 0; i < subdivisions; ++i) {
        int bottomIndex = sideStartIndex + i * 2;
        int topIndex = bottomIndex + 1;

        indices.push_back(bottomIndex);
        indices.push_back(topIndex);
        indices.push_back(bottomIndex + 2);
        indices.push_back(topIndex);
        indices.push_back(topIndex + 2);
        indices.push_back(bottomIndex + 2);
    }

    return indices;
}

std::vector<Vertex> GenerateCubeVertices() {
    std::vector<Vertex> vertices;
    glm::vec3 normals[] = {
        { 0.0f,  0.0f,  1.0f},
        { 0.0f,  0.0f, -1.0f},
        { 1.0f,  0.0f,  0.0f},
        {-1.0f,  0.0f,  0.0f},
        { 0.0f,  1.0f,  0.0f},
        { 0.0f, -1.0f,  0.0f},
    };
    glm::vec3 positions[] = {
        {-0.5f, -0.5f,  0.5f}, {0.5f, -0.5f,  0.5f}, {0.5f,  0.5f,  0.5f}, {-0.5f,  0.5f,  0.5f},
        {-0.5f, -0.5f, -0.5f}, {-0.5f,  0.5f, -0.5f}, {0.5f,  0.5f, -0.5f}, {0.5f, -0.5f, -0.5f},
        {0.5f,  -0.5f,  0.5f}, {0.5f, -0.5f, -0.5f}, {0.5f,  0.5f, -0.5f}, {0.5f,  0.5f,  0.5f},
        {-0.5f, -0.5f,  0.5f}, {-0.5f,  0.5f,  0.5f}, {-0.5f,  0.5f, -0.5f}, {-0.5f, -0.5f, -0.5f},
        {-0.5f,  0.5f,  0.5f}, {0.5f,  0.5f,  0.5f}, {0.5f,  0.5f, -0.5f}, {-0.5f,  0.5f, -0.5f},
        {-0.5f, -0.5f,  0.5f}, {-0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, -0.5f}, {0.5f, -0.5f,  0.5f},
    };
    const glm::vec2 uvs[] = {
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 1.0f}
    };

    for (int i = 0; i < 6; ++i) {
        const glm::vec3 tangent = glm::normalize(positions[i * 4 + 1] - positions[i * 4]);
        for (int j = 0; j < 4; ++j) {
            Vertex& vertex = vertices.emplace_back();
            vertex.position = positions[i * 4 + j];
            vertex.normal = normals[i];
            vertex.uv = uvs[j];
            vertex.tangent = tangent;
        }
    }

    return vertices;
}

std::vector<uint32_t> GenerateCubeIndices() {
    return {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4,
        8, 9, 10, 10, 11, 8,
        12, 13, 14, 14, 15, 12,
        16, 17, 18, 18, 19, 16,
        20, 21, 22, 22, 23, 20
    };
}

}
