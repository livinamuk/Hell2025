#include "PhysicsUtil.h"
#include "Physics.h"
#include "Hell/Common/Constants.h"

#include <algorithm>
#include <cmath>
#include <glm/geometric.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream> // TODO: cleanup logging

namespace Hell::Physics {

    bool PxTransformNearlyEqual(const physx::PxTransform& a, const physx::PxTransform& b, float positionEps, float rotationEps) {
        const float dxp = a.p.x - b.p.x;
        const float dyp = a.p.y - b.p.y;
        const float dzp = a.p.z - b.p.z;

        if (dxp > positionEps || dxp < -positionEps) return false;
        if (dyp > positionEps || dyp < -positionEps) return false;
        if (dzp > positionEps || dzp < -positionEps) return false;

        // q and -q represent the same rotation, so align via dot sign.
        const float dot = a.q.x * b.q.x + a.q.y * b.q.y + a.q.z * b.q.z + a.q.w * b.q.w;

        if (dot >= 0.0f) {
            const float dxq = a.q.x - b.q.x;
            const float dyq = a.q.y - b.q.y;
            const float dzq = a.q.z - b.q.z;
            const float dwq = a.q.w - b.q.w;

            if (dxq > rotationEps || dxq < -rotationEps) return false;
            if (dyq > rotationEps || dyq < -rotationEps) return false;
            if (dzq > rotationEps || dzq < -rotationEps) return false;
            if (dwq > rotationEps || dwq < -rotationEps) return false;
            return true;
        }
        else {
            // Compare a.q to -b.q without modifying b.
            const float dxq = a.q.x + b.q.x;
            const float dyq = a.q.y + b.q.y;
            const float dzq = a.q.z + b.q.z;
            const float dwq = a.q.w + b.q.w;

            if (dxq > rotationEps || dxq < -rotationEps) return false;
            if (dyq > rotationEps || dyq < -rotationEps) return false;
            if (dzq > rotationEps || dzq < -rotationEps) return false;
            if (dwq > rotationEps || dwq < -rotationEps) return false;
            return true;
        }
    }

    glm::vec3 PxVec3toGlmVec3(physx::PxVec3 vec) {
        return { vec.x, vec.y, vec.z };
    }
    
    glm::vec3 PxVec3toGlmVec3(physx::PxExtendedVec3 vec) {
        return { vec.x, vec.y, vec.z };
    }
    
    physx::PxVec3 GlmVec3toPxVec3(const glm::vec3& vec) {
        return { vec.x, vec.y, vec.z };
    }
    
    physx::PxQuat GlmQuatToPxQuat(const glm::quat& quat) {
        return { quat.x, quat.y, quat.z, quat.w };
    }
    
    glm::quat PxQuatToGlmQuat(physx::PxQuat quat) {
        return { quat.x, quat.y, quat.z, quat.w };
    }    

    glm::mat4 PxMat44ToGlmMat4(physx::PxMat44 pxMatrix) {
        glm::mat4 matrix;
        for (int x = 0; x < 4; x++)
            for (int y = 0; y < 4; y++)
                matrix[x][y] = pxMatrix[x][y];
        return matrix;
    }

    physx::PxMat44 GlmMat4ToPxMat44(const glm::mat4& glmMatrix) {
        physx::PxMat44 matrix;
        std::copy(glm::value_ptr(glmMatrix),
                  glm::value_ptr(glmMatrix) + 16,
                  reinterpret_cast<float*>(&matrix));
        return matrix;
    }

    glm::vec3 GetHeightMapPositionAtXZ(float x, float z) {
        ActivateAllHeightFields(); // TODO: Rewrite this function to only activate the heightfield that is beneath this ray origin

        PhysXRayResult rayResult = Hell::Physics::CastPhysXRayHeightMap(glm::vec3(x, 200, z), glm::vec3(0.0f, -1.0f, 0.0f), 250);
        if (rayResult.hitFound) {
            return rayResult.hitPosition;
        }
        else {
            return glm::vec3(0.0f);
        }
    }

    float GetDensity(float mass, float volume) {
        return mass / volume;
    }

    float GetCubeVolume(const glm::vec3& halfExtents) {
        return 8.0f * halfExtents.x * halfExtents.y * halfExtents.z;
    }

