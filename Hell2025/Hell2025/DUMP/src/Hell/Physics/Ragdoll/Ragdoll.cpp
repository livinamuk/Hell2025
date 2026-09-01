#include "Ragdoll.h"
#include "RagdollJoint.h"
#include "RagdollShapeMesh.h"

#include "RagdollUtil.h"

#include "Hell/Logging.h"
#include "Hell/Math/Matrix.h"
#include "Hell/Physics/PhysicsIds.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Unloved/Config/PhysicsConfig.h"

#pragma warning(push, 0)
#include <physx/foundation/PxMathUtils.h>
#pragma warning(pop)

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <unordered_map>
#include <unordered_set>

namespace {
    constexpr float JOINT_LIMIT_MARGIN = 0.999f;
    constexpr float RAGDOLL_SLEEP_THRESHOLD = 5e-6f;

    PxQuat SwingFromQuarterAngleTangents(float y, float z) {
        const float lengthSquared = y * y + z * z;
        const float inverseDenominator = 1.0f / (1.0f + lengthSquared);
        return PxQuat(
            0.0f,
            2.0f * y * inverseDenominator,
            2.0f * z * inverseDenominator,
            (1.0f - lengthSquared) * inverseDenominator
        );
    }

    PxQuat ProjectSwingToJointLimits(const PxD6Joint& joint, const PxQuat& swing) {
        const PxD6Motion::Enum swing1Motion = joint.getMotion(PxD6Axis::eSWING1);
        const PxD6Motion::Enum swing2Motion = joint.getMotion(PxD6Axis::eSWING2);

        if (swing1Motion == PxD6Motion::eFREE && swing2Motion == PxD6Motion::eFREE) {
            return swing;
        }
        if (swing1Motion == PxD6Motion::eLOCKED && swing2Motion == PxD6Motion::eLOCKED) {
            return PxQuat(PxIdentity);
        }

        const PxJointLimitCone limits = joint.getSwingLimit();
        const float denominator = std::max(0.000001f, 1.0f + swing.w);
        float swing1Angle = 4.0f * std::atan2(swing.y, denominator);
        float swing2Angle = 4.0f * std::atan2(swing.z, denominator);

        if (swing1Motion == PxD6Motion::eLOCKED) {
            swing1Angle = 0.0f;
        }
        if (swing2Motion == PxD6Motion::eLOCKED) {
            swing2Angle = 0.0f;
        }

        if (swing1Motion == PxD6Motion::eLIMITED && swing2Motion == PxD6Motion::eLIMITED) {
            const PxVec3 radii(
                0.0f,
                std::max(0.0001f, limits.yAngle * JOINT_LIMIT_MARGIN),
                std::max(0.0001f, limits.zAngle * JOINT_LIMIT_MARGIN)
            );
            const float ellipseDistance =
                PxSqr(swing1Angle / radii.y) +
                PxSqr(swing2Angle / radii.z);

            if (ellipseDistance > 1.0f) {
                const PxVec3 clamped = PxEllipseClamp(PxVec3(0.0f, swing1Angle, swing2Angle), radii);
                swing1Angle = clamped.y;
                swing2Angle = clamped.z;
            }
        }
        else {
            if (swing1Motion == PxD6Motion::eLIMITED) {
                const float limit = std::max(0.0001f, limits.yAngle * JOINT_LIMIT_MARGIN);
                swing1Angle = std::clamp(swing1Angle, -limit, limit);
            }
            if (swing2Motion == PxD6Motion::eLIMITED) {
                const float limit = std::max(0.0001f, limits.zAngle * JOINT_LIMIT_MARGIN);
                swing2Angle = std::clamp(swing2Angle, -limit, limit);
            }
        }

        return SwingFromQuarterAngleTangents(
            std::tan(swing1Angle * 0.25f),
            std::tan(swing2Angle * 0.25f)
        );
    }

    PxQuat ProjectRotationToJointLimits(const PxD6Joint& joint, PxQuat relativeRotation) {
        relativeRotation.normalize();
        if (relativeRotation.w < 0.0f) {
            relativeRotation = -relativeRotation;
        }

        PxQuat swing;
        PxQuat twist;
        PxSeparateSwingTwist(relativeRotation, swing, twist);

        const PxD6Motion::Enum twistMotion = joint.getMotion(PxD6Axis::eTWIST);
        if (twistMotion == PxD6Motion::eLOCKED) {
            twist = PxQuat(PxIdentity);
        }
        else if (twistMotion == PxD6Motion::eLIMITED) {
            const PxJointAngularLimitPair limits = joint.getTwistLimit();
            const float angle = 2.0f * std::atan2(twist.x, twist.w);
            const float clampedAngle = std::clamp(
                angle,
                limits.lower * JOINT_LIMIT_MARGIN,
                limits.upper * JOINT_LIMIT_MARGIN
            );
            twist = PxQuat(clampedAngle, PxVec3(1.0f, 0.0f, 0.0f));
        }

        return (ProjectSwingToJointLimits(joint, swing) * twist).getNormalized();
    }
}

