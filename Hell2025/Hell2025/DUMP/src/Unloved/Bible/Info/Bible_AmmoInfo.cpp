#include "Unloved/Bible/Bible.h"
#include "Hell/Logging.h"
#include <unordered_map>

namespace Bible {

    void InitAmmoInfo() {
        AmmoInfo& glockAmmo = CreateAmmoInfo("Glock");
        glockAmmo.name = "Glock";
        glockAmmo.modelName = "GlockAmmoBox";
        glockAmmo.convexMeshModelName = "GlockAmmoBox_ConvexMesh";
        glockAmmo.materialName = "GlockAmmoBox";
        glockAmmo.casingModelName = "Casing9mm";
        glockAmmo.casingMaterialName = "Casing9mm";
        glockAmmo.pickupAmount = 50;
        
        AmmoInfo& tokarevAmmo = CreateAmmoInfo("Tokarev");
        tokarevAmmo.modelName = "TokarevAmmoBox";
        tokarevAmmo.convexMeshModelName = "TokarevAmmoBox_ConvexMesh";
        tokarevAmmo.materialName = "TokarevAmmoBox";
        tokarevAmmo.casingModelName = "Casing9mm";
        tokarevAmmo.casingMaterialName = "Casing9mm";
        tokarevAmmo.pickupAmount = 50;
        
        AmmoInfo& smithAmmo = CreateAmmoInfo("Smith");
        smithAmmo.modelName = "YOU NEED THIS MODEL";
        smithAmmo.convexMeshModelName = "YOU NEED THIS MODEL";
        smithAmmo.materialName = "YOU NEED THIS MATERIAL";
        smithAmmo.casingModelName = "None";
        smithAmmo.casingMaterialName = "None";
        smithAmmo.pickupAmount = 50;
        
        AmmoInfo& aks74uAmmo = CreateAmmoInfo("AKS74U");
        aks74uAmmo.modelName = "TODO!!!";
        aks74uAmmo.convexMeshModelName = "TODO!!!";
        aks74uAmmo.materialName = "TODO!!!";
        aks74uAmmo.pickupAmount = 666;
        aks74uAmmo.casingModelName = "CasingAKS74U";
        aks74uAmmo.casingMaterialName = "Casing_AkS74U";
        
        AmmoInfo& shotgunBuckShotAmmo = CreateAmmoInfo("12GaugeBuckShot");
        shotgunBuckShotAmmo.modelName = "TODO!!!";
        shotgunBuckShotAmmo.convexMeshModelName = "TODO!!!";
        shotgunBuckShotAmmo.materialName = "TODO!!!";
        shotgunBuckShotAmmo.pickupAmount = 20;
        shotgunBuckShotAmmo.casingModelName = "Shell";
        shotgunBuckShotAmmo.casingMaterialName = "Shell";
        
        AmmoInfo& p90Ammo = CreateAmmoInfo("P90");
        p90Ammo.modelName = "TODO!!!";
        p90Ammo.convexMeshModelName = "TODO!!!";
        p90Ammo.materialName = "TODO!!!";
        p90Ammo.pickupAmount = 666;
        p90Ammo.casingModelName = "CasingAKS74U";
        p90Ammo.casingMaterialName = "Casing_AkS74U";
    }

    int32_t GetAmmoPickUpAmount(const std::string& name) {
        if (AmmoInfo* ammoInfo = GetAmmoInfoByName(name)) {
            return ammoInfo->pickupAmount;
        }
        return 0;
    }
}
