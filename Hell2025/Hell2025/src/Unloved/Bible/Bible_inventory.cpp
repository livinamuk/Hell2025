#include "Unloved/Bible/Bible.h"
#include "Hell/Logging.h"
#include <unordered_map>

namespace Unloved::Bible {

    void InitInventoryItemInfo() {

        ItemInfo& aks74u = CreateInventoryItemInfo(Item::AKS74U, "AKS74U");
        aks74u.m_type = ItemType::WEAPON;
        aks74u.m_mass = 2.7f;
        aks74u.m_cost = 100;
        aks74u.m_modelName = "AKS74U";
        aks74u.m_collisionModelName = "CollisionMesh_AKS74U";
        aks74u.m_physicsShapeType = PhysicsShapeType::CONVEX_MESH;
        aks74u.m_inventoryInfo.cellSize = 2;
        aks74u.m_inventoryInfo.combineable = false;
        aks74u.m_inventoryInfo.discardable = true;
        aks74u.m_inventoryInfo.equipable = true;
        aks74u.m_examineInfo.translation = glm::vec3(0.0f, 0.0f, 0.0f);
        aks74u.m_examineInfo.scale = glm::vec3(2.75f);
        aks74u.m_examineInfo.maxZoom = 1.75f;
        aks74u.m_inventoryInfo.heading = "AKS-74U";
        aks74u.m_inventoryInfo.description = R"(Pew pew pew.)";

        ItemInfo& glock = CreateInventoryItemInfo(Item::GLOCK, "Glock");
        glock.m_type = ItemType::WEAPON;
        glock.m_mass = 2.0f;
        glock.m_cost = 50;
        glock.m_modelName = "Weapon_Glock";
        glock.m_collisionModelName = "CollisionMesh_Glock";
        glock.m_physicsShapeType = PhysicsShapeType::CONVEX_MESH;
        glock.m_inventoryInfo.cellSize = 1;
        glock.m_inventoryInfo.combineable = true;
        glock.m_inventoryInfo.discardable = true;
        glock.m_inventoryInfo.equipable = true;
        glock.m_examineInfo.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
        glock.m_examineInfo.scale = glm::vec3(4.5f);
        glock.m_examineInfo.maxZoom = 1.85f;
        glock.m_inventoryInfo.heading = "GLOCK 22";
        glock.m_inventoryInfo.description = R"(Australian police issue. Matte and boxy, a cold
little companion. It does the paperwork duty
without drama. Dependable at short range,
underwhelming at a distance. A proper piece
of shit.)";

        ItemInfo& goldenGlock = CreateInventoryItemInfo(Item::GOLDEN_GLOCK, "GoldenGlock");
        goldenGlock.m_type = ItemType::WEAPON;
        goldenGlock.m_mass = 2.0f;
        goldenGlock.m_cost = 250;
        goldenGlock.m_modelName = "Weapon_GoldenGlock";
        goldenGlock.m_collisionModelName = "CollisionMesh_GoldenGlock";
        goldenGlock.m_physicsShapeType = PhysicsShapeType::CONVEX_MESH;
        goldenGlock.m_inventoryInfo.cellSize = 1;
        goldenGlock.m_inventoryInfo.combineable = true;
        goldenGlock.m_inventoryInfo.discardable = true;
        goldenGlock.m_inventoryInfo.equipable = true;
        goldenGlock.m_examineInfo.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
        goldenGlock.m_examineInfo.scale = glm::vec3(4.5f);
        goldenGlock.m_examineInfo.maxZoom = 1.85f;
        goldenGlock.m_inventoryInfo.heading = "GOLDEN GLOCK 22";
        goldenGlock.m_inventoryInfo.description = R"(Shaken naked, not stirred. Pierce Brosnan's
wet dream, wedding gift dipped in drip and
glitter. Natalia gonna be in the good books
for this one.)";

        ItemInfo& tokarev = CreateInventoryItemInfo(Item::TOKAREV, "Tokarev");
        tokarev.m_type = ItemType::WEAPON;
        tokarev.m_mass = 2.0f;
        tokarev.m_cost = 150;
        tokarev.m_modelName = "Weapon_Tokarev";
        tokarev.m_collisionModelName = "CollisionMesh_Tokarev";
        tokarev.m_physicsShapeType = PhysicsShapeType::CONVEX_MESH;
        tokarev.m_inventoryInfo.cellSize = 1;
        tokarev.m_inventoryInfo.combineable = false;
        tokarev.m_inventoryInfo.discardable = true;
        tokarev.m_inventoryInfo.equipable = true;
        tokarev.m_examineInfo.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
        tokarev.m_examineInfo.scale = glm::vec3(4.5f);
        tokarev.m_examineInfo.maxZoom = 1.85f;
        tokarev.m_inventoryInfo.heading = "TOKAREV";
        tokarev.m_inventoryInfo.description = R"(Soviet semi-automatic pistol developed in the
