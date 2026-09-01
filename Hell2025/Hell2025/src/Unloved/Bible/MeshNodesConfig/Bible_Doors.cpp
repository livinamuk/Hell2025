#include "Unloved/Bible/Bible.h"
namespace Unloved::Bible {
    using namespace Unloved;


    void ConfigureDoorMeshNodes(uint64_t id, DoorCreateInfo& createInfo, MeshNodes* meshNodes) {
        std::vector<MeshNodeCreateInfo> meshNodeCreateInfoSet;

        // Deadlock stuff
        if (createInfo.type != DoorType::STAINED_GLASS2) {
            MeshNodeCreateInfo& deadLock = meshNodeCreateInfoSet.emplace_back();
            deadLock.meshName = "Door_Deadlock";
            deadLock.materialName = "DoorMetals";
            if (!createInfo.hasDeadLock) deadLock.blendingMode = BlendingMode::DO_NOT_RENDER;

            MeshNodeCreateInfo& deadLockSwitch = meshNodeCreateInfoSet.emplace_back();
            deadLockSwitch.meshName = "Door_DeadLockSwitch";
            deadLockSwitch.materialName = "DoorMetals";
            if (!createInfo.hasDeadLock) deadLockSwitch.blendingMode = BlendingMode::DO_NOT_RENDER;

            MeshNodeCreateInfo& frameDeadLock = meshNodeCreateInfoSet.emplace_back();
            frameDeadLock.meshName = "DoorFrame_Deadlock";
            frameDeadLock.materialName = "DoorMetals";
            if (!createInfo.hasDeadLock) frameDeadLock.blendingMode = BlendingMode::DO_NOT_RENDER;
        }


        if (createInfo.type == DoorType::STANDARD_A) {

            // Collision mesh node
            MeshNodeCreateInfo& door = meshNodeCreateInfoSet.emplace_back();
            door.meshName = "DoorOld_Sides";
            door.rigidDynamic.createObject = true;
            door.rigidDynamic.kinematic = true;
            door.rigidDynamic.filterData.raycastGroup = RAYCAST_DISABLED;
            door.rigidDynamic.filterData.collisionGroup = CollisionGroup::ENVIROMENT_OBSTACLE;
            door.rigidDynamic.filterData.collidesWith = (CollisionGroup)(GENERIC_BOUNCEABLE | ITEM_PICK_UP | BULLET_CASING | RAGDOLL_PLAYER | RAGDOLL_ENEMY);
            door.addtoNavMesh = true;

            // Openable mesh node
            MeshNodeCreateInfo& hinges = meshNodeCreateInfoSet.emplace_back();
            hinges.meshName = "Door_Hinges";
            hinges.materialName = "Door_RE";
            hinges.openable.additionalTriggerMeshNames = {
                "DoorOld_Front",
                "DoorOld_Back",
                "DoorOld_Sides",
                "Door_Knob",
                "Door_Deadlock",
                "Door_DeadLockSwitch"
            };
            hinges.openable.isOpenable = true;
            hinges.openable.openAxis = Unloved::OpenAxis::ROTATE_Y_NEG;
            hinges.openable.initialOpenState = createInfo.openAtStart ? Unloved::OpenState::OPEN : Unloved::OpenState::CLOSED;
            hinges.openable.minOpenValue = 0.0f;
            hinges.openable.maxOpenValue = createInfo.maxOpenValue;
            hinges.openable.openSpeed = 5.208f;
            hinges.openable.closeSpeed = 5.208f;
            hinges.openable.openingAudio = "Door_Open.wav";
            hinges.openable.closingAudio = "Door_Open.wav";
            hinges.openable.isDeadLock = true;

            meshNodes->Init(id, "Door_StandardA", meshNodeCreateInfoSet);

            // Front
            if (createInfo.materialTypeFront == DoorMaterialType::WHITE_PAINT) {
                meshNodes->SetMaterialByMeshName("DoorOld_Front", "Door_WP");
            }
            else if (createInfo.materialTypeFront == DoorMaterialType::RESIDENT_EVIL) {
                meshNodes->SetMaterialByMeshName("DoorOld_Front", "Door_RE");
            }

            // Back
            if (createInfo.materialTypeBack == DoorMaterialType::WHITE_PAINT) {
                meshNodes->SetMaterialByMeshName("DoorOld_Back", "Door_WP");
                meshNodes->SetMaterialByMeshName("DoorOld_Sides", "Door_WP");
            }
            else if (createInfo.materialTypeBack == DoorMaterialType::RESIDENT_EVIL) {
                meshNodes->SetMaterialByMeshName("DoorOld_Back", "Door_RE");
                meshNodes->SetMaterialByMeshName("DoorOld_Sides", "Door_RE");
            }
        }

        if (createInfo.type == DoorType::STANDARD_B) {

            // Collision mesh node
            MeshNodeCreateInfo& door = meshNodeCreateInfoSet.emplace_back();
            door.meshName = "Door_Sides";
            door.rigidDynamic.createObject = true;
            door.rigidDynamic.kinematic = true;
            door.rigidDynamic.filterData.raycastGroup = RAYCAST_DISABLED;
            door.rigidDynamic.filterData.collisionGroup = CollisionGroup::ENVIROMENT_OBSTACLE;
            door.rigidDynamic.filterData.collidesWith = (CollisionGroup)(GENERIC_BOUNCEABLE | ITEM_PICK_UP | BULLET_CASING | RAGDOLL_PLAYER | RAGDOLL_ENEMY);
            door.addtoNavMesh = true;

            // Openable mesh node
            MeshNodeCreateInfo& hinges = meshNodeCreateInfoSet.emplace_back();
            hinges.meshName = "Door_Hinges";
            hinges.openable.additionalTriggerMeshNames = {
                "Door_Front",
                "Door_Back",
                "Door_Sides",
                "Door_Knob",
                "Door_Deadlock",
            };
            hinges.openable.isOpenable = true;
            hinges.openable.openAxis = Unloved::OpenAxis::ROTATE_Y_NEG;
            hinges.openable.initialOpenState = createInfo.openAtStart ? Unloved::OpenState::OPEN : Unloved::OpenState::CLOSED;
            hinges.openable.minOpenValue = 0.0f;
            hinges.openable.maxOpenValue = createInfo.maxOpenValue;
            hinges.openable.openSpeed = 5.208f;
            hinges.openable.closeSpeed = 5.208f;
            hinges.openable.openingAudio = "Door_Open.wav";
            hinges.openable.closingAudio = "Door_Open.wav";
            hinges.openable.isDeadLock = true;

            meshNodes->Init(id, "Door_StandardB", meshNodeCreateInfoSet);

            // Front
            if (createInfo.materialTypeFront == DoorMaterialType::WHITE_PAINT) {
                meshNodes->SetMaterialByMeshName("Door_Front", "Door_WP");
            }
            else if (createInfo.materialTypeFront == DoorMaterialType::RESIDENT_EVIL) {
                meshNodes->SetMaterialByMeshName("Door_Front", "Door_RE");
                meshNodes->SetMaterialByMeshName("Door_Sides", "Door_WP");
            }

            // Back
            if (createInfo.materialTypeBack == DoorMaterialType::WHITE_PAINT) {
                meshNodes->SetMaterialByMeshName("Door_Back", "Door_WP");
            }
            else if (createInfo.materialTypeBack == DoorMaterialType::RESIDENT_EVIL) {
                meshNodes->SetMaterialByMeshName("Door_Back", "Door_RE");
                meshNodes->SetMaterialByMeshName("Door_Sides", "Door_RE");
            }
        }

        if (createInfo.type == DoorType::STAINED_GLASS) {

            // Collision mesh node
            MeshNodeCreateInfo& door = meshNodeCreateInfoSet.emplace_back();
            door.meshName = "DoorGlass_Sides";
            door.rigidDynamic.createObject = true;
            door.rigidDynamic.kinematic = true;
            door.rigidDynamic.filterData.raycastGroup = RAYCAST_DISABLED;
            door.rigidDynamic.filterData.collisionGroup = CollisionGroup::ENVIROMENT_OBSTACLE;
            door.rigidDynamic.filterData.collidesWith = (CollisionGroup)(GENERIC_BOUNCEABLE | BULLET_CASING | ITEM_PICK_UP | RAGDOLL_PLAYER | RAGDOLL_ENEMY);
            door.addtoNavMesh = true;

            // Openable mesh node
            MeshNodeCreateInfo& hinges = meshNodeCreateInfoSet.emplace_back();
            hinges.meshName = "Door_Hinges";
            hinges.materialName = "Door_RE";
            hinges.openable.additionalTriggerMeshNames = {
                "DoorGlass_Front",
                "DoorGlass_Back",
                "DoorGlass_Sides",
                "Door_Knob",
                "Door_Deadlock",
                "DoorGlassArtFrame",
                "DoorGlass_Front",
                "DoorGlass_Back",
                "DoorGlass_Sides",
                "DoorGlass_Hinge_Sides",
                "Door_DeadLockSwitch",
                "DoorGlassArt_Clear",
                "DoorGlassArt_Green",
                "DoorGlassArt_Orange",
                "DoorGlassArt_Purple",
                "DoorGlassArt_Red",
                "DoorGlassArt_Yellow",
                "DoorGlassArt_CenterCircle"
            };
            hinges.openable.isOpenable = true;
            hinges.openable.openAxis = Unloved::OpenAxis::ROTATE_Y_NEG;
            hinges.openable.initialOpenState = createInfo.openAtStart ? Unloved::OpenState::OPEN : Unloved::OpenState::CLOSED;
            hinges.openable.minOpenValue = 0.0f;
            hinges.openable.maxOpenValue = createInfo.maxOpenValue;
            hinges.openable.openSpeed = 5.208f;
            hinges.openable.closeSpeed = 5.208f;
            hinges.openable.openingAudio = "Door_Open.wav";
            hinges.openable.closingAudio = "Door_Open.wav";
            hinges.openable.isDeadLock = true;

            // Glass pieces
            MeshNodeCreateInfo& glassClear = meshNodeCreateInfoSet.emplace_back();
            glassClear.meshName = "DoorGlassArt_Clear";
            glassClear.materialName = "DoorGlass";
            glassClear.blendingMode = BlendingMode::GLASS;
            glassClear.decalType = DecalType::GLASS;
            glassClear.tintColor = glm::vec3(1.0f, 1.0f, 1.0f);

            MeshNodeCreateInfo& glassGreen = meshNodeCreateInfoSet.emplace_back();
            glassGreen.meshName = "DoorGlassArt_Green";
            glassGreen.materialName = "DoorGlass";
            glassGreen.blendingMode = BlendingMode::GLASS;
            glassGreen.decalType = DecalType::GLASS;
            glassGreen.tintColor = glm::vec3(0.0f, 0.95f, 0.5f);

            MeshNodeCreateInfo& glassOrange = meshNodeCreateInfoSet.emplace_back();
            glassOrange.meshName = "DoorGlassArt_Orange";
            glassOrange.materialName = "DoorGlass";
            glassOrange.blendingMode = BlendingMode::GLASS;
            glassOrange.decalType = DecalType::GLASS;
            glassOrange.tintColor = glm::vec3(0.6f, 0.2f, 0.0f);

            MeshNodeCreateInfo& glassPurple = meshNodeCreateInfoSet.emplace_back();
            glassPurple.meshName = "DoorGlassArt_Purple";
            glassPurple.materialName = "DoorGlass";
            glassPurple.blendingMode = BlendingMode::GLASS;
            glassPurple.decalType = DecalType::GLASS;
            glassPurple.tintColor = glm::vec3(0.15f, 0.0f, 0.5f);

            MeshNodeCreateInfo& glassRed = meshNodeCreateInfoSet.emplace_back();
            glassRed.meshName = "DoorGlassArt_Red";
            glassRed.materialName = "DoorGlass";
            glassRed.blendingMode = BlendingMode::GLASS;
            glassRed.decalType = DecalType::GLASS;
            glassRed.tintColor = glm::vec3(0.95f, 0.0f, 0.0f);

            MeshNodeCreateInfo& glassYellow = meshNodeCreateInfoSet.emplace_back();
            glassYellow.meshName = "DoorGlassArt_Yellow";
            glassYellow.materialName = "DoorGlass";
            glassYellow.blendingMode = BlendingMode::GLASS;
            glassYellow.decalType = DecalType::GLASS;
            glassYellow.tintColor = glm::vec3(1.00f, 0.8f, 0.25f);

            MeshNodeCreateInfo& glassCenterCircle = meshNodeCreateInfoSet.emplace_back();
            glassCenterCircle.meshName = "DoorGlassArt_CenterCircle";
            glassCenterCircle.materialName = "DoorGlass";
            glassCenterCircle.blendingMode = BlendingMode::GLASS;
            glassCenterCircle.decalType = DecalType::GLASS;
            glassCenterCircle.tintColor = glm::vec3(0.5f, 0.35f, 0.0f);

            meshNodes->Init(id, "Door_StainedGlass", meshNodeCreateInfoSet);

            // Front
            if (createInfo.materialTypeFront == DoorMaterialType::WHITE_PAINT) {
                meshNodes->SetMaterialByMeshName("DoorGlass_Front", "Door_WP");
            }
            else if (createInfo.materialTypeFront == DoorMaterialType::RESIDENT_EVIL) {
                meshNodes->SetMaterialByMeshName("DoorGlass_Front", "Door_RE");
            }

            // Back
            if (createInfo.materialTypeBack == DoorMaterialType::WHITE_PAINT) {
                meshNodes->SetMaterialByMeshName("DoorGlass_Back", "Door_WP");
                meshNodes->SetMaterialByMeshName("DoorGlass_Sides", "Door_WP");
            }
            else if (createInfo.materialTypeBack == DoorMaterialType::RESIDENT_EVIL) {
                meshNodes->SetMaterialByMeshName("DoorGlass_Back", "Door_RE");
                meshNodes->SetMaterialByMeshName("DoorGlass_Sides", "Door_RE");
            }

            // Glass frame
            meshNodes->SetMaterialByMeshName("DoorGlassArtFrame", "DoorGlassFrame");
        }

        if (createInfo.type == DoorType::STAINED_GLASS2) {

            // Collision mesh node
            MeshNodeCreateInfo& door = meshNodeCreateInfoSet.emplace_back();
            door.meshName = "BlackDoor_Front";
            door.rigidDynamic.createObject = true;
            door.rigidDynamic.kinematic = true;
            door.rigidDynamic.filterData.raycastGroup = RAYCAST_DISABLED;
            door.rigidDynamic.filterData.collisionGroup = CollisionGroup::ENVIROMENT_OBSTACLE;
            door.rigidDynamic.filterData.collidesWith = (CollisionGroup)(GENERIC_BOUNCEABLE | BULLET_CASING | ITEM_PICK_UP | RAGDOLL_PLAYER | RAGDOLL_ENEMY);
            door.addtoNavMesh = true;

            // Openable mesh node
            MeshNodeCreateInfo& hinges = meshNodeCreateInfoSet.emplace_back();
            hinges.meshName = "Door_Hinges";
            hinges.materialName = "T_BlackDoor_Door";
            hinges.openable.additionalTriggerMeshNames = {
                "Glass",
                "BlackDoor_Sides",
                "BlackDoor_Front",
                "BlackDoor_Back",
                "DoorBell"
            };
            hinges.openable.isOpenable = true;
            hinges.openable.openAxis = Unloved::OpenAxis::ROTATE_Y_NEG;
            hinges.openable.initialOpenState = createInfo.openAtStart ? Unloved::OpenState::OPEN : Unloved::OpenState::CLOSED;
            hinges.openable.minOpenValue = 0.0f;
            hinges.openable.maxOpenValue = createInfo.maxOpenValue;
            hinges.openable.openSpeed = 5.208f;
            hinges.openable.closeSpeed = 5.208f;
            hinges.openable.openingAudio = "Door_Open.wav";
            hinges.openable.closingAudio = "Door_Open.wav";
            hinges.openable.isDeadLock = true;

            MeshNodeCreateInfo& glass = meshNodeCreateInfoSet.emplace_back();
            glass.meshName = "Glass";
            glass.materialName = "T_BlackDoor_Glass";
            glass.blendingMode = BlendingMode::GLASS;
            glass.decalType = DecalType::GLASS;
            glass.tintColor = glm::vec3(1.0f);

            meshNodes->Init(id, "Door_StainedGlass2", meshNodeCreateInfoSet);

           //meshNodes->SetBlendingModeByMeshName("BoltLockerGuide", BlendingMode::DO_NOT_RENDER);
           //meshNodes->SetBlendingModeByMeshName("BoltLockerSlide", BlendingMode::DO_NOT_RENDER);
           //meshNodes->SetBlendingModeByMeshName("BoltLockerHold", BlendingMode::DO_NOT_RENDER);

        std::string front = "T_BlackDoor_Door";
        std::string back = "T_BlackDoor_Door_BACK";
        std::string metals = "DoorMetals2";
        
        front = "T_BlackDoor_DoorWP";
        back = "T_BlackDoor_DoorWP_BACK";

        front = "Door2_RE";
        back = "Door2Back_RE";

        meshNodes->SetMaterialByMeshName("ChainLocker", metals);
        meshNodes->SetMaterialByMeshName("ChainLink1", metals);
        meshNodes->SetMaterialByMeshName("ChainLink2", metals);
        meshNodes->SetMaterialByMeshName("ChainLink3", metals);
        meshNodes->SetMaterialByMeshName("ChainLink4", metals);
        meshNodes->SetMaterialByMeshName("ChainLink5", metals);
        meshNodes->SetMaterialByMeshName("ChainLink6", metals);
        meshNodes->SetMaterialByMeshName("ChainLink7", metals);
        meshNodes->SetMaterialByMeshName("ChainLink8", metals);
        meshNodes->SetMaterialByMeshName("ChainLink9", metals);
        meshNodes->SetMaterialByMeshName("ChainLink10", metals);
        meshNodes->SetMaterialByMeshName("ChainLach", metals);

        meshNodes->SetMaterialByMeshName("DeadLockMain", metals);
        meshNodes->SetMaterialByMeshName("BoltLockerHold", metals);
        meshNodes->SetMaterialByMeshName("Plate", metals);
        meshNodes->SetMaterialByMeshName("BoltLockerGuide", metals);
        meshNodes->SetMaterialByMeshName("DeadLockVault", metals);
        meshNodes->SetMaterialByMeshName("KeyHole", metals);
        meshNodes->SetMaterialByMeshName("DeadLockSwitch", metals);
        meshNodes->SetMaterialByMeshName("DeadLockCylinder", metals);
        meshNodes->SetMaterialByMeshName("DoorBell", metals);
        meshNodes->SetMaterialByMeshName("BoltLockerSlide", metals);

        meshNodes->SetMaterialByMeshName("BlackDoor_Sides", front);
        meshNodes->SetMaterialByMeshName("BlackDoor_Front", front);
        meshNodes->SetMaterialByMeshName("BlackDoor_Back", back);

        meshNodes->SetMaterialByMeshName("DoorFrame_Front", "DoorFrame_WP");
        meshNodes->SetMaterialByMeshName("DoorFrame_Back", "DoorFrame_WP");
        meshNodes->SetMaterialByMeshName("DoorFrame_Inner", "DoorFrame_WP");

        meshNodes->SetMaterialByMeshName("Glass", "T_BlackDoor_Glass");
            
        meshNodes->SetMaterialByMeshName("Door_Hinges", "DoorMetals"); // set later anyway
        meshNodes->SetMaterialByMeshName("DoorFrame_Hinges", "DoorMetals"); // set later anyway

        //meshNodes->SetBlendingModeByMeshName("KeyHole", BlendingMode::DO_NOT_RENDER);
        //meshNodes->SetBlendingModeByMeshName("ChainLocker", BlendingMode::DO_NOT_RENDER);
        //meshNodes->SetBlendingModeByMeshName("ChainLach", BlendingMode::DO_NOT_RENDER);
                
        //meshNodes->PrintMeshNames();


        // ChainLocker
        // ChainLink1
        // ChainLink2
        // ChainLink3
        // ChainLink4
        // ChainLink5
        // ChainLink6
        // ChainLink7
        // DoorFrame_Hinges
        // BlackDoor_Front
        // BlackDoor_Sides
        // DeadLockMain
        // BoltLockerHold
        // DoorFrame_Back
        // Glass
        // ChainLink8
        // ChainLink9
        // ChainLink10
        // Plate
        // DoorFrame_Inner
        // BoltLockerGuide
        // DeadLockVault
        // KeyHole
        // Door_Hinges
        // DeadLockSwitch
        // ChainLach
        // DeadLockCylinder
        // DoorBell
        // BoltLockerSlide
        // DoorFrame_Front
        // BlackDoor_Back

        }

        // Frame material
        if (createInfo.materialTypeFrameFront == DoorMaterialType::RESIDENT_EVIL) {
            meshNodes->SetMaterialByMeshName("DoorFrame_Front", "DoorFrame_RE");
        }
        if (createInfo.materialTypeFrameFront == DoorMaterialType::WHITE_PAINT) {
            meshNodes->SetMaterialByMeshName("DoorFrame_Front", "DoorFrame_WP");
        }
        if (createInfo.materialTypeFrameBack == DoorMaterialType::RESIDENT_EVIL) {
            meshNodes->SetMaterialByMeshName("DoorFrame_Back", "DoorFrame_RE");
            meshNodes->SetMaterialByMeshName("DoorFrame_Inner", "DoorFrame_RE");
        }
        if (createInfo.materialTypeFrameBack == DoorMaterialType::WHITE_PAINT) {
            meshNodes->SetMaterialByMeshName("DoorFrame_Back", "DoorFrame_WP");
            meshNodes->SetMaterialByMeshName("DoorFrame_Inner", "DoorFrame_WP");
        }

        // Same for all combinations
        meshNodes->SetMaterialByMeshName("Door_Knob", "DoorOldKnob");
        meshNodes->SetMaterialByMeshName("Door_Hinges", "DoorMetals");
        meshNodes->SetMaterialByMeshName("DoorFrame_Hinges", "DoorMetals");
        meshNodes->SetMaterialByMeshName("DoorFrame_KnobLatch", "DoorOldKnob");
        meshNodes->EnableCSMShadows();
        meshNodes->DisableShadowsByBlendingMode(BlendingMode::GLASS);
        meshNodes->DisableShadowsByBlendingMode(BlendingMode::GLASS);
    }
}