inline PxTransform PxTransformFromRest(const glm::mat4& restMatrix) {
    const glm::vec3 position = glm::vec3(restMatrix[3]);
    const glm::quat rotation = Hell::Math::ExtractRotation(restMatrix);
    return PxTransform(
        Hell::Physics::GlmVec3toPxVec3(position),
        Hell::Physics::GlmQuatToPxQuat(rotation)
    );
}

inline PxU32 GetSolverIterationCount(uint32_t solverIterations, uint32_t rigidIterations) {
    return static_cast<PxU32>(std::min(255U, solverIterations * rigidIterations));
}

inline PxD6Motion::Enum ToPxMotion(RagdollAxisMotion motion) {
    switch (motion) {
        case RagdollAxisMotion::LOCKED:  return PxD6Motion::eLOCKED;
        case RagdollAxisMotion::LIMITED: return PxD6Motion::eLIMITED;
        case RagdollAxisMotion::FREE:    return PxD6Motion::eFREE;
    }
    return PxD6Motion::eLOCKED;
}

inline void ApplyAngularLimits(PxD6Joint* joint, const std::array<RagdollAxisLimit, 3>& limits, bool enabled, const PxSpring& spring) {
    const RagdollAxisLimit& twist = limits[0];
    const RagdollAxisLimit& swing1 = limits[1];
    const RagdollAxisLimit& swing2 = limits[2];

    joint->setMotion(PxD6Axis::eTWIST,  enabled ? ToPxMotion(twist.motion)  : PxD6Motion::eFREE);
    joint->setMotion(PxD6Axis::eSWING1, enabled ? ToPxMotion(swing1.motion) : PxD6Motion::eFREE);
    joint->setMotion(PxD6Axis::eSWING2, enabled ? ToPxMotion(swing2.motion) : PxD6Motion::eFREE);

    if (enabled && twist.motion == RagdollAxisMotion::LIMITED) {
        const float range = std::max(0.0001f, twist.limit);
        joint->setTwistLimit(PxJointAngularLimitPair(-range, range, spring));
    }
    if (enabled && swing1.motion == RagdollAxisMotion::LIMITED && swing2.motion == RagdollAxisMotion::LIMITED) {
        joint->setSwingLimit(PxJointLimitCone(
            std::max(0.0001f, swing1.limit),
            std::max(0.0001f, swing2.limit),
            spring
        ));
    }
}

