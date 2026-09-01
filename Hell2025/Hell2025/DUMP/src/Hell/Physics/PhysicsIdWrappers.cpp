#include "Physics.h"
#include "Hell/Logging.h"

namespace Hell::Physics {
    
    // Ragdolls

    void AddForceToRagdoll(uint64_t physicsId, const glm::vec3& force, bool wakeIfDisabled) {
        for (auto& pair : GetRagdolls()) {
            pair.second.AddForce(physicsId, force, wakeIfDisabled);
        }
    }

    void AddImpulseToRagdoll(uint64_t physicsId, const glm::vec3& impulse, bool wakeIfDisabled) {
        for (auto& pair : GetRagdolls()) {
            pair.second.AddImpulse(physicsId, impulse, wakeIfDisabled);
        }
    }

    void AddImpulseAtPositionToRagdoll(uint64_t physicsId, const glm::vec3& impulse, const glm::vec3& position, bool wakeIfDisabled) {
        for (auto& pair : GetRagdolls()) {
            pair.second.AddImpulseAtPosition(physicsId, impulse, position, wakeIfDisabled);
        }
    }

    void AddAngularVelocityChangeAtPositionToRagdoll(uint64_t physicsId, const glm::vec3& velocityChange, const glm::vec3& position, bool wakeIfDisabled) {
        for (auto& pair : GetRagdolls()) {
            pair.second.AddAngularVelocityChangeAtPosition(physicsId, velocityChange, position, wakeIfDisabled);
        }
    }

    std::vector<physx::PxRigidActor*> GetRagdollPxRigidActors(uint64_t ragdollId) {
        std::vector<physx::PxRigidActor*> actors;
        if (ragdollId == 0) {
            return actors;
        }

        if (Ragdoll* ragdoll = Hell::Physics::GetRagdollById(ragdollId)) {
            actors.reserve(ragdoll->m_pxRigidDynamics.size());
            for (physx::PxRigidDynamic* pxRigidDynamic : ragdoll->m_pxRigidDynamics) {
                if (pxRigidDynamic) {
                    actors.push_back(pxRigidDynamic);
                }
            }
        }

        return actors;
    }

    // Rigid Dynamics

    bool RigidDynamicIsKinematic(uint64_t rigidDynamicId) {
        RigidDynamic* rigidDynamic = GetRigidDynamicById(rigidDynamicId);
        if (!rigidDynamic) return false;

        return rigidDynamic->IsKinematic();
    }

    bool RigidDynamicIsDirty(uint64_t rigidDynamicId) {
        RigidDynamic* rigidDynamic = GetRigidDynamicById(rigidDynamicId);
        if (!rigidDynamic) return false;

        return rigidDynamic->IsDirty();
    }

    glm::mat4 GetRigidDynamicWorldMatrix(uint64_t rigidDynamicId) {
        RigidDynamic* rigidDynamic = GetRigidDynamicById(rigidDynamicId);
        if (!rigidDynamic) return glm::mat4(1.0f);

        return rigidDynamic->GetWorldMatrix();
    }

    void AddFoceToRigidDynamic(uint64_t rigidDynamicId, glm::vec3 force) {
        RigidDynamic* rigidDynamic = GetRigidDynamicById(rigidDynamicId);
        if (!rigidDynamic) {
            Logging::Error() << "Hell::Physics::AddFoceToRigidDynamic() failed: id " << rigidDynamicId << " not found\n";
            return;
        }

        rigidDynamic->AddForce(force);
    }

    void AddVelocityChangeToRigidDynamic(uint64_t rigidDynamicId, glm::vec3 velocityChange) {
        RigidDynamic* rigidDynamic = GetRigidDynamicById(rigidDynamicId);
        if (!rigidDynamic) {
            Logging::Error() << "Hell::Physics::AddVelocityChangeToRigidDynamic() failed: id " << rigidDynamicId << " not found\n";
            return;
        }

        rigidDynamic->AddVelocityChange(velocityChange);
    }

    void AddImpulseToRigidDynamic(uint64_t rigidDynamicId, glm::vec3 impulse) {
        RigidDynamic* rigidDynamic = GetRigidDynamicById(rigidDynamicId);
        if (!rigidDynamic) {
            Logging::Error() << "Hell::Physics::AddImpulseToRigidDynamic() failed: id " << rigidDynamicId << " not found\n";
            return;
        }

        rigidDynamic->AddImpulse(impulse);
    }

    void AddImpulseAtPositionToRigidDynamic(uint64_t rigidDynamicId, glm::vec3 impulse, glm::vec3 position) {
        RigidDynamic* rigidDynamic = GetRigidDynamicById(rigidDynamicId);
        if (!rigidDynamic) {
            Logging::Error() << "Hell::Physics::AddImpulseAtPositionToRigidDynamic() failed: id " << rigidDynamicId << " not found\n";
            return;
        }

        rigidDynamic->AddImpulseAtPosition(impulse, position);
    }

