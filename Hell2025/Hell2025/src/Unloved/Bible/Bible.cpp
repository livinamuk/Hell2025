#include "Bible.h"
#include "Hell/Common/Enum.h"
#include "Hell/Logging.h"

#include <algorithm>
#include <unordered_map>

#include <iostream> // TODO: cleanup logging

namespace Unloved::Bible {

    std::unordered_map<Item, ItemInfo> g_inventoryItemInfos;
    std::unordered_map<std::string, WeaponAttachmentInfo> g_weaponAttachmentInfos;
    std::unordered_map<std::string, WeaponInfo> g_weaponsInfos;

    std::vector<std::string> g_sortedWeaponNames;  // Sorted by weapon type, then damage

    void InitAmmoCatalog();
    void InitInventoryItemInfo();
    void InitWeaponInfo();
    void InitWeaponAttachmentInfo();
    void InitAnimationCatalog();
    void InitHumanoidCatalog();
    void CreateSortedWeaponNameList();
    void Validate();

    void Init() {
        g_inventoryItemInfos.clear();
        g_weaponAttachmentInfos.clear();
        g_weaponsInfos.clear();
        g_sortedWeaponNames.clear();

        InitAnimationCatalog();
        InitHumanoidCatalog();
        InitAmmoCatalog();
        InitInventoryItemInfo();
        InitWeaponInfo();
        InitWeaponAttachmentInfo();

        Validate();

        CreateSortedWeaponNameList();

        //PrintDebugInfo();

        Logging::Init() << "The Bible has been read";
    }

    void ConfigureMeshNodes(uint64_t id, GenericObjectType type, MeshNodes* meshNodes) {
        if (!meshNodes) return;

        switch (type) {
            case GenericObjectType::CHRISTMAS_PRESENT_SMALL:    return ConfigureMeshNodesChristmasPresentSmall(id, meshNodes);
            case GenericObjectType::CHRISTMAS_PRESENT_LARGE:    return ConfigureMeshNodesChristmasPresentLarge(id, meshNodes);
            case GenericObjectType::CHRISTMAS_TREE:             return ConfigureMeshNodesChristmasTree(id, meshNodes);
            case GenericObjectType::BATHROOM_BASIN:             return ConfigureMeshNodesBathroomBasin(id, meshNodes);
            case GenericObjectType::BATHROOM_CABINET:           return ConfigureMeshNodesBathroomCabinet(id, meshNodes);
            case GenericObjectType::CHAIR_RE:                   return ConfigureMeshNodesChairRE(id, meshNodes);
            case GenericObjectType::CHAIR_SPINDLE_BACK:         return ConfigureMeshNodesChairSpindleBack(id, meshNodes);
            case GenericObjectType::DEER_HEAD:                  return ConfigureMeshNodesDeerHead(id, meshNodes);
            case GenericObjectType::DRAWERS_SMALL:              return ConfigureMeshNodesDrawersSmall(id, meshNodes);
            case GenericObjectType::DRAWERS_LARGE:              return ConfigureMeshNodesDrawersLarge(id, meshNodes);
            case GenericObjectType::MERMAID_ROCK:               return ConfigureMeshNodesMermaidRock(id, meshNodes);
            case GenericObjectType::TOILET:                     return ConfigureMeshNodesToilet(id, meshNodes);
            case GenericObjectType::COUCH:                      return ConfigureMeshNodesCouch(id, meshNodes);
            case GenericObjectType::PLANT_BLACKBERRIES:         return ConfigureMeshNodesPlantBlackBerries(id, meshNodes);
            case GenericObjectType::PLANT_TREE:                 return ConfigureMeshNodesPlantTree(id, meshNodes);
            case GenericObjectType::TEST_MODEL:                 return ConfigureTestModel(id, meshNodes);
            case GenericObjectType::TEST_MODEL2:                return ConfigureTestModel2(id, meshNodes);
            case GenericObjectType::TEST_MODEL3:                return ConfigureTestModel3(id, meshNodes);
            case GenericObjectType::TEST_MODEL4:                return ConfigureTestModel4(id, meshNodes);

            default: Logging::Error() << "Bible::ConfigureMeshNodes(...) failed: non-implemented GenericObjectType: '" << Hell::Enum::ToString(type) << "'";
        }
    }

