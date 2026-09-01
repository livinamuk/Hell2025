#include "Unloved/Bible/Bible.h"
namespace Bible {
    using namespace Unloved;

    void ConfigureMeshNodesCouch(uint64_t id, MeshNodes* meshNodes) {
        std::vector<MeshNodeCreateInfo> meshNodeCreateInfoSet;

        MeshNodeCreateInfo& body = meshNodeCreateInfoSet.emplace_back();
        body.meshName = "CouchBody";
        body.materialName = "Couch"; 
        body.rigidDynamic.createObject = true;
        body.rigidDynamic.kinematic = true;
        body.rigidDynamic.filterData.raycastGroup = RAYCAST_DISABLED;
        body.rigidDynamic.filterData.collisionGroup = CollisionGroup::ENVIROMENT_OBSTACLE;
        body.rigidDynamic.filterData.collidesWith = (CollisionGroup)(GENERIC_BOUNCEABLE | BULLET_CASING | RAGDOLL_PLAYER | ITEM_PICK_UP | RAGDOLL_ENEMY);
        body.addtoNavMesh = true;

        PhysicsFilterData filterData;
        filterData.raycastGroup = RaycastGroup::RAYCAST_ENABLED;
        filterData.collisionGroup = CollisionGroup::GENERIC_BOUNCEABLE;
        filterData.collidesWith = CollisionGroup(ENVIROMENT_OBSTACLE | GENERIC_BOUNCEABLE);

        MeshNodeCreateInfo& cushion0 = meshNodeCreateInfoSet.emplace_back();
        cushion0.meshName = "Cushion0";
        cushion0.materialName = "Couch";
        //cushion0.type = MeshNodeType::RIGID_DYNAMIC;
        //cushion0.rigidDynamic.filterData = filterData;
        
        MeshNodeCreateInfo& cushion1 = meshNodeCreateInfoSet.emplace_back();
        cushion1.meshName = "Cushion1";
        cushion1.materialName = "Couch";

        MeshNodeCreateInfo& cushion2 = meshNodeCreateInfoSet.emplace_back();
        cushion2.meshName = "Cushion2";
        cushion2.materialName = "Couch";

        MeshNodeCreateInfo& cushion3 = meshNodeCreateInfoSet.emplace_back();
        cushion3.meshName = "Cushion3";
        cushion3.materialName = "Couch";

        MeshNodeCreateInfo& cushion4 = meshNodeCreateInfoSet.emplace_back();
        cushion4.meshName = "Cushion4";
        cushion4.materialName = "Couch";

        meshNodes->Init(id, "Couch", meshNodeCreateInfoSet);
    }


    void ConfigureMeshNodesChairRE(uint64_t id, MeshNodes* meshNodes) {
        std::vector<MeshNodeCreateInfo> meshNodeCreateInfoSet;

        MeshNodeCreateInfo& cushion4 = meshNodeCreateInfoSet.emplace_back();
        cushion4.meshName = "ChairRE";
        cushion4.materialName = "ChairRE";

        meshNodes->Init(id, "ChairRE", meshNodeCreateInfoSet);
    }
    
    void ConfigureMeshNodesChairSpindleBack(uint64_t id, MeshNodes* meshNodes) {
        std::vector<MeshNodeCreateInfo> meshNodeCreateInfoSet;

        MeshNodeCreateInfo& cushion4 = meshNodeCreateInfoSet.emplace_back();
        cushion4.meshName = "ChairSpindleBack";
        cushion4.materialName = "Chair";

        meshNodes->Init(id, "ChairSpindleBack", meshNodeCreateInfoSet);
    }
}