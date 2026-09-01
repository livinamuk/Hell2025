#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

#include "Hell/Math/AABB.h"
#include "Hell/Math/VecXZ.h"
#include "Hell/Physics/PhysicsTypes.h"
#include "Hell/Physics/Types/CharacterController.h"
#include "Hell/Physics/Types/D6Joint.h"
#include "Hell/Physics/Types/HeightField.h"
#include "Hell/Physics/Types/RigidDynamic.h"
#include "Hell/Physics/Types/RigidStatic.h"
#include "Hell/Physics/Ragdoll/Ragdoll.h"
#include "Hell/Render/VertexAttributes.h"
#include "Hell/Transform.h"

#pragma warning(push, 0)
#include <physx/PxPhysicsAPI.h>
#pragma warning(pop)

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

using namespace physx;

namespace Hell::Physics {
    std::unordered_map<uint64_t, CharacterController>& GetCharacterControllers();
    std::unordered_map<uint64_t, D6Joint>& GetD6Joints();
    std::unordered_map<uint64_t, Ragdoll>& GetRagdolls();
    std::unordered_map<uint64_t, RigidDynamic>& GetRigidDynamics();
    std::unordered_map<uint64_t, RigidStatic>& GetRigidStatics();

    std::vector<AABB>& GetActiveRididDynamicAABBs();
    std::vector<HeightField>& GetHeightFields();

    CharacterController* GetCharacterControllerById(uint64_t characterControllerId);
    void RemoveAnyCharacterControllerMarkedForRemoval();
    void MarkCharacterControllerForRemoval(uint64_t characterControllerId);
    bool CharacterControllerExists(uint64_t characterControllerId);
    int GetCharacterControllerCount();

    D6Joint* GetD6JointById(uint64_t d6JointId);
    void RemoveAnyD6JointMarkedForRemoval();
    void MarkD6JointForRemoval(uint64_t d6JointId);
    bool D6JointExists(uint64_t d6JointId);
    int GetD6JointCount();

    void RemoveAnyHeightFieldMarkedForRemoval();
    void MarkAllHeightFieldsForRemoval();
    int GetHeightFieldCount();

    Ragdoll* GetRagdollById(uint64_t ragdollId);
    uint64_t SpawnRagdoll(const glm::vec3& position, const glm::vec3& eulerRotation, const std::string& ragdollName, uint64_t parentObjectId);
    uint64_t SpawnRagdoll(const glm::vec3& position, const glm::vec3& eulerRotation, const RagdollAsset& asset, uint64_t parentObjectId);
    void RemoveAnyRagdollMarkedForRemoval();
    void MarkRagdollForRemoval(uint64_t ragdollId);

    bool RigidDynamicExists(uint64_t rigidDynamicId);
    void MarkRigidDynamicForRemoval(uint64_t rigidDynamicId);
    void RemoveAnyRigidDynamicMarkedForRemoval();
    RigidDynamic* GetRigidDynamicById(uint64_t rigidDynamicId);
    int GetRigidDynamicCount();

    bool RigidStaticExists(uint64_t rigidStaticId);
    RigidStatic* GetRigidStaitcById(uint64_t rigidStaticId);
    void MarkRigidStaticForRemoval(uint64_t rigidStaticId);
    void RemoveRigidStatic(uint64_t rigidStaticId);
    void RemoveAnyRigidStaticMarkedForRemoval();
    int GetRigidStaticCount();

    void CreateHeightField(Hell::vecXZ& worldSpaceOffset, const float* heightValues, float heightScale, float rowScale, float colScale);

