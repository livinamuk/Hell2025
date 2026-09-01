#include "HeightField.h"
#include "Hell/Physics/Physics.h"
#include "Hell/Transform.h"

#include "Hell/Logging.h"

void HeightField::Create(Hell::vecXZ& worldSpaceOffset, const float* heightValues, float heightScale, float rowScale, float colScale) {
    m_physicsId = Hell::Physics::CreatePhysicsId(Hell::Physics::PhysicsObjectType::HEIGHT_FIELD);

    PxPhysics* pxPhysics = Hell::Physics::GetPxPhysics();
    PxScene* pxScene = Hell::Physics::GetPxScene();

    int numRows = 33;
    int numCols = 33;
    const float heightScaleFactor = 32767.0f;  // Using the full PxI16 range

    std::vector<PxHeightFieldSample> samples(numRows * numCols);

    for (int z = 0; z < numCols; z++) {
        for (int x = 0; x < numRows; x++) {
            int heightValueIndex = (z * numRows + x);
            int sampleIndex = (x * numCols + z);
            samples[sampleIndex].height = static_cast<PxI16>(heightValues[heightValueIndex] * heightScaleFactor);
            samples[sampleIndex].materialIndex0 = 0;
            samples[sampleIndex].materialIndex1 = 0;
            samples[sampleIndex].setTessFlag();
        }
    }
    // Height field description
    PxHeightFieldDesc heightFieldDesc;
    heightFieldDesc.format = PxHeightFieldFormat::eS16_TM;
    heightFieldDesc.nbRows = numRows;
    heightFieldDesc.nbColumns = numCols;
    heightFieldDesc.samples.data = samples.data();
    heightFieldDesc.samples.stride = sizeof(PxHeightFieldSample);
    if (!heightFieldDesc.isValid()) {
        Logging::Error() << "HeightField::Create(..) failed to create PxHeightField\n";
        m_pxHeightField = nullptr; // Invalid height field description
        return;
    }
    m_pxHeightField = PxCreateHeightField(heightFieldDesc, pxPhysics->getPhysicsInsertionCallback());

    PxShapeFlags shapeFlags(PxShapeFlag::eSCENE_QUERY_SHAPE | PxShapeFlag::eSIMULATION_SHAPE);

    PhysicsFilterData filterData;
    filterData.raycastGroup = RAYCAST_ENABLED;
    filterData.collisionGroup = ENVIROMENT_OBSTACLE;
    filterData.collidesWith = (CollisionGroup)(GENERIC_BOUNCEABLE | BULLET_CASING | CHARACTER_CONTROLLER | RAGDOLL_PLAYER | RAGDOLL_ENEMY | ITEM_PICK_UP);

    // Create shape
    PxMaterial* material = Hell::Physics::GetGrassMaterial();
    float worldHeightScale = 1.0f / 32767.0f * heightScale;
    PxHeightFieldGeometry hfGeom(m_pxHeightField, PxMeshGeometryFlags(), worldHeightScale, rowScale, colScale);
    m_pxShape = pxPhysics->createShape(hfGeom, *material, shapeFlags);
    m_pxShape->setFlag(PxShapeFlag::eVISUALIZATION, true);

    // Create rigid static
    Hell::Transform transform;
    transform.position = glm::vec3(worldSpaceOffset.x, 0.0f, worldSpaceOffset.z);
    transform.position *= glm::vec3(0.5f); // no idea why you gotta divide by 2

    PxQuat quat = Hell::Physics::GlmQuatToPxQuat(glm::quat(transform.rotation));
    PxTransform trans = PxTransform(PxVec3(transform.position.x, transform.position.y, transform.position.z), quat);
    m_pxRigidStatic = pxPhysics->createRigidStatic(trans);

    PxFilterData pxFilterData;
    pxFilterData.word0 = (PxU32)filterData.raycastGroup;
    pxFilterData.word1 = (PxU32)filterData.collisionGroup;
    pxFilterData.word2 = (PxU32)filterData.collidesWith;
    m_pxShape->setQueryFilterData(pxFilterData);       // ray casts
    m_pxShape->setSimulationFilterData(pxFilterData);  // collisions
    PxMat44 localShapeMatrix = Hell::Physics::GlmMat4ToPxMat44(transform.to_mat4());
    PxTransform localShapeTransform(localShapeMatrix);
    m_pxShape->setLocalPose(localShapeTransform);
    m_pxRigidStatic->attachShape(*m_pxShape);

    m_worldSpaceOffset = worldSpaceOffset;

    // Set user data
    PhysicsUserData userData;
    userData.physicsId = m_physicsId;
    userData.objectId = 0;
    userData.physicsType = PhysicsType::HEIGHT_FIELD;
    m_pxRigidStatic->userData = new PhysicsUserData(userData);

    // Update AABB
    PxBounds3 bounds = m_pxRigidStatic->getWorldBounds();
    glm::vec3 aabbMin(bounds.minimum.x, bounds.minimum.y, bounds.minimum.z);
    glm::vec3 aabbMax(bounds.maximum.x, bounds.maximum.y, bounds.maximum.z);
    m_aabb = AABB(aabbMin, aabbMax);

    //pxScene->addActor(*m_pxRigidStatic);
    ActivatePhsyics();
}

void HeightField::ActivatePhsyics() {
    if (!m_activePhysics) {
        PxScene* pxScene = Hell::Physics::GetPxScene();
        pxScene->addActor(*m_pxRigidStatic);
        m_pxShape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, true);
        m_pxShape->setFlag(PxShapeFlag::eSCENE_QUERY_SHAPE, true);
    }
    m_activePhysics = true;
}

void HeightField::DisablePhsyics() {
    if (m_activePhysics) {
        PxScene* pxScene = Hell::Physics::GetPxScene();
        m_pxShape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, false);
        m_pxShape->setFlag(PxShapeFlag::eSCENE_QUERY_SHAPE, false);
        pxScene->removeActor(*m_pxRigidStatic);
    }
    m_activePhysics = false;
}

void HeightField::MarkForRemoval() {
    m_markedForRemoval = true;
}
