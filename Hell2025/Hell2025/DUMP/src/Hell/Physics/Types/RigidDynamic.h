#pragma once
#pragma warning(push, 0)
#include <physx/PxShape.h>
#include <physx/PxRigidDynamic.h>
#pragma warning(pop)

#include "Hell/Math/AABB.h"
#include "Hell/Physics/PhysicsTypes.h"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <vector>

using namespace physx;

enum class RigidDynamicMotionState {
    KINEMATIC, // Game drives transform and PhysX forces do not move it
    AWAKE,     // PhysX owns it and it is actively moving/colliding
    SETTLING,  // PhysX owns it, but velocity is tiny for a short window
    ASLEEP     // PhysX has stopped simulating it until woken
};

struct RigidDynamic {
    void Update(float deltaTime);
    void SetPxRigidDynamic(PxRigidDynamic* rigidDynamic);
    void SetPxShapes(const std::vector<PxShape*>& pxShapes);
    void SetFilterData(PhysicsFilterData filterData);
    void MarkForRemoval();
    void UpdateMassAndInertia(float density);
    void AddForce(glm::vec3 force);
    void AddVelocityChange(glm::vec3 velocityChange);
    void AddImpulse(glm::vec3 impulse);
    void AddImpulseAtPosition(glm::vec3 impulse, glm::vec3 position);
    void AddAngularVelocityChangeAtPosition(glm::vec3 velocityChange, glm::vec3 position);
    void SetGlobalPose(const glm::mat4& globalPoseMatrix);
    void SetKinematicTarget(const glm::mat4& globalPoseMatrix);
    void SetUserData(PhysicsUserData physicsUserData);
    bool IsKinematic() const;
    glm::mat4 GetWorldMatrix() const;
    float GetVolume();

    const RigidDynamicMotionState& GetMotionState() const { return m_motionState; }
    const glm::mat4& GetWorldTransform() const            { return m_worldTransform; }
    const bool IsDirty() const                            { return m_isDirty; }
    bool IsMarkedForRemoval()                             { return m_markedForRemoval; }
    PxRigidDynamic* GetPxRigidDynamic()                   { return m_pxRigidDynamic; }
    std::vector<PxShape*>& GetPxShapes()                  { return m_pxShapes; }
    const AABB& GetAABB()                                 { return m_aabb; }
    size_t GetPxShapeCount() const                        { return m_pxShapes.size(); }

private:
    AABB m_aabb; 
    std::vector<PxShape*> m_pxShapes;
    PxRigidDynamic* m_pxRigidDynamic = nullptr;
    PxTransform m_previousGlobalPose = PxTransform(physx::PxIdentity);
    glm::mat4 m_worldTransform = glm::mat4(1.0f);
    bool m_markedForRemoval = false;
    bool m_isDirty = true;
    float m_lifeTime = 0.0f;
    float m_settleTimer = 0.0f;
    RigidDynamicMotionState m_motionState = RigidDynamicMotionState::ASLEEP;
};
