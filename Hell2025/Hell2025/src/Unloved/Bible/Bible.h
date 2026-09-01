#pragma once

#include "Unloved/Bible/Bible_enums.h"
#include "Unloved/Bible/Info/ItemInfo.h"
#include "Unloved/Objects/ObjectEnums.h"
#include "Unloved/Objects/Renderables/MeshNodes.h"
#include "Unloved/Weapons/WeaponCommon.h"

#include <string>
#include <vector>

namespace Unloved {
    struct SkinnedGameObject;
}

namespace Unloved::Bible {

    struct HumanoidInfo {
        std::vector<std::string> lowerBodyBoneMasks;
        std::vector<std::string> chestBoneMasks;
        std::vector<std::string> upperBodyBoneMasks;
    };

    void Init();
    void ConfigureMeshNodes(uint64_t id, GenericObjectType type, MeshNodes* meshNodes);

    ItemInfo& CreateInventoryItemInfo(Item item, const std::string& name);
    WeaponAttachmentInfo& CreateWeaponAttachmentInfo(const std::string& name);
    WeaponInfo& CreateWeaponInfo(const std::string& name);

    bool ItemInfoExists(Item item);
    bool ItemInfoExists(const std::string& name);
    bool WeaponAttachmentInfoExists(const std::string& name);
    bool WeaponInfoExists(const std::string& name);

    // Animation
    const std::string& GetAnimation(AnimationProfile animationProfile, AnimationSlot animationSlot);
    bool HasAnimation(AnimationProfile animationProfile, AnimationSlot animationSlot);

    // Characters
    const HumanoidInfo* GetHumanoidInfo(SkinnedModelPreset bodyPreset);
    AnimationProfile GetHumanoidAnimationProfile(SkinnedModelPreset bodyPreset, Weapon weapon);

    // Text
    const std::string& MermaidShopGreeting();
    const std::string& MermaidShopWeaponPurchaseConfirmationText();
    const std::string& MermaidShopFailedPurchaseText();

    // Misc
    void PrintDebugInfo();

    void ConfigureMeshNodesByItem(uint64_t id, Item item, MeshNodes* meshNodes, bool createPhysicsObjects);
    void ConfigureDoorMeshNodes(uint64_t id, DoorCreateInfo& createInfo, MeshNodes* meshNodes);

    void ConfigureTestModel(uint64_t id, MeshNodes* meshNodes);
    void ConfigureTestModel2(uint64_t id, MeshNodes* meshNodes);
    void ConfigureTestModel3(uint64_t id, MeshNodes* meshNodes);
    void ConfigureTestModel4(uint64_t id, MeshNodes* meshNodes);

    void ConfigureSkinnedModel(SkinnedGameObject& object, SkinnedModelPreset preset);
    const std::vector<std::string>& GetSkinnedModelPresetNames();

    // Generic Objects
    void ConfigureMeshNodesChristmasPresentSmall(uint64_t id, MeshNodes* meshNodes);
    void ConfigureMeshNodesChristmasPresentLarge(uint64_t id, MeshNodes* meshNodes);
    void ConfigureMeshNodesChristmasTree(uint64_t id, MeshNodes* meshNodes);
    void ConfigureMeshNodesChairRE(uint64_t id, MeshNodes* meshNodes);
    void ConfigureMeshNodesChairSpindleBack(uint64_t id, MeshNodes* meshNodes);
    void ConfigureMeshNodesCouch(uint64_t, MeshNodes* meshNodes);
    void ConfigureMeshNodesDeerHead(uint64_t id, MeshNodes* meshNodes);
    void ConfigureMeshNodesDrawersSmall(uint64_t id, MeshNodes* meshNodes);
    void ConfigureMeshNodesDrawersLarge(uint64_t id, MeshNodes* meshNodes);
    void ConfigureMeshNodesMermaidRock(uint64_t id, MeshNodes* meshNodes);
    void ConfigureMeshNodesPlantBlackBerries(uint64_t id, MeshNodes* meshNodes);
    void ConfigureMeshNodesPlantTree(uint64_t id, MeshNodes* meshNodes);
    void ConfigureMeshNodesToilet(uint64_t id, MeshNodes* meshNodes);
    void ConfigureMeshNodesBathroomBasin(uint64_t id, MeshNodes* meshNodes);
    void ConfigureMeshNodesBathroomCabinet(uint64_t id, MeshNodes* meshNodes);

    // Weapons
	void ConfigureP90MagazineMeshNodes(uint64_t id, MeshNodes* meshNodes);

    const std::vector<std::string>& GetWeaponNameList();

    const AmmoInfo* GetAmmoInfo(Ammo ammo);
    ItemInfo* GetItemInfo(Item item);
    ItemInfo* GetItemInfoByName(const std::string& name);
    WeaponInfo* GetWeaponInfo(Weapon weapon);
    WeaponInfo* GetWeaponInfoByName(const std::string& name);
    WeaponAttachmentInfo* GetWeaponAttachmentInfoByName(const std::string& name);

    int GetInventoryItemSizeByName(const std::string& name);
    int32_t GetWeaponIndexFromWeaponName(const std::string& weaponName);
    int32_t GetWeaponMagSize(const std::string& name);
    float GetItemMass(const std::string& name);
    ItemType GetItemType(const std::string& name);

    int GetPlayerKillCashReward();
    int GetPlayerHeadShotCashReward();

    int GetItemCost(const std::string& name);
    Item GetItemByName(const std::string& name);
    const std::string& GetItemName(Item item);
}
