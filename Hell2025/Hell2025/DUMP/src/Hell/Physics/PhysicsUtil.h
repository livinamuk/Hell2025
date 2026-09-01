#pragma once

#pragma warning(push, 0)
#include <physx/PxPhysicsAPI.h>
#pragma warning(pop)

#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include "Hell/Render/VertexAttributes.h"
#include <span>
#include <string>

namespace Hell::Physics {
    bool PxTransformNearlyEqual(const physx::PxTransform& a, const physx::PxTransform& b, float positionEps = 1e-4f, float rotationEps = 1e-4f);
    std::string GetPxShapeTypeAsString(physx::PxShape* pxShape);
    float ComputeShapeVolume(physx::PxShape* pxShape);
    glm::vec3 PxVec3toGlmVec3(physx::PxVec3 vec);
    glm::vec3 PxVec3toGlmVec3(physx::PxExtendedVec3 vec);
    glm::quat PxQuatToGlmQuat(physx::PxQuat quat);
    glm::mat4 PxMat44ToGlmMat4(physx::PxMat44 pxMatrix);
    glm::vec3 GetHeightMapPositionAtXZ(float x, float z);
    float GetDensity(float mass, float volume);
    float GetConvexHullVolume(const std::span<Vertex>& vertices, const std::span<unsigned int>& indices);
    float GetCubeVolume(const glm::vec3& halfExtents);
    float GetCubeVolume(float halfWidth, float halfHeight, float halfDepth);
    float GetSphereVolume(float radius);
    float GetCapsuleVolume(float radius, float halfHeight);
    physx::PxVec3 GlmVec3toPxVec3(const glm::vec3& vec);
    physx::PxQuat GlmQuatToPxQuat(const glm::quat& quat);
    physx::PxMat44 GlmMat4ToPxMat44(const glm::mat4& glmMatrix);
}
