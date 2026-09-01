#include "RigidDynamic.h"
#include "Hell/Physics/Physics.h"

namespace {
    void ConfigureDefaultSleepBehavior(PxRigidDynamic* rigidDynamic) {
        if (!rigidDynamic) return;

        rigidDynamic->setLinearDamping(0.2f);
        rigidDynamic->setAngularDamping(1.0f);
        rigidDynamic->setSleepThreshold(0.01f);
    }
}

void RigidDynamic::Update(float deltaTime) {
    if (!m_pxRigidDynamic) return;

    constexpr float linearSleepSpeed = 0.03f;
    constexpr float angularSleepSpeed = 0.03f;
    constexpr float settleTimeToSleep = 0.35f;
    constexpr float renderDirtyPositionEps = 0.005f;
    constexpr float renderDirtyRotationEps = 0.002f;

    const PxBounds3 bounds = m_pxRigidDynamic->getWorldBounds();
    const glm::vec3 aabbMin(bounds.minimum.x, bounds.minimum.y, bounds.minimum.z);
    const glm::vec3 aabbMax(bounds.maximum.x, bounds.maximum.y, bounds.maximum.z);
    m_aabb = AABB(aabbMin, aabbMax);

    const PxTransform currentGlobalPose = m_pxRigidDynamic->getGlobalPose();
    const bool poseDirty = m_lifeTime < 0.1f || !Hell::Physics::PxTransformNearlyEqual(m_previousGlobalPose, currentGlobalPose, renderDirtyPositionEps, renderDirtyRotationEps);

    m_worldTransform = Hell::Physics::PxMat44ToGlmMat4(currentGlobalPose);

    if (IsKinematic()) {
        m_motionState = RigidDynamicMotionState::KINEMATIC;
        m_settleTimer = 0.0f;
        m_isDirty = false;
        m_previousGlobalPose = currentGlobalPose;
        m_lifeTime += deltaTime;
        return;
    }

    if (m_pxRigidDynamic->isSleeping()) {
        m_motionState = RigidDynamicMotionState::ASLEEP;
        m_settleTimer = 0.0f;
        m_isDirty = false;
        m_previousGlobalPose = currentGlobalPose;
        m_lifeTime += deltaTime;
        return;
    }

    const PxVec3 linearVelocity = m_pxRigidDynamic->getLinearVelocity();
    const PxVec3 angularVelocity = m_pxRigidDynamic->getAngularVelocity();
    const bool almostStill =
        linearVelocity.magnitudeSquared() < linearSleepSpeed * linearSleepSpeed &&
        angularVelocity.magnitudeSquared() < angularSleepSpeed * angularSleepSpeed;

    if (almostStill) {
        m_settleTimer += deltaTime;
        m_motionState = RigidDynamicMotionState::SETTLING;
    }
    else {
        m_settleTimer = 0.0f;
        m_motionState = RigidDynamicMotionState::AWAKE;
    }

    m_isDirty = poseDirty;

    if (m_settleTimer >= settleTimeToSleep) {
        m_pxRigidDynamic->clearForce(PxForceMode::eFORCE);
        m_pxRigidDynamic->clearTorque(PxForceMode::eFORCE);
        m_pxRigidDynamic->setLinearVelocity(PxVec3(0.0f));
        m_pxRigidDynamic->setAngularVelocity(PxVec3(0.0f));
        m_pxRigidDynamic->putToSleep();
        m_motionState = RigidDynamicMotionState::ASLEEP;
        m_settleTimer = 0.0f;
        m_isDirty = false;
    }

    m_previousGlobalPose = currentGlobalPose;
    m_lifeTime += deltaTime;
}

void RigidDynamic::MarkForRemoval() {
    m_markedForRemoval = true;
}

void RigidDynamic::SetPxRigidDynamic(PxRigidDynamic* rigidDynamic) {
    m_pxRigidDynamic = rigidDynamic;

    if (m_pxRigidDynamic) {
        ConfigureDefaultSleepBehavior(m_pxRigidDynamic);
    }
}

//void RigidDynamic::SetPxShape(PxShape* shape) {
//    m_pxShape = shape; 
//}

void RigidDynamic::SetPxShapes(const std::vector<PxShape*>& pxShapes) {
    m_pxShapes = pxShapes;
}

void RigidDynamic::SetFilterData(PhysicsFilterData filterData) {
    PxFilterData pxFilterData;
    pxFilterData.word0 = (PxU32)filterData.raycastGroup;
    pxFilterData.word1 = (PxU32)filterData.collisionGroup;
    pxFilterData.word2 = (PxU32)filterData.collidesWith;

    for (PxShape* pxShape : m_pxShapes) {
        pxShape->setQueryFilterData(pxFilterData);       // ray casts
        pxShape->setSimulationFilterData(pxFilterData);  // collisions
    }
}

void RigidDynamic::AddForce(glm::vec3 force) {
    if (!m_pxRigidDynamic) return;

    PxVec3 pxForce = Hell::Physics::GlmVec3toPxVec3(force);
    m_settleTimer = 0.0f;
    m_motionState = RigidDynamicMotionState::AWAKE;
    m_pxRigidDynamic->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, false);
    m_pxRigidDynamic->addForce(pxForce);
    m_pxRigidDynamic->wakeUp();
}

