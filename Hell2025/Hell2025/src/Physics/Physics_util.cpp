#include "Physics.h""
#include "Util.h"

namespace Physics {

    #include <physx/PxPhysicsAPI.h>

    bool PxTransformNearlyEqual(const PxTransform& a, const PxTransform& b) {
        constexpr float positionEps = 1e-4f;
        constexpr float rotationEps = 1e-4f;

        const float dxp = a.p.x - b.p.x;
        const float dyp = a.p.y - b.p.y;
        const float dzp = a.p.z - b.p.z;

        if (dxp > positionEps || dxp < -positionEps) return false;
        if (dyp > positionEps || dyp < -positionEps) return false;
        if (dzp > positionEps || dzp < -positionEps) return false;

        // q and -q represent the same rotation, so align via dot sign.
        const float dot = a.q.x * b.q.x + a.q.y * b.q.y + a.q.z * b.q.z + a.q.w * b.q.w;

        if (dot >= 0.0f) {
            const float dxq = a.q.x - b.q.x;
            const float dyq = a.q.y - b.q.y;
            const float dzq = a.q.z - b.q.z;
            const float dwq = a.q.w - b.q.w;

            if (dxq > rotationEps || dxq < -rotationEps) return false;
            if (dyq > rotationEps || dyq < -rotationEps) return false;
            if (dzq > rotationEps || dzq < -rotationEps) return false;
            if (dwq > rotationEps || dwq < -rotationEps) return false;
            return true;
        }
        else {
            // Compare a.q to -b.q without modifying b.
            const float dxq = a.q.x + b.q.x;
            const float dyq = a.q.y + b.q.y;
            const float dzq = a.q.z + b.q.z;
            const float dwq = a.q.w + b.q.w;

            if (dxq > rotationEps || dxq < -rotationEps) return false;
            if (dyq > rotationEps || dyq < -rotationEps) return false;
            if (dzq > rotationEps || dzq < -rotationEps) return false;
            if (dwq > rotationEps || dwq < -rotationEps) return false;
            return true;
        }
    }

    glm::vec3 PxVec3toGlmVec3(PxVec3 vec) {
        return { vec.x, vec.y, vec.z };
    }
    
    glm::vec3 PxVec3toGlmVec3(PxExtendedVec3 vec) {
        return { vec.x, vec.y, vec.z };
    }
    
    PxVec3 GlmVec3toPxVec3(const glm::vec3& vec) {
        return { vec.x, vec.y, vec.z };
    }
    
    PxQuat GlmQuatToPxQuat(const glm::quat& quat) {
        return { quat.x, quat.y, quat.z, quat.w };
    }
    
    glm::quat PxQuatToGlmQuat(PxQuat quat) {
        return { quat.x, quat.y, quat.z, quat.w };
    }    

    glm::mat4 PxMat44ToGlmMat4(physx::PxMat44 pxMatrix) {
        glm::mat4 matrix;
        for (int x = 0; x < 4; x++)
            for (int y = 0; y < 4; y++)
                matrix[x][y] = pxMatrix[x][y];
        return matrix;
    }

    physx::PxMat44 GlmMat4ToPxMat44(const glm::mat4& glmMatrix) {
        physx::PxMat44 matrix;
        std::copy(glm::value_ptr(glmMatrix),
                  glm::value_ptr(glmMatrix) + 16,
                  reinterpret_cast<float*>(&matrix));
        return matrix;
    }

    glm::vec3 GetHeightMapPositionAtXZ(float x, float z) {
        ActivateAllHeightFields(); // TODO: Rewrite this function to only activate the heightfield that is beneath this ray origin

        PhysXRayResult rayResult = Physics::CastPhysXRayHeightMap(glm::vec3(x, 200, z), glm::vec3(0.0f, -1.0f, 0.0f), 250);
        if (rayResult.hitFound) {
            return rayResult.hitPosition;
        }
        else {
            return glm::vec3(0.0f);
        }
    }

    PhysXRayResult CastPhysXRayStaticEnvironment(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, float rayLength) {
        PxScene* scene = Physics::GetPxScene();
        PxVec3 origin = PxVec3(rayOrigin.x, rayOrigin.y, rayOrigin.z);
        PxVec3 unitDir = PxVec3(rayDirection.x, rayDirection.y, rayDirection.z);
        PxReal maxDistance = rayLength;
        PxRaycastBuffer hit;
        PxHitFlags outputFlags = PxHitFlag::ePOSITION | PxHitFlag::eNORMAL | PxHitFlag::eMESH_BOTH_SIDES;

        PxQueryFilterData filterData = PxQueryFilterData();
        filterData.flags = PxQueryFlag::eSTATIC | PxQueryFlag::eDYNAMIC | PxQueryFlag::ePREFILTER;

        // Defaults
        PhysXRayResult result;
        result.hitObjectName = "NO_USERDATA";
        result.hitPosition = glm::vec3(0, 0, 0);
        result.hitNormal = glm::vec3(0, 0, 0);
        result.rayDirection = rayDirection;
        result.userData = PhysicsUserData();
        result.distanceToHit = rayLength;

        RaycastStaticEnviromentFilterCallback callback;
        result.hitFound = scene->raycast(origin, unitDir, maxDistance, hit, outputFlags, filterData, &callback);

        // On hit
        if (result.hitFound) {
            result.hitPosition = glm::vec3(hit.block.position.x, hit.block.position.y, hit.block.position.z);
            result.hitNormal = glm::vec3(hit.block.normal.x, hit.block.normal.y, hit.block.normal.z);
            result.distanceToHit = glm::distance(rayOrigin, result.hitPosition);
            result.hitFound = true;
            PhysicsUserData* userData = (PhysicsUserData*)hit.block.actor->userData;
            if (userData) {
                result.userData = *userData;
                result.hitObjectName = "HAS_USERDATA";
            }
        }

        return result;
    }