bool Ragdoll::Init(const glm::vec3& spawnPosition, const glm::vec3& spawnEulerRotation, const RagdollAsset& asset, uint64_t ragdollId, uint64_t parentObjectId, PhysicsFilterData filterData) {
    CleanUp();

    const Config::Physics::Settings& physicsSettings = Config::Physics::GetSettings();

    m_ragdollId = ragdollId;
    m_ragdollName = asset.name;
    m_spawnTransform.position = spawnPosition;
    m_spawnTransform.rotation = spawnEulerRotation;
    m_markedForRemoval = false;

    PxPhysics* physics = Hell::Physics::GetPxPhysics();
    PxScene* scene = Hell::Physics::GetPxScene();
    if (!physics || !scene || asset.markers.empty()) return false;

    const PxTransform rootPose(Hell::Physics::GlmMat4ToPxMat44(m_spawnTransform.to_mat4()));
    std::unordered_map<RagdollMarkerId, PxRigidDynamic*> actorByMarkerId;
    actorByMarkerId.reserve(asset.markers.size());

    for (const RagdollMarkerAsset& marker : asset.markers) {
        PxShape* shape = RagdollUtil::CreateShape(marker);
        if (!shape) {
            Logging::Error() << "Ragdoll::Init() failed to create shape for marker '" << marker.name << "'";
            CleanUp();
            return false;
        }

        const PxTransform restTransform = PxTransformFromRest(marker.bodyTransform);
        PxRigidDynamic* rigid = physics->createRigidDynamic(rootPose.transform(restTransform));
        if (!rigid) {
            shape->release();
            CleanUp();
            return false;
        }

        PxFilterData pxFilterData;
        pxFilterData.word0 = static_cast<PxU32>(filterData.raycastGroup);
        pxFilterData.word1 = static_cast<PxU32>(filterData.collisionGroup);
        pxFilterData.word2 = static_cast<PxU32>(filterData.collidesWith);
        pxFilterData.word3 = 0;
        shape->setQueryFilterData(pxFilterData);
        shape->setSimulationFilterData(pxFilterData);

        rigid->attachShape(*shape);
        shape->release();

        PxRigidBodyExt::setMassAndUpdateInertia(*rigid, std::max(0.001f, marker.rigidBody.mass));
        rigid->setLinearDamping(marker.rigidBody.linearDamping);
        rigid->setAngularDamping(marker.rigidBody.angularDamping);
        rigid->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD, marker.rigidBody.enableCCD);
        rigid->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, marker.rigidBody.isKinematic);
        rigid->setSleepThreshold(RAGDOLL_SLEEP_THRESHOLD);
        rigid->setSolverIterationCounts(
            GetSolverIterationCount(physicsSettings.positionIterations, marker.rigidBody.positionIterations),
            GetSolverIterationCount(physicsSettings.velocityIterations, marker.rigidBody.velocityIterations)
        );

        rigid->setMaxDepenetrationVelocity(marker.rigidBody.maxDepenetrationVelocity > 0.0f ? marker.rigidBody.maxDepenetrationVelocity : PX_MAX_F32);

        PhysicsUserData physicsUserData;
        physicsUserData.physicsType = PhysicsType::RIGID_DYNAMIC;
        physicsUserData.physicsId = Hell::Physics::CreatePhysicsId(Hell::Physics::PhysicsObjectType::RAGDOLL);
        physicsUserData.objectId = parentObjectId;
        rigid->userData = new PhysicsUserData(physicsUserData);

        scene->addActor(*rigid);
        actorByMarkerId.emplace(marker.id, rigid);
        m_pxRigidDynamics.push_back(rigid);
        m_markerBoneNames.push_back(marker.boneName);
        m_markerColors.push_back(glm::vec3(marker.color));
        m_markerRestTransforms.push_back(marker.bodyTransform);
        AddMarkerMeshData(marker);
    }

    for (const RagdollJointAsset& joint : asset.joints) {
        const auto parent = actorByMarkerId.find(joint.parentMarkerId);
        const auto child = actorByMarkerId.find(joint.childMarkerId);
        if (parent == actorByMarkerId.end() || child == actorByMarkerId.end()) {
            Logging::Error() << "Ragdoll::Init() failed to resolve actors for joint '" << joint.name << "'";
            CleanUp();
            return false;
        }

        const RagdollJointAsset runtimeJoint = RagdollJoint::CreatePhysicsReadyCopy(joint);
        glm::mat4 parentFrameMatrix = Hell::Math::RemoveScaleAndShear(runtimeJoint.parentFrame);
        glm::mat4 childFrameMatrix = Hell::Math::RemoveScaleAndShear(runtimeJoint.childFrame);
        PxTransform parentFrame(Hell::Physics::GlmMat4ToPxMat44(parentFrameMatrix));
        PxTransform childFrame(Hell::Physics::GlmMat4ToPxMat44(childFrameMatrix));

        PxD6Joint* d6 = PxD6JointCreate(*physics, parent->second, parentFrame, child->second, childFrame);
        if (!d6) {
            Logging::Error() << "Ragdoll::Init() failed to create joint '" << joint.name << "'";
            CleanUp();
            return false;
        }

        const float linearStiffness = joint.linearLimitStiffness > 0.0f ? joint.linearLimitStiffness : asset.solver.linearLimitStiffness;
        const float linearDamping = joint.linearLimitDamping > 0.0f ? joint.linearLimitDamping : asset.solver.linearLimitDamping;
        const float angularStiffness = joint.angularLimitStiffness > 0.0f ? joint.angularLimitStiffness : asset.solver.angularLimitStiffness;
        const float angularDamping = joint.angularLimitDamping > 0.0f ? joint.angularLimitDamping : asset.solver.angularLimitDamping;
        const PxSpring linearSpring(linearStiffness, linearDamping);
        const PxSpring angularSpring(angularStiffness, angularDamping);

        constexpr std::array<PxD6Axis::Enum, 3> LINEAR_AXES = { PxD6Axis::eX, PxD6Axis::eY, PxD6Axis::eZ };
        for (size_t axisIndex = 0; axisIndex < LINEAR_AXES.size(); axisIndex++) {
            const RagdollAxisLimit& limit = joint.linearLimits[axisIndex];
            const PxD6Axis::Enum axis = LINEAR_AXES[axisIndex];
            d6->setMotion(axis, joint.limitEnabled ? ToPxMotion(limit.motion) : PxD6Motion::eFREE);
            if (joint.limitEnabled && limit.motion == RagdollAxisMotion::LIMITED) {
                const float range = std::max(0.0f, limit.limit);
                d6->setLinearLimit(axis, PxJointLinearLimitPair(-range, range, linearSpring));
            }
        }

        ApplyAngularLimits(d6, runtimeJoint.angularLimits, runtimeJoint.limitEnabled, angularSpring);
        d6->setConstraintFlag(PxConstraintFlag::eCOLLISION_ENABLED, false);

        m_pxD6Joints.push_back(d6);
    }

    DisableSimulation();
    return true;
}

