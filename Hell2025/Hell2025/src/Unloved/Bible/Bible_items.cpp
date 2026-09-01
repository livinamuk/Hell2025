#include "Unloved/Bible/Bible.h"

namespace Unloved::Bible {

    float GetItemMass(const std::string& name) {
        if (ItemInfo* itemInfo = GetItemInfoByName(name)) {
            return itemInfo->GetMass();
        }
        return 0;
    }

    ItemType GetItemType(const std::string& name) {
        if (ItemInfo* itemInfo = GetItemInfoByName(name)) {
            return itemInfo->GetType();
        }
        return ItemType::UNDEFINED;
    }
}
