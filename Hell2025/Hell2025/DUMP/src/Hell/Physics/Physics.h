#pragma once
#include "Hell/Math/AABB.h"
#include "Hell/Math/VecXZ.h"
#include "Hell/Physics/PhysicsIds.h"
#include "Hell/Physics/PhysicsTypes.h"
#include "Hell/Physics/PhysicsDebug.h"
#include "Hell/Physics/PhysicsResourceManagement.h"
#include "Hell/Physics/PhysicsIdWrappers.h"
#include "Hell/Physics/PhysicsContactQuery.h"
#include "Hell/Physics/PhysicsOverlapQuery.h"
#include "Hell/Physics/PhysicsRayQuery.h"
#include "Hell/Physics/PhysicsUtil.h"
#include "Hell/Render/VertexAttributes.h"

#include "CollisionReports.h"

#pragma warning(push, 0)
#include <physx/PxPhysicsAPI.h>
#include <physx/geometry/PxGeometryHelpers.h>
#include <physx/PxQueryFiltering.h>
#pragma warning(pop)
#include "Hell/Physics/Types/CharacterController.h"
#include "Hell/Physics/Types/D6Joint.h"
#include "Hell/Physics/Types/HeightField.h"
#include "Hell/Physics/Types/RigidDynamic.h"
#include "Hell/Physics/Types/RigidStatic.h"

#include <string>
#include <span>
#include <vector>

using namespace physx;

namespace Hell::Physics {
    void Init();
    void BeginFrame();
    void FlushPendingRemovals();
    void StepSimulation();
    void SyncRuntimeState();
    void ForceZeroStepUpdate();
    void AddCollisionReport(CollisionReport& collisionReport);
    void ClearCollisionReports();
    void ClearCharacterControllerCollsionReports();
    std::vector<CollisionReport>& GetCollisionReports();
    std::vector<CharacterCollisionReport>& GetCharacterCollisionReports();
    PxPhysics* GetPxPhysics();
    PxScene* GetPxScene();
    CCTHitCallback& GetCharacterControllerHitCallback();
    PxControllerManager* GetCharacterControllerManager();

    // Materials
    PxMaterial* GetDefaultMaterial();
    PxMaterial* GetGrassMaterial();
    void SetDefaultMaterialProperties(float staticFriction, float dynamicFriction, float restitution);

    // Height fields
    void ActivateAllHeightFields();
    void UpdateHeightFields();

    // Misc

    // CLEAN ME UP
    PxRigidStatic* GetGroundPlanePxRigidStatic();
}
