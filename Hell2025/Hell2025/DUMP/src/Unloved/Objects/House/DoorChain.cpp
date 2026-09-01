#include "DoorChain.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Unloved/Debug/DebugDraw.h"
#include "Hell/Math/Math.h"
#include "Hell/Physics/Physics.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Render/RendererUtil.h"
#include "Hell/Input.h"

namespace Unloved {
namespace Input = Hell::Input;


DoorChain::DoorChain(uint64_t id, DoorChainCreateInfo& createInfo, SpawnOffset& spawnOffset) {

    Model* model = Hell::ResourceManager::GetModelByName("ChainLink");
    if (!model) return;

    glm::mat4 linkBeginLocalBoneMatrix = model->GetBoneLocalMatrix("LinkBegin");
    glm::mat4 linkEndBoneLocalBoneMatrix = model->GetBoneLocalMatrix("LinkEnd");

    glm::vec3 linkBeginOffset = linkBeginLocalBoneMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    glm::vec3 linkEndOffset = linkEndBoneLocalBoneMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    glm::vec3 linkAxisLocal = glm::normalize(linkEndOffset - linkBeginOffset);

    // Hack yourself the world axis
    Transform worldRotationTransform;
    worldRotationTransform.rotation = createInfo.rotation;
    glm::mat4 worldRotatioMatrix = worldRotationTransform.to_mat4();
    glm::vec3 linkAxisWorld = worldRotatioMatrix * glm::vec4(linkAxisLocal, 0.0f);

    float magicSpacingPercentage = 0.795f;
    float linkSpacing = (model->GetExtents().z * magicSpacingPercentage);

    m_chainLinkMeshId = model->GetMeshIndices()[0];
    m_chainLinkEndMeshId = model->GetMeshIndices()[0];

    Mesh* chainLinkMesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(m_chainLinkMeshId);

    glm::vec3 extents = chainLinkMesh->extents;// glm::vec3(0.18f, 0.08f, 0.6f) * 1.0f;


    glm::vec3 chainOrigin = createInfo.position;
    glm::vec3 sizeOfOriginBox = glm::vec3(0.05f, 0.05f, 0.05f); // 5cm cube. This doesn't change anything.


    Transform originTransform;
    originTransform.position = chainOrigin;
    originTransform.rotation = createInfo.rotation;

    PhysicsFilterData filterData;
    filterData.raycastGroup = RaycastGroup::RAYCAST_ENABLED;
    filterData.collisionGroup = CollisionGroup::BULLET_CASING;
    filterData.collidesWith = CollisionGroup::ENVIROMENT_OBSTACLE;

    // Origin kinematic body
    {
        float mass = 1.0f;
        m_kinematicOriginPhysicsId = Hell::Physics::CreateRigidDynamicFromBoxExtents(originTransform, sizeOfOriginBox, true, mass, filterData, glm::mat4(1.0f));
        RigidDynamic* rigidDynamic = Hell::Physics::GetRigidDynamicById(m_kinematicOriginPhysicsId);
    }

    // Chain link
    int linkCount = 4;

    for (int i = 0; i < linkCount; i++) {

        bool isEven = (i % 2) == 0;

        Transform translation;
        translation.position = chainOrigin;
        translation.position -= linkBeginOffset;
        translation.position += (linkAxisWorld * linkSpacing * glm::vec3(i));

        Transform localRotation;
        if (!isEven) {
            localRotation.rotation.z = HELL_PI * 0.5;
        }

        glm::mat4 finalLinkTransform = translation.to_mat4() * worldRotatioMatrix * localRotation.to_mat4();

        // Shape local offset
        glm::mat4 localOffsetTransform = chainLinkMesh->localTransform;
        glm::mat4 shapeLocalPose = glm::inverse(localOffsetTransform);

        float mass = 10.0f;

        // Create link rigid
        uint64_t physicsId = Hell::Physics::CreateRigidDynamicFromBoxExtents(finalLinkTransform, extents, false, mass, filterData, shapeLocalPose);
        RigidDynamic* rigidDynamic = Hell::Physics::GetRigidDynamicById(physicsId);
        m_chainLinkPhysicsIds.push_back(physicsId);


        PxRigidDynamic* pxRigidDynamic = rigidDynamic->GetPxRigidDynamic();
        pxRigidDynamic->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, false);
        pxRigidDynamic->wakeUp();
        PxRigidBodyExt::setMassAndUpdateInertia(*pxRigidDynamic, mass);
        pxRigidDynamic->setAngularDamping(2.0f); // start 1 to 5
        pxRigidDynamic->setLinearDamping(0.1f);

        pxRigidDynamic->setSolverIterationCounts(24, 8);

        //sceneDesc.flags |= PxSceneFlag::eENABLE_CCD;
        pxRigidDynamic->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD, true); // this might not doing anything without the scene having CCD enabled

