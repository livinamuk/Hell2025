#include "Unloved/Bible/Bible.h"
#include "Hell/Common/Constants.h"
#include "Hell/Common/Random.h"
#include "Hell/Logging.h"

namespace Unloved::Bible {
    using namespace Unloved;

  void ConfigureMeshNodesChristmasPresentSmall(uint64_t id, MeshNodes* meshNodes) {
        std::vector<MeshNodeCreateInfo> meshNodeCreateInfoSet;

        std::string materialName = UNDEFINED_STRING;

        switch (Hell::Random::Int(0, 3)) {
            case 0: materialName = "PresentSmallRed";    break;
            case 1: materialName = "PresentSmallGreen";  break;
            case 2: materialName = "PresentSmallYellow"; break;
            case 3: materialName = "PresentSmallBlue";   break;
        }

        PhysicsFilterData filterData;
        filterData.raycastGroup = RaycastGroup::RAYCAST_DISABLED;
        filterData.collisionGroup = CollisionGroup::ITEM_PICK_UP;
        filterData.collidesWith = CollisionGroup::ENVIROMENT_OBSTACLE;
        filterData.collidesWith = (CollisionGroup)(ENVIROMENT_OBSTACLE | ITEM_PICK_UP);

        MeshNodeCreateInfo& present = meshNodeCreateInfoSet.emplace_back();
        present.meshName = "PresentSmall";
        present.materialName = materialName;
        present.rigidDynamic.createObject = true;
        present.rigidDynamic.kinematic = false;
        present.rigidDynamic.offsetTransform = Transform();
        present.rigidDynamic.filterData = filterData;
        present.rigidDynamic.mass = 1.0;
        present.rigidDynamic.shapeType = PhysicsShapeType::BOX;

        meshNodes->Init(id, "ChristmasPresentSmall", meshNodeCreateInfoSet);
        //meshNodes->PrintMeshNames();
    }

  void ConfigureMeshNodesChristmasPresentLarge(uint64_t id, MeshNodes* meshNodes) {
      std::vector<MeshNodeCreateInfo> meshNodeCreateInfoSet;

      std::string materialName = UNDEFINED_STRING;

      switch (Hell::Random::Int(0, 3)) {
        case 0: materialName = "PresentBigRed";    break;
        case 1: materialName = "PresentBigGreen";  break;
        case 2: materialName = "PresentBigYellow"; break;
        case 3: materialName = "PresentBigBlue";   break;
      }

      PhysicsFilterData filterData;
      filterData.raycastGroup = RaycastGroup::RAYCAST_DISABLED;
      filterData.collisionGroup = CollisionGroup::ITEM_PICK_UP;
      filterData.collidesWith = CollisionGroup::ENVIROMENT_OBSTACLE;
      filterData.collidesWith = (CollisionGroup)(ENVIROMENT_OBSTACLE | ITEM_PICK_UP);

      MeshNodeCreateInfo& present = meshNodeCreateInfoSet.emplace_back();
      present.meshName = "PresentBig";
      present.materialName = materialName;
      present.rigidDynamic.createObject = true;
      present.rigidDynamic.kinematic = false;
      present.rigidDynamic.offsetTransform = Transform();
      present.rigidDynamic.filterData = filterData;
      present.rigidDynamic.mass = 1.0;
      present.rigidDynamic.shapeType = PhysicsShapeType::BOX;

      meshNodes->Init(id, "ChristmasPresentBig", meshNodeCreateInfoSet);
    }

    void ConfigureMeshNodesChristmasTree(uint64_t id, MeshNodes* meshNodes) {
        std::vector<MeshNodeCreateInfo> meshNodeCreateInfoSet;

        MeshNodeCreateInfo& tree = meshNodeCreateInfoSet.emplace_back();
        tree.meshName = "tree";
        tree.materialName = "ChristmasTree";
        tree.blendingMode = BlendingMode::ALPHA_DISCARD;
        tree.scale = glm::vec3(1.4f);

        meshNodes->Init(id, "ChristmasTree", meshNodeCreateInfoSet);
    }
}