    void AddAngularVelocityChangeAtPositionToRigidDynamic(uint64_t rigidDynamicId, glm::vec3 velocityChange, glm::vec3 position) {
        RigidDynamic* rigidDynamic = GetRigidDynamicById(rigidDynamicId);
        if (!rigidDynamic) {
            Logging::Error() << "Hell::Physics::AddAngularVelocityChangeAtPositionToRigidDynamic() failed: id " << rigidDynamicId << " not found\n";
            return;
        }

        rigidDynamic->AddAngularVelocityChangeAtPosition(velocityChange, position);
    }

    void ActivateRigidDynamicPhysics(uint64_t rigidDynamicId) {
        Logging::ToDo() << "Hell::Physics::ActivateRigidDynamicPhysics(" << rigidDynamicId << ") is not implemented\n";
    }

    void DeactivateRigidDynamicPhysics(uint64_t rigidDynamicId) {
        Logging::ToDo() << "Hell::Physics::DeactivateRigidDynamicPhysics(" << rigidDynamicId << ") is not implemented\n";
    }

    void SetRigidDynamicUserData(uint64_t rigidDynamicId, PhysicsUserData physicsUserData) {
        RigidDynamic* rigidDynamic = GetRigidDynamicById(rigidDynamicId);
        if (!rigidDynamic) {
            Logging::Error() << "Hell::Physics::SetRigidDynamicUserData() failed: id " << rigidDynamicId << " not found\n";
            return;
        }

        rigidDynamic->SetUserData(physicsUserData);
    }

    void SetRigidDynamicGlobalPose(uint64_t rigidDynamicId, const glm::mat4& globalPoseMatrix) {
        RigidDynamic* rigidDynamic = GetRigidDynamicById(rigidDynamicId);
        if (!rigidDynamic) {
            Logging::Error() << "Hell::Physics::SetRigidDynamicGlobalPose() failed: id " << rigidDynamicId << " not found\n";
            return;
        }

        rigidDynamic->SetGlobalPose(globalPoseMatrix);
    }

    void SetRigidDynamicKinematicTarget(uint64_t rigidDynamicId, const glm::mat4& globalPoseMatrix) {
        RigidDynamic* rigidDynamic = GetRigidDynamicById(rigidDynamicId);
        if (!rigidDynamic) {
            Logging::Error() << "Hell::Physics::SetRigidDynamicKinematicTarget() failed: id " << rigidDynamicId << " not found\n";
            return;
        }

        rigidDynamic->SetKinematicTarget(globalPoseMatrix);
    }


    // Rigid Statics

    glm::mat4 GetRigidStaticGlobalPose(uint64_t rigidStaticId) {
        RigidStatic* rigidStatic = GetRigidStaitcById(rigidStaticId);
        if (!rigidStatic) return glm::mat4(1.0f);

        return rigidStatic->GetGlobalPose();
    }

    void SetRigidStaticWorldTransform(uint64_t rigidStaticId, glm::mat4 worldMatrix) {
        RigidStatic* rigidStatic = GetRigidStaitcById(rigidStaticId);
        if (!rigidStatic) {
            Logging::Error() << "Hell::Physics::SetRigidStaticWorldTransform() failed: id " << rigidStaticId << " not found\n";
            return;
        }

        rigidStatic->SetWorldTransform(worldMatrix);
    }

    void SetRigidStaticUserData(uint64_t rigidStaticId, PhysicsUserData physicsUserData) {
        RigidStatic* rigidStatic = GetRigidStaitcById(rigidStaticId);
        if (!rigidStatic) {
            Logging::Error() << "Hell::Physics::SetRigidStaticUserData() failed: id " << rigidStaticId << " not found\n";
            return;
        }

        rigidStatic->SetUserData(physicsUserData);
    }

    // Character Controllers

    AABB GetCharacterControllerAABB(uint64_t characterControllerId) {
        CharacterController* characterController = GetCharacterControllerById(characterControllerId);
        if (characterController) {
            return characterController->GetAABB();
        }

        Logging::Error() << "Hell::Physics::GetCharacterControllerAABB() failed: " << characterControllerId << " not found, or it had an invalid PxController pointer\n";
        return AABB();
    }

    glm::vec3 GetCharacterControllerPosition(uint64_t characterControllerId) {
        CharacterController* characterController = GetCharacterControllerById(characterControllerId);
        if (!characterController) {
            Logging::Error() << "Hell::Physics::GetCharacterControlPosition() failed: " << characterControllerId << " not found! WARNING VEC3(0.0f) returned\n";
            return glm::vec3(0.0f);
        }

        return characterController->GetFootPosition();
    }

    void MoveCharacterController(uint64_t characterControllerId, glm::vec3 displacement) {
        CharacterController* characterController = GetCharacterControllerById(characterControllerId);
        if (!characterController) {
            Logging::Error() << "Hell::Physics::MoveCharacterController() failed: id " << characterControllerId << " not found\n";
            return;
        }

        characterController->Move(displacement);
    }
}