    float GetCubeVolume(float halfWidth, float halfHeight, float halfDepth) {
        return GetCubeVolume(glm::vec3(halfWidth, halfHeight, halfDepth));
    }

    float GetSphereVolume(float radius) {
        return (4.0f / 3.0f) * HELL_PI * radius * radius * radius;
    }

    float GetCapsuleVolume(float radius, float halfHeight) {
        float cylHeight = halfHeight * 2.0f;
        float cylVol = HELL_PI * radius * radius * cylHeight;
        float sphVol = GetSphereVolume(radius);
        return cylVol + sphVol;
    }

    float GetConvexHullVolume(const std::span<Vertex>& vertices, const std::span<unsigned int>& indices) {
        glm::vec3 reference(0.0f);
        for (const Vertex& vertex : vertices) {
            reference += vertex.position;
        }
        reference /= static_cast<float>(vertices.size());

        float totalVolume = 0.0f;
        for (size_t i = 0; i < indices.size(); i += 3) {
            const glm::vec3& v0 = vertices[indices[i]].position;
            const glm::vec3& v1 = vertices[indices[i + 1]].position;
            const glm::vec3& v2 = vertices[indices[i + 2]].position;

            const glm::vec3 crossProd = glm::cross(v1 - v0, v2 - v0);
            const float tetraVolume = std::abs(glm::dot(crossProd, reference - v0)) / 6.0f;
            totalVolume += tetraVolume;
        }
        return totalVolume;
    }

    float ComputeShapeVolume(physx::PxShape* pxShape) {
        if (!pxShape) {
            std::cout << "Hell::Physics::ComputeShapeDenisty() failed: pxShape was nullptr\n";
            return 0.0f;
        }

        const physx::PxGeometry& pxGeometry = pxShape->getGeometry();
        const physx::PxGeometryHolder pxGeometryHolder = pxShape->getGeometry();
        const physx::PxGeometryType::Enum pxGeometryType = pxGeometry.getType();

        if (pxGeometryType == physx::PxGeometryType::Enum::eBOX) {
            const physx::PxBoxGeometry& box = pxGeometryHolder.box();
            return GetCubeVolume(box.halfExtents.x, box.halfExtents.y, box.halfExtents.z);
        }
        else if (pxGeometryType == physx::PxGeometryType::Enum::eSPHERE) {
            const physx::PxSphereGeometry& sphere = pxGeometryHolder.sphere();
            return GetSphereVolume(sphere.radius);
        }
        else if (pxGeometryType == physx::PxGeometryType::Enum::eCAPSULE) {
            const physx::PxCapsuleGeometry& capsule = pxGeometryHolder.capsule();
            return GetCapsuleVolume(capsule.radius, capsule.halfHeight);
        }
        else {
            std::cout << "Hell::Physics::ComputeShapeVolume() failed: pxShape was not cube, sphere, or capsule\n";
            return 0.0f;
        }
    }

    std::string GetPxShapeTypeAsString(physx::PxShape* pxShape) {
        if (!pxShape) {
            std::cout << "Hell::Physics::ComputeShapeDenisty() failed: pxShape was nullptr\n";
            return "Invalid shape";
        }

        const physx::PxGeometry& pxGeometry = pxShape->getGeometry();
        const physx::PxGeometryHolder pxGeometryHolder = pxShape->getGeometry();
        const physx::PxGeometryType::Enum pxGeometryType = pxGeometry.getType();

        switch (pxGeometryType) {
            case physx::PxGeometryType::Enum::eBOX:             return "Box";
            case physx::PxGeometryType::Enum::eSPHERE:          return "Sphere";
            case physx::PxGeometryType::Enum::eCAPSULE:         return "Capsule";
            case physx::PxGeometryType::Enum::ePLANE:           return "Plane";
            case physx::PxGeometryType::Enum::eCONVEXMESH:      return "ConvexMesh";
            case physx::PxGeometryType::Enum::eTRIANGLEMESH:    return "TriangleMesh";
            case physx::PxGeometryType::Enum::eHEIGHTFIELD:     return "HeightField";
            default:                                            return "Unknown shape type";
        }
    }
}
