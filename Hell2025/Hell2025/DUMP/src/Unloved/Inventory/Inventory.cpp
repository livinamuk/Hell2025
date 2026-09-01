#include "Inventory.h"

#include "Hell/Logging.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/UI/TextBlitter.h"
#include "Hell/UI/UIBackEnd.h"

#include "Unloved/Bible/Bible.h"

#include <iostream> // TODO: cleanup logging

namespace Unloved {

void Inventory::Init() {
    CloseInventory();
    ClearInventory();
}

void Inventory::CleanUp() {
    m_examineItemMeshNodes.CleanUp();
}

void Inventory::OpenInventory() {
    GoToMainScreen();
}

void Inventory::OpenAsShop() {
    SetState(InventoryState::SHOP);
    m_selectedCellX = 0;
    m_selectedCellY = 0;
    m_examineRotationX = 0.0f;
    m_examineRotationY = 0.0f;
    m_examineZoom = 0.0f;
}

void Inventory::GoToMainScreen() {
    SetState(InventoryState::MAIN_SCREEN);
    m_selectedCellX = 0;
    m_selectedCellY = 0;
    m_examineRotationX = 0.0f;
    m_examineRotationY = 0.0f;
    m_examineZoom = 0.0f;
}

void Inventory::CloseInventory() {
    SetState(InventoryState::CLOSED);
}

void Inventory::AddInventoryItem(const std::string& name) {
    ItemInfo* itemInfo = Bible::GetItemInfoByName(name);
    if (!itemInfo) return;

    // Find the next free location
    glm::ivec2 nextFreeLocation = GetNextFreeLocation(itemInfo->m_inventoryInfo.cellSize);

    // Oh there wasn't one?
    if (nextFreeLocation == glm::ivec2(-1, -1)) {
        std::cout << "NO FREE SPACE FOR ITEM '" << name << "'\n";
        return;
    }

    // Yay, there was, insert it!
    InventoryItem item;
    item.m_name = name;
    item.m_gridLocation = nextFreeLocation;
    m_items.push_back(item);

    UpdateOccupiedSlotsArray();
}

void Inventory::SetGridCountX(int count) {
    m_gridCountX = count;
}

void Inventory::SetGridCountY(int count) {
    m_gridCountY = count;
}

void Inventory::SetLocalPlayerIndex(int localPlayerIndex) {
    m_localPlayerIndex = localPlayerIndex;
}

void Inventory::SetState(InventoryState state) {
    m_state = state;
}

bool Inventory::MoveItem(int itemIndex, int cellX, int cellY, bool rotated) {
    InventoryItem& item = m_items[itemIndex];
    item.m_gridLocation = glm::ivec2(cellX, cellY);
    item.m_rotatedInGrid = true;

    UpdateOccupiedSlotsArray();
    return true;
}

void Inventory::ClearInventory() {
    m_items.clear();
    m_weaponStates.clear();
    m_ammoStates.clear();

    // Reset weapon states
    for (const std::string& name : Bible::GetWeaponNameList()) {
        WeaponState& state = m_weaponStates.emplace_back();
        state.name = name;
        state.has = false;
        state.ammoInMag = 0;
    }

    // Reset ammo states
    m_ammoStates.clear();
    for (const std::string& name : Bible::GetAmmoNameList()) {
        AmmoState& state = m_ammoStates.emplace_back();
        state.name = name;
        state.ammoOnHand = 0;
    }

    UpdateOccupiedSlotsArray();
}

int Inventory::GetSelectedItemCost() {
    if (!ItemSelected()) return 0;
    return Bible::GetItemCost(GetSelectedItemName());
}

glm::ivec2 Inventory::GetSelectedItemHeadingSize() {
    if (!ItemSelected()) return glm::ivec2(0, 0);
    return TextBlitter::GetTextSize(GetSelectedItemHeading(), m_style.itemHeadingFont, 1.0f);
}

glm::ivec2 Inventory::GetSelectedItemDescriptionSize() {
    if (!ItemSelected()) return glm::ivec2(0, 0);
    return TextBlitter::GetTextSize(GetSelectedItemDescription(), m_style.itemDescriptionFont, 1.0f);
}

int Inventory::GetCellSizeInPixels() {
    static int size = 0;
    if (size == 0) {
        if (Texture* texture = Hell::ResourceManager::GetTextureByName("InvSquare_Size1")) {
            size = texture->GetWidth();
        }
    }
    return size;
}

bool Inventory::HasItem(const std::string& itemName) {
    for (InventoryItem& item : m_items) {
        if (item.m_name == itemName) {
            return true;
        }
    }
    return false;
}

void Inventory::RemoveItemByIndex(int index) {
    if (index >= 0 && index < m_items.size()) {
        m_items.erase(m_items.begin() + index);
		UpdateOccupiedSlotsArray();
    }
    else {
        Logging::Error() << "Inventory::RemoveItemByIndex(..) failed to remove item with index '" << index << "' from m_items of size '" << m_items.size() << "'\n";
    }
}

glm::ivec2 Inventory::GetItemGridSize() {
    return GetCellSizeInPixels() * glm::ivec2(m_gridCountX, m_gridCountY);
}

int Inventory::GetSelectedItemIndex() {
    if (!InBounds(m_selectedCellX, m_selectedCellY)) {
        return -1;
    }
    else {
        return m_itemIndex2DArray[m_selectedCellX][m_selectedCellY];
    }
}


const std::string& Inventory::GetSelectedItemName() {
    static std::string noItem = "NO_ITEM";

    InventoryItem* selectedItem = GetItemAtIndex(GetSelectedItemIndex());
    if (!selectedItem) return noItem;

    return selectedItem->m_name;
}

InventoryItem* Inventory::GetItemAtIndex(int index) {
    if (index < 0 || index >= m_items.size()) return nullptr;
    else return &m_items[index];
}

ItemInfo* Inventory::GetSelectedItemInfo() {
    int selectedItemIndex = GetSelectedItemIndex();
    if (selectedItemIndex == -1) return nullptr; // nothing at this slot

    InventoryItem* selectedItem = GetItemAtIndex(selectedItemIndex);
    if (!selectedItem) return nullptr; // This should never happen

    return Bible::GetItemInfoByName(selectedItem->m_name);
}

const std::string& Inventory::GetItemNameAtLocation(int x, int y) {
    static std::string noItem = "NO_ITEM";

    if (!InBounds(x, y)) {
        return noItem;
    }

    int itemIndex = m_itemIndex2DArray[x][y];

    if (itemIndex < 0 || itemIndex >= (int)m_items.size()) { // -1 is no item, and it should never be larger than size, but you never know.
        return noItem;
    }
    else {
        return m_items[itemIndex].m_name;
    }
}

const std::string& Inventory::GetSelectedItemHeading() {
    static std::string noItem = "";

    ItemInfo* itemInfo = GetSelectedItemInfo();
    if (!itemInfo) return noItem;

    return itemInfo->m_inventoryInfo.heading;
}

const std::string& Inventory::GetSelectedItemDescription() {
    static std::string noItem = "";

    ItemInfo* itemInfo = GetSelectedItemInfo();
    if (!itemInfo) return noItem;

    return itemInfo->m_inventoryInfo.description;
}

int Inventory::GetItemSizeAtLocation(int x, int y) {
    if (!InBounds(x, y)) {
        return 1;
    }
    const std::string& itemName = GetItemNameAtLocation(x, y);

    if (itemName != "NO_ITEM") {
        return Bible::GetInventoryItemSizeByName(itemName);
    }
    else {
        return 1;
    }
}

bool Inventory::IsCellOccupied(int x, int y) {
    if (!InBounds(x, y)) {
        return true;
    }
    else {
        return m_itemIndex2DArray[x][y] != -1;
    }
}

void Inventory::UpdateOccupiedSlotsArray() {
    // Initialize to empty
    for (int x = 0; x < MAX_INVENTORY_X_SIZE; x++)
        for (int y = 0; y < MAX_INVENTORY_Y_SIZE; y++)
            m_itemIndex2DArray[x][y] = -1;

    // Mark occupied cells
    for (int i = 0; i < (int)m_items.size(); i++) {
        InventoryItem& item = m_items[i];
        ItemInfo* itemInfo = Bible::GetItemInfoByName(item.m_name);
        if (!itemInfo) continue;

        int w = itemInfo->GetCellSize(); // horizontal width by default
        int h = 1;
        int originX = item.m_gridLocation.x;
        int originY = item.m_gridLocation.y;

        if (item.m_rotatedInGrid) {
            // vertical footprint; anchor is BOTTOM cell
            std::swap(w, h);                              // w=1, h=size
            originY -= (itemInfo->GetCellSize()  - 1);    // move origin to the TOP cell
        }

        for (int dx = 0; dx < w; ++dx) {
            for (int dy = 0; dy < h; ++dy) {
                int x = originX + dx;
                int y = originY + dy;
                if (InBounds(x, y)) {
                    m_itemIndex2DArray[x][y] = i;
                }
            }
        }
    }
}

glm::ivec2 Inventory::GetNextFreeLocation(int itemCellSize) {
    UpdateOccupiedSlotsArray();

    int w = itemCellSize;
    int h = 1; // horizontal for now

    for (int y = 0; y < m_gridCountY; y++) {
        for (int x = 0; x <= m_gridCountX - w; x++) {
            bool fits = true;
            for (int dx = 0; dx < w && fits; ++dx) {
                for (int dy = 0; dy < h && fits; ++dy) {
                    if (IsCellOccupied(x + dx, y + dy)) {
                        fits = false;
                        break;
                    }
                }
            }
            if (fits) {
                return glm::ivec2(x, y);
            }
        }
    }

    // No space available
    return glm::ivec2(-1, -1);
}

void Inventory::PrintGridOccupiedStateToConsole() {
    std::cout << " ";
    for (int y = 0; y < m_gridCountY; y++) {
        for (int x = 0; x < m_gridCountX; x++) {
            std::cout << "[";
            if (m_itemIndex2DArray[x][y] != -1) {
                std::cout << "X";
            }
            else {
                std::cout << " ";
            }
            std::cout << "]";
        }
        std::cout << "\n ";
    }
}

bool Inventory::InBounds(int x, int y) {
    return x >= 0 && y >= 0 && x < m_gridCountX && y < m_gridCountY;
}

bool Inventory::ItemSelected() {
    return GetItemAtIndex(GetSelectedItemIndex()) != nullptr;
}

void Inventory::GiveAmmo(const std::string& name, int amount) {
    if (amount == 0) {
        Logging::Warning() << "Inventory::GiveAmmo(..) tried to give 0 '" << name << "' ammo";
        return;
    }

    for (AmmoState& ammoState : m_ammoStates) {
        if (ammoState.name == name) {
            ammoState.ammoOnHand += amount;
            return;
        }
    }

    AddInventoryItem(name);

    Logging::Warning() << "Inventory::GiveAmmo(..) failed: '" << name << "' not found in m_ammoStates";
}


//void Inventory::GiveItem(const std::string& name) {
//
//}

void Inventory::GiveWeapon(const std::string& name) {
    for (WeaponState& weaponState : m_weaponStates) {
        if (weaponState.name == name) {

            // If you don't already have it
            if (!weaponState.has) {
                weaponState.has = true;
                weaponState.ammoInMag = Bible::GetWeaponMagSize(name);
                AddInventoryItem(name);
            }
            // If you do already have it
            else {
                if (WeaponInfo* weaponInfo = Bible::GetWeaponInfoByName(name)) {
                    GiveAmmo(weaponInfo->ammoInfoName, Bible::GetWeaponMagSize(name));
                }
            }
            return;
        }
    }

    Logging::Warning() << "Inventory::GiveWeapon(..) failed: '" << name << "' not found in g_weaponStates";
}

WeaponState* Inventory::GetWeaponStateByName(const std::string& name) {
    for (int i = 0; i < m_weaponStates.size(); i++) {
        if (m_weaponStates[i].name == name) {
            return &m_weaponStates[i];
        }
    }
    return nullptr;
}

AmmoState* Inventory::GetAmmoStateByName(const std::string& name) {
    for (int i = 0; i < m_ammoStates.size(); i++) {
        if (m_ammoStates[i].name == name) {
            return &m_ammoStates[i];
        }
    }
    return nullptr;
}

}