    PhysXRayResult CastPhysXRayHeightMap(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, float rayLength) {
        PxScene* scene = Physics::GetPxScene();
        PxVec3 origin = PxVec3(rayOrigin.x, rayOrigin.y, rayOrigin.z);
        PxVec3 unitDir = PxVec3(rayDirection.x, rayDirection.y, rayDirection.z);
        PxReal maxDistance = rayLength;
        PxRaycastBuffer hit;
        PxHitFlags outputFlags = PxHitFlag::ePOSITION | PxHitFlag::eNORMAL | PxHitFlag::eMESH_BOTH_SIDES;

        PxQueryFilterData filterData = PxQueryFilterData();
        filterData.flags = PxQueryFlag::eSTATIC | PxQueryFlag::eDYNAMIC | PxQueryFlag::ePREFILTER;

        // Defaults
        PhysXRayResult result;
        result.hitObjectName = "NO_USERDATA";
        result.hitPosition = glm::vec3(0, 0, 0);
        result.hitNormal = glm::vec3(0, 0, 0);
        result.rayDirection = rayDirection;
        result.userData = PhysicsUserData();

        RaycastHeightFieldFilterCallback callback;
        result.hitFound = scene->raycast(origin, unitDir, maxDistance, hit, outputFlags, filterData, &callback);

        // On hit
        if (result.hitFound) {
            result.hitPosition = glm::vec3(hit.block.position.x, hit.block.position.y, hit.block.position.z);
            result.hitNormal = glm::vec3(hit.block.normal.x, hit.block.normal.y, hit.block.normal.z);
            result.hitFound = true;
            result.distanceToHit = glm::distance(rayOrigin, result.hitPosition);
            PhysicsUserData* userData = (PhysicsUserData*)hit.block.actor->userData;
            if (userData) {
                result.userData = *userData;
                result.hitObjectName = "HAS_USERDATA";
            }
        }

        return result;
    }

    PhysXRayResult CastPhysXRay(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, float rayLength, bool cullBackFacing, RaycastIgnoreFlags ignoreFlags, std::vector<PxRigidActor*> ignoredActors) {
        PxScene* scene = Physics::GetPxScene();
        PxVec3 origin = PxVec3(rayOrigin.x, rayOrigin.y, rayOrigin.z);
        PxVec3 unitDir = PxVec3(rayDirection.x, rayDirection.y, rayDirection.z);
        PxReal maxDistance = rayLength;
        PxRaycastBuffer hit;
        PxHitFlags outputFlags = PxHitFlag::ePOSITION | PxHitFlag::eNORMAL;// | PxHitFlag::eMESH_BOTH_SIDES;

        if (!cullBackFacing) {
            outputFlags |= PxHitFlag::eMESH_BOTH_SIDES;
        }

        PxQueryFilterData filterData = PxQueryFilterData();
        filterData.data.word0 = 0xFFFFFFFF; // Or a specific bit for "ray"
        filterData.data.word1 = 0xFFFFFFFF; // Collide with all layers/types defined in your shape's word0
        filterData.data.word2 = 0;
        filterData.flags = PxQueryFlag::eSTATIC | PxQueryFlag::eDYNAMIC | PxQueryFlag::ePREFILTER;

        // Defaults
        PhysXRayResult result;
        result.hitObjectName = "NO_USERDATA";
        result.hitPosition = glm::vec3(0, 0, 0);
        result.hitNormal = glm::vec3(0, 0, 0);
        result.rayDirection = rayDirection;
        result.userData = PhysicsUserData();

        RaycastFilterCallback callback;
        callback.m_ignoredActors = GetIgnoreList(ignoreFlags);
        callback.m_ignoredActors.insert(callback.m_ignoredActors.end(), ignoredActors.begin(), ignoredActors.end());

        result.hitFound = scene->raycast(origin, unitDir, maxDistance, hit, outputFlags, filterData, &callback);
        //std::cout << "hit found: " << result.hitFound << "\n";

        // On hit
        if (result.hitFound) {
            result.hitPosition = glm::vec3(hit.block.position.x, hit.block.position.y, hit.block.position.z);
            result.hitNormal = glm::vec3(hit.block.normal.x, hit.block.normal.y, hit.block.normal.z);
            result.distanceToHit = glm::distance(rayOrigin, result.hitPosition);
            result.hitFound = true;
            PhysicsUserData* userData = (PhysicsUserData*)hit.block.actor->userData;
            if (userData) {
                result.userData = *userData;
                result.hitObjectName = "HAS_USERDATA";
            }
        }
        return result;
    }

