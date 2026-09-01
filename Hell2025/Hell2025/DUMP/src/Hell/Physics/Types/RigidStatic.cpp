#include "RigidStatic.h"
#include "Hell/Physics/Physics.h"

void RigidStatic::Update(float /*deltaTime*/) {
    
}

void RigidStatic::MarkForRemoval() {
    m_markedForRemoval = true;
}

void RigidStatic::SetPxRigidStatic(PxRigidStatic* rigidStatic) {
    m_pxRigidStatic = rigidStatic;
}

void RigidStatic::AddPxShape(PxShape* shape) {
    m_pxShapes.push_back(shape);
}

void RigidStatic::SetWorldTransform(glm::mat4 worldMatrix) {
    if (!m_pxRigidStatic) return;

    PxMat44 pxWorldMatrix = Hell::Physics::GlmMat4ToPxMat44(worldMatrix);
    m_pxRigidStatic->setGlobalPose(PxTransform(pxWorldMatrix));
}

void RigidStatic::SetUserData(PhysicsUserData physicsUserData) {
    if (!m_pxRigidStatic) return;

    m_pxRigidStatic->userData = new PhysicsUserData(physicsUserData);
}

glm::mat4 RigidStatic::GetGlobalPose() const {
    if (!m_pxRigidStatic) return glm::mat4(1.0f);

    PxTransform pxTransform = m_pxRigidStatic->getGlobalPose();
    return Hell::Physics::PxMat44ToGlmMat4(pxTransform);
}
