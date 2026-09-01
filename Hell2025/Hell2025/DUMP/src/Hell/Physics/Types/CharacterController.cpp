#include "CharacterController.h"
#include "Hell/Physics/Physics.h"

namespace {
    struct CharacterControllerQueryFilter : PxQueryFilterCallback {
        PxQueryHitType::Enum preFilter(
            const PxFilterData&,
            const PxShape* shape,
            const PxRigidActor*,
            PxHitFlags&) override {
            const PxFilterData shapeFilterData = shape->getQueryFilterData();
            return (shapeFilterData.word1 & CollisionGroup::LADDER)
                ? PxQueryHitType::eNONE
                : PxQueryHitType::eBLOCK;
        }

        PxQueryHitType::Enum postFilter(
            const PxFilterData&,
            const PxQueryHit&,
            const PxShape*,
            const PxRigidActor*) override {
            return PxQueryHitType::eBLOCK;
        }
    };
}

void CharacterController::SetPxController(PxController* pxController) {
    m_pxController = pxController;
}

void CharacterController::SetGroundedState(bool state) {
    m_grounded = state;
}

void CharacterController::MarkForRemoval() {
    m_markedForRemoval = true;
}

void CharacterController::SetPosition(glm::vec3 position) {
    if (!m_pxController) return;
    
    PxExtendedVec3 pxExtendedVec3 = { position.x, position.y, position.z };
    m_pxController->setFootPosition(pxExtendedVec3);

}
void CharacterController::Move(const glm::vec3& displacement) {
    if (!m_pxController) return;

    PxFilterData filterData;
    filterData.word0 = 0;
    filterData.word1 = CollisionGroup::ENVIROMENT_OBSTACLE | CollisionGroup::ENVIROMENT_OBSTACLE_NO_DOG | CollisionGroup::CHARACTER_CONTROLLER; // Things to collide with
    CharacterControllerQueryFilter queryFilter;
    PxControllerFilters data;
    data.mFilterData = &filterData;
    data.mFilterCallback = &queryFilter;
    PxF32 minDist = 0.001f;
    float fixedDeltaTime = (1.0f / 60.0f);
    m_pxController->move(PxVec3(displacement.x, displacement.y, displacement.z), minDist, fixedDeltaTime, data);
}

glm::vec3 CharacterController::GetFootPosition() {
    if (!m_pxController) return glm::vec3(0.0f);

    PxExtendedVec3 pxPos = m_pxController->getFootPosition();
    return glm::vec3(static_cast<float>(pxPos.x), static_cast<float>(pxPos.y), static_cast<float>(pxPos.z));
}

const AABB CharacterController::GetAABB() const {
    if (m_pxController) {
        PxRigidDynamic* actor = m_pxController->getActor();
        PxBounds3 bounds = actor->getWorldBounds();
        glm::vec3 aabbMin = glm::vec3(bounds.minimum.x, bounds.minimum.y, bounds.minimum.z);
        glm::vec3 aabbMax = glm::vec3(bounds.maximum.x, bounds.maximum.y, bounds.maximum.z);
        return AABB(aabbMin, aabbMax);
    }
    else {
        // This should never happen
        return AABB();
    }
}