    PhysXOverlapReport OverlapTest(const PxGeometry& overlapShape, const PxTransform& shapePose, PxU32 collisionGroup) {
        PxScene* pxScene = Physics::GetPxScene();

        PxQueryFilterData overlapFilterData = PxQueryFilterData();
        overlapFilterData.data.word1 = collisionGroup;

        const PxU32 bufferSize = 256;
        PxOverlapHit hitBuffer[bufferSize];
        PxOverlapBuffer buf(hitBuffer, bufferSize);

        std::vector<PxActor*> hitActors;

        if (pxScene->overlap(overlapShape, shapePose, buf, overlapFilterData)) {
            for (int i = 0; i < buf.getNbTouches(); i++) {
                PxActor* hit = buf.getTouch(i).actor;
                // Check for duplicates
                bool found = false;
                for (const PxActor* foundHit : hitActors) {
                    if (foundHit == hit) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    hitActors.push_back(hit);
                }
            }
        }

        // Fill out the shit you need
        PhysXOverlapReport overlapReport;
        for (PxActor* hitActor : hitActors) {
            PhysicsUserData* userData = (PhysicsUserData*)hitActor->userData;
            if (userData) {
                if (userData->physicsType == PhysicsType::RIGID_DYNAMIC) {
                    PxRigidDynamic* rigid = (PxRigidDynamic*)hitActor;
                    PhysXOverlapResult& overlapResult = overlapReport.hits.emplace_back();
                    overlapResult.userData = *userData;
                    overlapResult.objectPosition.x = rigid->getGlobalPose().p.x;
                    overlapResult.objectPosition.y = rigid->getGlobalPose().p.y;
                    overlapResult.objectPosition.z = rigid->getGlobalPose().p.z;
                }
                if (userData->physicsType == PhysicsType::RIGID_STATIC ||
                    userData->physicsType == PhysicsType::HEIGHT_FIELD) {
                    PxRigidStatic* rigid = (PxRigidStatic*)hitActor;
                    PhysXOverlapResult& overlapResult = overlapReport.hits.emplace_back();
                    overlapResult.userData = *userData;
                    overlapResult.objectPosition.x = rigid->getGlobalPose().p.x;
                    overlapResult.objectPosition.y = rigid->getGlobalPose().p.y;
                    overlapResult.objectPosition.z = rigid->getGlobalPose().p.z;
                }
            }
        }
        return overlapReport;
    }

    float ComputeShapeVolume(PxShape* pxShape) {
        if (!pxShape) {
            std::cout << "Physics::ComputeShapeDenisty() failed: pxShape was nullptr\n";
            return 0.0f;
        }

        const PxGeometry& pxGeometry = pxShape->getGeometry();
        const PxGeometryHolder pxGeometryHolder = pxShape->getGeometry();
        const PxGeometryType::Enum pxGeometryType = pxGeometry.getType();

        if (pxGeometryType == PxGeometryType::Enum::eBOX) {
            const PxBoxGeometry& box = pxGeometryHolder.box();
            return Util::GetCubeVolume(box.halfExtents.x, box.halfExtents.y, box.halfExtents.z);
        }
        else if (pxGeometryType == PxGeometryType::Enum::eSPHERE) {
            const PxSphereGeometry& sphere = pxGeometryHolder.sphere();
            return Util::GetSphereVolume(sphere.radius);
        }
        else if (pxGeometryType == PxGeometryType::Enum::eCAPSULE) {
            const PxCapsuleGeometry& capsule = pxGeometryHolder.capsule();
            return Util::GetCapsuleVolume(capsule.radius, capsule.halfHeight);
        }
        else {
            std::cout << "Physics::ComputeShapeVolume() failed: pxShape was not cube, sphere, or capsule\n";
            return 0.0f;
        }
    }

    std::string GetPxShapeTypeAsString(PxShape* pxShape) {
        if (!pxShape) {
            std::cout << "Physics::ComputeShapeDenisty() failed: pxShape was nullptr\n";
            return "Invalid shape";
        }

        const PxGeometry& pxGeometry = pxShape->getGeometry();
        const PxGeometryHolder pxGeometryHolder = pxShape->getGeometry();
        const PxGeometryType::Enum pxGeometryType = pxGeometry.getType();

        switch (pxGeometryType) {
            case physx::PxGeometryType::Enum::eBOX:             return "Box";
            case physx::PxGeometryType::Enum::eSPHERE:          return "Sphere";
            case physx::PxGeometryType::Enum::eCAPSULE:         return "Capsule";
            case physx::PxGeometryType::Enum::ePLANE:           return "Plane";
            case physx::PxGeometryType::Enum::eCONVEXMESH:      return "ConvexMesh";
            case physx::PxGeometryType::Enum::eTRIANGLEMESH:    return "TriangleMesh";
            case physx::PxGeometryType::Enum::eHEIGHTFIELD:     return "HeightField";
            default:                                            return "Unknown shape type";
        }
    }
}
