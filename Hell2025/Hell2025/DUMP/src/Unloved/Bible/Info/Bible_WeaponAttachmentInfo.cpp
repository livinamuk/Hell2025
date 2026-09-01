#include "Unloved/Bible/Bible.h"
#include "Hell/Logging.h"
#include <unordered_map>

namespace Bible {

    void InitWeaponAttachmentInfo() {
        WeaponAttachmentInfo& glockSilencer = CreateWeaponAttachmentInfo("GLOCK_SILENCER");
        glockSilencer.modelName = "Glock_Silencer";
        glockSilencer.boneName = "Suppressor";
        glockSilencer.meshMaterialNames["Glock_silencer"] = "Glock_Silencer";

        WeaponAttachmentInfo& glockRedDot = CreateWeaponAttachmentInfo("GLOCK_RED_DOT");
        glockRedDot.modelName = "Glock_RedDot";
        glockRedDot.boneName = "Sight";
        glockRedDot.meshMaterialNames["RedDotSight"] = "Glock_RedDot";
        glockRedDot.meshMaterialNames["RedDotSightGlass"] = "Glock_RedDot";
        glockRedDot.meshEmmisveTextureNames["RedDotSight"] = "Glock_RedDot_EMI";
        glockRedDot.glassMeshNames.push_back("RedDotSightGlass");
    }
}
