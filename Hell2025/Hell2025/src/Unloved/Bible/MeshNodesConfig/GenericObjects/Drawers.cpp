#include "Unloved/Bible/Bible.h"

namespace Unloved::Bible {
    using namespace Unloved;

    void ConfigureMeshNodesDrawersSmall(uint64_t id, MeshNodes* meshNodes) {
        std::vector<MeshNodeCreateInfo> meshNodeCreateInfoSet;

        MeshNodeCreateInfo& frame = meshNodeCreateInfoSet.emplace_back();
        frame.meshName = "DrawersSmallFrame";
        frame.materialName = "Drawers_Frame";
        frame.rigidDynamic.createObject = true;
        frame.rigidDynamic.kinematic = true; 
        frame.rigidDynamic.shapeType = PhysicsShapeType::BOX;
        frame.rigidDynamic.filterData.raycastGroup = RAYCAST_DISABLED;
        frame.rigidDynamic.filterData.collisionGroup = CollisionGroup::ENVIROMENT_OBSTACLE;
        frame.rigidDynamic.filterData.collidesWith = (CollisionGroup)(GENERIC_BOUNCEABLE | ITEM_PICK_UP | BULLET_CASING | RAGDOLL_PLAYER | RAGDOLL_ENEMY);
        frame.addtoNavMesh = true;

        MeshNodeCreateInfo& drawer1st = meshNodeCreateInfoSet.emplace_back();
        drawer1st.materialName = "Drawers_Drawers";
        drawer1st.meshName = "DrawersSmall1st";
        drawer1st.openable.isOpenable = true;
        drawer1st.openable.additionalTriggerMeshNames = { "Handle1st" };
        drawer1st.openable.openAxis = Unloved::OpenAxis::TRANSLATE_Z;
        drawer1st.openable.initialOpenState = Unloved::OpenState::CLOSED;
        drawer1st.openable.minOpenValue = 0.0f;
        drawer1st.openable.maxOpenValue = 0.155f + 0.05f;
        drawer1st.openable.openSpeed = 1.5f;
        drawer1st.openable.closeSpeed = 1.5f;
        drawer1st.openable.openingAudio = "DrawerOpen.wav";
        drawer1st.openable.closingAudio = "DrawerClose.wav";
        drawer1st.openable.lockedAudio = "Locked.wav";

        MeshNodeCreateInfo& drawer2nd = meshNodeCreateInfoSet.emplace_back();
        drawer2nd.materialName = "Drawers_Drawers";
        drawer2nd.meshName = "DrawersSmall2nd";
        drawer2nd.openable.isOpenable = true;
        drawer2nd.openable.additionalTriggerMeshNames = { "Handle2st" };
        drawer2nd.openable.openAxis = Unloved::OpenAxis::TRANSLATE_Z;
        drawer2nd.openable.initialOpenState = Unloved::OpenState::CLOSED;
        drawer2nd.openable.minOpenValue = 0.0f;
        drawer2nd.openable.maxOpenValue = 0.175f + 0.05f;
        drawer2nd.openable.openSpeed = 1.5f;
        drawer2nd.openable.closeSpeed = 1.5f;
        drawer2nd.openable.openingAudio = "DrawerOpen.wav";
        drawer2nd.openable.closingAudio = "DrawerClose.wav";
        drawer2nd.openable.lockedAudio = "Locked.wav";

        MeshNodeCreateInfo& drawer3rd = meshNodeCreateInfoSet.emplace_back();
        drawer3rd.materialName = "Drawers_Drawers";
        drawer3rd.meshName = "DrawersSmall3rd";
        drawer3rd.openable.isOpenable = true;
        drawer3rd.openable.additionalTriggerMeshNames = { "Handle3rd" };
        drawer3rd.openable.openAxis = Unloved::OpenAxis::TRANSLATE_Z;
        drawer3rd.openable.initialOpenState = Unloved::OpenState::CLOSED;
        drawer3rd.openable.minOpenValue = 0.0f;
        drawer3rd.openable.maxOpenValue = 0.170f + 0.05f;
        drawer3rd.openable.openSpeed = 1.5f;
        drawer3rd.openable.closeSpeed = 1.5f;
        drawer3rd.openable.openingAudio = "DrawerOpen.wav";
        drawer3rd.openable.closingAudio = "DrawerClose.wav";
        drawer3rd.openable.lockedAudio = "Locked.wav";

        MeshNodeCreateInfo& drawer4th = meshNodeCreateInfoSet.emplace_back();
        drawer4th.materialName = "Drawers_Drawers";
        drawer4th.meshName = "DrawersSmall4th";
        drawer4th.openable.isOpenable = true;
        drawer4th.openable.additionalTriggerMeshNames = { "Handle4th" };
        drawer4th.openable.openAxis = Unloved::OpenAxis::TRANSLATE_Z;
        drawer4th.openable.initialOpenState = Unloved::OpenState::CLOSED;
        drawer4th.openable.minOpenValue = 0.0f;
        drawer4th.openable.maxOpenValue = 0.180f + 0.05f;
        drawer4th.openable.openSpeed = 1.5f;
        drawer4th.openable.closeSpeed = 1.5f;
        drawer4th.openable.openingAudio = "DrawerOpen.wav";
        drawer4th.openable.closingAudio = "DrawerClose.wav";
        drawer4th.openable.lockedAudio = "Locked.wav";

        MeshNodeCreateInfo& handle1st = meshNodeCreateInfoSet.emplace_back();
        handle1st.materialName = "Drawers_Handles";
        handle1st.meshName = "Handle1st";

        MeshNodeCreateInfo& handle2nd = meshNodeCreateInfoSet.emplace_back();
        handle2nd.materialName = "Drawers_Handles";
        handle2nd.meshName = "Handle2nd";

        MeshNodeCreateInfo& handle3rd = meshNodeCreateInfoSet.emplace_back();
        handle3rd.materialName = "Drawers_Handles";
        handle3rd.meshName = "Handle3rd";

        MeshNodeCreateInfo& handle4th = meshNodeCreateInfoSet.emplace_back();
        handle4th.materialName = "Drawers_Handles";
        handle4th.meshName = "Handle4th";

        meshNodes->Init(id, "DrawersSmall", meshNodeCreateInfoSet);
    }