1920s. This baby runs hot and she got no frills,
just prints pretty little holes. Straight out
of Ourumov's briefcase.)";

        ItemInfo& knife = CreateInventoryItemInfo(Item::KNIFE, "Knife");
        knife.m_type = ItemType::WEAPON;
        knife.m_mass = 2.0f;
        knife.m_modelName = "Weapon_Knife";
        knife.m_collisionModelName = "CollisionMesh_Knife";
        knife.m_physicsShapeType = PhysicsShapeType::CONVEX_MESH;
        knife.m_inventoryInfo.cellSize = 1;
        knife.m_inventoryInfo.combineable = true;
        knife.m_inventoryInfo.discardable = false;
        knife.m_inventoryInfo.equipable = true;
        knife.m_examineInfo.rotation = glm::vec3(0.0f, -0.2f, -0.6f);
        knife.m_examineInfo.scale = glm::vec3(3.5f);
        knife.m_examineInfo.maxZoom = 1.75f;
        knife.m_inventoryInfo.heading = "KNIFE";
        knife.m_inventoryInfo.description = R"(From fish to pharynx, this rusty little dagger
takes the same short and messy path. Might
wanna bring a cloth.)";

        ItemInfo& remington870 = CreateInventoryItemInfo(Item::REMINGTON_870, "Remington870");
        remington870.m_type = ItemType::WEAPON;
        remington870.m_mass = 3.2f;
        remington870.m_cost = 250;
        remington870.m_modelName = "Weapon_Remington870";
        remington870.m_collisionModelName = "CollisionMesh_Remington870";
        remington870.m_physicsShapeType = PhysicsShapeType::CONVEX_MESH;
        remington870.m_inventoryInfo.cellSize = 3;
        remington870.m_inventoryInfo.combineable = false;
        remington870.m_inventoryInfo.discardable = true;
        remington870.m_inventoryInfo.equipable = true;
        remington870.m_examineInfo.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
        remington870.m_examineInfo.scale = glm::vec3(2.125f);
        remington870.m_examineInfo.maxZoom = 1.75f;
        remington870.m_inventoryInfo.heading = "REMINGTON 870";
        remington870.m_inventoryInfo.description = R"(American pump-action built like a fence post.
This twelve-gauge thunder will clear every
damn room and barn in time for Judgment
Day supper.)";

        ItemInfo& spas = CreateInventoryItemInfo(Item::SPAS, "SPAS");
        spas.m_type = ItemType::WEAPON;
        spas.m_mass = 3.2f;
        spas.m_cost = 400;
        spas.m_modelName = "Weapon_SPAS";
        spas.m_collisionModelName = "CollisionMesh_SPAS";
        spas.m_physicsShapeType = PhysicsShapeType::CONVEX_MESH;
        spas.m_inventoryInfo.cellSize = 3;
        spas.m_inventoryInfo.combineable = false;
        spas.m_inventoryInfo.discardable = true;
        spas.m_inventoryInfo.equipable = true;
        spas.m_examineInfo.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
        spas.m_examineInfo.scale = glm::vec3(2.95f);
        spas.m_examineInfo.maxZoom = 1.75f;
        spas.m_inventoryInfo.heading = "SPAS";
        spas.m_inventoryInfo.description = R"(Dual modal, Italian menace. If killing is a sport,
