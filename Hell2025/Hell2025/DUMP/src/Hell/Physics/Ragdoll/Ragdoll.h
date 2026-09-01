#pragma once
#include "RagdollAsset.h"

#include "Hell/Math/AABB.h"
#include "Hell/Physics/PhysicsTypes.h"
#include "Hell/Transform.h"

#pragma warning(push, 0)
#include <physx/PxRigidDynamic.h>
#include <physx/extensions/PxD6Joint.h>
#pragma warning(pop)

#include <cstdint>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <string>
#include <vector>

struct Ragdoll {
    bool Init(const glm::vec3& spawnPosition, const glm::vec3& spawnEulerRotation, const RagdollAsset& asset, uint64_t ragdollId, uint64_t parentObjectId, PhysicsFilterData filterData);
    void CleanUp();
    void Update();
    void DisableSimulation();
    void EnableSimulation();
    void SetSpawnPosition(const glm::vec3& position);
    void SetSpawnRotation(const glm::vec3& rotation);
    void SetToInitialPose();
    void MarkForRemoval();
    void AddForce(const std::string& boneName, const glm::vec3& force, bool wakeIfDisabled);
    void SetAngularVelocity(const std::string& boneName, const glm::vec3& angularVelocity, bool wakeIfDisabled);
    void AddForce(uint64_t physicsId, const glm::vec3& force, bool wakeIfDisabled);
    void AddImpulse(uint64_t physicsId, const glm::vec3& impulse, bool wakeIfDisabled);
    void AddImpulseAtPosition(uint64_t physicsId, const glm::vec3& impulse, const glm::vec3& position, bool wakeIfDisabled);
    void AddAngularVelocityChangeAtPosition(uint64_t physicsId, const glm::vec3& velocityChange, const glm::vec3& position, bool wakeIfDisabled);
    const std::string& GetBoneNameByPhysicsId(uint64_t physicsId) const;
    uint64_t GetPhysicsIdByBoneName(const std::string& boneName) const;

    bool IsInMotion();
    bool IsMarkedForRemoval() const;
    AABB GetWorldSpaceAABB();
    void GetWorldSpaceAABBs(std::vector<AABB>& aabbs);
    void UpdateWorldSpaceAABBs(float changeThreshold);
    const std::vector<AABB>& GetWorldSpaceAABBs() const          { return m_worldSpaceAABBs; }
    glm::mat4 GetRigidWorldTransform(const std::string& boneName) const;
    uint64_t GetRagdollId()                     { return m_ragdollId; }
    const std::string& GetRagdollName() const   { return m_ragdollName; }
    uint32_t GetMarkerMeshIdByRigidIndex(uint32_t index) const;
    glm::vec3 GetMarkerColorByRigidIndex(uint32_t index) const;
    glm::mat4 GetModelMatrixByRigidIndex(uint32_t index) const;

    std::vector<std::string> m_markerBoneNames;
    std::vector<physx::PxRigidDynamic*> m_pxRigidDynamics;

    bool IsDirty() const { return m_dirty; }

private:
    void AddMarkerMeshData(const RagdollMarkerAsset& marker);
    void ProjectPoseToJointLimits();

    std::vector<physx::PxD6Joint*> m_pxD6Joints;
    std::vector<uint32_t> m_markerMeshIds;
    std::vector<glm::vec3> m_markerColors;
    std::vector<glm::mat4> m_markerRestTransforms;
    std::vector<AABB> m_worldSpaceAABBs;
    std::vector<physx::PxTransform> m_previousRigidWorldTransforms;
    std::string m_ragdollName;
    Hell::Transform m_spawnTransform;
    uint64_t m_ragdollId;
    bool m_simulationEnabled = false;
    bool m_renderingEnabled = true;
    bool m_markedForRemoval = false;
    bool m_dirty = false;
};