void Ragdoll::Update() {
    //for (PxRigidDynamic* pxRigidDynamic : m_pxRigidDynamics) {
    //    PxTransform pxTransform = pxRigidDynamic->getGlobalPose();
    //    PxMat44 pxMatrix(pxTransform);
    //    glm::mat4 matrix = Hell::Physics::PxMat44ToGlmMat4(pxMatrix);
    //}
}

void Ragdoll::MarkForRemoval() {
    m_markedForRemoval = true;
}

bool Ragdoll::IsMarkedForRemoval() const {
    return m_markedForRemoval;
}

void Ragdoll::AddForce(const std::string& boneName, const glm::vec3& force, bool wakeIfDisabled) {
    const size_t count = std::min(m_markerBoneNames.size(), m_pxRigidDynamics.size());

    for (size_t i = 0; i < count; i++) {
        if (m_markerBoneNames[i] != boneName) continue;

        PxRigidDynamic* pxRigidDynamic = m_pxRigidDynamics[i];
        if (!pxRigidDynamic) return;

        if (!m_simulationEnabled) {
            if (!wakeIfDisabled) return;
            EnableSimulation();
        }

        pxRigidDynamic->addForce(PxVec3(force.x, force.y, force.z), PxForceMode::eVELOCITY_CHANGE, true);
        return;
    }
}

void Ragdoll::SetAngularVelocity(const std::string& boneName, const glm::vec3& angularVelocity, bool wakeIfDisabled) {
    const size_t count = std::min(m_markerBoneNames.size(), m_pxRigidDynamics.size());

    for (size_t i = 0; i < count; i++) {
        if (m_markerBoneNames[i] != boneName) continue;

        PxRigidDynamic* pxRigidDynamic = m_pxRigidDynamics[i];
        if (!pxRigidDynamic) return;

        if (!m_simulationEnabled) {
            if (!wakeIfDisabled) return;
            EnableSimulation();
        }

        pxRigidDynamic->setAngularVelocity(PxVec3(angularVelocity.x, angularVelocity.y, angularVelocity.z), true);
        return;
    }
}

void Ragdoll::AddForce(uint64_t physicsId, const glm::vec3& force, bool wakeIfDisabled) {
    for (PxRigidDynamic* pxRigidDynamic : m_pxRigidDynamics) {
        if (!pxRigidDynamic) continue;

        PhysicsUserData* physicsUserData = static_cast<PhysicsUserData*>(pxRigidDynamic->userData);
        if (!physicsUserData) continue;

        if (physicsUserData->physicsId == physicsId) {
            if (!m_simulationEnabled) {
                if (!wakeIfDisabled) return;
                EnableSimulation();
            }

            pxRigidDynamic->addForce(PxVec3(force.x, force.y, force.z), PxForceMode::eVELOCITY_CHANGE, true);

            return;
        }
    }
}