then Franchi's Special Purpose Automatic
Shotgun will put you on the podium with the
cadence of a fucking riot.)";

        ItemInfo& shotgunSlugBox = CreateInventoryItemInfo(Item::SHOTGUN_SHELLS, "12GaugeBuckShot");
        shotgunSlugBox.m_type = ItemType::AMMO;
        shotgunSlugBox.m_ammo = Ammo::SHOTGUN_SHELLS;
        shotgunSlugBox.m_mass = 0.5f;
        shotgunSlugBox.m_cost = 250;
        shotgunSlugBox.m_modelName = "Ammo_ShotgunBox";
        shotgunSlugBox.m_collisionModelName = UNDEFINED_STRING;
        shotgunSlugBox.m_physicsShapeType = PhysicsShapeType::BOX;
        shotgunSlugBox.m_inventoryInfo.cellSize = 3;
        shotgunSlugBox.m_inventoryInfo.discardable = true;
        shotgunSlugBox.m_inventoryInfo.equipable = false;
        shotgunSlugBox.m_examineInfo.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
        shotgunSlugBox.m_examineInfo.scale = glm::vec3(2.5f);
        shotgunSlugBox.m_examineInfo.maxZoom = 1.75f;
        shotgunSlugBox.m_inventoryInfo.heading = "12 Gauge Ammo";
        shotgunSlugBox.m_inventoryInfo.description = R"(Neque porro quisquam est qui dolorem
ipsum quia dolor sit amet, consectetur,
adipisci velit...)";

        ItemInfo& blackSkull = CreateInventoryItemInfo(Item::BLACK_SKULL, "BlackSkull");
        blackSkull.m_type = ItemType::USELESS;
        blackSkull.m_mass = 2.0f;
        blackSkull.m_cost = 250;
        blackSkull.m_modelName = "Item_BlackSkull";
        blackSkull.m_collisionModelName = "CollisionMesh_BlackSkull";
        blackSkull.m_physicsShapeType = PhysicsShapeType::CONVEX_MESH;
        blackSkull.m_inventoryInfo.cellSize = 1;
        blackSkull.m_inventoryInfo.combineable = false;
        blackSkull.m_inventoryInfo.discardable = true;
        blackSkull.m_inventoryInfo.equipable = false;
        blackSkull.m_examineInfo.translation = glm::vec3(0.0f, 0.2f, 0.0f);
        blackSkull.m_examineInfo.rotation = glm::vec3(1.1f, 0.0f, 0.0f);
        blackSkull.m_examineInfo.scale = glm::vec3(4.5f);
        blackSkull.m_examineInfo.maxZoom = 1.75f;
        blackSkull.m_inventoryInfo.heading = "BLACK SKULL";
        blackSkull.m_inventoryInfo.description = R"(Worth little to you, worth everything to them.)";

        ItemInfo& smallKey = CreateInventoryItemInfo(Item::SMALL_KEY, "SmallKey");
        smallKey.m_type = ItemType::KEY;
        smallKey.m_mass = 0.5f;
        smallKey.m_cost = 400;
        smallKey.m_modelName = "Item_SmallKey";
        smallKey.m_collisionModelName = UNDEFINED_STRING;
        smallKey.m_physicsShapeType = PhysicsShapeType::CONVEX_MESH;
        smallKey.m_inventoryInfo.cellSize = 1;
        smallKey.m_inventoryInfo.combineable = false;
        smallKey.m_inventoryInfo.discardable = true;
        smallKey.m_inventoryInfo.equipable = false;
        smallKey.m_examineInfo.rotation = glm::vec3(0.0f, -0.2f, -0.6f);
        smallKey.m_examineInfo.scale = glm::vec3(8.5f);
        smallKey.m_examineInfo.maxZoom = 1.75f;
        smallKey.m_inventoryInfo.heading = "SMALL KEY";
        smallKey.m_inventoryInfo.description = R"(Smells of dust and secrets. One turn, one less