    void CreateSortedWeaponNameList() {
        struct TempWeaponName {
            std::string name;
            int damage;
        };

        std::vector<TempWeaponName> melees;
        std::vector<TempWeaponName> pistols;
        std::vector<TempWeaponName> shotguns;
        std::vector<TempWeaponName> automatics;

        for (const auto& [key, value] : g_weaponsInfos) {
            switch (value.type) {
                case WeaponType::MELEE:     melees.push_back(TempWeaponName(key, value.damage));        break;
                case WeaponType::PISTOL:    pistols.push_back(TempWeaponName(key, value.damage));       break;
                case WeaponType::SHOTGUN:   shotguns.push_back(TempWeaponName(key, value.damage));      break;
                case WeaponType::AUTOMATIC: automatics.push_back(TempWeaponName(key, value.damage));    break;
                default: break;
            }
        }

        auto less_than_damage = [](const TempWeaponName& a, const TempWeaponName& b) {
            return a.damage < b.damage;
        };

        std::sort(melees.begin(), melees.end(), less_than_damage);
        std::sort(pistols.begin(), pistols.end(), less_than_damage);
        std::sort(shotguns.begin(), shotguns.end(), less_than_damage);
        std::sort(automatics.begin(), automatics.end(), less_than_damage);

        g_sortedWeaponNames.clear();
        g_sortedWeaponNames.reserve(melees.size() + pistols.size() + shotguns.size() + automatics.size());

        for (const auto& weapon : melees)      g_sortedWeaponNames.push_back(weapon.name);
        for (const auto& weapon : pistols)     g_sortedWeaponNames.push_back(weapon.name);
        for (const auto& weapon : shotguns)    g_sortedWeaponNames.push_back(weapon.name);
        for (const auto& weapon : automatics)  g_sortedWeaponNames.push_back(weapon.name);
    }

    void Validate() {
        // Weapon Info
        for (const auto& [name, value] : g_weaponsInfos) {
            const WeaponInfo& weaponInfo = value;

            // Check ammo
            if (!GetAmmoInfo(weaponInfo.ammo) && weaponInfo.weapon != Weapon::KNIFE) {
                Logging::Warning() << "WeaponInfo '" << name << "' has a not found AmmoInfo '" << Hell::Enum::ToString(weaponInfo.ammo) << "'";
            }

            // Check item info
            if (!ItemInfoExists(name) && weaponInfo.weapon != Weapon::KNIFE) {
                Logging::Warning() << "WeaponInfo '" << name << "' has a not found ItemInfo";
            }
        }
    }

    void PrintDebugInfo() {
        std::cout << "\n** BIBLE **\n";

        std::cout << "\nAmmo\n";
        for (int i = 1; i < Hell::Enum::GetCount<Ammo>(); ++i) {
            std::cout << " " << i << ": " << Hell::Enum::ToString(Hell::Enum::FromInt<Ammo>(i)) << "\n";
        }

        //std::cout << "\nPickups\n";
        //for (const auto& [name, value] : g_pickUpInfos) {
        //    std::cout << " - " << name << "\n";
        //}

        std::cout << "\nWeapons\n";
        for (size_t i = 0; i < g_sortedWeaponNames.size(); ++i) {
            std::cout << " " << i << ": " << g_sortedWeaponNames[i] << "\n";
        }

        std::cout << "\n";
    }

    ItemInfo& CreateInventoryItemInfo(Item item, const std::string& name) {
        ItemInfo& info = g_inventoryItemInfos[item];
        info.m_item = item;
        info.m_name = name;
        return info;
    }