void RigidDynamic::AddVelocityChange(glm::vec3 velocityChange) {
    if (!m_pxRigidDynamic) return;

    PxVec3 pxVelocityChange = Hell::Physics::GlmVec3toPxVec3(velocityChange);
    m_settleTimer = 0.0f;
    m_motionState = RigidDynamicMotionState::AWAKE;
    m_pxRigidDynamic->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, false);
    m_pxRigidDynamic->addForce(pxVelocityChange, PxForceMode::eVELOCITY_CHANGE);
    m_pxRigidDynamic->wakeUp();
}

void RigidDynamic::AddImpulse(glm::vec3 impulse) {
    if (!m_pxRigidDynamic) return;

    PxVec3 pxImpulse = Hell::Physics::GlmVec3toPxVec3(impulse);
    m_settleTimer = 0.0f;
    m_motionState = RigidDynamicMotionState::AWAKE;
    m_pxRigidDynamic->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, false);
    m_pxRigidDynamic->addForce(pxImpulse, PxForceMode::eIMPULSE);
    m_pxRigidDynamic->wakeUp();
}

void RigidDynamic::AddImpulseAtPosition(glm::vec3 impulse, glm::vec3 position) {
    if (!m_pxRigidDynamic) return;

    PxVec3 pxImpulse = Hell::Physics::GlmVec3toPxVec3(impulse);
    PxVec3 pxPosition = Hell::Physics::GlmVec3toPxVec3(position);
    m_settleTimer = 0.0f;
    m_motionState = RigidDynamicMotionState::AWAKE;
    m_pxRigidDynamic->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, false);
    PxRigidBodyExt::addForceAtPos(*m_pxRigidDynamic, pxImpulse, pxPosition, PxForceMode::eIMPULSE, true);
    m_pxRigidDynamic->wakeUp();
}

void RigidDynamic::AddAngularVelocityChangeAtPosition(glm::vec3 velocityChange, glm::vec3 position) {
    if (!m_pxRigidDynamic) return;

    PxVec3 pxVelocityChange = Hell::Physics::GlmVec3toPxVec3(velocityChange);
    PxVec3 pxPosition = Hell::Physics::GlmVec3toPxVec3(position);
    PxVec3 centerOfMass = m_pxRigidDynamic->getGlobalPose().transform(m_pxRigidDynamic->getCMassLocalPose().p);
    PxVec3 angularVelocityChange = (pxPosition - centerOfMass).cross(pxVelocityChange);

    m_settleTimer = 0.0f;
    m_motionState = RigidDynamicMotionState::AWAKE;
    m_pxRigidDynamic->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, false);
    m_pxRigidDynamic->addTorque(angularVelocityChange, PxForceMode::eVELOCITY_CHANGE, true);
    m_pxRigidDynamic->wakeUp();
}

void RigidDynamic::SetGlobalPose(const glm::mat4& globalPoseMatrix) {
    if (!m_pxRigidDynamic) return;

    m_settleTimer = 0.0f;
    PxMat44 pxMatrix = Hell::Physics::GlmMat4ToPxMat44(globalPoseMatrix);
    PxTransform pxTransform = PxTransform(pxMatrix);
    m_pxRigidDynamic->setGlobalPose(pxTransform);
}

void RigidDynamic::SetKinematicTarget(const glm::mat4& globalPoseMatrix) {
    if (!m_pxRigidDynamic) return;
    if (m_pxRigidDynamic->getScene() == nullptr) return;
    if (!m_pxRigidDynamic->getRigidBodyFlags().isSet(PxRigidBodyFlag::eKINEMATIC)) return;

    PxTransform targetPose(PxMat44(Hell::Physics::GlmMat4ToPxMat44(globalPoseMatrix)));
    if (!targetPose.isValid()) return;

    m_settleTimer = 0.0f;
    m_pxRigidDynamic->setKinematicTarget(targetPose);
}

void RigidDynamic::SetUserData(PhysicsUserData physicsUserData) {
    if (!m_pxRigidDynamic) return;

    m_pxRigidDynamic->userData = new PhysicsUserData(physicsUserData);
}

bool RigidDynamic::IsKinematic() const {
    if (!m_pxRigidDynamic) return false;

    return m_pxRigidDynamic->getRigidBodyFlags().isSet(physx::PxRigidBodyFlag::eKINEMATIC);
}

glm::mat4 RigidDynamic::GetWorldMatrix() const {
    if (!m_pxRigidDynamic) return glm::mat4(1.0f);

    return Hell::Physics::PxMat44ToGlmMat4(m_pxRigidDynamic->getGlobalPose());
}

float RigidDynamic::GetVolume() {
    float volume = 0.0f;
    for (PxShape* pxShape : m_pxShapes) {
        volume += Hell::Physics::ComputeShapeVolume(pxShape);
    }
    return volume;
}

void RigidDynamic::UpdateMassAndInertia(float density) {
    PxRigidBodyExt::updateMassAndInertia(*m_pxRigidDynamic, density);
}
