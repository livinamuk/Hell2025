#pragma once

#include "Unloved/Animation/AnimationTypes.h"
#include "Unloved/Common/Constants.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace Unloved {

enum class WeaponType {
    MELEE,
    PISTOL,
    SHOTGUN,
    AUTOMATIC,
    UNDEFINED
};

struct AnimationCancelFrames {
    int fire = 0;
    int fireAutoShotgun = 0;
    int dryFire = 0;
    int reload = 0;
    int reloadFromEmpty = 0;
    int draw = 0;
    int adsFire = 0;
    int shotgunMelee = 0;
};

struct MeleeAttackInfo {
    AnimationSlot animationSlot = AnimationSlot::MELEE;
    uint32_t startFrame = 0;
    uint32_t endFrame = 0;
    uint32_t damage = 0;
    float range = 1.5f;
    float width = 0.5f;
    float height = 0.2f;
};

struct AnimationSpeeds {
    float idle = 1.0f;
    float walk = 1.0f;
    float reload = 1.0f;
    float reloadempty = 1.0f;
    float fire = 1.0f;
    float draw = 1.0f;
    float drawFirst = 1.0f;
    float shotgunDrawPump = 1.0f;
    float shotgunReloadStart = 1.0f;
    float shotgunReloadEnd = 1.0f;
    float shotgunReloadEndPump = 1.0f;
    float shotgunReloadOneShell = 1.0f;
    float shotgunReloadTwoShells = 1.0f;
    float shotgunFireNoPump = 1.0f;
    float adsIn = 1.0f;
    float adsOut = 1.0f;
    float adsFire = 1.0;
    float adsWalk = 1.0;
    float adsIdle = 1.0f;
    float melee = 1.0f;
};

struct AudioFiles {
    std::vector<std::string> fire;
    std::vector<std::string> revolverCocks;
    std::string reload;
    std::string reloadEmpty;
    std::string shotgunPump;
    std::string drawFirst;
};

struct WeaponInfo {
    std::string ammoInfoName = UNDEFINED_STRING;
    std::string itemInfoName = UNDEFINED_STRING;
    std::string modelName = UNDEFINED_STRING;
    std::string muzzleFlashBoneName = UNDEFINED_STRING;
    std::string casingEjectionBoneName = UNDEFINED_STRING;
    std::string pistolSlideBoneName = UNDEFINED_STRING;
    AnimationProfile viewWeaponAnimationProfile = AnimationProfile::UNDEFINED;
    AnimationSpeeds animationSpeeds;
    AudioFiles audioFiles;
    WeaponType type;
    std::unordered_map<const char*, const char*> meshMaterials;
    std::unordered_map<unsigned int, const char*> meshMaterialsByIndex;
    std::vector<const char*> hiddenMeshAtStart;
    std::vector<MeleeAttackInfo> meleeAttacks;
    int damage = 0;
    int magSize = 0;
    AnimationCancelFrames animationCancelFrames;
    bool pistolIsAuto = false;
    bool hasAutoSwitch = false;
    float muzzleFlashScale = 1;
    float casingEjectionImpulse = 0.025f;
    float pistolSlideOffset = 0;
    int reloadMagInFrameNumber = 0;
    float reloadEmptyMagInFrameNumber = 0;
    int revolverCockFrameNumber = 0;
    bool relolverStyleReload = false;
    bool emptyReloadRequiresSlideOffset = false;
    bool hasADS = false;
    std::string sightName = UNDEFINED_STRING;
    std::string silencerName = UNDEFINED_STRING;
};

struct WeaponAttachmentInfo {
    // Config
    std::string name = UNDEFINED_STRING;
    std::string modelName = UNDEFINED_STRING;
    std::string boneName = UNDEFINED_STRING;
    std::unordered_map<std::string, std::string> meshMaterialNames;
    std::unordered_map<std::string, std::string> meshEmmisveTextureNames;
    std::vector<std::string> glassMeshNames;

    // Runtime
    void Init();
    // TODO: std::vector<uint32_t> meshIndices;
    // TODO: std::vector<uint32_t> emissiveTextureIndices;
    // TODO: std::vector<Material*> meshMaterials;
    // TODO: std::vector<bool> meshGlassFlags;
};

struct AmmoInfo {
    const char* name = UNDEFINED_STRING;
    const char* convexMeshModelName = UNDEFINED_STRING;
    const char* modelName = UNDEFINED_STRING;
    const char* materialName = UNDEFINED_STRING;
    const char* casingModelName = UNDEFINED_STRING;
    const char* casingMaterialName = UNDEFINED_STRING;
    int pickupAmount = 0;
};

struct WeaponState {
    bool has = false;
    bool requiresSlideOffset = false;
    bool shotgunAwaitingFirstShellReload = false;
    bool shotgunAwaitingSecondShellReload = false;
    bool shotgunRequiresPump = true;
    bool shotgunAwaitingPumpAudio = true;
    bool shotgunShellChambered = false;
    bool shotgunInAutoMode = false;
    bool shotgunSlug = false;
    bool awaitingDrawFirst = true;
    bool awaitingMagReload = false;
    int ammoInMag = 0;
    std::string name = "UNDEFINED_STRING";
    bool hasSight = false;
    bool hasSilencer = false;
};

struct AmmoState {
    std::string name = "UNDEFINED_STRING";
    int ammoOnHand = 0;
};

}