void Ragdoll::AddImpulse(uint64_t physicsId, const glm::vec3& impulse, bool wakeIfDisabled) {
    bool ownsPhysicsId = false;
    for (PxRigidDynamic* pxRigidDynamic : m_pxRigidDynamics) {
        if (!pxRigidDynamic) continue;

        PhysicsUserData* physicsUserData = static_cast<PhysicsUserData*>(pxRigidDynamic->userData);
        if (!physicsUserData) continue;

        if (physicsUserData->physicsId == physicsId) {
            ownsPhysicsId = true;
            break;
        }
    }

    if (!ownsPhysicsId) return;
    if (!m_simulationEnabled) {
        if (!wakeIfDisabled) return;
        EnableSimulation();
    }

    float totalMass = 0.0f;
    for (PxRigidDynamic* pxRigidDynamic : m_pxRigidDynamics) {
        if (pxRigidDynamic) {
            totalMass += pxRigidDynamic->getMass();
        }
    }
    if (totalMass <= 0.0f) return;

    const PxVec3 pxImpulse(impulse.x, impulse.y, impulse.z);
    for (PxRigidDynamic* pxRigidDynamic : m_pxRigidDynamics) {
        if (!pxRigidDynamic) continue;

        const float massFraction = pxRigidDynamic->getMass() / totalMass;
        pxRigidDynamic->addForce(pxImpulse * massFraction, PxForceMode::eIMPULSE, true);
    }
}

void Ragdoll::AddImpulseAtPosition(uint64_t physicsId, const glm::vec3& impulse, const glm::vec3& position, bool wakeIfDisabled) {
    for (PxRigidDynamic* pxRigidDynamic : m_pxRigidDynamics) {
        if (!pxRigidDynamic) continue;

        PhysicsUserData* physicsUserData = static_cast<PhysicsUserData*>(pxRigidDynamic->userData);
        if (!physicsUserData) continue;

        if (physicsUserData->physicsId == physicsId) {
            if (!m_simulationEnabled) {
                if (!wakeIfDisabled) return;
                EnableSimulation();
            }

            PxVec3 pxImpulse(impulse.x, impulse.y, impulse.z);
            PxVec3 pxPosition(position.x, position.y, position.z);
            PxRigidBodyExt::addForceAtPos(*pxRigidDynamic, pxImpulse, pxPosition, PxForceMode::eIMPULSE, true);

            return;
        }
    }
}

void Ragdoll::AddAngularVelocityChangeAtPosition(uint64_t physicsId, const glm::vec3& velocityChange, const glm::vec3& position, bool wakeIfDisabled) {
    for (PxRigidDynamic* pxRigidDynamic : m_pxRigidDynamics) {
        if (!pxRigidDynamic) continue;

        PhysicsUserData* physicsUserData = static_cast<PhysicsUserData*>(pxRigidDynamic->userData);
        if (!physicsUserData) continue;

        if (physicsUserData->physicsId == physicsId) {
            if (!m_simulationEnabled) {
                if (!wakeIfDisabled) return;
                EnableSimulation();
            }

            PxVec3 pxVelocityChange(velocityChange.x, velocityChange.y, velocityChange.z);
            PxVec3 pxPosition(position.x, position.y, position.z);
            PxVec3 centerOfMass = pxRigidDynamic->getGlobalPose().transform(pxRigidDynamic->getCMassLocalPose().p);
            PxVec3 angularVelocityChange = (pxPosition - centerOfMass).cross(pxVelocityChange);
            pxRigidDynamic->addTorque(angularVelocityChange, PxForceMode::eVELOCITY_CHANGE, true);

            return;
        }
    }
}

void Ragdoll::DisableSimulation() {
    for (PxRigidDynamic* pxRigidDynamic : m_pxRigidDynamics) {
        if (!pxRigidDynamic) continue;
        pxRigidDynamic->setLinearVelocity(PxVec3(0.0f));
        pxRigidDynamic->setAngularVelocity(PxVec3(0.0f));
        pxRigidDynamic->setActorFlag(PxActorFlag::eDISABLE_SIMULATION, true);
    }
    m_simulationEnabled = false;
}