        PhysicsUserData physicsUserData;
        physicsUserData.physicsId = physicsId;
        Hell::Physics::SetRigidDynamicUserData(physicsId, physicsUserData);

        auto PxTransformToGlmMat4 = [](const PxTransform& t) {
            glm::quat q(t.q.w, t.q.x, t.q.y, t.q.z);
            glm::mat4 m = glm::mat4_cast(q);
            m[3] = glm::vec4(t.p.x, t.p.y, t.p.z, 1.0f);
            return m;
        };


        uint64_t parentId = (i == 0) ? m_kinematicOriginPhysicsId : m_chainLinkPhysicsIds[i - 1];
        uint64_t childId = m_chainLinkPhysicsIds[i];

        PxRigidDynamic* pxParent = Hell::Physics::GetRigidDynamicById(parentId)->GetPxRigidDynamic();
        PxRigidDynamic* pxChild = Hell::Physics::GetRigidDynamicById(childId)->GetPxRigidDynamic();

        PxTransform parentPose = pxParent->getGlobalPose();
        PxTransform childPose = pxChild->getGlobalPose();

        // Use the child's "begin" point as the joint anchor in world
        PxVec3 childAnchorLocal(linkBeginOffset.x, linkBeginOffset.y, linkBeginOffset.z);
        PxVec3 anchorWorld = childPose.transform(childAnchorLocal);

        // Build a world joint orientation (columns are X=right, Y=up, Z=forward)
        PxVec3 forwardLocal(linkAxisLocal.x, linkAxisLocal.y, linkAxisLocal.z);
        PxVec3 forwardWorld = childPose.q.rotate(forwardLocal);
        forwardWorld.normalize();

        PxVec3 upWorld(0.0f, 1.0f, 0.0f);
        PxVec3 rightWorld = upWorld.cross(forwardWorld);
        if (rightWorld.magnitudeSquared() < 1e-8f) {
            rightWorld = PxVec3(1.0f, 0.0f, 0.0f).cross(forwardWorld);
        }
        rightWorld.normalize();
        PxVec3 trueUpWorld = forwardWorld.cross(rightWorld);

        PxQuat jointRotWorld(PxMat33(rightWorld, trueUpWorld, forwardWorld));
        PxTransform jointWorld(anchorWorld, jointRotWorld);

        // Convert joint world frame into each actor's local space
        PxTransform parentLocal = parentPose.transformInv(jointWorld);
        PxTransform childLocal = childPose.transformInv(jointWorld);

        glm::mat4 parentFrame = PxTransformToGlmMat4(parentLocal);
        glm::mat4 childFrame = PxTransformToGlmMat4(childLocal);

        uint64_t jointId = Hell::Physics::CreateD6Joint(parentId, childId, parentFrame, childFrame);

        D6Joint* d6 = Hell::Physics::GetD6JointById(jointId);
        auto pxD6 = d6->GetPxD6Joint();

        // Start FREE on all axes
        pxD6->setMotion(PxD6Axis::eX, PxD6Motion::eLOCKED);
        pxD6->setMotion(PxD6Axis::eY, PxD6Motion::eLOCKED);
        pxD6->setMotion(PxD6Axis::eZ, PxD6Motion::eLOCKED);

        // Allow it to rotate
        pxD6->setMotion(PxD6Axis::eSWING1, PxD6Motion::eLIMITED);
        pxD6->setMotion(PxD6Axis::eSWING2, PxD6Motion::eLIMITED);
        pxD6->setMotion(PxD6Axis::eTWIST, PxD6Motion::eLIMITED);

        // First link can only rotate on twist axis
        if (i == 0) {
            pxD6->setMotion(PxD6Axis::eSWING1, PxD6Motion::eLOCKED);
            pxD6->setMotion(PxD6Axis::eSWING2, PxD6Motion::eLOCKED);
            pxD6->setMotion(PxD6Axis::eTWIST, PxD6Motion::eLIMITED);
        }

