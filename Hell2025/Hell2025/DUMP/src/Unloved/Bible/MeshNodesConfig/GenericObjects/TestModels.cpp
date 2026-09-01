#include "Unloved/Bible/Bible.h"
namespace Bible {
    using namespace Unloved;

    void ConfigureTestModel(uint64_t id, MeshNodes* meshNodes) {
        std::vector<MeshNodeCreateInfo> meshNodeCreateInfoSet;

        meshNodes->Init(id, "StainGlassTest", meshNodeCreateInfoSet);

        meshNodes->SetMaterialByMeshName("Glass", "BathroomCabinetMirror");
        meshNodes->SetBlendingModeByMeshName("Glass", BlendingMode::GLASS);
    }

    void ConfigureMeshNodesDeerHead(uint64_t id, MeshNodes* meshNodes) {
        std::vector<MeshNodeCreateInfo> meshNodeCreateInfoSet;

        meshNodes->Init(id, "DeerTest", meshNodeCreateInfoSet);
        meshNodes->SetMeshMaterials("Deer");
        meshNodes->SetMaterialByMeshName("DeerIris", "DeerIris");
        meshNodes->SetMaterialByMeshName("DeerSclera", "Scelra");
        meshNodes->SetBlendingModeByMeshName("DeerSclera", BlendingMode::GLASS);
    }

    void ConfigureTestModel2(uint64_t id, MeshNodes* meshNodes) {
        ConfigureMeshNodesDeerHead(id, meshNodes);
    }

    void ConfigureTestModel3(uint64_t id, MeshNodes* meshNodes) {
        std::vector<MeshNodeCreateInfo> meshNodeCreateInfoSet;

        MeshNodeCreateInfo& door = meshNodeCreateInfoSet.emplace_back();
        door.meshName = "DoorOld_Sides";
        door.rigidDynamic.createObject = true;
        door.rigidDynamic.kinematic = true;
        door.rigidDynamic.filterData.raycastGroup = RAYCAST_DISABLED;
        door.rigidDynamic.filterData.collisionGroup = CollisionGroup::ENVIROMENT_OBSTACLE;
        door.rigidDynamic.filterData.collidesWith = (CollisionGroup)(GENERIC_BOUNCEABLE | BULLET_CASING | RAGDOLL_PLAYER | RAGDOLL_ENEMY);

        MeshNodeCreateInfo& hinges = meshNodeCreateInfoSet.emplace_back();
        hinges.meshName = "Door_Hinges";
        hinges.materialName = "T_Door_RE";
        hinges.openable.additionalTriggerMeshNames = {
            "Door_Front",
            "Door_Back",
            "Door_Sides",

            "DoorGlass_Front",
            "DoorGlass_Back",
            "DoorGlass_Sides",

            "DoorOld_Front",
            "DoorOld_Back",
            "DoorOld_Sides",

            "Door_Knob",
            "Door_Deadlock",

            "DoorGlassArtFrame",
            "DoorGlassArt_Clear",
            "DoorGlassArt_Green",
            "DoorGlassArt_Orange",
            "DoorGlassArt_Purple",
            "DoorGlassArt_Red",
            "DoorGlassArt_Yellow",

            "DoorGlass_Front",
            "DoorGlass_Back",
            "DoorGlass_Sides",
            "DoorGlass_Hinge_Sides",
        };
        hinges.openable.isOpenable = true;
        hinges.openable.openAxis = Unloved::OpenAxis::ROTATE_Y_NEG;
        hinges.openable.initialOpenState = Unloved::OpenState::CLOSED;
        hinges.openable.minOpenValue = 0.0f;
        hinges.openable.maxOpenValue = 2.1f;
        hinges.openable.openSpeed = 5.208f;
        hinges.openable.closeSpeed = 5.208f;
        hinges.openable.openingAudio = "Door_Open.wav";
        hinges.openable.closingAudio = "Door_Open.wav";

        meshNodes->Init(id, "DoorV6", meshNodeCreateInfoSet);

        meshNodes->SetMaterialByMeshName("Door_Back", "Door_RE");
        meshNodes->SetMaterialByMeshName("Door_Front", "Door_RE");
        meshNodes->SetMaterialByMeshName("Door_Sides", "Door_RE");

        meshNodes->SetMaterialByMeshName("DoorGlass_Front", "Door_RE");
        meshNodes->SetMaterialByMeshName("DoorGlass_Back", "Door_RE");
        meshNodes->SetMaterialByMeshName("DoorGlass_Sides", "Door_RE");

        meshNodes->SetMaterialByMeshName("DoorOld_Back", "Door_RE");
        meshNodes->SetMaterialByMeshName("DoorOld_Front", "Door_RE");
        meshNodes->SetMaterialByMeshName("DoorOld_Sides", "Door_RE");

        meshNodes->SetMaterialByMeshName("DoorGlassArtFrame", "DoorGlassFrame");
        meshNodes->SetMaterialByMeshName("DoorGlassArt_Clear", "DoorGlass");
        meshNodes->SetMaterialByMeshName("DoorGlassArt_Green", "DoorGlass");
        meshNodes->SetMaterialByMeshName("DoorGlassArt_Orange", "DoorGlass");
        meshNodes->SetMaterialByMeshName("DoorGlassArt_Purple", "DoorGlass");
        meshNodes->SetMaterialByMeshName("DoorGlassArt_Red", "DoorGlass");
        meshNodes->SetMaterialByMeshName("DoorGlassArt_Yellow", "DoorGlass");

        meshNodes->SetMaterialByMeshName("Door_Knob", "DoorOldKnob");
        meshNodes->SetMaterialByMeshName("Door_Hinges", "DoorMetals");
        meshNodes->SetMaterialByMeshName("Door_Deadlock", "DoorMetals");
        meshNodes->SetMaterialByMeshName("Door_DeadLockSwitch", "DoorMetals");

        meshNodes->SetMaterialByMeshName("DoorFrame_Front", "DoorFrame_RE");
        meshNodes->SetMaterialByMeshName("DoorFrame_Back", "DoorFrame_RE");
        meshNodes->SetMaterialByMeshName("DoorFrame_Inner", "DoorFrame_RE");

        meshNodes->SetMaterialByMeshName("DoorFrame_Deadlock", "DoorMetals");
        meshNodes->SetMaterialByMeshName("DoorFrame_Hinges", "DoorMetals");
        meshNodes->SetMaterialByMeshName("DoorFrame_KnobLatch", "DoorOldKnob");
    }

