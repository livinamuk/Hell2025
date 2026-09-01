#include "RagdollShapeMesh.h"
#include "Hell/Geometry/PrimitiveMesh.h"

#include <cmath>
#include <cstddef>
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace {
    constexpr int SPHERE_LATITUDE_SEGMENTS = 16;
    constexpr int SPHERE_LONGITUDE_SEGMENTS = 24;
    constexpr int CAPSULE_HEMISPHERE_RINGS = 12;
    constexpr int CAPSULE_SEGMENTS = 24;

    void ApplyLocalShapeTransform(RagdollShapeMeshData& meshData, const RagdollShape& shape) {
        glm::mat4 rotationMatrix(1.0f);
        rotationMatrix = glm::rotate(rotationMatrix, shape.rotationRadians.z, glm::vec3(0.0f, 0.0f, 1.0f));
        rotationMatrix = glm::rotate(rotationMatrix, shape.rotationRadians.y, glm::vec3(0.0f, 1.0f, 0.0f));
        rotationMatrix = glm::rotate(rotationMatrix, shape.rotationRadians.x, glm::vec3(1.0f, 0.0f, 0.0f));
        const glm::mat3 normalMatrix(rotationMatrix);

        const glm::vec3 translation = shape.offset;
        for (Vertex& vertex : meshData.vertices) {
            vertex.position = glm::vec3(rotationMatrix * glm::vec4(vertex.position, 1.0f)) + translation;
            if (glm::dot(vertex.normal, vertex.normal) > 0.0f) {
                vertex.normal = glm::normalize(normalMatrix * vertex.normal);
            }
            if (glm::dot(vertex.tangent, vertex.tangent) > 0.0f) {
                vertex.tangent = glm::normalize(normalMatrix * vertex.tangent);
            }
        }
    }

    void AddBox(RagdollShapeMeshData& meshData, const RagdollShape& shape) {
        meshData.vertices = Hell::PrimitiveMesh::GenerateCubeVertices();
        meshData.indices = Hell::PrimitiveMesh::GenerateCubeIndices();

        const glm::vec3 dimensions = shape.extents;
        for (Vertex& vertex : meshData.vertices) {
            vertex.position *= dimensions;
        }
        ApplyLocalShapeTransform(meshData, shape);
    }

    void AddSphere(RagdollShapeMeshData& meshData, const RagdollShape& shape) {
        meshData.vertices = Hell::PrimitiveMesh::GenerateSphereVertices(shape.radius, SPHERE_LATITUDE_SEGMENTS, SPHERE_LONGITUDE_SEGMENTS);
        meshData.indices = Hell::PrimitiveMesh::GenerateSphereIndices(SPHERE_LATITUDE_SEGMENTS, SPHERE_LONGITUDE_SEGMENTS);
        ApplyLocalShapeTransform(meshData, shape);
    }

    void AddCapsule(RagdollShapeMeshData& meshData, const RagdollShape& shape) {
        meshData.vertices = Hell::PrimitiveMesh::GenerateCapsuleVertices(shape.radius, shape.length, CAPSULE_HEMISPHERE_RINGS, CAPSULE_SEGMENTS);
        meshData.indices = Hell::PrimitiveMesh::GenerateCapsuleIndices(CAPSULE_HEMISPHERE_RINGS, CAPSULE_SEGMENTS);
        ApplyLocalShapeTransform(meshData, shape);
    }

    void GenerateNormalsAndTangents(RagdollShapeMeshData& meshData) {
        constexpr float UV_AREA_EPSILON = 0.000001f;

        for (size_t index = 0; index + 2 < meshData.indices.size(); index += 3) {
            const uint32_t index0 = meshData.indices[index];
            const uint32_t index1 = meshData.indices[index + 1];
            const uint32_t index2 = meshData.indices[index + 2];
            if (index0 >= meshData.vertices.size() || index1 >= meshData.vertices.size() || index2 >= meshData.vertices.size()) {
                continue;
            }

            Vertex& vertex0 = meshData.vertices[index0];
            Vertex& vertex1 = meshData.vertices[index1];
            Vertex& vertex2 = meshData.vertices[index2];
            const glm::vec3 faceNormal = glm::cross(vertex1.position - vertex0.position, vertex2.position - vertex0.position);
            if (glm::dot(faceNormal, faceNormal) > 0.0f) {
                vertex0.normal += faceNormal;
                vertex1.normal += faceNormal;
                vertex2.normal += faceNormal;
            }

            const glm::vec2 deltaUv1 = vertex1.uv - vertex0.uv;
            const glm::vec2 deltaUv2 = vertex2.uv - vertex0.uv;
            const float uvArea = deltaUv1.x * deltaUv2.y - deltaUv1.y * deltaUv2.x;
            if (std::abs(uvArea) > UV_AREA_EPSILON) {
                const glm::vec3 deltaPosition1 = vertex1.position - vertex0.position;
                const glm::vec3 deltaPosition2 = vertex2.position - vertex0.position;
                const glm::vec3 tangent = (deltaPosition1 * deltaUv2.y - deltaPosition2 * deltaUv1.y) / uvArea;
                vertex0.tangent += tangent;
                vertex1.tangent += tangent;
                vertex2.tangent += tangent;
            }
        }

        for (Vertex& vertex : meshData.vertices) {
            if (glm::dot(vertex.normal, vertex.normal) > 0.0f) {
                vertex.normal = glm::normalize(vertex.normal);
            }
            if (glm::dot(vertex.tangent, vertex.tangent) > 0.0f) {
                vertex.tangent = glm::normalize(vertex.tangent);
            }
        }
    }

    void AddConvexHull(RagdollShapeMeshData& meshData, const RagdollShape& shape) {
        meshData.vertices.reserve(shape.convexVertices.size());
        for (const glm::vec3& position : shape.convexVertices) {
            Vertex& vertex = meshData.vertices.emplace_back();
            vertex.position = position;
        }
        meshData.indices = shape.convexIndices;
        GenerateNormalsAndTangents(meshData);
    }
}

namespace RagdollShapeMesh {

    RagdollShapeMeshData Create(const RagdollShape& shape) {
        RagdollShapeMeshData meshData;

        switch (shape.type) {
            case RagdollShapeType::BOX:
                AddBox(meshData, shape);
                break;
            case RagdollShapeType::SPHERE:
                AddSphere(meshData, shape);
                break;
            case RagdollShapeType::CAPSULE:
                // Zero-length capsules are spheres.
                if (shape.length <= 0.0f) {
                    AddSphere(meshData, shape);
                }
                else {
                    AddCapsule(meshData, shape);
                }
                break;
            case RagdollShapeType::CONVEX_HULL:
                AddConvexHull(meshData, shape);
                break;
        }

        return meshData;
    }
}
