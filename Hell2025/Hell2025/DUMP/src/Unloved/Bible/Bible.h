#pragma once

#include "Unloved/Animation/AnimationTypes.h"
#include "Unloved/Bible/Info/ItemInfo.h"
#include "Unloved/Inventory/Inventory.h"
#include "Unloved/Objects/ObjectEnums.h"
#include "Unloved/Objects/Renderables/AnimatedMeshNodes.h"
#include "Unloved/Objects/Renderables/MeshNodes.h"
#include "Unloved/Weapons/WeaponCommon.h"

#include <string>
#include <vector>

namespace Unloved {
    struct AnimatorInstance;
    struct HumanoidAnimatorState;
}

namespace Bible {
    using namespace Unloved;

    void Init();
    void ConfigureMeshNodes(uint64_t id, GenericObjectType type, MeshNodes* meshNodes);

    AmmoInfo& CreateAmmoInfo(const std::string& name);
    ItemInfo& CreateInventoryItemInfo(const std::string& name);
    WeaponAttachmentInfo& CreateWeaponAttachmentInfo(const std::string& name);
    WeaponInfo& CreateWeaponInfo(const std::string& name);

    bool AmmoInfoExists(const std::string& name);
    bool ItemInfoExists(const std::string& name);
    bool WeaponAttachmentInfoExists(const std::string& name);
    bool WeaponInfoExists(const std::string& name);

    // Animation
    const std::string& GetAnimation(AnimationProfile animationProfile, AnimationSlot animationSlot);
    HumanoidAnimatorState ConfigureHumanoidAnimator(AnimatorInstance& animatorInstance, AnimationProfile animationProfile);

    // Text
    const std::string& MermaidShopGreeting();
    const std::string& MermaidShopWeaponPurchaseConfirmationText();
    const std::string& MermaidShopFailedPurchaseText();

    // Misc
    void PrintDebugInfo();

    void ConfigureMeshNodesByItemName(uint64_t id, const std::string& itemName, MeshNodes* meshNodes, bool createPhysicsObjects);
    void ConfigureDoorMeshNodes(uint64_t id, DoorCreateInfo& createInfo, MeshNodes* meshNodes);

    void ConfigureTestModel(uint64_t id, MeshNodes* meshNodes);
    void ConfigureTestModel2(uint64_t id, MeshNodes* meshNodes);
    void ConfigureTestModel3(uint64_t id, MeshNodes* meshNodes);
    void ConfigureTestModel4(uint64_t id, MeshNodes* meshNodes);

    void ConfigureAnimatedMeshNodes(uint64_t id, AnimatedMeshNodes* meshNodes, SkinnedModelPreset preset);
    void ConfigureAnimatedMeshNodes(uint64_t id, AnimatedMeshNodes* meshNodes, const std::string& presetName);
    std::vector<std::string> GetAnimatedMeshNodePresetNames();

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

    const std::vector<std::string>& GetAmmoNameList();
    const std::vector<std::string>& GetWeaponNameList();

    AmmoInfo* GetAmmoInfoByName(const std::string& name);
    ItemInfo* GetItemInfoByName(const std::string& name);
    WeaponInfo* GetWeaponInfoByName(const std::string& name);
    WeaponAttachmentInfo* GetWeaponAttachmentInfoByName(const std::string& name);

    int GetInventoryItemSizeByName(const std::string& name);
    int32_t GetWeaponIndexFromWeaponName(const std::string& weaponName);
    int32_t GetWeaponMagSize(const std::string& name);
    int32_t GetAmmoPickUpAmount(const std::string& name);
    float GetItemMass(const std::string& name);
    ItemType GetItemType(const std::string& name);

    int GetPlayerKillCashReward();
    int GetPlayerHeadShotCashReward();

    int GetItemCost(const std::string& name);
}
