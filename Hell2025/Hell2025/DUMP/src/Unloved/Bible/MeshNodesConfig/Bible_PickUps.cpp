#include "Unloved/Bible/Bible.h"
#include "Hell/Logging.h"

namespace Bible {
    using namespace Unloved;


    // This whole file is pretty messy now that you merged all the inventory item/pickup item stuff
    // This whole file is pretty messy now that you merged all the inventory item/pickup item stuff
    // This whole file is pretty messy now that you merged all the inventory item/pickup item stuff

    void ConfigureMeshNodesByItemName(uint64_t id, const std::string& itemName, MeshNodes* meshNodes, bool createPhysicsObjects) {
        ItemInfo* inventoryItemInfo = Bible::GetItemInfoByName(itemName);
        if (!inventoryItemInfo) {
            Logging::Error() << "Bible::ConfigureMeshNodesByitemName(..) failed: '" << itemName << "' InventoryItemInfo not found in Bible\n";
            return;
        }

        std::vector<MeshNodeCreateInfo> meshNodeCreateInfoSet;

        PhysicsFilterData pickUpFilterData;
        pickUpFilterData.raycastGroup = RaycastGroup::RAYCAST_DISABLED;
        pickUpFilterData.collisionGroup = CollisionGroup::ITEM_PICK_UP;
        pickUpFilterData.collidesWith = CollisionGroup::ENVIROMENT_OBSTACLE;

        // AKS74U
        if (itemName == "AKS74U") {
            MeshNodeCreateInfo& receiver = meshNodeCreateInfoSet.emplace_back();
            receiver.meshName = "AKS74UReceiver";
            receiver.materialName = "AKS74U_1";
            if (createPhysicsObjects) {
                receiver.rigidDynamic.createObject = true;
                receiver.rigidDynamic.kinematic = false;
                receiver.rigidDynamic.offsetTransform = Transform();
                receiver.rigidDynamic.filterData = pickUpFilterData;
                receiver.rigidDynamic.mass = Bible::GetItemMass(itemName);
                receiver.rigidDynamic.shapeType = inventoryItemInfo->GetPhysicsShapeType();
                receiver.rigidDynamic.convexMeshModelName = inventoryItemInfo->GetCollisionModelName();
            }

            MeshNodeCreateInfo& barrel = meshNodeCreateInfoSet.emplace_back();
            barrel.meshName = "AKS74UBarrel";
            barrel.materialName = "AKS74U_4";

            MeshNodeCreateInfo& bolt = meshNodeCreateInfoSet.emplace_back();
            bolt.meshName = "AKS74UBolt";
            bolt.materialName = "AKS74U_1";

            MeshNodeCreateInfo& handGuard = meshNodeCreateInfoSet.emplace_back();
            handGuard.meshName = "AKS74UHandGuard";
            handGuard.materialName = "AKS74U_0";

            MeshNodeCreateInfo& mag = meshNodeCreateInfoSet.emplace_back();
            mag.meshName = "AKS74UMag";
            mag.materialName = "AKS74U_3";

            MeshNodeCreateInfo& pistolGrip = meshNodeCreateInfoSet.emplace_back();
            pistolGrip.meshName = "AKS74UPistolGrip";
            pistolGrip.materialName = "AKS74U_2";

            meshNodes->Init(id, inventoryItemInfo->GetModelName(), meshNodeCreateInfoSet);
            return;
        }

        // Black Skull
        if (itemName == "BlackSkull") {
            MeshNodeCreateInfo& blackSkull = meshNodeCreateInfoSet.emplace_back();
            blackSkull.meshName = "BlackSkull";
            blackSkull.materialName = "BlackSkull";
            if (createPhysicsObjects) {
                blackSkull.rigidDynamic.createObject = true;
                blackSkull.rigidDynamic.kinematic = false;
                blackSkull.rigidDynamic.offsetTransform = Transform();
                blackSkull.rigidDynamic.filterData = pickUpFilterData;
                blackSkull.rigidDynamic.mass = Bible::GetItemMass(itemName);
                blackSkull.rigidDynamic.shapeType = inventoryItemInfo->GetPhysicsShapeType();
                blackSkull.rigidDynamic.convexMeshModelName = inventoryItemInfo->GetCollisionModelName();
            }

            meshNodes->Init(id, inventoryItemInfo->GetModelName(), meshNodeCreateInfoSet);
            return;
        }


        // Glock
        if (itemName == "Glock") {
            MeshNodeCreateInfo& glock = meshNodeCreateInfoSet.emplace_back();
            glock.meshName = "Glock";
            glock.materialName = "Glock";
            if (createPhysicsObjects) {
                glock.rigidDynamic.createObject = true;
                glock.rigidDynamic.kinematic = false;
                glock.rigidDynamic.offsetTransform = Transform();
                glock.rigidDynamic.filterData = pickUpFilterData;
                glock.rigidDynamic.mass = Bible::GetItemMass(itemName);
                glock.rigidDynamic.shapeType = inventoryItemInfo->GetPhysicsShapeType();
                glock.rigidDynamic.convexMeshModelName = inventoryItemInfo->GetCollisionModelName();
            }

            meshNodes->Init(id, inventoryItemInfo->GetModelName(), meshNodeCreateInfoSet);
            return;
        }

        // Knife
        if (itemName == "Knife") {
            MeshNodeCreateInfo& knife = meshNodeCreateInfoSet.emplace_back();
            knife.meshName = "Knife";
            knife.materialName = "Knife";
            if (createPhysicsObjects) {
                knife.rigidDynamic.createObject = true;
                knife.rigidDynamic.kinematic = false;
                knife.rigidDynamic.offsetTransform = Transform();
                knife.rigidDynamic.filterData = pickUpFilterData;
                knife.rigidDynamic.mass = Bible::GetItemMass(itemName);
                knife.rigidDynamic.shapeType = inventoryItemInfo->GetPhysicsShapeType();
                knife.rigidDynamic.convexMeshModelName = inventoryItemInfo->GetCollisionModelName();
            }

            meshNodes->Init(id, inventoryItemInfo->GetModelName(), meshNodeCreateInfoSet);
            return;
        }

        // Golden Glock
        if (itemName == "GoldenGlock") {
            MeshNodeCreateInfo& glock = meshNodeCreateInfoSet.emplace_back();
            glock.meshName = "GoldenGlock";
            glock.materialName = "GlockGold";
            if (createPhysicsObjects) {
                glock.rigidDynamic.createObject = true;
                glock.rigidDynamic.kinematic = false;
                glock.rigidDynamic.offsetTransform = Transform();
                glock.rigidDynamic.filterData = pickUpFilterData;
                glock.rigidDynamic.mass = Bible::GetItemMass(itemName);
                glock.rigidDynamic.shapeType = inventoryItemInfo->GetPhysicsShapeType();
                glock.rigidDynamic.convexMeshModelName = inventoryItemInfo->GetCollisionModelName();
            }

            meshNodes->Init(id, inventoryItemInfo->GetModelName(), meshNodeCreateInfoSet);
            return;
        }

        // Remington 870
        if (itemName == "Remington870") {
            MeshNodeCreateInfo& shotgun = meshNodeCreateInfoSet.emplace_back();
            shotgun.meshName = "Remington870";
            shotgun.materialName = "Shotgun";
            if (createPhysicsObjects) {
                shotgun.rigidDynamic.createObject = true;
                shotgun.rigidDynamic.kinematic = false;
                shotgun.rigidDynamic.offsetTransform = Transform();
                shotgun.rigidDynamic.filterData = pickUpFilterData;
                shotgun.rigidDynamic.mass = Bible::GetItemMass(itemName);
                shotgun.rigidDynamic.shapeType = inventoryItemInfo->GetPhysicsShapeType();
                shotgun.rigidDynamic.convexMeshModelName = inventoryItemInfo->GetCollisionModelName();
            }

            meshNodes->Init(id, inventoryItemInfo->GetModelName(), meshNodeCreateInfoSet);
            return;
        }

        // Small Key
        if (itemName == "SmallKey") {
            MeshNodeCreateInfo& smallKey = meshNodeCreateInfoSet.emplace_back();
            smallKey.meshName = "SmallKey";
            smallKey.materialName = "SmallKey";
            if (createPhysicsObjects) {
                smallKey.rigidDynamic.createObject = true;
                smallKey.rigidDynamic.kinematic = false;
                smallKey.rigidDynamic.offsetTransform = Transform();
                smallKey.rigidDynamic.filterData = pickUpFilterData;
                smallKey.rigidDynamic.mass = Bible::GetItemMass(itemName);
                smallKey.rigidDynamic.shapeType = inventoryItemInfo->GetPhysicsShapeType();
                smallKey.rigidDynamic.convexMeshModelName = inventoryItemInfo->GetCollisionModelName();
            }

            meshNodes->Init(id, inventoryItemInfo->GetModelName(), meshNodeCreateInfoSet);
            return;
        }

        // Small Key Silver
        if (itemName == "SmallKeySilver") {
            MeshNodeCreateInfo& smallKey = meshNodeCreateInfoSet.emplace_back();
            smallKey.meshName = "SmallKey";
            smallKey.materialName = "SmallKeySilver";
            if (createPhysicsObjects) {
                smallKey.rigidDynamic.createObject = true;
                smallKey.rigidDynamic.kinematic = false;
                smallKey.rigidDynamic.offsetTransform = Transform();
                smallKey.rigidDynamic.filterData = pickUpFilterData;
                smallKey.rigidDynamic.mass = Bible::GetItemMass(itemName);
                smallKey.rigidDynamic.shapeType = inventoryItemInfo->GetPhysicsShapeType();
                smallKey.rigidDynamic.convexMeshModelName = inventoryItemInfo->GetCollisionModelName();
            }

            meshNodes->Init(id, inventoryItemInfo->GetModelName(), meshNodeCreateInfoSet);
            return;
        }

        // SPAS
        if (itemName == "SPAS") {
            MeshNodeCreateInfo& main = meshNodeCreateInfoSet.emplace_back();
            main.meshName = "SPAS12_Main";
            main.materialName = "SPAS2_Main";
            if (createPhysicsObjects) {
                main.rigidDynamic.createObject = true;
                main.rigidDynamic.kinematic = false;
                main.rigidDynamic.offsetTransform = Transform();
                main.rigidDynamic.filterData = pickUpFilterData;
                main.rigidDynamic.mass = Bible::GetItemMass(itemName);
                main.rigidDynamic.shapeType = inventoryItemInfo->GetPhysicsShapeType();
                main.rigidDynamic.convexMeshModelName = inventoryItemInfo->GetCollisionModelName();
            }

            MeshNodeCreateInfo& moving = meshNodeCreateInfoSet.emplace_back();
            moving.meshName = "SPAS12_Moving";
            moving.materialName = "SPAS2_Moving";

            MeshNodeCreateInfo& stamped = meshNodeCreateInfoSet.emplace_back();
            stamped.meshName = "SPAS12_Stamped";
            stamped.materialName = "SPAS2_Stamped";

            meshNodes->Init(id, inventoryItemInfo->GetModelName(), meshNodeCreateInfoSet);
            return;
        }

        // Shotty Buckshot Box
        if (itemName == "12GaugeBuckShot") {
            MeshNodeCreateInfo& ammo = meshNodeCreateInfoSet.emplace_back();
            ammo.meshName = "Ammo_ShotgunBox";
            ammo.materialName = "Shotgun_AmmoBox";
            if (createPhysicsObjects) {
                ammo.rigidDynamic.createObject = true;
                ammo.rigidDynamic.kinematic = false;
                ammo.rigidDynamic.offsetTransform = Transform();
                ammo.rigidDynamic.filterData = pickUpFilterData;
                ammo.rigidDynamic.mass = Bible::GetItemMass(itemName);
                ammo.rigidDynamic.shapeType = inventoryItemInfo->GetPhysicsShapeType();
            }

            meshNodes->Init(id, inventoryItemInfo->GetModelName(), meshNodeCreateInfoSet);
            return;
        }

        // Shotty Slug Box
        if (itemName == "12GaugeBuckShot") {
            MeshNodeCreateInfo& ammo = meshNodeCreateInfoSet.emplace_back();
            ammo.meshName = "Ammo_ShotgunBox";
            ammo.materialName = "Shotgun_AmmoBoxSlug";
            if (createPhysicsObjects) {
                ammo.rigidDynamic.createObject = true;
                ammo.rigidDynamic.kinematic = false;
                ammo.rigidDynamic.offsetTransform = Transform();
                ammo.rigidDynamic.filterData = pickUpFilterData;
                ammo.rigidDynamic.mass = Bible::GetItemMass(itemName);
                ammo.rigidDynamic.shapeType = inventoryItemInfo->GetPhysicsShapeType();
            }

            meshNodes->Init(id, inventoryItemInfo->GetModelName(), meshNodeCreateInfoSet);
            return;
        }

        // Tokarev
        if (itemName == "Tokarev") {
            MeshNodeCreateInfo& body = meshNodeCreateInfoSet.emplace_back();
            body.meshName = "TokarevBody";
            body.materialName = "Tokarev";
            if (createPhysicsObjects) {
                body.rigidDynamic.createObject = true;
                body.rigidDynamic.kinematic = false;
                body.rigidDynamic.offsetTransform = Transform();
                body.rigidDynamic.filterData = pickUpFilterData;
                body.rigidDynamic.mass = Bible::GetItemMass(itemName);
                body.rigidDynamic.shapeType = inventoryItemInfo->GetPhysicsShapeType();
                body.rigidDynamic.convexMeshModelName = inventoryItemInfo->GetCollisionModelName();
            }

            MeshNodeCreateInfo& grip = meshNodeCreateInfoSet.emplace_back();
            grip.meshName = "TokarevGripPolymer";
            grip.materialName = "TokarevGrip";

            meshNodes->Init(id, inventoryItemInfo->GetModelName(), meshNodeCreateInfoSet);
            return;
        }


		// Pills
		if (itemName == "Pills") {
			MeshNodeCreateInfo& cover = meshNodeCreateInfoSet.emplace_back();
            cover.meshName = "Cover";
            cover.materialName = "Pills";
            cover.blendingMode = BlendingMode::GLASS;

			MeshNodeCreateInfo& pills = meshNodeCreateInfoSet.emplace_back();
			pills.meshName = "Pills";
			pills.materialName = "Pills";
			if (createPhysicsObjects) {
				pills.rigidDynamic.createObject = true;
				pills.rigidDynamic.kinematic = false;
				pills.rigidDynamic.offsetTransform = Transform();
				pills.rigidDynamic.filterData = pickUpFilterData;
				pills.rigidDynamic.mass = Bible::GetItemMass(itemName);
				pills.rigidDynamic.shapeType = inventoryItemInfo->GetPhysicsShapeType();
				pills.rigidDynamic.convexMeshModelName = inventoryItemInfo->GetCollisionModelName();
			}

			meshNodes->Init(id, inventoryItemInfo->GetModelName(), meshNodeCreateInfoSet);
			return;
		}


		// P90
		if (itemName == "P90") {
			MeshNodeCreateInfo& main = meshNodeCreateInfoSet.emplace_back();
			main.meshName = "Main";
			main.materialName = "P90_Main";

			if (createPhysicsObjects) {
				main.rigidDynamic.createObject = true;
				main.rigidDynamic.kinematic = false;
				main.rigidDynamic.offsetTransform = Transform();
				main.rigidDynamic.filterData = pickUpFilterData;
				main.rigidDynamic.mass = Bible::GetItemMass(itemName);
				main.rigidDynamic.shapeType = inventoryItemInfo->GetPhysicsShapeType();
				main.rigidDynamic.convexMeshModelName = inventoryItemInfo->GetCollisionModelName();
			}

			MeshNodeCreateInfo& magazine = meshNodeCreateInfoSet.emplace_back();
			magazine.meshName = "Magazine";
			magazine.materialName = "P90_Mag";
			magazine.blendingMode = BlendingMode::GLASS;

			MeshNodeCreateInfo& otherMagazineShit = meshNodeCreateInfoSet.emplace_back();
			otherMagazineShit.meshName = "UsesMagTexture";
			otherMagazineShit.materialName = "P90_FrontEnd";

			MeshNodeCreateInfo& rails = meshNodeCreateInfoSet.emplace_back();
			rails.meshName = "Rails";
			rails.materialName = "P90_Rails";

			MeshNodeCreateInfo& sling = meshNodeCreateInfoSet.emplace_back();
			sling.meshName = "Sling";
			sling.materialName = "P90_Sling";

			MeshNodeCreateInfo& frontEnd = meshNodeCreateInfoSet.emplace_back();
			frontEnd.meshName = "FrontEnd";
			frontEnd.materialName = "P90_FrontEnd";


			meshNodes->Init(id, inventoryItemInfo->GetModelName(), meshNodeCreateInfoSet);
			return;
		}


        Logging::Error() << "Bible::ConfigureMeshNodesByitemName(..) failed: '" << itemName << "' not implemented\n";
    }
}
