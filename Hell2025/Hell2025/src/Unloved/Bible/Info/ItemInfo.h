#pragma once

#include "Hell/Common/Constants.h"

#include "Unloved/Bible/Bible_enums.h"
#include "Unloved/Common/Enums.h"
#include "Unloved/Common/Types.h"
#include "Unloved/Objects/ObjectEnums.h"
#include "Unloved/Physics/PhysicsEnums.h"

#include <glm/vec3.hpp>

#include <string>

namespace Unloved::Bible {

struct ItemInfo {
    Item m_item = Item::UNDEFINED;
    std::string m_name = UNDEFINED_STRING;
    std::string m_modelName = UNDEFINED_STRING;
    std::string m_collisionModelName = UNDEFINED_STRING;
    PhysicsShapeType m_physicsShapeType = PhysicsShapeType::BOX;
    ItemType m_type = ItemType::USELESS;
    Ammo m_ammo = Ammo::UNDEFINED;
    float m_mass = 1.0f;
    int m_cost = 1;

    struct ExamineInfo {
        glm::vec3 translation = glm::vec3(0.0f);
        glm::vec3 rotation = glm::vec3(0.0);
        glm::vec3 scale = glm::vec3(1.0);
        float maxZoom = 2.0f;
    } m_examineInfo;

    struct HealInfo {
        int amount = 0;
    } m_healInfo;

    struct InventoryInfo {
        std::string heading = UNDEFINED_STRING;
        std::string description = UNDEFINED_STRING;
        int cellSize = 1;
        bool combineable = false;
		bool discardable = true;
		bool equipable = true;
		bool usable = false;
    } m_inventoryInfo;

    struct PickUpInfo {
        std::string name = UNDEFINED_STRING;
        std::string subName = UNDEFINED_STRING;
    } m_pickUpInfo;

    struct WeaponInfo {
        std::string casingModelName = UNDEFINED_STRING;
        std::string casingMaterialName = UNDEFINED_STRING;
    } m_weaponInfo;

    bool IsCombineable() const                          { return m_inventoryInfo.combineable; }
	bool IsDiscardable() const                          { return m_inventoryInfo.discardable; }
	bool IsEquipable() const                            { return m_inventoryInfo.equipable; }
	bool IsUsable() const                               { return m_inventoryInfo.usable; }
    int GetCellSize() const                             { return m_inventoryInfo.cellSize; }
    int GetCost() const                                 { return m_cost; }
    float GetMass() const                               { return m_mass; }
    const std::string& GetCasingModelName() const       { return m_weaponInfo.casingModelName; }
    const std::string& GetCasingMaterialName() const    { return m_weaponInfo.casingMaterialName; }
    const std::string& GetCollisionModelName() const    { return m_collisionModelName; }
    const std::string& GetModelName() const             { return m_modelName; }
    const std::string& GetName() const                  { return m_name; }
    Ammo GetAmmo() const                                { return m_ammo; }
    Item GetItem() const                                { return m_item; }
    ItemType GetType() const                            { return m_type; }
    PhysicsShapeType GetPhysicsShapeType() const        { return m_physicsShapeType ;}
};

}