void Ragdoll::ProjectPoseToJointLimits() {
    std::unordered_map<PxRigidActor*, std::vector<PxD6Joint*>> jointsByParent;
    std::unordered_set<PxRigidActor*> childActors;

    for (PxD6Joint* joint : m_pxD6Joints) {
        if (!joint) continue;

        PxRigidActor* parentActor = nullptr;
        PxRigidActor* childActor = nullptr;
        joint->getActors(parentActor, childActor);
        if (!parentActor || !childActor) continue;

        jointsByParent[parentActor].push_back(joint);
        childActors.insert(childActor);
    }

    std::vector<PxRigidActor*> pendingParents;
    for (PxRigidDynamic* rigid : m_pxRigidDynamics) {
        if (rigid && !childActors.contains(rigid)) {
            pendingParents.push_back(rigid);
        }
    }

    while (!pendingParents.empty()) {
        PxRigidActor* parentActor = pendingParents.back();
        pendingParents.pop_back();

        const auto joints = jointsByParent.find(parentActor);
        if (joints == jointsByParent.end()) continue;

        for (PxD6Joint* joint : joints->second) {
            PxRigidActor* ignoredParent = nullptr;
            PxRigidActor* childActor = nullptr;
            joint->getActors(ignoredParent, childActor);
            if (!childActor) continue;

            PxRigidDynamic* childRigid = static_cast<PxRigidDynamic*>(childActor);
            const PxTransform parentAnchor = parentActor->getGlobalPose() * joint->getLocalPose(PxJointActorIndex::eACTOR0);
            const PxTransform childFrame = joint->getLocalPose(PxJointActorIndex::eACTOR1);
            PxTransform childPose = childRigid->getGlobalPose();

            const PxQuat animatedChildFrameRotation = childPose.q * childFrame.q;
            const PxQuat animatedJointRotation = parentAnchor.q.getConjugate() * animatedChildFrameRotation;
            const PxQuat projectedJointRotation = ProjectRotationToJointLimits(*joint, animatedJointRotation);

            childPose.q = (parentAnchor.q * projectedJointRotation * childFrame.q.getConjugate()).getNormalized();
            childPose.p = parentAnchor.p - childPose.q.rotate(childFrame.p);
            childRigid->setGlobalPose(childPose, false);
            pendingParents.push_back(childActor);
        }
    }
}

void Ragdoll::EnableSimulation() {
    ProjectPoseToJointLimits();

    for (PxRigidDynamic* pxRigidDynamic : m_pxRigidDynamics) {
        if (!pxRigidDynamic) continue;

        pxRigidDynamic->setActorFlag(PxActorFlag::eDISABLE_SIMULATION, false);
        pxRigidDynamic->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, false);
        pxRigidDynamic->setLinearVelocity(PxVec3(0.0f), false);
        pxRigidDynamic->setAngularVelocity(PxVec3(0.0f), false);
        pxRigidDynamic->clearForce();
        pxRigidDynamic->clearTorque();
        pxRigidDynamic->wakeUp();
    }
    m_simulationEnabled = true;
}

void Ragdoll::SetSpawnPosition(const glm::vec3& position) {
    m_spawnTransform.position = position;
    SetToInitialPose();
    UpdateWorldSpaceAABBs(0.0f);
}

void Ragdoll::SetSpawnRotation(const glm::vec3& rotation) {
    m_spawnTransform.rotation = rotation;
    SetToInitialPose();
    UpdateWorldSpaceAABBs(0.0f);
}

void Ragdoll::SetToInitialPose() {
    PxTransform rootPose(Hell::Physics::GlmMat4ToPxMat44(m_spawnTransform.to_mat4()));

    const size_t count = std::min(m_pxRigidDynamics.size(), m_markerRestTransforms.size());
    for (size_t i = 0; i < count; ++i) {
        PxTransform restTransform = PxTransformFromRest(m_markerRestTransforms[i]);
        m_pxRigidDynamics[i]->setGlobalPose(rootPose.transform(restTransform));
        m_pxRigidDynamics[i]->setLinearVelocity(PxVec3(0));
        m_pxRigidDynamics[i]->setAngularVelocity(PxVec3(0));
    }
}

void Ragdoll::CleanUp() {
    if (Hell::MeshBuffer* meshBuffer = Hell::ResourceManager::GetMeshBufferPtr("PhysicsShapeGeometry")) {
        for (uint32_t meshId : m_markerMeshIds) {
            if (meshId != 0) {
                meshBuffer->RemoveMesh(meshId);
            }
        }
    }
    m_markerMeshIds.clear();
    m_markerColors.clear();
    m_markerBoneNames.clear();
    m_markerRestTransforms.clear();

    for (PxD6Joint* pxD6Joint : m_pxD6Joints) {
        if (pxD6Joint) {
            pxD6Joint->release();
        }
    }
    m_pxD6Joints.clear();

    for (PxRigidDynamic*& pxRigidDynamic : m_pxRigidDynamics) {
        if (pxRigidDynamic) {
            if (pxRigidDynamic->userData) {
                delete static_cast<PhysicsUserData*>(pxRigidDynamic->userData);
                pxRigidDynamic->userData = nullptr;
            }

            if (pxRigidDynamic->getScene()) {
                pxRigidDynamic->getScene()->removeActor(*pxRigidDynamic);
            }

            pxRigidDynamic->release();
            pxRigidDynamic = nullptr;
        }
    }
    m_pxRigidDynamics.clear();
    m_previousRigidWorldTransforms.clear();
}

