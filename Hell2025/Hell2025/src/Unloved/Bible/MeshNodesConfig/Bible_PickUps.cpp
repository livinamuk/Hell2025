#include "Unloved/Bible/Bible.h"
#include "Hell/Common/Enum.h"
#include "Hell/Logging.h"

namespace Unloved::Bible {
    void ConfigureMeshNodesByItem(uint64_t id, Item item, MeshNodes* meshNodes, bool createPhysicsObjects) {
        ItemInfo* inventoryItemInfo = GetItemInfo(item);
        if (!inventoryItemInfo) {
            Logging::Error() << "Bible::ConfigureMeshNodesByItem(..) failed: '" << Hell::Enum::ToString(item) << "' ItemInfo not found in Bible\n";
            return;
        }

        std::vector<MeshNodeCreateInfo> meshNodeCreateInfoSet;

        PhysicsFilterData pickUpFilterData;
        pickUpFilterData.raycastGroup = RaycastGroup::RAYCAST_DISABLED;
        pickUpFilterData.collisionGroup = CollisionGroup::ITEM_PICK_UP;
        pickUpFilterData.collidesWith = CollisionGroup::ENVIROMENT_OBSTACLE;

        // AKS74U
        if (item == Item::AKS74U) {
            MeshNodeCreateInfo& receiver = meshNodeCreateInfoSet.emplace_back();
            receiver.meshName = "AKS74UReceiver";
            receiver.materialName = "AKS74U_1";
            if (createPhysicsObjects) {
                receiver.rigidDynamic.createObject = true;
                receiver.rigidDynamic.kinematic = false;
                receiver.rigidDynamic.offsetTransform = Transform();
                receiver.rigidDynamic.filterData = pickUpFilterData;
                receiver.rigidDynamic.mass = inventoryItemInfo->GetMass();
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
        if (item == Item::BLACK_SKULL) {
            MeshNodeCreateInfo& blackSkull = meshNodeCreateInfoSet.emplace_back();
            blackSkull.meshName = "BlackSkull";
            blackSkull.materialName = "BlackSkull";
            if (createPhysicsObjects) {
                blackSkull.rigidDynamic.createObject = true;
                blackSkull.rigidDynamic.kinematic = false;
                blackSkull.rigidDynamic.offsetTransform = Transform();
                blackSkull.rigidDynamic.filterData = pickUpFilterData;
                blackSkull.rigidDynamic.mass = inventoryItemInfo->GetMass();
                blackSkull.rigidDynamic.shapeType = inventoryItemInfo->GetPhysicsShapeType();
                blackSkull.rigidDynamic.convexMeshModelName = inventoryItemInfo->GetCollisionModelName();
            }

            meshNodes->Init(id, inventoryItemInfo->GetModelName(), meshNodeCreateInfoSet);
            return;
        }


        // Glock
        if (item == Item::GLOCK) {
            MeshNodeCreateInfo& glock = meshNodeCreateInfoSet.emplace_back();
            glock.meshName = "Glock";
            glock.materialName = "Glock";
            if (createPhysicsObjects) {
                glock.rigidDynamic.createObject = true;
                glock.rigidDynamic.kinematic = false;
                glock.rigidDynamic.offsetTransform = Transform();
                glock.rigidDynamic.filterData = pickUpFilterData;
                glock.rigidDynamic.mass = inventoryItemInfo->GetMass();
                glock.rigidDynamic.shapeType = inventoryItemInfo->GetPhysicsShapeType();
                glock.rigidDynamic.convexMeshModelName = inventoryItemInfo->GetCollisionModelName();
            }

            meshNodes->Init(id, inventoryItemInfo->GetModelName(), meshNodeCreateInfoSet);
            return;
        }

        // Knife
        if (item == Item::KNIFE) {
            MeshNodeCreateInfo& knife = meshNodeCreateInfoSet.emplace_back();
            knife.meshName = "Knife";
            knife.materialName = "Knife";
            if (createPhysicsObjects) {
                knife.rigidDynamic.createObject = true;
                knife.rigidDynamic.kinematic = false;
                knife.rigidDynamic.offsetTransform = Transform();
                knife.rigidDynamic.filterData = pickUpFilterData;
                knife.rigidDynamic.mass = inventoryItemInfo->GetMass();
                knife.rigidDynamic.shapeType = inventoryItemInfo->GetPhysicsShapeType();
                knife.rigidDynamic.convexMeshModelName = inventoryItemInfo->GetCollisionModelName();
            }

            meshNodes->Init(id, inventoryItemInfo->GetModelName(), meshNodeCreateInfoSet);
            return;
        }

        // Golden Glock
        if (item == Item::GOLDEN_GLOCK) {
            MeshNodeCreateInfo& glock = meshNodeCreateInfoSet.emplace_back();
            glock.meshName = "GoldenGlock";
            glock.materialName = "GlockGold";
            if (createPhysicsObjects) {
                glock.rigidDynamic.createObject = true;
                glock.rigidDynamic.kinematic = false;
                glock.rigidDynamic.offsetTransform = Transform();
                glock.rigidDynamic.filterData = pickUpFilterData;
                glock.rigidDynamic.mass = inventoryItemInfo->GetMass();
                glock.rigidDynamic.shapeType = inventoryItemInfo->GetPhysicsShapeType();
                glock.rigidDynamic.convexMeshModelName = inventoryItemInfo->GetCollisionModelName();
            }

            meshNodes->Init(id, inventoryItemInfo->GetModelName(), meshNodeCreateInfoSet);
            return;
        }

        // Remington 870
        if (item == Item::REMINGTON_870) {
            MeshNodeCreateInfo& shotgun = meshNodeCreateInfoSet.emplace_back();
            shotgun.meshName = "Remington870";
            shotgun.materialName = "Shotgun";
            if (createPhysicsObjects) {
                shotgun.rigidDynamic.createObject = true;
                shotgun.rigidDynamic.kinematic = false;
                shotgun.rigidDynamic.offsetTransform = Transform();
                shotgun.rigidDynamic.filterData = pickUpFilterData;
                shotgun.rigidDynamic.mass = inventoryItemInfo->GetMass();
                shotgun.rigidDynamic.shapeType = inventoryItemInfo->GetPhysicsShapeType();
                shotgun.rigidDynamic.convexMeshModelName = inventoryItemInfo->GetCollisionModelName();
            }

            meshNodes->Init(id, inventoryItemInfo->GetModelName(), meshNodeCreateInfoSet);
            return;
        }

        // Small Key
        if (item == Item::SMALL_KEY) {
            MeshNodeCreateInfo& smallKey = meshNodeCreateInfoSet.emplace_back();
            smallKey.meshName = "SmallKey";
            smallKey.materialName = "SmallKey";
            if (createPhysicsObjects) {
                smallKey.rigidDynamic.createObject = true;
                smallKey.rigidDynamic.kinematic = false;
                smallKey.rigidDynamic.offsetTransform = Transform();
                smallKey.rigidDynamic.filterData = pickUpFilterData;
                smallKey.rigidDynamic.mass = inventoryItemInfo->GetMass();
                smallKey.rigidDynamic.shapeType = inventoryItemInfo->GetPhysicsShapeType();
                smallKey.rigidDynamic.convexMeshModelName = inventoryItemInfo->GetCollisionModelName();
            }

            meshNodes->Init(id, inventoryItemInfo->GetModelName(), meshNodeCreateInfoSet);
            return;
        }

        // Small Key Silver
        if (item == Item::SMALL_KEY_SILVER) {
            MeshNodeCreateInfo& smallKey = meshNodeCreateInfoSet.emplace_back();
            smallKey.meshName = "SmallKey";
            smallKey.materialName = "SmallKeySilver";
            if (createPhysicsObjects) {
                smallKey.rigidDynamic.createObject = true;
                smallKey.rigidDynamic.kinematic = false;
                smallKey.rigidDynamic.offsetTransform = Transform();
                smallKey.rigidDynamic.filterData = pickUpFilterData;
                smallKey.rigidDynamic.mass = inventoryItemInfo->GetMass();
                smallKey.rigidDynamic.shapeType = inventoryItemInfo->GetPhysicsShapeType();
                smallKey.rigidDynamic.convexMeshModelName = inventoryItemInfo->GetCollisionModelName();
            }

            meshNodes->Init(id, inventoryItemInfo->GetModelName(), meshNodeCreateInfoSet);
            return;
        }

        // SPAS
        if (item == Item::SPAS) {
            MeshNodeCreateInfo& main = meshNodeCreateInfoSet.emplace_back();
            main.meshName = "SPAS12_Main";
            main.materialName = "SPAS2_Main";
            if (createPhysicsObjects) {
                main.rigidDynamic.createObject = true;
                main.rigidDynamic.kinematic = false;
                main.rigidDynamic.offsetTransform = Transform();
                main.rigidDynamic.filterData = pickUpFilterData;
                main.rigidDynamic.mass = inventoryItemInfo->GetMass();
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
        if (item == Item::SHOTGUN_SHELLS) {
            MeshNodeCreateInfo& ammo = meshNodeCreateInfoSet.emplace_back();
            ammo.meshName = "Ammo_ShotgunBox";
            ammo.materialName = "Shotgun_AmmoBox";
            if (createPhysicsObjects) {
                ammo.rigidDynamic.createObject = true;
                ammo.rigidDynamic.kinematic = false;
                ammo.rigidDynamic.offsetTransform = Transform();
                ammo.rigidDynamic.filterData = pickUpFilterData;
                ammo.rigidDynamic.mass = inventoryItemInfo->GetMass();
                ammo.rigidDynamic.shapeType = inventoryItemInfo->GetPhysicsShapeType();
            }

            meshNodes->Init(id, inventoryItemInfo->GetModelName(), meshNodeCreateInfoSet);
            return;
        }

        // Tokarev
        if (item == Item::TOKAREV) {
            MeshNodeCreateInfo& body = meshNodeCreateInfoSet.emplace_back();
            body.meshName = "TokarevBody";
            body.materialName = "Tokarev";
            if (createPhysicsObjects) {
                body.rigidDynamic.createObject = true;
                body.rigidDynamic.kinematic = false;
                body.rigidDynamic.offsetTransform = Transform();
                body.rigidDynamic.filterData = pickUpFilterData;
                body.rigidDynamic.mass = inventoryItemInfo->GetMass();
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
		if (item == Item::PILLS) {
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
				pills.rigidDynamic.mass = inventoryItemInfo->GetMass();
				pills.rigidDynamic.shapeType = inventoryItemInfo->GetPhysicsShapeType();
				pills.rigidDynamic.convexMeshModelName = inventoryItemInfo->GetCollisionModelName();
			}

			meshNodes->Init(id, inventoryItemInfo->GetModelName(), meshNodeCreateInfoSet);
			return;
		}


		// P90
		if (item == Item::P90) {
			MeshNodeCreateInfo& main = meshNodeCreateInfoSet.emplace_back();
			main.meshName = "Main";
			main.materialName = "P90_Main";

			if (createPhysicsObjects) {
				main.rigidDynamic.createObject = true;
				main.rigidDynamic.kinematic = false;
				main.rigidDynamic.offsetTransform = Transform();
				main.rigidDynamic.filterData = pickUpFilterData;
				main.rigidDynamic.mass = inventoryItemInfo->GetMass();
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


        Logging::Error() << "Bible::ConfigureMeshNodesByItem(..) failed: '" << Hell::Enum::ToString(item) << "' not implemented\n";
    }
}