    uint64_t CreateRigidDynamicFromConvexMeshVertices(Transform transform, const std::span<Vertex>& vertices, const std::span<uint32_t>& indices, float mass, PhysicsFilterData filterData, glm::vec3 initialForce = glm::vec3(0.0f), glm::vec3 initialTorque = glm::vec3(0.0f));
    uint64_t CreateRigidDynamicWithCompoundConvexMeshesFromModel(const std::string& modelName, float mass, bool kinematic, PhysicsFilterData filterData);
    uint64_t CreateRigidDynamicFromBoxExtents(const Transform& transform, const glm::vec3& boxExtents, bool kinematic, float mass, PhysicsFilterData filterData, const Transform& localOffset);
    uint64_t CreateRigidDynamicFromBoxExtents(const Transform& transform, const glm::vec3& boxExtents, bool kinematic, float mass, PhysicsFilterData filterData, const glm::mat4& localOffset);
    uint64_t CreateRigidDynamicFromBoxExtents(const glm::mat4& transform, const glm::vec3& boxExtents, bool kinematic, float mass, PhysicsFilterData filterData, const Transform& localOffset);
    uint64_t CreateRigidDynamicFromBoxExtents(const glm::mat4& transform, const glm::vec3& boxExtents, bool kinematic, float mass, PhysicsFilterData filterData, const glm::mat4& localOffset);
    uint64_t CreateRigidDynamicFromBoxExtents(Transform transform, glm::vec3 boxExtents, float mass, PhysicsFilterData filterData, glm::vec3 initialForce = glm::vec3(0.0f), glm::vec3 initialTorque = glm::vec3(0.0f));
    uint64_t CreateRigidDynamicFromPxShape(PxShape* pxShape, glm::mat4 initialPose, glm::mat4 shapeOffsetMatrix);

    uint64_t CreateRigidStaticPlane(glm::vec3 planeOrigin, glm::vec3 planeNormal, PhysicsFilterData filterData);
    uint64_t CreateRigidStaticFromCapsule(Transform transform, float radius, float halfHeight, PhysicsFilterData filterData, Transform localOffset);
    uint64_t CreateRigidStaticBoxFromExtents(Transform transform, glm::vec3 boxExtents, PhysicsFilterData filterData, Transform localOffset = Transform());
    uint64_t CreateRigidStaticConvexMeshFromModel(Transform transform, const std::string& modelName, PhysicsFilterData filterData);
    uint64_t CreateRigidStaticConvexMeshFromVertices(Transform transform, const std::span<Vertex>& vertices, PhysicsFilterData filterData);
    uint64_t CreateRigidStaticTriangleMeshFromVertexData(Transform transform, const std::span<Vertex>& vertices, const std::span<uint32_t>& indices, PhysicsFilterData filterData);
    uint64_t CreateRigidStaticTriangleMeshFromModel(Transform transform, const std::string& modelName, PhysicsFilterData filterData);

    uint64_t CreateD6Joint(uint64_t parentRigidDynamicId, uint64_t childRigidDynamicId, glm::mat4 parentFrame, glm::mat4 childFrame);
    uint64_t CreateCharacterController(uint64_t parentObjectId, glm::vec3 position, float height, float radius, PhysicsFilterData physicsFilterData);

    PxShape* CreateBoxShape(float width, float height, float depth, Transform shapeOffset = Transform(), PxMaterial* material = nullptr);
    PxRigidDynamic* CreateRigidDynamic(Transform worldTransform, PhysicsFilterData filterData, PxShape* shape, Transform shapeOffset = Transform());
    PxRigidDynamic* CreateRigidDynamic(PxShape* shape, glm::mat4 worldMatrix, glm::mat4 shapeOffsetMatrix, PhysicsFilterData filterData);
    PxShape* CreateConvexShapeFromVertexList(std::span<Vertex>& vertices);
    PxShape* CreateConvexShapeFromVertexList(
        std::span<const glm::vec3> vertices,
        glm::vec3 scale = glm::vec3(1.0f),
        PxMaterial* material = nullptr
    );

    void Destroy(PxRigidDynamic*& rigidDynamic);
    void Destroy(PxRigidStatic*& rigidStatic);
    void Destroy(PxShape*& shape);
    void Destroy(PxRigidBody*& rigidBody);
    void Destroy(PxTriangleMesh*& triangleMesh);
}