    WeaponAttachmentInfo& CreateWeaponAttachmentInfo(const std::string& name) {
        WeaponAttachmentInfo& info = g_weaponAttachmentInfos[name];
        info.name = name;
        return info;
    }

    WeaponInfo& CreateWeaponInfo(const std::string& name) {
        return g_weaponsInfos[name];
    }

    bool ItemInfoExists(Item item) {
        return g_inventoryItemInfos.contains(item);
    }

    bool ItemInfoExists(const std::string& name) {
        for (const auto& entry : g_inventoryItemInfos) {
            if (entry.second.m_name == name) return true;
        }
        return false;
    }

    bool WeaponAttachmentInfoExists(const std::string& name) {
        return g_weaponAttachmentInfos.find(name) != g_weaponAttachmentInfos.end();
    }

    bool WeaponInfoExists(const std::string& name) {
        return g_weaponsInfos.find(name) != g_weaponsInfos.end();
    }

    int GetItemCost(const std::string& name) {
        ItemInfo* itemInfo = GetItemInfoByName(name);
        if (!itemInfo) return 0;

        return itemInfo->GetCost();
    }

    ItemInfo* GetItemInfo(Item item) {
        auto it = g_inventoryItemInfos.find(item);
        return it == g_inventoryItemInfos.end() ? nullptr : &it->second;
    }

    ItemInfo* GetItemInfoByName(const std::string& name) {
        for (auto& [item, itemInfo] : g_inventoryItemInfos) {
            if (itemInfo.m_name == name) return &itemInfo;
        }

        Logging::Warning() << "Bible::GetInventoryItemInfoByName::(...) failed: '" << name << "' not found\n";
        return nullptr;
    }

    WeaponInfo* GetWeaponInfo(Weapon weapon) {
        for (auto& [name, weaponInfo] : g_weaponsInfos) {
            if (weaponInfo.weapon == weapon) return &weaponInfo;
        }

        Logging::Error() << "Bible::GetWeaponInfo(..) failed: '" << Hell::Enum::ToString(weapon) << "' not found\n";
        return nullptr;
    }

    Item GetItemByName(const std::string& name) {
        ItemInfo* itemInfo = GetItemInfoByName(name);
        return itemInfo ? itemInfo->m_item : Item::UNDEFINED;
    }

    const std::string& GetItemName(Item item) {
        static const std::string undefined = UNDEFINED_STRING;
        ItemInfo* itemInfo = GetItemInfo(item);
        return itemInfo ? itemInfo->m_name : undefined;
    }

    WeaponInfo* GetWeaponInfoByName(const std::string& name) {
        if (WeaponInfoExists(name))
            return &g_weaponsInfos[name];

        Logging::Warning() << "Bible::GetWeaponInfoByName::(...) failed: '" << name << "' not found\n";
        return nullptr;
    }


    WeaponAttachmentInfo* GetWeaponAttachmentInfoByName(const std::string& name) {
        if (WeaponAttachmentInfoExists(name))
            return &g_weaponAttachmentInfos[name];

        Logging::Warning() << "Bible::GetWeaponAttachmentInfoByName::(...) failed: '" << name << "' not found\n";
        return nullptr;
    }

    int32_t GetWeaponIndexFromWeaponName(const std::string& weaponName) {
        for (int i = 0; i < g_sortedWeaponNames.size(); i++) {
            if (g_sortedWeaponNames[i] == weaponName) {
                return i;
            }
        }
        return -1;
    }

    int GetInventoryItemSizeByName(const std::string& name) {
        if (ItemInfo* itemInfo = GetItemInfoByName(name))
            return itemInfo->m_inventoryInfo.cellSize;

        return 0;
    }

    const std::vector<std::string>& GetWeaponNameList() {
        return g_sortedWeaponNames;
    }
}
