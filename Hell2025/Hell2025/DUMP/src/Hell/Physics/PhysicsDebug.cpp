#include "PhysicsDebug.h"
#include "Physics.h"

#include "Unloved/Common/Constants.h"
#include "Hell/Debug/DebugDraw.h"

#include <iostream>

namespace {

    glm::vec4 GetPhysicsDebugLineColor(Hell::Physics::DebugMode debugMode) {
        switch (debugMode) {
            case Hell::Physics::DebugMode::ALL:              return glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
            case Hell::Physics::DebugMode::COLLISION_SHAPES: return glm::vec4(0.0f, 1.0f, 1.0f, 1.0f);
            case Hell::Physics::DebugMode::RAGDOLLS:         return glm::vec4(0.0f, 1.0f, 1.0f, 1.0f);
            case Hell::Physics::DebugMode::RAYCAST_SHAPES:   return glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
            default:                                         return glm::vec4(1.0f);
        }
    }

    bool RagdollOwnsPxRigidDynamic(physx::PxRigidDynamic* pxRigidDynamic) {
        for (auto it = Hell::Physics::GetRagdolls().begin(); it != Hell::Physics::GetRagdolls().end(); ) {
            Ragdoll& ragdoll = it->second;
            for (physx::PxRigidDynamic* ragdollRigidDynamic : ragdoll.m_pxRigidDynamics) {
                if (pxRigidDynamic == ragdollRigidDynamic) {
                    return true;
                }
            }
            it++;
        }

        return false;
    }
}

namespace Hell::Physics {

    void PrintSceneInfo() {
        std::cout << "\n";
        std::cout << " **************************\n";
        std::cout << " *                        *\n";
        std::cout << " *    PhysX Scene Info    *\n";
        std::cout << " *                        *\n";
        std::cout << " **************************\n";
        std::cout << "\n";

        PrintSceneRigidInfo();
        PrintSceneRagdollInfo();
    }

    void DebugDrawRigidDynamicStateAABBs() {
        for (auto& [id, rigidDynamic] : GetRigidDynamics()) {
            switch (rigidDynamic.GetMotionState()) {
                case RigidDynamicMotionState::KINEMATIC: DebugDraw::DrawAABB(rigidDynamic.GetAABB(), BLUE); break;
                case RigidDynamicMotionState::AWAKE:     DebugDraw::DrawAABB(rigidDynamic.GetAABB(), GREEN); break;
                case RigidDynamicMotionState::SETTLING:  DebugDraw::DrawAABB(rigidDynamic.GetAABB(), YELLOW); break;
                case RigidDynamicMotionState::ASLEEP:    DebugDraw::DrawAABB(rigidDynamic.GetAABB(), RED); break;
            }
        }
    }