mystery.)";

        ItemInfo& smallKeySilver = CreateInventoryItemInfo(Item::SMALL_KEY_SILVER, "SmallKeySilver");
        smallKeySilver.m_type = ItemType::KEY;
        smallKeySilver.m_mass = 0.5f;
        smallKeySilver.m_cost = 900;
        smallKeySilver.m_modelName = "Item_SmallKey";
        smallKeySilver.m_collisionModelName = UNDEFINED_STRING;
        smallKeySilver.m_physicsShapeType = PhysicsShapeType::CONVEX_MESH;
        smallKeySilver.m_inventoryInfo.cellSize = 1;
        smallKeySilver.m_inventoryInfo.combineable = false;
        smallKeySilver.m_inventoryInfo.discardable = true;
        smallKeySilver.m_inventoryInfo.equipable = false;
        smallKeySilver.m_examineInfo.rotation = glm::vec3(0.0f, -0.2f, -0.6f);
        smallKeySilver.m_examineInfo.scale = glm::vec3(8.5f);
        smallKeySilver.m_examineInfo.maxZoom = 1.75f;
        smallKeySilver.m_inventoryInfo.heading = "SMALL KEY SILVER";
        smallKeySilver.m_inventoryInfo.description = R"(Someone carried this close for a long time.
Probably for a reason.)";

        ItemInfo& pills = CreateInventoryItemInfo(Item::PILLS, "Pills");
        pills.m_type = ItemType::HEAL;
        pills.m_mass = 0.5f;
        pills.m_cost = 25;
        pills.m_modelName = "Pills";
        pills.m_collisionModelName = "Pills_ColiisionMesh";
        pills.m_physicsShapeType = PhysicsShapeType::CONVEX_MESH;
        pills.m_inventoryInfo.cellSize = 1;
        pills.m_inventoryInfo.combineable = false;
		pills.m_inventoryInfo.discardable = true;
		pills.m_inventoryInfo.equipable = false;
		pills.m_inventoryInfo.usable = true;
        pills.m_examineInfo.rotation = glm::vec3(0.0f, -0.2f, -0.6f);
        pills.m_examineInfo.scale = glm::vec3(8.5f);
        pills.m_examineInfo.maxZoom = 1.75f;
        pills.m_inventoryInfo.heading = "RELIEF PILLS";
        pills.m_inventoryInfo.description = R"(Not a cure, but enough to keep you standing.)";
        pills.m_healInfo.amount = 10;

        // The kind of key someone swore they destroyed.
        // A spare for something important. Or something you were never meant to open.
        // Someone carried this close for a long time. Probably for a reason.


		ItemInfo& p90 = CreateInventoryItemInfo(Item::P90, "P90");
		p90.m_type = ItemType::WEAPON;
		p90.m_mass = 2.0f;
		p90.m_cost = 600;
		p90.m_modelName = "Weapon_P90";
		p90.m_collisionModelName = "CollisionMesh_AKS74U";
		p90.m_physicsShapeType = PhysicsShapeType::CONVEX_MESH;
		p90.m_inventoryInfo.cellSize = 2;
		p90.m_inventoryInfo.combineable = false;
		p90.m_inventoryInfo.discardable = true;
		p90.m_inventoryInfo.equipable = true;
		p90.m_examineInfo.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
		p90.m_examineInfo.scale = glm::vec3(4.5f);
		p90.m_examineInfo.maxZoom = 1.85f;
		p90.m_inventoryInfo.heading = "FN-P90";
		p90.m_inventoryInfo.description = R"(A P90.)";


    }
}
