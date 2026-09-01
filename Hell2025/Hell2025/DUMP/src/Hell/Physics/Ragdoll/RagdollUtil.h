#pragma once
#include "Hell/Physics/Physics.h"

#include "RagdollAsset.h"

namespace RagdollUtil {

    inline PxCombineMode::Enum ToPxCombineMode(RagdollCombineMode mode) {
        switch (mode) {
            case RagdollCombineMode::AVERAGE:  return PxCombineMode::eAVERAGE;
            case RagdollCombineMode::MINIMUM:  return PxCombineMode::eMIN;
            case RagdollCombineMode::MULTIPLY: return PxCombineMode::eMULTIPLY;
            case RagdollCombineMode::MAXIMUM:  return PxCombineMode::eMAX;
        }
        return PxCombineMode::eAVERAGE;
    }

    inline PxShape* CreateShape(const RagdollMarkerAsset& marker) {
        PxPhysics* pxPhysics = Hell::Physics::GetPxPhysics();
        const RagdollShape& shape = marker.shape;
        const RagdollRigidBodyAsset& rigidBody = marker.rigidBody;

        PxMaterial* material = pxPhysics->createMaterial(rigidBody.friction, rigidBody.friction, rigidBody.restitution);
        material->setFrictionCombineMode(ToPxCombineMode(rigidBody.frictionCombineMode));
        material->setRestitutionCombineMode(ToPxCombineMode(rigidBody.restitutionCombineMode));

        PxShape* pxShape = nullptr;
        switch (shape.type) {
            case RagdollShapeType::BOX: {
                const PxBoxGeometry geometry{
                    std::max(0.001f, shape.extents.x * 0.5f),
                    std::max(0.001f, shape.extents.y * 0.5f),
                    std::max(0.001f, shape.extents.z * 0.5f)
                };
                pxShape = pxPhysics->createShape(geometry, *material);
                break;
            }
            case RagdollShapeType::SPHERE: {
                const PxSphereGeometry geometry{ std::max(0.001f, shape.radius) };
                pxShape = pxPhysics->createShape(geometry, *material);
                break;
            }
            case RagdollShapeType::CAPSULE: {
                const float halfHeight = std::max(0.001f, shape.length * 0.5f);
                const float radius = std::max(0.001f, shape.radius);
                pxShape = pxPhysics->createShape(PxCapsuleGeometry{ radius, halfHeight }, *material);
                break;
            }
            case RagdollShapeType::CONVEX_HULL:
                pxShape = Hell::Physics::CreateConvexShapeFromVertexList(
                    shape.convexVertices,
                    glm::vec3(1.0f),
                    material
                );
                break;
        }

        material->release();
        if (!pxShape) return nullptr;

        if (shape.type != RagdollShapeType::CONVEX_HULL) {
            const glm::vec3 localOffset = shape.offset;
            const PxQuat rotation =
                PxQuat(shape.rotationRadians.z, PxVec3(0.0f, 0.0f, 1.0f)) *
                PxQuat(shape.rotationRadians.y, PxVec3(0.0f, 1.0f, 0.0f)) *
                PxQuat(shape.rotationRadians.x, PxVec3(1.0f, 0.0f, 0.0f));
            const PxTransform localPose{ PxVec3(localOffset.x, localOffset.y, localOffset.z), rotation };
            pxShape->setLocalPose(pxShape->getLocalPose().transform(localPose));
        }

        const PxTolerancesScale tolerancesScale = pxPhysics->getTolerancesScale();
        pxShape->setContactOffset(rigidBody.thickness * tolerancesScale.length);
        return pxShape;
    }

}