    std::vector<PhysicsDebugLine> GetPhysicsDebugLines(DebugMode debugMode) {
        std::vector<PhysicsDebugLine> debugLines;
        if (debugMode == DebugMode::NONE) {
            return debugLines;
        }

        PxScene* pxScene = GetPxScene();
        if (!pxScene) {
            return debugLines;
        }

        std::vector<PxRigidActor*> ignoreList;

        PxU32 constraintCount = pxScene->getNbConstraints();
        if (constraintCount) {
            std::vector<PxConstraint*> constraints(constraintCount);
            pxScene->getConstraints(constraints.data(), constraintCount);

            for (PxConstraint* constraint : constraints) {
                constraint->setFlag(PxConstraintFlag::eVISUALIZATION, true);
            }
        }

        //pxScene->setFlag(PxSceneFlag::eENABLE_DEBUG_VISUALIZATION, true);
        pxScene->setVisualizationParameter(PxVisualizationParameter::eSCALE, 0.05f);
        pxScene->setVisualizationParameter(PxVisualizationParameter::eJOINT_LOCAL_FRAMES, 1.0f);
        pxScene->setVisualizationParameter(PxVisualizationParameter::eJOINT_LIMITS, 1.0f);

        // Prepare
        PxU32 nbActors = pxScene->getNbActors(PxActorTypeFlag::eRIGID_DYNAMIC | PxActorTypeFlag::eRIGID_STATIC);
        if (nbActors) {
            std::vector<PxRigidActor*> actors(nbActors);
            pxScene->getActors(PxActorTypeFlag::eRIGID_DYNAMIC | PxActorTypeFlag::eRIGID_STATIC, reinterpret_cast<PxActor**>(&actors[0]), nbActors);
            for (PxRigidActor* actor : actors) {
                PxShape* shape = nullptr;
                actor->getShapes(&shape, 1);
                if (!shape) {
                    continue;
                }

                actor->setActorFlag(PxActorFlag::eVISUALIZATION, true);
                if (debugMode == DebugMode::RAYCAST_SHAPES) {
                    if (shape->getQueryFilterData().word0 == RaycastGroup::RAYCAST_DISABLED) {
                        actor->setActorFlag(PxActorFlag::eVISUALIZATION, false);
                    }
                }
                else if (debugMode == DebugMode::COLLISION_SHAPES) {
                    if (shape->getQueryFilterData().word1 == CollisionGroup::NO_COLLISION) {
                        actor->setActorFlag(PxActorFlag::eVISUALIZATION, false);
                    }
                }
                else if (debugMode == DebugMode::RAGDOLLS) {
                    PxRigidDynamic* pxRigidDynamic = actor->is<PxRigidDynamic>();
                    if (pxRigidDynamic && RagdollOwnsPxRigidDynamic(pxRigidDynamic)) {
                        actor->setActorFlag(PxActorFlag::eVISUALIZATION, true);
                    }
                    else {
                        actor->setActorFlag(PxActorFlag::eVISUALIZATION, false);
                    }
                }
                for (PxRigidActor* ignoredActor : ignoreList) {
                    if (ignoredActor == actor) {
                        actor->setActorFlag(PxActorFlag::eVISUALIZATION, false);
                    }
                }

                //actor->setActorFlag(PxActorFlag::eVISUALIZATION, true);
            }
        }
        const PxRenderBuffer& renderBuffer = pxScene->getRenderBuffer();
        debugLines.reserve(renderBuffer.getNbLines());

        for (unsigned int i = 0; i < renderBuffer.getNbLines(); i++) {
            const PxDebugLine& pxLine = renderBuffer.getLines()[i];

            PhysicsDebugLine debugLine;
            debugLine.p1 = Hell::Physics::PxVec3toGlmVec3(pxLine.pos0);
            debugLine.p2 = Hell::Physics::PxVec3toGlmVec3(pxLine.pos1);
            debugLine.color = GetPhysicsDebugLineColor(debugMode);
            debugLines.push_back(debugLine);
        }

        return debugLines;
    }

    std::string GetObjectCountsAsString() {
        std::string result = "PhysX Object counts\n";
        result += "- D6 Joints: " + std::to_string(GetD6JointCount()) + "\n";
        result += "- Height Fields: " + std::to_string(GetHeightFieldCount()) + "\n";
        result += "- Ragdolls: " + std::to_string(Hell::Physics::GetRagdolls().size()) + "\n";
        result += "- Rigid Dynamics: " + std::to_string(GetRigidDynamicCount()) + "\n";
        result += "- Rigid Statics: " + std::to_string(GetRigidStaticCount()) + "\n";
        return result;
    }

    void PrintSceneRigidInfo() {
        std::cout << " Rigid Dynamics\n\n";

        for (auto it = GetRigidDynamics().begin(); it != GetRigidDynamics().end(); ) {
            uint64_t id = it->first;

            std::cout << " " << id << " - ";
            //std::cout << "[" << GetPxShapeTypeAsString(rigidDynamic.GetPxShape()) << "] ";
            //std::cout << "position: " << rigidDynamic.GetCurrentPosition() << " ";
            std::cout << "\n";

            it++;
        }
    }

    void PrintSceneRagdollInfo() {
        std::cout << "\n Ragdolls\n\n";

        for (auto it = Hell::Physics::GetRagdolls().begin(); it != Hell::Physics::GetRagdolls().end(); ) {
            const uint64_t id = it->first;
            Ragdoll& ragdoll = it->second;

            std::cout << " " << id << " - " << ragdoll.m_pxRigidDynamics.size() << " rigids\n";
            it++;
        }
    }
}
