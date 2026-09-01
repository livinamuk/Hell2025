#include "Unloved/Bible/Bible.h"
#include "Hell/Logging.h"

namespace Unloved::Bible {
    using namespace Unloved;

    void ConfigureMeshNodesPlantBlackBerries(uint64_t id, MeshNodes* meshNodes) {
        std::vector<MeshNodeCreateInfo> meshNodeCreateInfoSet;

        return;

        glm::vec3 scale = glm::vec3(1.25f);

        MeshNodeCreateInfo& leaves = meshNodeCreateInfoSet.emplace_back();
        leaves.meshName = "Leaves";
        leaves.materialName = "Leaves_BlackBerry";
        leaves.blendingMode = BlendingMode::ALPHA_DISCARD;
        leaves.scale = scale;

        MeshNodeCreateInfo& trunk0 = meshNodeCreateInfoSet.emplace_back();
        trunk0.meshName = "Trunk";
        trunk0.materialName = "TreeLarge_0";
        trunk0.scale = scale;

        MeshNodeCreateInfo& trunk1 = meshNodeCreateInfoSet.emplace_back();
        trunk1.meshName = "Trunk2";
        trunk1.materialName = "TreeLarge_0";
        trunk1.scale = scale;

        meshNodes->Init(id, "BlackBerries", meshNodeCreateInfoSet);
        //meshNodes->EnableCSMShadows();
        meshNodes->DisablePointLightShadows();
        meshNodes->DisableCSMShadows();
    }

    void ConfigureMeshNodesPlantTree(uint64_t id, MeshNodes* meshNodes) {
        if (!meshNodes) return;

        std::vector<MeshNodeCreateInfo> meshNodeCreateInfoSet;

        MeshNodeCreateInfo& tree = meshNodeCreateInfoSet.emplace_back();
        tree.meshName = "Tree";
        tree.shadowModelName = "TreeLarge_0_ShadowCaster";
        tree.shadowMeshName = "ShadowCaster";
        tree.materialName = "Tree";
        tree.rigidDynamic.createObject = true;
        tree.rigidDynamic.kinematic = true;
        tree.rigidDynamic.shapeType = PhysicsShapeType::CONVEX_MESH;
        tree.rigidDynamic.convexMeshModelName = "CollisionMesh_TreeLarge_0";
        tree.rigidDynamic.filterData.raycastGroup = RAYCAST_DISABLED;
        tree.rigidDynamic.filterData.collisionGroup = CollisionGroup::ENVIROMENT_OBSTACLE;
        tree.rigidDynamic.filterData.collidesWith = (CollisionGroup)(GENERIC_BOUNCEABLE | ITEM_PICK_UP | BULLET_CASING | RAGDOLL_PLAYER | RAGDOLL_ENEMY);

        meshNodes->Init(id, "TreeLarge_0", meshNodeCreateInfoSet);

        meshNodes->SetMaterialByMeshName("Tree", "TreeLarge_0");
        meshNodes->EnableCSMShadows();
        meshNodes->EnablePointLightShadows();
    }

    void ConfigureMeshNodesMermaidRock(uint64_t id, MeshNodes* meshNodes) {
        std::vector<MeshNodeCreateInfo> meshNodeCreateInfoSet;

        MeshNodeCreateInfo& tree = meshNodeCreateInfoSet.emplace_back();
        tree.meshName = "Rock2";
        tree.materialName = "Rock";
        tree.rigidDynamic.createObject = true;
        tree.rigidDynamic.kinematic = true;
        tree.rigidDynamic.shapeType = PhysicsShapeType::CONVEX_MESH;
        tree.rigidDynamic.convexMeshModelName = "CollisionMesh_Rock2";
        tree.rigidDynamic.filterData.raycastGroup = RAYCAST_DISABLED;
        tree.rigidDynamic.filterData.collisionGroup = CollisionGroup::ENVIROMENT_OBSTACLE;
        tree.rigidDynamic.filterData.collidesWith = (CollisionGroup)(GENERIC_BOUNCEABLE | ITEM_PICK_UP | BULLET_CASING | RAGDOLL_PLAYER | RAGDOLL_ENEMY);

        meshNodes->Init(id, "Rock2", meshNodeCreateInfoSet);
        meshNodes->EnableCSMShadows();
    }
}