bool Ragdoll::IsInMotion() {
    const float linearThreshold = 0.01f;
    const float angularThreshold = 0.01f;
    const float linearThresholdSq = linearThreshold * linearThreshold;
    const float angularThresholdSq = angularThreshold * angularThreshold;

    for (PxRigidDynamic* pxRigidDynamic : m_pxRigidDynamics) {
        if (!pxRigidDynamic) continue;
        if (pxRigidDynamic->getRigidBodyFlags().isSet(PxRigidBodyFlag::eKINEMATIC)) continue;
        if (pxRigidDynamic->isSleeping()) continue;

        const PxVec3 v = pxRigidDynamic->getLinearVelocity();
        const PxVec3 w = pxRigidDynamic->getAngularVelocity();
        if (v.magnitudeSquared() > linearThresholdSq || w.magnitudeSquared() > angularThresholdSq) {
            return true;
        }
    }
    return false;
}

const std::string& Ragdoll::GetBoneNameByPhysicsId(uint64_t physicsId) const {
    static const std::string empty = "";

    const size_t count = std::min(m_pxRigidDynamics.size(), m_markerBoneNames.size());
    for (size_t i = 0; i < count; i++) {
        PxRigidDynamic* pxRigidDynamic = m_pxRigidDynamics[i];
        if (!pxRigidDynamic) continue;

        PhysicsUserData* physicsUserData = static_cast<PhysicsUserData*>(pxRigidDynamic->userData);
        if (physicsUserData && physicsUserData->physicsId == physicsId) {
            return m_markerBoneNames[i];
        }
    }

    return empty;
}

uint64_t Ragdoll::GetPhysicsIdByBoneName(const std::string& boneName) const {
    const size_t count = std::min(m_pxRigidDynamics.size(), m_markerBoneNames.size());
    for (size_t i = 0; i < count; i++) {
        if (m_markerBoneNames[i] != boneName) continue;

        PxRigidDynamic* pxRigidDynamic = m_pxRigidDynamics[i];
        if (!pxRigidDynamic) return 0;

        PhysicsUserData* physicsUserData = static_cast<PhysicsUserData*>(pxRigidDynamic->userData);
        return physicsUserData ? physicsUserData->physicsId : 0;
    }

    return 0;
}

AABB Ragdoll::GetWorldSpaceAABB() {
    glm::vec3 min = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 max = glm::vec3(-std::numeric_limits<float>::max());

    for (PxRigidDynamic* pxRigidDynamic : m_pxRigidDynamics) {
        if (pxRigidDynamic) {
            const PxU32 nbShapes = pxRigidDynamic->getNbShapes();
            if (!nbShapes) continue;
            std::vector<PxShape*> shapes(nbShapes);
            pxRigidDynamic->getShapes(shapes.data(), nbShapes);

            const PxTransform pose = pxRigidDynamic->getGlobalPose();
            for (PxShape * s : shapes) {
                const PxBounds3 b = PxShapeExt::getWorldBounds(*s, *pxRigidDynamic, 1.0f);
                glm::vec3 bmin(b.minimum.x, b.minimum.y, b.minimum.z);
                glm::vec3 bmax(b.maximum.x, b.maximum.y, b.maximum.z);
                min = glm::min(min, bmin);
                max = glm::max(max, bmax);
            }
        }
    }

    return AABB(min, max);
}

void Ragdoll::GetWorldSpaceAABBs(std::vector<AABB>& aabbs) {
    aabbs.clear();

    for (PxRigidDynamic* pxRigidDynamic : m_pxRigidDynamics) {
        if (!pxRigidDynamic) {
            continue;
        }

        const PxU32 nbShapes = pxRigidDynamic->getNbShapes();
        if (!nbShapes) {
            continue;
        }

        glm::vec3 min = glm::vec3(std::numeric_limits<float>::max());
        glm::vec3 max = glm::vec3(-std::numeric_limits<float>::max());

        std::vector<PxShape*> shapes(nbShapes);
        pxRigidDynamic->getShapes(shapes.data(), nbShapes);

        for (PxShape* shape : shapes) {
            const PxBounds3 bounds = PxShapeExt::getWorldBounds(*shape, *pxRigidDynamic, 1.0f);
            const glm::vec3 boundsMin(bounds.minimum.x, bounds.minimum.y, bounds.minimum.z);
            const glm::vec3 boundsMax(bounds.maximum.x, bounds.maximum.y, bounds.maximum.z);

            min = glm::min(min, boundsMin);
            max = glm::max(max, boundsMax);
        }

        aabbs.push_back(AABB(min, max));
    }
}

