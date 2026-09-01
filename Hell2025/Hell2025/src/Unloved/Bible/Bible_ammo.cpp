#include "Bible.h"

#include <map>

namespace Unloved::Bible {

    namespace {
        std::map<Ammo, AmmoInfo> g_ammoInfos;
    }

    void InitAmmoCatalog() {
        g_ammoInfos.clear();

        AmmoInfo& glock = g_ammoInfos[Ammo::GLOCK];
        glock.casingModelName = "Casing9mm";
        glock.casingMaterialName = "Casing9mm";
        glock.pickupAmount = 50;

        AmmoInfo& tokarev = g_ammoInfos[Ammo::TOKAREV];
        tokarev.casingModelName = "Casing9mm";
        tokarev.casingMaterialName = "Casing9mm";
        tokarev.pickupAmount = 50;

        AmmoInfo& aks74u = g_ammoInfos[Ammo::AKS74U];
        aks74u.casingModelName = "CasingAKS74U";
        aks74u.casingMaterialName = "Casing_AkS74U";
        aks74u.pickupAmount = 666;

        AmmoInfo& shotgunShells = g_ammoInfos[Ammo::SHOTGUN_SHELLS];
        shotgunShells.casingModelName = "Shell";
        shotgunShells.casingMaterialName = "Shell";
        shotgunShells.pickupAmount = 20;

        AmmoInfo& p90 = g_ammoInfos[Ammo::P90];
        p90.casingModelName = "CasingAKS74U";
        p90.casingMaterialName = "Casing_AkS74U";
        p90.pickupAmount = 666;
    }

    const AmmoInfo* GetAmmoInfo(Ammo ammo) {
        auto it = g_ammoInfos.find(ammo);
        return it == g_ammoInfos.end() ? nullptr : &it->second;
    }
}