    void ConfigureMeshNodesDrawersLarge(uint64_t id, MeshNodes* meshNodes) {
        std::vector<MeshNodeCreateInfo> meshNodeCreateInfoSet;

        MeshNodeCreateInfo& frame = meshNodeCreateInfoSet.emplace_back();
        frame.meshName = "DrawersLargeFrame";
        frame.materialName = "Drawers_Frame";
        frame.rigidDynamic.shapeType = PhysicsShapeType::BOX;
        frame.rigidDynamic.createObject = false;
        frame.rigidDynamic.kinematic = true;
        frame.rigidDynamic.filterData.raycastGroup = RAYCAST_DISABLED;
        frame.rigidDynamic.filterData.collisionGroup = CollisionGroup::ENVIROMENT_OBSTACLE;
        frame.rigidDynamic.filterData.collidesWith = (CollisionGroup)(GENERIC_BOUNCEABLE | ITEM_PICK_UP | BULLET_CASING | RAGDOLL_PLAYER | RAGDOLL_ENEMY);
        frame.addtoNavMesh = true;

        MeshNodeCreateInfo& drawerTopL = meshNodeCreateInfoSet.emplace_back();
        drawerTopL.materialName = "Drawers_Drawers";
        drawerTopL.meshName = "DrawersLarge_TopL";
        drawerTopL.openable.isOpenable = true;
        drawerTopL.openable.additionalTriggerMeshNames = { "Handle_TopL" };
        drawerTopL.openable.openAxis = Unloved::OpenAxis::TRANSLATE_Z;
        drawerTopL.openable.initialOpenState = Unloved::OpenState::CLOSED;
        drawerTopL.openable.minOpenValue = 0.0f;
        drawerTopL.openable.maxOpenValue = 0.158f + 0.05f;
        drawerTopL.openable.openSpeed = 1.5f;
        drawerTopL.openable.closeSpeed = 1.5f;
        drawerTopL.openable.openingAudio = "DrawerOpen.wav";
        drawerTopL.openable.closingAudio = "DrawerClose.wav";
        drawerTopL.openable.lockedAudio = "Locked.wav";

        MeshNodeCreateInfo& drawerTopR = meshNodeCreateInfoSet.emplace_back();
        drawerTopR.materialName = "Drawers_Drawers";
        drawerTopR.meshName = "DrawersLarge_TopR";
        drawerTopR.openable.isOpenable = true;
        drawerTopR.openable.additionalTriggerMeshNames = { "Handle_TopR" };
        drawerTopR.openable.openAxis = Unloved::OpenAxis::TRANSLATE_Z;
        drawerTopR.openable.initialOpenState = Unloved::OpenState::CLOSED;
        drawerTopR.openable.minOpenValue = 0.0f;
        drawerTopR.openable.maxOpenValue = 0.155f + 0.05f;
        drawerTopR.openable.openSpeed = 1.5f;
        drawerTopR.openable.closeSpeed = 1.5f;
        drawerTopR.openable.openingAudio = "DrawerOpen.wav";
        drawerTopR.openable.closingAudio = "DrawerClose.wav";
        drawerTopR.openable.lockedAudio = "Locked.wav";

        MeshNodeCreateInfo& drawer2nd = meshNodeCreateInfoSet.emplace_back();
        drawer2nd.materialName = "Drawers_Drawers";
        drawer2nd.meshName = "DrawersLarge_2nd";
        drawer2nd.openable.isOpenable = true;
        drawer2nd.openable.additionalTriggerMeshNames = { "Handle_2nd" };
        drawer2nd.openable.openAxis = Unloved::OpenAxis::TRANSLATE_Z;
        drawer2nd.openable.initialOpenState = Unloved::OpenState::CLOSED;
        drawer2nd.openable.minOpenValue = 0.0f;
        drawer2nd.openable.maxOpenValue = 0.175f + 0.05f;
        drawer2nd.openable.openSpeed = 1.5f;
        drawer2nd.openable.closeSpeed = 1.5f;
        drawer2nd.openable.openingAudio = "DrawerOpen.wav";
        drawer2nd.openable.closingAudio = "DrawerClose.wav";
        drawer2nd.openable.lockedAudio = "Locked.wav";
        drawer2nd.rigidDynamic.createObject = true;
        drawer2nd.rigidDynamic.kinematic = true;
        drawer2nd.rigidDynamic.filterData.raycastGroup = RAYCAST_DISABLED;
        drawer2nd.rigidDynamic.filterData.collisionGroup = CollisionGroup::ENVIROMENT_OBSTACLE;
        drawer2nd.rigidDynamic.filterData.collidesWith = (CollisionGroup)(GENERIC_BOUNCEABLE | ITEM_PICK_UP | BULLET_CASING | RAGDOLL_PLAYER | RAGDOLL_ENEMY);
        drawer2nd.rigidDynamic.shapeType = PhysicsShapeType::CONVEX_MESH;
        drawer2nd.rigidDynamic.convexMeshModelName = "CollisionMesh_DrawersLarge_2nd";

        MeshNodeCreateInfo& drawer3rd = meshNodeCreateInfoSet.emplace_back();
        drawer3rd.materialName = "Drawers_Drawers";
        drawer3rd.meshName = "DrawersLarge_3rd";
        drawer3rd.openable.isOpenable = true;
        drawer3rd.openable.additionalTriggerMeshNames = { "Handle_3rd" };
        drawer3rd.openable.openAxis = Unloved::OpenAxis::TRANSLATE_Z;
        drawer3rd.openable.initialOpenState = Unloved::OpenState::CLOSED;
        drawer3rd.openable.minOpenValue = 0.0f;
        drawer3rd.openable.maxOpenValue = 0.170f + 0.05f;
        drawer3rd.openable.openSpeed = 1.5f;
        drawer3rd.openable.closeSpeed = 1.5f;
        drawer3rd.openable.openingAudio = "DrawerOpen.wav";
        drawer3rd.openable.closingAudio = "DrawerClose.wav";
        drawer3rd.openable.lockedAudio = "Locked.wav";

        MeshNodeCreateInfo& drawer4th = meshNodeCreateInfoSet.emplace_back();
        drawer4th.materialName = "Drawers_Drawers";
        drawer4th.meshName = "DrawersLarge_4th";
        drawer4th.openable.isOpenable = true;
        drawer4th.openable.additionalTriggerMeshNames = { "Handle_4th" };
        drawer4th.openable.openAxis = Unloved::OpenAxis::TRANSLATE_Z;
        drawer4th.openable.initialOpenState = Unloved::OpenState::CLOSED;
        drawer4th.openable.minOpenValue = 0.0f;
        drawer4th.openable.maxOpenValue = 0.180f + 0.05f;
        drawer4th.openable.openSpeed = 1.5f;
        drawer4th.openable.closeSpeed = 1.5f;
        drawer4th.openable.openingAudio = "DrawerOpen.wav";
        drawer4th.openable.closingAudio = "DrawerClose.wav";
        drawer4th.openable.lockedAudio = "Locked.wav";

        MeshNodeCreateInfo& handleTopL = meshNodeCreateInfoSet.emplace_back();
        handleTopL.materialName = "Drawers_Handles";
        handleTopL.meshName = "Handle_TopL";

        MeshNodeCreateInfo& handleTopR = meshNodeCreateInfoSet.emplace_back();
        handleTopR.materialName = "Drawers_Handles";
        handleTopR.meshName = "Handle_TopR";

        MeshNodeCreateInfo& handle2nd = meshNodeCreateInfoSet.emplace_back();
        handle2nd.materialName = "Drawers_Handles";
        handle2nd.meshName = "Handle_2nd";

        MeshNodeCreateInfo& handle3rd = meshNodeCreateInfoSet.emplace_back();
        handle3rd.materialName = "Drawers_Handles";
        handle3rd.meshName = "Handle_3rd";

        MeshNodeCreateInfo& handle4th = meshNodeCreateInfoSet.emplace_back();
        handle4th.materialName = "Drawers_Handles";
        handle4th.meshName = "Handle_4th";

        meshNodes->Init(id, "DrawersLarge", meshNodeCreateInfoSet);
    }
}