void Ragdoll::UpdateWorldSpaceAABBs(float changeThreshold) {
    GetWorldSpaceAABBs(m_worldSpaceAABBs);

    std::vector<PxTransform> currentRigidWorldTransforms;
    currentRigidWorldTransforms.reserve(m_pxRigidDynamics.size());

    for (PxRigidDynamic* pxRigidDynamic : m_pxRigidDynamics) {
        currentRigidWorldTransforms.push_back(pxRigidDynamic
            ? pxRigidDynamic->getGlobalPose()
            : PxTransform(PxIdentity));
    }

    m_dirty = currentRigidWorldTransforms.size() != m_previousRigidWorldTransforms.size();

    if (!m_dirty) {
        // Use the same threshold as world-space distance for translation and radians for rotation.
        const float positionThresholdSquared = changeThreshold * changeThreshold;
        const float rotationDotThreshold = std::cos(changeThreshold * 0.5f);

        for (size_t i = 0; i < currentRigidWorldTransforms.size(); i++) {
            const PxTransform& currentTransform = currentRigidWorldTransforms[i];
            const PxTransform& previousTransform = m_previousRigidWorldTransforms[i];
            const PxVec3 positionDelta = currentTransform.p - previousTransform.p;
            const float rotationSimilarity = std::abs(currentTransform.q.dot(previousTransform.q));

            if (positionDelta.magnitudeSquared() > positionThresholdSquared ||
                rotationSimilarity < rotationDotThreshold) {
                m_dirty = true;
                break;
            }
        }
    }

    m_previousRigidWorldTransforms = currentRigidWorldTransforms;
}

glm::vec3 Ragdoll::GetMarkerColorByRigidIndex(uint32_t index) const {
    if (index >= m_markerColors.size()) {
        Logging::Error() << "Ragdoll::GetMarkerColorByRigidIndex() failed, index " << index << " out of range of size " << m_pxRigidDynamics.size();
        return glm::vec3(1.0f);
    }
    return m_markerColors[index];
}

uint32_t Ragdoll::GetMarkerMeshIdByRigidIndex(uint32_t index) const {
    if (index >= m_markerMeshIds.size()) {
        Logging::Error() << "Ragdoll::GetMarkerMeshIdByRigidIndex() failed, index " << index << " out of range of size " << m_pxRigidDynamics.size();
        return 0;
    }
    return m_markerMeshIds[index];
}

glm::mat4 Ragdoll::GetModelMatrixByRigidIndex(uint32_t index) const {
    if (index >= m_pxRigidDynamics.size()) {
        Logging::Error() << "Ragdoll::GetModelMatrixByRigidIndex() failed, index " << index << " out of range of size " << m_pxRigidDynamics.size();
        return glm::mat4(1.0f);
    }

    // Marker meshes are authored directly in rigid-local units, so only the
    // rigid actor pose belongs in the model matrix.
    return Hell::Physics::PxMat44ToGlmMat4(m_pxRigidDynamics[index]->getGlobalPose());
}

glm::mat4 Ragdoll::GetRigidWorldTransform(const std::string& boneName) const {
    const size_t count = std::min(m_pxRigidDynamics.size(), m_markerBoneNames.size());
    for (size_t i = 0; i < count; i++) {
        if (m_markerBoneNames[i] != boneName) continue;

        PxRigidDynamic* pxRigidDynamic = m_pxRigidDynamics[i];
        if (pxRigidDynamic) {
            return Hell::Physics::PxMat44ToGlmMat4(pxRigidDynamic->getGlobalPose());
        }
    }

    return glm::mat4(1.0f);
}

void Ragdoll::AddMarkerMeshData(const RagdollMarkerAsset& marker) {
    RagdollShapeMeshData meshData = RagdollShapeMesh::Create(marker.shape);

    uint32_t meshId = 0;
    if (Hell::MeshBuffer* meshBuffer = Hell::ResourceManager::GetMeshBufferPtr("PhysicsShapeGeometry")) {
        meshId = meshBuffer->AddMesh(meshData.vertices, meshData.indices, marker.name);
    }
    m_markerMeshIds.push_back(meshId);
}

