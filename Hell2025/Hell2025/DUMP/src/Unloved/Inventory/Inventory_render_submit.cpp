#include "Inventory.h"

#include "Hell/Logging.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/UI/TextBlitter.h"
#include "Hell/UI/UIBackEnd.h"

#include "Unloved/Bible/Bible.h"
#include "Unloved/Config/Config.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Viewport/ViewportManager.h"

#include <iostream> // TODO: cleanup logging

namespace Unloved {

void Inventory::SubmitRenderItems() {
    if (m_state == InventoryState::MAIN_SCREEN) {
        m_locations.background = glm::ivec2(m_style.invOriginX, m_style.invOriginY);
        m_locations.itemGrid = m_locations.background + glm::ivec2(m_style.gridMargin);
        m_locations.theLine = m_locations.itemGrid + glm::ivec2(0, GetItemGridSize().y + m_style.theLinePadding);
        m_locations.itemHeading = m_locations.theLine + glm::ivec2(0, m_style.itemHeadingTopPadding);
        m_locations.itemDescription = m_locations.itemHeading + glm::ivec2(0, GetSelectedItemHeadingSize().y + m_style.itemDescriptionTopPadding);
        m_locations.itemButtons = m_locations.itemDescription + glm::ivec2(0, GetSelectedItemDescriptionSize().y + m_style.itemButtonsTopPadding);

        int width = GetItemGridSize().x + (m_style.gridMargin * 2);
        int height = 880;

        if (Unloved::Session::GetSplitscreenMode() == SplitscreenMode::TWO_PLAYER) {
            const Resolutions& resolutions = Config::GetResolutions();
            int halfScreenHeight = resolutions.ui.y * 0.5f;

            int magicExtraWidth = 400;
            width = GetItemGridSize().x + (m_style.gridMargin * 2) + magicExtraWidth;
            height = GetItemGridSize().y + (m_style.gridMargin * 2);

            int newInvOriginX = (halfScreenHeight - height) * 0.5f;
            int newInvOriginY = (halfScreenHeight - height) * 0.5f;

            if (m_localPlayerIndex == 0) {
                m_locations.background = glm::ivec2(newInvOriginX, newInvOriginY);
            }
            if (m_localPlayerIndex == 1) {
                m_locations.background = glm::ivec2(newInvOriginX, newInvOriginY + (halfScreenHeight));
            }

            m_locations.itemGrid = m_locations.background + glm::ivec2(m_style.gridMargin);
            m_locations.theLine = m_locations.itemGrid + glm::ivec2(GetItemGridSize().x + m_style.theLinePadding, 0);
            m_locations.itemHeading = m_locations.theLine + glm::ivec2(0, m_style.itemHeadingTopPadding);
            m_locations.itemDescription = m_locations.itemHeading + glm::ivec2(0, GetSelectedItemHeadingSize().y + m_style.itemDescriptionTopPadding);
            m_locations.itemButtons = m_locations.itemDescription + glm::ivec2(0, GetSelectedItemDescriptionSize().y + m_style.itemButtonsTopPadding);
        }

        BlitInventoryBackground(m_locations.background, width, height);
        BlitItemGrid(m_locations.itemGrid);
        BlitTheLine(m_locations.theLine);
        BlitItemHeading(m_locations.itemHeading);
        BlitItemDescription(m_locations.itemDescription);
        BlitItemButtons(m_locations.itemButtons);
    }

    if (m_state == InventoryState::EXAMINE_ITEM) SubmitItemExamineRenderItems();


    if (m_state == InventoryState::SHOP) {
        //m_locations.background = glm::ivec2(m_style.invOriginX, m_style.invOriginY);
        //m_locations.itemGrid = m_locations.background + glm::ivec2(m_style.gridMargin);
        //m_locations.theLine = m_locations.itemGrid + glm::ivec2(0, GetItemGridSize().y + m_style.theLinePadding);
        //m_locations.itemHeading = m_locations.theLine + glm::ivec2(0, m_style.itemHeadingTopPadding);
        //m_locations.itemDescription = m_locations.itemHeading + glm::ivec2(0, GetSelectedItemHeadingSize().y + m_style.itemDescriptionTopPadding);
        //m_locations.itemButtons = m_locations.itemDescription + glm::ivec2(0, GetSelectedItemDescriptionSize().y + m_style.itemButtonsTopPadding);

        const Resolutions& resolutions = Config::GetResolutions();
        int halfScreenHeight = resolutions.ui.y * 0.5f;
        int screenWidth = resolutions.ui.x;
        int screenHeight = resolutions.ui.y;

        int magicExtraWidth = 400;
        int width = GetItemGridSize().x + (m_style.gridMargin * 2) + magicExtraWidth;
        int height = GetItemGridSize().y + (m_style.gridMargin * 2);

        int newInvOriginX = screenWidth * 0.475f;
        int newInvOriginY = screenHeight * 0.125f;
        newInvOriginY = screenHeight * 0.25f;

        if (Unloved::Session::GetSplitscreenMode() == SplitscreenMode::TWO_PLAYER) {
            newInvOriginX = screenWidth * 0.475f;
            newInvOriginY = (halfScreenHeight - height) * 0.5f;
        }

        if (m_localPlayerIndex == 0) {
            m_locations.background = glm::ivec2(newInvOriginX, newInvOriginY);
        }
        if (m_localPlayerIndex == 1) {
            m_locations.background = glm::ivec2(newInvOriginX, newInvOriginY + (halfScreenHeight));
        }

        m_locations.itemGrid = m_locations.background + glm::ivec2(m_style.gridMargin);
        m_locations.theLine = m_locations.itemGrid + glm::ivec2(GetItemGridSize().x + m_style.theLinePadding, 0);
        m_locations.itemHeading = m_locations.theLine + glm::ivec2(0, m_style.itemHeadingTopPadding);
        m_locations.itemDescription = m_locations.itemHeading + glm::ivec2(0, GetSelectedItemHeadingSize().y + m_style.itemDescriptionTopPadding);
        m_locations.itemButtons = m_locations.itemDescription + glm::ivec2(0, GetSelectedItemDescriptionSize().y + m_style.itemButtonsTopPadding);

        BlitInventoryBackground(m_locations.background, width, height);
        BlitItemGrid(m_locations.itemGrid);
        BlitTheLine(m_locations.theLine);
        BlitItemHeading(m_locations.itemHeading);
        BlitItemDescription(m_locations.itemDescription);
        BlitItemButtons(m_locations.itemButtons);

        if (ItemSelected()) {
            int coinMargin = 13;
            glm::ivec2 coinIconLocation = m_locations.itemHeading + glm::ivec2(GetSelectedItemHeadingSize().x + coinMargin, 16);
            glm::ivec2 priceLocation = coinIconLocation + glm::ivec2(22, 0);
            std::string cost = std::to_string(GetSelectedItemCost());

            BlitCoinIcon(coinIconLocation);
            BlitGenericText(cost, priceLocation, Alignment::TOP_LEFT);
        }
    }

}

void Inventory::SubmitItemExamineRenderItems() {
    // Render item inspect item
}

void Inventory::BlitInventoryBackground(glm::ivec2 origin, int width, int height) {
    Texture* bgTexture = Hell::ResourceManager::GetTextureByName("inv_background");
    Texture* borderTexture = Hell::ResourceManager::GetTextureByName("inv_border");
    Texture* cornerTexture = Hell::ResourceManager::GetTextureByName("inv_border_corner");

    if (!bgTexture) return;
    if (!borderTexture) return;
    if (!cornerTexture) return;

    // Clip rect in top left origin pixels
    int clipMinX = origin.x;
    int clipMinY = origin.y;
    int clipMaxX = origin.x + width;
    int clipMaxY = origin.y + height;

    // Blit background
    const int bgWidth = std::max(1, bgTexture->GetWidth());
    const int bgHeight = std::max(1, bgTexture->GetHeight());
    const int bgCellCountX = (width + bgWidth - 1) / bgWidth;
    const int bgCellCountY = (height + bgHeight - 1) / bgHeight;

    for (int x = 0; x < bgCellCountX; ++x) {
        for (int y = 0; y < bgCellCountY; ++y) {
            BlitTextureInfo info{};
            info.textureName = bgTexture->GetFileName();
            info.location = glm::ivec2(origin.x + (x * bgWidth), origin.y + (y * bgHeight));
            info.alignment = Alignment::TOP_LEFT;
            info.textureFilter = TextureFilter::LINEAR;
            info.clipMinX = clipMinX;
            info.clipMinY = clipMinY;
            info.clipMaxX = clipMaxX;
            info.clipMaxY = clipMaxY;
            UIBackEnd::BlitTexture(info);
        }
    }

    // Blit border
    const int borderWidth = std::max(1, borderTexture->GetWidth());
    const int borderHeight = std::max(1, borderTexture->GetHeight());
    const int borderCellCountX = (width + borderWidth - 1) / borderWidth;
    const int borderCellCountY = (height + borderHeight - 1) / borderHeight;

    for (int x = 0; x < borderCellCountX; ++x) {
        BlitTextureInfo info{};
        info.textureName = borderTexture->GetFileName();
        info.textureFilter = TextureFilter::LINEAR;
        info.clipMinX = clipMinX;
        info.clipMinY = clipMinY;
        info.clipMaxX = clipMaxX;
        info.clipMaxY = clipMaxY;

        // Top
        info.location = glm::ivec2(origin.x + (x * borderWidth), origin.y);
        info.alignment = Alignment::TOP_LEFT;
        UIBackEnd::BlitTexture(info);

        // Bottom
        info.location = glm::ivec2(origin.x + (x * borderWidth) + width, origin.y + height);
        info.alignment = Alignment::TOP_LEFT;
        info.rotation = HELL_PI;
        UIBackEnd::BlitTexture(info);
    }

    for (int y = 0; y < borderCellCountY; ++y) {
        BlitTextureInfo info{};
        info.textureName = borderTexture->GetFileName();
        info.textureFilter = TextureFilter::LINEAR;
        info.clipMinX = clipMinX;
        info.clipMinY = clipMinY;
        info.clipMaxX = clipMaxX;
        info.clipMaxY = clipMaxY;

        // Left
        info.location = glm::ivec2(origin.x, origin.y - (y * borderWidth) + height);
        info.alignment = Alignment::TOP_LEFT;
        info.rotation = HELL_PI * -0.5f;
        UIBackEnd::BlitTexture(info);

        // Right
        info.location = glm::ivec2(origin.x + width, origin.y + (y * borderWidth));
        info.alignment = Alignment::TOP_LEFT;
        info.rotation = HELL_PI * 0.5f;
        UIBackEnd::BlitTexture(info);
    }

    // Top left corner
    BlitTextureInfo info{};
    info.textureName = cornerTexture->GetFileName();
    info.textureFilter = TextureFilter::LINEAR;
    info.clipMinX = clipMinX;
    info.clipMinY = clipMinY;
    info.clipMaxX = clipMaxX;
    info.clipMaxY = clipMaxY;
    info.location = glm::ivec2(origin.x, origin.y);
    info.alignment = Alignment::TOP_LEFT;
    info.rotation = 0.0f;
    UIBackEnd::BlitTexture(info);

    // Top right
    info.location = glm::ivec2(origin.x + width, origin.y);
    info.alignment = Alignment::TOP_LEFT;
    info.rotation = HELL_PI * 0.5f;
    UIBackEnd::BlitTexture(info);

    // Bottom left
    info.location = glm::ivec2(origin.x, origin.y + height);
    info.alignment = Alignment::TOP_LEFT;
    info.rotation = HELL_PI * 1.5f;
    UIBackEnd::BlitTexture(info);

    // Bottom right
    info.location = glm::ivec2(origin.x + width, origin.y + height);
    info.alignment = Alignment::TOP_LEFT;
    info.rotation = HELL_PI;
    UIBackEnd::BlitTexture(info);
}

void Inventory::BlitItemGrid(glm::ivec2 origin) {
    Texture* squareSize1Texture = Hell::ResourceManager::GetTextureByName("InvSquare_Size1");
    Texture* squareSize2Texture = Hell::ResourceManager::GetTextureByName("InvSquare_Size2");
    Texture* squareSize3Texture = Hell::ResourceManager::GetTextureByName("InvSquare_Size3");
    Texture* squareSize1SelectedTexture = Hell::ResourceManager::GetTextureByName("InvSquare_Size1Selected");
    Texture* squareSize2SelectedTexture = Hell::ResourceManager::GetTextureByName("InvSquare_Size2Selected");
    Texture* squareSize3SelectedTexture = Hell::ResourceManager::GetTextureByName("InvSquare_Size3Selected");
    Texture* gridBorder = Hell::ResourceManager::GetTextureByName("Inv_GridBorder");
    Texture* gridBorderCorner = Hell::ResourceManager::GetTextureByName("Inv_GridBorderCorner");
    Texture* gridDivider = Hell::ResourceManager::GetTextureByName("Inv_GridDivider");

    if (!squareSize1Texture) return;
    if (!squareSize2Texture) return;
    if (!squareSize3Texture) return;
    if (!squareSize1SelectedTexture) return;
    if (!squareSize2SelectedTexture) return;
    if (!squareSize3SelectedTexture) return;
    if (!gridBorder) return;
    if (!gridBorderCorner) return;
    if (!gridDivider) return;

    // Render background squares (size 1)
    for (int x = 0; x < m_gridCountX; x++) {
        for (int y = 0; y < m_gridCountY; y++) {
            BlitTextureInfo blitTextureInfo;
            blitTextureInfo.textureName = squareSize1Texture->GetFileName();
            blitTextureInfo.location.x = origin.x + (GetCellSizeInPixels() * x);
            blitTextureInfo.location.y = origin.y + (GetCellSizeInPixels() * y);
            blitTextureInfo.alignment = Alignment::TOP_LEFT;
            blitTextureInfo.textureFilter = TextureFilter::LINEAR;
            UIBackEnd::BlitTexture(blitTextureInfo);
        }
    }

    // Render bigger background squares for items that need it (size 2 & 3)
    for (InventoryItem& item : m_items) {
        int itemSize = Bible::GetInventoryItemSizeByName(item.m_name);
        if (itemSize <= 1) continue;

        BlitTextureInfo blitTextureInfo;
        blitTextureInfo.textureName = (itemSize == 2 ? squareSize2Texture->GetFileName() : squareSize3Texture->GetFileName());
        blitTextureInfo.location.x = origin.x + (GetCellSizeInPixels() * item.m_gridLocation.x);
        blitTextureInfo.location.y = origin.y + (GetCellSizeInPixels() * item.m_gridLocation.y);
        blitTextureInfo.alignment = Alignment::TOP_LEFT;
        blitTextureInfo.textureFilter = TextureFilter::LINEAR;
        blitTextureInfo.rotation = 0.0f;

        if (item.m_rotatedInGrid) {
            blitTextureInfo.rotation = HELL_PI * -0.5f;
            blitTextureInfo.location.y += ((itemSize - 1) * GetCellSizeInPixels()) - GetCellSizeInPixels();
        }

        UIBackEnd::BlitTexture(blitTextureInfo);
    }

    // Render the selected cell
    if (InBounds(m_selectedCellX, m_selectedCellY)) {
        int itemIndex = m_itemIndex2DArray[m_selectedCellX][m_selectedCellY];

        BlitTextureInfo blitTextureInfo;
        blitTextureInfo.alignment = Alignment::TOP_LEFT;
        blitTextureInfo.textureFilter = TextureFilter::LINEAR;

        if (itemIndex != -1 && itemIndex < (int)m_items.size()) {
            // Selection sits on an item, so snap to the item's anchor and use matching selected texture
            InventoryItem& item = m_items[itemIndex];
            int itemSize = Bible::GetInventoryItemSizeByName(item.m_name);

            blitTextureInfo.textureName = (itemSize == 1) ? squareSize1SelectedTexture->GetFileName() :(itemSize == 2) ? squareSize2SelectedTexture->GetFileName() : squareSize3SelectedTexture->GetFileName();
            blitTextureInfo.location.x = origin.x + (GetCellSizeInPixels() * item.m_gridLocation.x);
            blitTextureInfo.location.y = origin.y + (GetCellSizeInPixels() * item.m_gridLocation.y);
            blitTextureInfo.rotation = 0.0f;

            if (item.m_rotatedInGrid) {
                blitTextureInfo.rotation = HELL_PI * -0.5f;
                blitTextureInfo.location.y += ((itemSize - 1) * GetCellSizeInPixels()) - GetCellSizeInPixels();
            }
        }
        else {
            // For empty cells, use the single size 1 selected highlight
            blitTextureInfo.textureName = squareSize1SelectedTexture->GetFileName();
            blitTextureInfo.location.x = origin.x + (GetCellSizeInPixels() * m_selectedCellX);
            blitTextureInfo.location.y = origin.y + (GetCellSizeInPixels() * m_selectedCellY);
        }

        UIBackEnd::BlitTexture(blitTextureInfo);
    }

    // Icons
    for (InventoryItem& item : m_items) {
        std::string textureName = "InvItem_" + item.m_name;
        Texture* texture = Hell::ResourceManager::GetTextureByName(textureName);
        if (!texture) {
            std::cout << "Could not render inventory icon for '" << item.m_name << "' coz ;" << textureName << "' was not found\n";
            continue;
        }

        ItemInfo* itemInfo = Bible::GetItemInfoByName(item.m_name);
        if (!itemInfo) continue;

        glm::ivec2 itemLocation;
        itemLocation.x = origin.x + (GetCellSizeInPixels() * item.m_gridLocation.x);
        itemLocation.y = origin.y + (GetCellSizeInPixels() * item.m_gridLocation.y);

        // Render icon
        BlitTextureInfo blitTextureInfo;
        blitTextureInfo.textureName = textureName;
        blitTextureInfo.location = itemLocation;
        blitTextureInfo.alignment = Alignment::TOP_LEFT;
        blitTextureInfo.textureFilter = TextureFilter::LINEAR;

        if (item.m_rotatedInGrid) {
            blitTextureInfo.rotation = HELL_PI * -0.5f;
            blitTextureInfo.location.y += GetCellSizeInPixels();
        }

        UIBackEnd::BlitTexture(blitTextureInfo);
    }

    // Grid border
    for (int x = 0; x < m_gridCountX; x++) {
        BlitTextureInfo blitTextureInfo;
        blitTextureInfo.textureName = gridBorder->GetFileName();
        blitTextureInfo.location.x = origin.x + (GetCellSizeInPixels() * x);
        blitTextureInfo.location.y = origin.y;
        blitTextureInfo.alignment = Alignment::CENTERED_VERTICAL;
        blitTextureInfo.textureFilter = TextureFilter::LINEAR;
        UIBackEnd::BlitTexture(blitTextureInfo);

        blitTextureInfo.location.x = origin.x + (GetCellSizeInPixels() * (x + 1));
        blitTextureInfo.location.y = origin.y + GetItemGridSize().y;
        blitTextureInfo.rotation = HELL_PI;
        blitTextureInfo.alignment = Alignment::CENTERED_VERTICAL;
        UIBackEnd::BlitTexture(blitTextureInfo);
    }

    for (int y = 0; y < m_gridCountY; y++) {
        BlitTextureInfo blitTextureInfo;
        blitTextureInfo.textureName = gridBorder->GetFileName();
        blitTextureInfo.location.x = origin.x;
        blitTextureInfo.location.y = origin.y + (GetCellSizeInPixels() * (y + 1));
        blitTextureInfo.alignment = Alignment::CENTERED_VERTICAL;
        blitTextureInfo.textureFilter = TextureFilter::LINEAR;
        blitTextureInfo.rotation = HELL_PI * -0.5f;
        UIBackEnd::BlitTexture(blitTextureInfo);

        blitTextureInfo.location.x = origin.x + GetItemGridSize().x;
        blitTextureInfo.location.y = origin.y + (GetCellSizeInPixels() * y);
        blitTextureInfo.rotation = HELL_PI;
        blitTextureInfo.alignment = Alignment::CENTERED_VERTICAL;
        blitTextureInfo.rotation = HELL_PI * 0.5f;
        UIBackEnd::BlitTexture(blitTextureInfo);
    }

    // Grid border corners
    BlitTextureInfo blitTextureInfo;
    blitTextureInfo.textureName = gridBorderCorner->GetFileName();
    blitTextureInfo.textureFilter = TextureFilter::LINEAR;
    blitTextureInfo.location.x = origin.x;
    blitTextureInfo.location.y = origin.y;
    blitTextureInfo.rotation = 0.0f;
    blitTextureInfo.alignment = Alignment::CENTERED;
    UIBackEnd::BlitTexture(blitTextureInfo);

    blitTextureInfo.location.x = origin.x + GetItemGridSize().x;
    blitTextureInfo.location.y = origin.y;
    blitTextureInfo.rotation = HELL_PI * 0.5f;
    blitTextureInfo.alignment = Alignment::CENTERED;
    UIBackEnd::BlitTexture(blitTextureInfo);

    blitTextureInfo.location.x = origin.x;
    blitTextureInfo.location.y = origin.y + GetItemGridSize().y;
    blitTextureInfo.rotation = HELL_PI * -0.5f;
    blitTextureInfo.alignment = Alignment::CENTERED;
    UIBackEnd::BlitTexture(blitTextureInfo);

    blitTextureInfo.location.x = origin.x + GetItemGridSize().x;
    blitTextureInfo.location.y = origin.y + GetItemGridSize().y;
    blitTextureInfo.rotation = HELL_PI;
    blitTextureInfo.alignment = Alignment::CENTERED;
    UIBackEnd::BlitTexture(blitTextureInfo);

    // Grid dividers horizontal
    for (int x = 0; x < m_gridCountX; x++) {
        for (int y = 0; y < m_gridCountY - 1; y++) {
            int itemIndex = m_itemIndex2DArray[x][y];
            int nextItemIndex = m_itemIndex2DArray[x][y + 1];

            if (itemIndex != nextItemIndex || nextItemIndex == -1) {
                BlitTextureInfo blitTextureInfo;
                blitTextureInfo.textureName = gridDivider->GetFileName();
                blitTextureInfo.textureFilter = TextureFilter::LINEAR;
                blitTextureInfo.location.y = origin.y + ((y + 1) * GetCellSizeInPixels());
                blitTextureInfo.location.x = origin.x + (x * GetCellSizeInPixels());
                blitTextureInfo.rotation = 0.0f;
                blitTextureInfo.alignment = Alignment::CENTERED_VERTICAL;
                blitTextureInfo.colorTint = glm::vec4(glm::vec3(0.9f), 1.0f);
                UIBackEnd::BlitTexture(blitTextureInfo);
            }
        }
    }

    // Grid dividers vertical
    for (int x = 0; x < m_gridCountX - 1; x++) {
        for (int y = 0; y < m_gridCountY; y++) {
            int itemIndex = m_itemIndex2DArray[x][y];
            int nextItemIndex = m_itemIndex2DArray[x + 1][y];

            if (itemIndex != nextItemIndex || nextItemIndex == -1) {
                BlitTextureInfo blitTextureInfo;
                blitTextureInfo.textureName = gridDivider->GetFileName();
                blitTextureInfo.textureFilter = TextureFilter::LINEAR;
                blitTextureInfo.location.x = origin.x + ((x + 1) * GetCellSizeInPixels());
                blitTextureInfo.location.y = origin.y + (y * GetCellSizeInPixels());
                blitTextureInfo.rotation = HELL_PI * 0.5f;
                blitTextureInfo.alignment = Alignment::CENTERED_VERTICAL;
                blitTextureInfo.colorTint = glm::vec4(glm::vec3(0.9), 1.0f);
                UIBackEnd::BlitTexture(blitTextureInfo);
            }
        }
    }
}


void Inventory::BlitCoinIcon(const glm::ivec2& origin) {
    BlitTextureInfo blitTextureInfo;
    blitTextureInfo.textureName = "CoinIcon";
    blitTextureInfo.location = origin;
    blitTextureInfo.alignment = Alignment::TOP_LEFT;
    blitTextureInfo.textureFilter = TextureFilter::LINEAR;
    UIBackEnd::BlitTexture(blitTextureInfo);
}

void Inventory::BlitTheLine(glm::ivec2 origin) {
    BlitTextureInfo blitTextureInfo;
    blitTextureInfo.textureName = "Inv_TheLine";
    blitTextureInfo.location = origin;
    blitTextureInfo.alignment = Alignment::TOP_LEFT;
    blitTextureInfo.textureFilter = TextureFilter::LINEAR;

    for (int x = 0; x < m_gridCountX; x++) {
        blitTextureInfo.location.x = origin.x + (GetCellSizeInPixels() * x);
        blitTextureInfo.location.y = origin.y;
        UIBackEnd::BlitTexture(blitTextureInfo);
    }
}

void Inventory::BlitItemHeading(glm::ivec2 origin) {
    if (!ItemSelected()) return;
    UIBackEnd::BlitText("[COL=0.839,0.784,0.635]" + GetSelectedItemHeading(), m_style.itemHeadingFont, origin, Alignment::TOP_LEFT, 1.0f, TextureFilter::LINEAR);
}

void Inventory::BlitShopHeading(const glm::ivec2& centerOrigin) {
    BlitTextureInfo info;
    info.textureName = "ShopHeading";
    info.location = centerOrigin;
    info.alignment = Alignment::TOP_LEFT;
    info.textureFilter = TextureFilter::LINEAR;
    //glm::vec4 colorTint = glm::vec4(1, 1, 1, 1);
    //glm::ivec2 size = glm::ivec2(-1, -1);
    //float rotation = 0.0f;
    //int clipMinX = -1;
    //int clipMinY = -1;
    //int clipMaxX = -1;
    //int clipMaxY = -1;
    UIBackEnd::BlitTexture(info);
}

void Inventory::BlitItemDescription(glm::ivec2 origin) {
    if (!ItemSelected()) return;
    UIBackEnd::BlitText("[COL=0.839,0.784,0.635]" + GetSelectedItemDescription(), m_style.itemDescriptionFont, origin, Alignment::TOP_LEFT, 1.0f, TextureFilter::LINEAR);
}

void Inventory::BlitGenericText(const std::string& text, const glm::ivec2& origin, Alignment alignment) {
    UIBackEnd::BlitText("[COL=0.839,0.784,0.635]" + text, m_style.itemDescriptionFont, origin, alignment, 1.0f, TextureFilter::LINEAR);
}

void Inventory::BlitItemButtons(glm::ivec2 origin) {
    ItemInfo* itemInfo = GetSelectedItemInfo();
    if (!itemInfo) return;


    glm::ivec2 buttonLocation = origin;

    if (m_state == InventoryState::SHOP) {
        RenderButton(buttonLocation, "E", "Buy");
        buttonLocation.y += m_style.itemButtonLineHeight;
        return;
    }

	if (itemInfo->IsEquipable()) {
		RenderButton(buttonLocation, "E", "Equip");
		buttonLocation.y += m_style.itemButtonLineHeight;
	}

	if (itemInfo->IsUsable()) {
		RenderButton(buttonLocation, "U", "Use");
		buttonLocation.y += m_style.itemButtonLineHeight;
	}

    if (itemInfo->IsCombineable()) {
        RenderButton(buttonLocation, "C", "Combine");
        buttonLocation.y += m_style.itemButtonLineHeight;
    }

    if (true) { // Everything is examinable
        RenderButton(buttonLocation, "F", "Examine");
        buttonLocation.y += m_style.itemButtonLineHeight;
    }

    if (true) { // Everything is moveable
        RenderButton(buttonLocation, "M", "Move");
        buttonLocation.y += m_style.itemButtonLineHeight;
    }

    if (itemInfo->IsDiscardable()) {
        RenderButton(buttonLocation, "G", "Discard");
        buttonLocation.y += m_style.itemButtonLineHeight;
    }
}

void Inventory::RenderButton(glm::ivec2 location, const std::string& letter, const std::string& description) {
    Texture* buttonTexture = Hell::ResourceManager::GetTextureByName("inventory_green_button");
    if (!buttonTexture) return;

    int buttonCenterX = location.x + (buttonTexture->GetWidth() * 0.5f);
    int descriptionLeftY = buttonCenterX + 19;

    UIBackEnd::BlitTexture("inventory_green_button", glm::ivec2(buttonCenterX, location.y), Alignment::CENTERED, WHITE, glm::ivec2(-1, -1), TextureFilter::LINEAR);
    UIBackEnd::BlitText("[COL=0.839,0.784,0.635]" + letter, "RobotoCondensed", buttonCenterX, location.y + 2, Alignment::CENTERED, 0.75f, TextureFilter::LINEAR);
    UIBackEnd::BlitText("[COL=0.839,0.784,0.635]" + description, "RobotoCondensed", descriptionLeftY, location.y + 2, Alignment::CENTERED_VERTICAL, 1.0f, TextureFilter::LINEAR);
}

}