        physx::PxJointLimitCone swingLimit(
            Hell::Math::DegToRad(45.0f), // swing1
            Hell::Math::DegToRad(45.0f)  // swing2
        );
        pxD6->setSwingLimit(swingLimit);

        physx::PxJointAngularLimitPair twistLimit(
            Hell::Math::DegToRad(-00.0f), // lower limit
            Hell::Math::DegToRad(180.0f)  // upper limit
        );
        pxD6->setTwistLimit(twistLimit);
    }
}

void DoorChain::SubmitRenderItems() {
    for (int i = 0; i < m_chainLinkPhysicsIds.size(); i++) {
        if (RigidDynamic* rigidDynamic = Hell::Physics::GetRigidDynamicById(m_chainLinkPhysicsIds[i])) {
            glm::mat4 modelMatrix = rigidDynamic->GetWorldTransform();

            const int32_t materialIndex = Hell::ResourceManager::GetMaterialIndexByName("Tokarev");
            Material* material = Hell::ResourceManager::GetMaterialByIndex(materialIndex);
            RenderItem renderItem;
            renderItem.modelMatrix = modelMatrix;
            renderItem.inverseModelMatrix = glm::inverse(renderItem.modelMatrix);
            renderItem.materialIndex = materialIndex;
            renderItem.meshId = m_chainLinkMeshId;
            RendererUtil::UpdateRenderItemAABB(renderItem);

            RenderDataManager::SubmitRenderItem(renderItem);

            DebugDraw::DrawPoint(rigidDynamic->GetWorldTransform()[3], BLUE);

        }
    }
}

void DoorChain::CleanUp() {
    // Remove origin
    Hell::Physics::MarkRigidDynamicForRemoval(m_kinematicOriginPhysicsId);

    // Remove links
    for (uint64_t id : m_chainLinkPhysicsIds) {
        Hell::Physics::MarkRigidDynamicForRemoval(id);
    }
}

void DoorChain::Update(float deltaTime) {
    return;
    size_t linkCount = m_chainLinkPhysicsIds.size();
    if (linkCount == 0) return;

    uint64_t id = m_chainLinkPhysicsIds[linkCount - 1];

    RigidDynamic* finalLinkRigidDynamic = Hell::Physics::GetRigidDynamicById(id);
    if (!finalLinkRigidDynamic) return;

    PxRigidDynamic* pxRigidDynamic = finalLinkRigidDynamic->GetPxRigidDynamic();
    if (!pxRigidDynamic) return;

    if (Input::KeyPressed(HELL_KEY_ENTER)) {

        m_animateFinalLink = !m_animateFinalLink;

        if (m_animateFinalLink) {

            PxTransform globalPose = pxRigidDynamic->getGlobalPose();
            m_finalLinkPosition = glm::vec3(globalPose.p.x, globalPose.p.y, globalPose.p.z);
            m_finalLinkRotation = globalPose.q;

            pxRigidDynamic->clearForce(PxForceMode::eFORCE);
            pxRigidDynamic->clearTorque(PxForceMode::eFORCE);
            pxRigidDynamic->setLinearVelocity(PxVec3(0.0f));
            pxRigidDynamic->setAngularVelocity(PxVec3(0.0f));

            pxRigidDynamic->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
            //pxRigidDynamic->wakeUp();
        }
        else {
            pxRigidDynamic->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, false);
            pxRigidDynamic->wakeUp();
        }
    }

    if (m_animateFinalLink) {

        float speed = 1.0f;

        // Use your "held" input here if you have it (KeyDown/KeyHeld).
        if (Input::KeyDown(HELL_KEY_LEFT))  m_finalLinkPosition.z -= speed * deltaTime;
        if (Input::KeyDown(HELL_KEY_RIGHT)) m_finalLinkPosition.z += speed * deltaTime;
        if (Input::KeyDown(HELL_KEY_UP))    m_finalLinkPosition.y += speed * deltaTime;
        if (Input::KeyDown(HELL_KEY_DOWN))  m_finalLinkPosition.y -= speed * deltaTime;

        PxTransform target(
            PxVec3(m_finalLinkPosition.x, m_finalLinkPosition.y, m_finalLinkPosition.z),
            m_finalLinkRotation
        );

        pxRigidDynamic->setKinematicTarget(target);
        pxRigidDynamic->wakeUp();
    }
}
}