    void ConfigureTestModel4(uint64_t id, MeshNodes* meshNodes) {

        std::vector<MeshNodeCreateInfo> meshNodeCreateInfoSet;

        MeshNodeCreateInfo& door = meshNodeCreateInfoSet.emplace_back();
        door.meshName = "DoorGlass_Sides";
        door.rigidDynamic.createObject = true;
        door.rigidDynamic.kinematic = true;
        door.rigidDynamic.filterData.raycastGroup = RAYCAST_DISABLED;
        door.rigidDynamic.filterData.collisionGroup = CollisionGroup::ENVIROMENT_OBSTACLE;
        door.rigidDynamic.filterData.collidesWith = (CollisionGroup)(GENERIC_BOUNCEABLE | BULLET_CASING | RAGDOLL_PLAYER | RAGDOLL_ENEMY);

        MeshNodeCreateInfo& hinges = meshNodeCreateInfoSet.emplace_back();
        hinges.meshName = "Door_Hinges";
        hinges.materialName = "Door_RE";
        hinges.openable.additionalTriggerMeshNames = {
            "Door_Front",
            "Door_Back",
            "Door_Sides",

            "DoorGlass_Front",
            "DoorGlass_Back",
            "DoorGlass_Sides",

            "DoorOld_Front",
            "DoorOld_Back",
            "DoorOld_Sides",

            "Door_Knob",
            "Door_Deadlock",

            "DoorGlassArtFrame",
            "DoorGlassArt_Clear",
            "DoorGlassArt_Green",
            "DoorGlassArt_Orange",
            "DoorGlassArt_Purple",
            "DoorGlassArt_Red",
            "DoorGlassArt_Yellow",

            "DoorGlass_Front",
            "DoorGlass_Back",
            "DoorGlass_Sides",
            "DoorGlass_Hinge_Sides",
        };
        hinges.openable.isOpenable = true;
        hinges.openable.openAxis = Unloved::OpenAxis::ROTATE_Y_NEG;
        hinges.openable.initialOpenState = Unloved::OpenState::CLOSED;
        hinges.openable.minOpenValue = 0.0f;
        hinges.openable.maxOpenValue = 2.1f;
        hinges.openable.openSpeed = 5.208f;
        hinges.openable.closeSpeed = 5.208f;
        hinges.openable.openingAudio = "Door_Open.wav";
        hinges.openable.closingAudio = "Door_Open.wav";

        meshNodes->Init(id, "DoorV6", meshNodeCreateInfoSet);

        meshNodes->SetMaterialByMeshName("Door_Back", "Door_WP");
        meshNodes->SetMaterialByMeshName("Door_Front", "Door_WP");
        meshNodes->SetMaterialByMeshName("Door_Sides", "Door_WP");

        meshNodes->SetMaterialByMeshName("DoorGlass_Front", "Door_WP");
        meshNodes->SetMaterialByMeshName("DoorGlass_Back", "Door_WP");
        meshNodes->SetMaterialByMeshName("DoorGlass_Sides", "Door_WP");

        meshNodes->SetMaterialByMeshName("DoorOld_Back", "Door_WP");
        meshNodes->SetMaterialByMeshName("DoorOld_Front", "Door_WP");
        meshNodes->SetMaterialByMeshName("DoorOld_Sides", "Door_WP");

        meshNodes->SetMaterialByMeshName("DoorGlassArtFrame", "DoorGlassFrame");
        meshNodes->SetMaterialByMeshName("DoorGlassArt_Clear", "DoorGlass");
        meshNodes->SetMaterialByMeshName("DoorGlassArt_Green", "DoorGlass");
        meshNodes->SetMaterialByMeshName("DoorGlassArt_Orange", "DoorGlass");
        meshNodes->SetMaterialByMeshName("DoorGlassArt_Purple", "DoorGlass");
        meshNodes->SetMaterialByMeshName("DoorGlassArt_Red", "DoorGlass");
        meshNodes->SetMaterialByMeshName("DoorGlassArt_Yellow", "DoorGlass");

        meshNodes->SetMaterialByMeshName("Door_Knob", "DoorOldKnob");
        meshNodes->SetMaterialByMeshName("Door_Hinges", "DoorMetals");
        meshNodes->SetMaterialByMeshName("Door_Deadlock", "DoorMetals");
        meshNodes->SetMaterialByMeshName("Door_DeadLockSwitch", "DoorMetals");

        meshNodes->SetMaterialByMeshName("DoorFrame_Front", "DoorFrame_WP");
        meshNodes->SetMaterialByMeshName("DoorFrame_Back", "DoorFrame_WP");
        meshNodes->SetMaterialByMeshName("DoorFrame_Inner", "DoorFrame_WP");

        meshNodes->SetMaterialByMeshName("DoorFrame_Deadlock", "DoorMetals");
        meshNodes->SetMaterialByMeshName("DoorFrame_Hinges", "DoorMetals");
        meshNodes->SetMaterialByMeshName("DoorFrame_KnobLatch", "DoorOldKnob");
    }
}
