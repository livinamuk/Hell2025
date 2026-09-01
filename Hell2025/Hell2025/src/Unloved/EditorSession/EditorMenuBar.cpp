#include "EditorMenuBar.h"

#include "Unloved/EditorSession/UI/EditorLayout.h"
#include "Unloved/EditorSession/UI/EditorStyle.h"
#include "Unloved/EditorSession/UI/EditorUI.h"

#include "Hell/Backend/BackEnd.h"
#include "Hell/Input.h"
#include "Hell/UI/TextBlitter.h"
#include "Hell/UI/UIBackEnd.h"

#include "Unloved/Common/Constants.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace Unloved::EditorSession::MenuBar {
    namespace {
        enum class MenuItemKind : uint8_t {
            ACTION,
            SEPARATOR
        };

        struct MenuItem {
            MenuItemKind kind = MenuItemKind::ACTION;
            std::string label;
            std::string shortcut;
            EditorMenuAction action = EditorMenuAction::NONE;
            PlacementTool placementTool = PlacementTool::NONE;
            EditorRect rect;
            EditorRect popupRect;
            std::vector<MenuItem> children;
        };

        struct Menu {
            std::string label;
            std::vector<MenuItem> items;
            EditorRect buttonRect;
            EditorRect popupRect;
        };

        std::vector<Menu> g_menus;
        int32_t g_openMenuIndex = -1;
        int32_t g_hoveredMenuIndex = -1;
        MenuItem* g_hoveredItem = nullptr;
        std::vector<MenuItem*> g_openSubmenus;
        bool g_wantsMouseCapture = false;
        bool g_wantsKeyboardCapture = false;
        EditorMenuAction g_pendingAction = EditorMenuAction::NONE;
        PlacementTool g_pendingPlacementTool = PlacementTool::NONE;
        EditorSessionMode g_mode = EditorSessionMode::HOUSE;

        MenuItem Action(const char* label, const char* shortcut, EditorMenuAction action) {
            MenuItem item;
            item.label = label;
            item.shortcut = shortcut;
            item.action = action;
            return item;
        }

        MenuItem Separator() {
            MenuItem item;
            item.kind = MenuItemKind::SEPARATOR;
            return item;
        }

        MenuItem Tool(const char* label, PlacementTool placementTool) {
            MenuItem item;
            item.label = label;
            item.placementTool = placementTool;
            return item;
        }

        MenuItem Submenu(const char* label, std::vector<MenuItem> children) {
            MenuItem item;
            item.label = label;
            item.children = std::move(children);
            return item;
        }

        Menu CreateFileMenu(EditorSessionMode mode) {
            Menu menu;
            menu.label = "File";

            menu.items.push_back(Action("New", "Ctrl+N", EditorMenuAction::NEW_FILE));
            menu.items.push_back(Action("Open", "Ctrl+O", EditorMenuAction::OPEN_FILE));
            if (mode == EditorSessionMode::RAGDOLL) {
                menu.items.push_back(Action("Import rag", "", EditorMenuAction::IMPORT_RAG));
            }
            menu.items.push_back(Action("Save", "Ctrl+S", EditorMenuAction::SAVE));
            if (mode == EditorSessionMode::RAGDOLL) {
                menu.items.push_back(Action("Save As", "Ctrl+Shift+S", EditorMenuAction::SAVE_AS));
            }
            menu.items.push_back(Separator());
            menu.items.push_back(Action("Close Editor", "`", EditorMenuAction::CLOSE_EDITOR));
            menu.items.push_back(Action("Exit", "", EditorMenuAction::EXIT_APPLICATION));
            return menu;
        }

        bool CanEmit(EditorMenuAction action) {
            return action != EditorMenuAction::NONE;
        }

        void CloseMenus() {
            g_openMenuIndex = -1;
            g_hoveredItem = nullptr;
            g_openSubmenus.clear();
        }

        glm::ivec2 GetPopupSize(const std::vector<MenuItem>& items) {
            const EditorStyle& style = GetStyle();
            int32_t width = style.menu.popupMinimumWidth;
            int32_t height = style.menu.popupVerticalPadding * 2;

            for (const MenuItem& item : items) {
                if (item.kind == MenuItemKind::SEPARATOR) {
                    height += style.menu.separatorHeight;
                    continue;
                }

                const int32_t labelWidth = TextBlitter::GetTextSize(item.label, style.font.name, style.font.scale).x;
                const int32_t shortcutWidth = TextBlitter::GetTextSize(item.shortcut, style.font.name, style.font.scale).x;
                const int32_t shortcutGap = item.shortcut.empty() ? 0 : style.menu.shortcutGap;
                const int32_t submenuWidth = item.children.empty() ? 0 : style.menu.submenuArrowSize + style.menu.itemHorizontalPadding;
                width = std::max(width, style.menu.itemHorizontalPadding * 2 + labelWidth + shortcutGap + shortcutWidth + submenuWidth);
                height += style.menu.itemHeight;
            }

            return { width, height };
        }

        void UpdateItemGeometry(std::vector<MenuItem>& items, const EditorRect& popupRect, const EditorRect& menuBarRect) {
            const EditorMenuStyle& style = GetStyle().menu;
            int32_t itemY = popupRect.y + style.popupVerticalPadding;
            for (MenuItem& item : items) {
                const int32_t itemHeight = item.kind == MenuItemKind::SEPARATOR ? style.separatorHeight : style.itemHeight;
                item.rect = { popupRect.x + 1, itemY, popupRect.width - 2, itemHeight };
                itemY += itemHeight;

                if (item.children.empty()) continue;

                const glm::ivec2 popupSize = GetPopupSize(item.children);
                int32_t popupX = popupRect.Right();
                if (popupX + popupSize.x > menuBarRect.Right()) {
                    popupX = popupRect.x - popupSize.x;
                }
                const int32_t maximumPopupY = std::max(menuBarRect.Bottom(), Hell::BackEnd::GetDrawableHeight() - popupSize.y);
                const int32_t popupY = std::clamp(item.rect.y - style.popupVerticalPadding, menuBarRect.Bottom(), maximumPopupY);
                item.popupRect = { popupX, popupY, popupSize.x, popupSize.y };
                UpdateItemGeometry(item.children, item.popupRect, menuBarRect);
            }
        }

        void UpdatePopupGeometry(Menu& menu, const EditorRect& menuBarRect) {
            const glm::ivec2 popupSize = GetPopupSize(menu.items);

            const int32_t maxPopupX = std::max(menuBarRect.x, menuBarRect.Right() - popupSize.x);
            const int32_t popupX = std::clamp(menu.buttonRect.x, menuBarRect.x, maxPopupX);
            menu.popupRect = { popupX, menuBarRect.Bottom(), popupSize.x, popupSize.y };
            UpdateItemGeometry(menu.items, menu.popupRect, menuBarRect);
        }

        void UpdateGeometry() {
            const EditorStyle& style = GetStyle();
            const EditorRect& menuBarRect = Layout::GetFileMenuPanel().rect;
            int32_t buttonX = menuBarRect.x + style.menu.barLeftPadding;
            const int32_t buttonHeight = std::max(0, menuBarRect.height - 1);

            for (Menu& menu : g_menus) {
                const int32_t textWidth = TextBlitter::GetTextSize(menu.label, style.font.name, style.font.scale).x;
                const int32_t buttonWidth = std::max(40, textWidth + style.menu.buttonHorizontalPadding * 2);
                menu.buttonRect = { buttonX, menuBarRect.y, buttonWidth, buttonHeight };
                buttonX += buttonWidth;
                UpdatePopupGeometry(menu, menuBarRect);
            }
        }

        int32_t GetHoveredMenuIndex(const glm::ivec2& mousePosition) {
            for (size_t i = 0; i < g_menus.size(); i++) {
                if (g_menus[i].buttonRect.Contains(mousePosition)) {
                    return static_cast<int32_t>(i);
                }
            }
            return -1;
        }

        struct HoveredItem {
            MenuItem* item = nullptr;
            size_t level = 0;
        };

        HoveredItem GetHoveredItem(Menu& menu, const glm::ivec2& mousePosition) {
            for (int32_t level = static_cast<int32_t>(g_openSubmenus.size()); level >= 0; level--) {
                std::vector<MenuItem>& items = level == 0 ? menu.items : g_openSubmenus[level - 1]->children;
                for (MenuItem& item : items) {
                    if (item.rect.Contains(mousePosition)) return { &item, static_cast<size_t>(level) };
                }
            }
            return {};
        }

        bool ControlIsDown() {
            return Hell::Input::KeyDown(HELL_KEY_LEFT_CONTROL_GLFW) || Hell::Input::KeyDown(HELL_KEY_RIGHT_CONTROL);
        }

        bool ShiftIsDown() {
            return Hell::Input::KeyDown(HELL_KEY_LEFT_SHIFT_GLFW) || Hell::Input::KeyDown(HELL_KEY_RIGHT_SHIFT);
        }

        EditorMenuAction GetShortcutAction() {
            if (!ControlIsDown()) return EditorMenuAction::NONE;
            if (Hell::Input::KeyPressed(HELL_KEY_N)) return EditorMenuAction::NEW_FILE;
            if (Hell::Input::KeyPressed(HELL_KEY_O)) return EditorMenuAction::OPEN_FILE;
            if (Hell::Input::KeyPressed(HELL_KEY_S)) {
                return ShiftIsDown() && g_mode == EditorSessionMode::RAGDOLL
                    ? EditorMenuAction::SAVE_AS
                    : EditorMenuAction::SAVE;
            }
            return EditorMenuAction::NONE;
        }

        std::string WithColor(const char* color, const std::string& text) {
            return std::string(color) + text;
        }

        void RenderPopup(const std::vector<MenuItem>& items, const EditorRect& popupRect, size_t level) {
            const EditorStyle& style = GetStyle();
            UI::DrawSolidRect(popupRect, style.colors.panelBackground);

            for (const MenuItem& item : items) {
                const EditorRect& itemRect = item.rect;

                // Separator
                if (item.kind == MenuItemKind::SEPARATOR) {
                    UI::DrawSolidRect({ itemRect.x + style.menu.itemHorizontalPadding, itemRect.y + itemRect.height / 2, itemRect.width - style.menu.itemHorizontalPadding * 2, 1 }, style.colors.separator);
                    continue;
                }

                // Hover highlight
                if (&item == g_hoveredItem) {
                    UI::DrawSolidRect(itemRect, style.colors.hover);
                }

                // Item label
                const int32_t centerY = itemRect.y + itemRect.height / 2;
                UIBackEnd::BlitText(UICanvas::NATIVE, WithColor(style.font.textColorTag, item.label), style.font.name, glm::ivec2(itemRect.x + style.menu.itemHorizontalPadding, centerY), Alignment::CENTERED_VERTICAL, style.font.scale, TextureFilter::NEAREST);

                // Submenu arrow or keyboard shortcut
                if (!item.children.empty()) {
                    UIBackEnd::BlitTexture(UICanvas::NATIVE, "DropDownArrow", glm::ivec2(itemRect.Right() - style.menu.itemHorizontalPadding - style.menu.submenuArrowSize / 2, centerY), Alignment::CENTERED, style.colors.text, glm::ivec2(style.menu.submenuArrowSize), TextureFilter::NEAREST, HELL_PI * -0.5f);
                }
                else if (!item.shortcut.empty()) {
                    const int32_t shortcutWidth = TextBlitter::GetTextSize(item.shortcut, style.font.name, style.font.scale).x;
                    UIBackEnd::BlitText(UICanvas::NATIVE, WithColor(style.font.textColorTag, item.shortcut), style.font.name, glm::ivec2(itemRect.Right() - style.menu.itemHorizontalPadding - shortcutWidth, centerY), Alignment::CENTERED_VERTICAL, style.font.scale, TextureFilter::NEAREST);
                }
            }

            EditorPanel popupPanel;
            popupPanel.rect = popupRect;
            popupPanel.edges = EditorPanelEdge::ALL;
            popupPanel.borderColor = style.colors.border;
            popupPanel.borderThickness = 1;
            popupPanel.drawBackground = false;
            UI::DrawPanelEdges(popupPanel);

            if (level >= g_openSubmenus.size()) return;
            const MenuItem* submenu = g_openSubmenus[level];
            RenderPopup(submenu->children, submenu->popupRect, level + 1);
        }
    }

    void Init() {
        g_menus.clear();
        g_menus.push_back(CreateFileMenu(g_mode));

        Menu insertMenu;
        insertMenu.label = "Insert";
        insertMenu.items = {
            Action("Reinsert last", "Ctrl+T", EditorMenuAction::NONE),
            Submenu("Bathroom", {
                Tool("Basin", PlacementTool::GENERIC_BATHROOM_BASIN),
                Tool("Cabinet", PlacementTool::GENERIC_BATHROOM_CABINET),
                Tool("Toilet", PlacementTool::GENERIC_TOILET),
            }),
            Submenu("Christmas", {
                Tool("Christmas Lights", PlacementTool::CHRISTMAS_LIGHTS),
                Tool("Present Small", PlacementTool::GENERIC_CHRISTMAS_PRESENT_SMALL),
                Tool("Present Large", PlacementTool::GENERIC_CHRISTMAS_PRESENT_LARGE),
                Tool("Tree", PlacementTool::GENERIC_CHRISTMAS_TREE),
            }),
            Submenu("Enemies", {
                Tool("Dobermann", PlacementTool::DOBERMANN),
                Tool("Kangaroo", PlacementTool::KANGAROO),
                Tool("Shark", PlacementTool::SHARK),
                Tool("Snake", PlacementTool::SNAKE),
            }),
            Submenu("Furniture", {
                Tool("Couch", PlacementTool::GENERIC_COUCH),
                Submenu("Chairs", {
                    Tool("Chair RE", PlacementTool::GENERIC_CHAIR_RE),
                    Tool("Chair Spindle Back", PlacementTool::GENERIC_CHAIR_SPINDLE_BACK),
                }),
                Submenu("Drawers", {
                    Tool("Small", PlacementTool::GENERIC_DRAWERS_SMALL),
                    Tool("Large", PlacementTool::GENERIC_DRAWERS_LARGE),
                }),
            }),
            Submenu("Fishing", {
                Tool("Jetty", PlacementTool::JETTY),
            }),
            Submenu("Rural", {
                Tool("Fence", PlacementTool::FENCE_FARM),
                Tool("Power Pole", PlacementTool::POWER_POLES),
            }),
            Submenu("House", {
                Tool("Ceiling", PlacementTool::WORLD_PLANE_CEILING),
                Tool("Floor", PlacementTool::WORLD_PLANE_FLOOR),
                Submenu("Wall", {
                    Tool("Interior", PlacementTool::WALL_INTERIOR),
                    Tool("Weather Boards", PlacementTool::WALL_WEATHER_BOARDS),
                }),
                Tool("Door", PlacementTool::DOOR_STANDARD_A),
                Tool("Window", PlacementTool::WINDOW),
                Tool("Staircase", PlacementTool::STAIRCASE),
                Submenu("Fireplace", {
                    Tool("Open", PlacementTool::FIREPLACE_OPEN),
                    Tool("Stove", PlacementTool::FIREPLACE_WOOD_STOVE),
                }),
                Submenu("Picture Frames", {
                    Tool("Picture Frame Small", PlacementTool::PICTURE_FRAME_REGULAR_LANDSCAPE),
                    Tool("Picture Frame Large", PlacementTool::PICTURE_FRAME_BIG_LANDSCAPE),
                    Tool("Picture Frame Portrait", PlacementTool::PICTURE_FRAME_REGULAR_PORTRAIT),
                    Tool("Picture Frame Tall Thin", PlacementTool::PICTURE_FRAME_TALL_THIN),
                }),
                Submenu("Roofing", {
                    Tool("Ridge Capping", PlacementTool::RIDGE_CAPPING),
                    Tool("Down Pipe", PlacementTool::DOWN_PIPE),
                    Tool("Gutter", PlacementTool::GUTTER),
                    Tool("Roofing Iron", PlacementTool::ROOFING_IRON),
                }),
                Submenu("Decking", {
                    Tool("Decking Bearer", PlacementTool::DECKING_BEARER),
                    Tool("Decking Boards", PlacementTool::DECKING_BOARDS),
                    Tool("Decking Post",   PlacementTool::DECKING_POST),
                }),
            }),
            Submenu("Ladders", {
                Tool("Ladder", PlacementTool::LADDER),
                Tool("Ladder Dismount", PlacementTool::LADDER_DISMOUNT),
            }),
            Submenu("Misc", {
                Tool("Piano", PlacementTool::PIANO),
            }),
            Submenu("Ornaments", {
                Tool("Deer Head", PlacementTool::GENERIC_DEER_HEAD),
            }),
            Submenu("Plants", {
                Tool("Tree", PlacementTool::GENERIC_PLANT_TREE),
                Tool("Black Berries", PlacementTool::GENERIC_PLANT_BLACKBERRIES),
            }),
            Submenu("Lighting", {
                Tool("Christmas Lights", PlacementTool::CHRISTMAS_LIGHTS),
                Tool("DDGI Volume", PlacementTool::DDGI_VOLUME),
                Tool("Light", PlacementTool::LIGHT_HANGING),
            }),
            Submenu("Locations", {
                Tool("House", PlacementTool::HOUSE_LOCATION),
            }),
            Submenu("Mermaids", {
                Tool("Mermaid Shop Owner", PlacementTool::MERMAID),
                Tool("Mermaid Visitor Rock", PlacementTool::GENERIC_MERMAID_ROCK),
            }),
            Submenu("Pick Ups", {
                Submenu("Weapons", {
                    Tool("AKS74U", PlacementTool::PICKUP_AKS74U),
                    Tool("FN-P90", PlacementTool::PICKUP_P90),
                    Tool("Glock", PlacementTool::PICKUP_GLOCK),
                    Tool("Golden Glock", PlacementTool::PICKUP_GOLDEN_GLOCK),
                    Tool("Knife", PlacementTool::PICKUP_KNIFE),
                    Tool("Remington 870", PlacementTool::PICKUP_REMINGTON_870),
                    Tool("SPAS", PlacementTool::PICKUP_SPAS),
                    Tool("Tokarev", PlacementTool::PICKUP_TOKAREV),
                }),
                Submenu("Ammo", {
                    Tool("Shotgun Shells Buckshot", PlacementTool::PICKUP_12_GAUGE_BUCKSHOT),
                }),
                Submenu("Items", {
                    Tool("Black Skull", PlacementTool::PICKUP_BLACK_SKULL),
                    Tool("Relief Pills", PlacementTool::PICKUP_PILLS),
                    Tool("Small Key", PlacementTool::PICKUP_SMALL_KEY),
                    Tool("Small Key Silver", PlacementTool::PICKUP_SMALL_KEY_SILVER),
                }),
            }),
            Submenu("Spawn Points", {
                Tool("Campaign", PlacementTool::PLAYER_CAMPAIGN_SPAWN),
                Tool("Deathmatch", PlacementTool::PLAYER_DEATHMATCH_SPAWN),
            }),
            Submenu("Test Models", {
                Tool("Animated Rat King", PlacementTool::GENERIC_ANIMATED_RAT_KING),
                Action("Test Model 1", "", EditorMenuAction::NONE),
                Action("Test Model 2", "", EditorMenuAction::NONE),
                Action("Test Model 3", "", EditorMenuAction::NONE),
                Action("Test Model 4", "", EditorMenuAction::NONE),
            }),
        };
        g_menus.push_back(std::move(insertMenu));

        Menu viewportMenu;
        viewportMenu.label = "Viewport";
        viewportMenu.items = {
            Action("Single",             "", EditorMenuAction::VIEWPORT_SINGLE),
            Action("Left / Right split", "", EditorMenuAction::VIEWPORT_LEFT_RIGHT),
            Action("Top / Bottom split", "", EditorMenuAction::VIEWPORT_TOP_BOTTOM),
            Action("Four way split",     "", EditorMenuAction::VIEWPORT_FOUR)
        };
        g_menus.push_back(std::move(viewportMenu));

        g_pendingAction = EditorMenuAction::NONE;
        g_pendingPlacementTool = PlacementTool::NONE;
        Close();
    }

    void RefreshLayout() {
        UpdateGeometry();
    }

    void SetMode(EditorSessionMode mode) {
        if (g_mode == mode) return;

        Close();
        g_mode = mode;
        if (!g_menus.empty()) {
            g_menus.front() = CreateFileMenu(mode);
            UpdateGeometry();
        }
    }

    void Update() {
        g_wantsMouseCapture = false;
        g_wantsKeyboardCapture = false;
        UpdateGeometry();

        const glm::ivec2 mousePosition = Coordinates::GetMousePositionUI();
        const EditorRect& menuBarRect = Layout::GetFileMenuPanel().rect;
        const int32_t openMenuAtFrameStart = g_openMenuIndex;
        const bool menuWasOpen = g_openMenuIndex >= 0;
        bool mousePressConsumed = false;

        // Switch between open top-level menus
        g_hoveredMenuIndex = GetHoveredMenuIndex(mousePosition);
        if (g_openMenuIndex >= 0 && g_hoveredMenuIndex >= 0 && g_hoveredMenuIndex != g_openMenuIndex) {
            g_openMenuIndex = g_hoveredMenuIndex;
            g_openSubmenus.clear();
        }

        // Resolve the hovered popup item
        g_hoveredItem = nullptr;
        if (g_openMenuIndex >= 0) {
            const HoveredItem hoveredItem = GetHoveredItem(g_menus[g_openMenuIndex], mousePosition);
            g_hoveredItem = hoveredItem.item;
            if (g_hoveredItem && g_hoveredItem->kind == MenuItemKind::ACTION) {
                g_openSubmenus.resize(hoveredItem.level);
                if (!g_hoveredItem->children.empty()) {
                    g_openSubmenus.push_back(g_hoveredItem);
                }
            }
        }

        // Handle mouse selection
        if (Hell::Input::LeftMousePressed()) {
            if (g_hoveredMenuIndex >= 0) {
                g_openMenuIndex = openMenuAtFrameStart == g_hoveredMenuIndex ? -1 : g_hoveredMenuIndex;
                g_hoveredItem = nullptr;
                g_openSubmenus.clear();
                mousePressConsumed = true;
            }
            else if (g_openMenuIndex >= 0) {
                if (g_hoveredItem) {
                    if (g_hoveredItem->kind == MenuItemKind::ACTION && g_hoveredItem->children.empty()) {
                        if (CanEmit(g_hoveredItem->action)) {
                            g_pendingAction = g_hoveredItem->action;
                        }
                        if (g_hoveredItem->placementTool != PlacementTool::NONE) {
                            g_pendingPlacementTool = g_hoveredItem->placementTool;
                        }
                        CloseMenus();
                    }
                    mousePressConsumed = true;
                }
                else {
                    CloseMenus();
                    mousePressConsumed = true;
                }
            }
        }

        // Handle keyboard shortcuts
        const EditorMenuAction shortcutAction = GetShortcutAction();
        if (CanEmit(shortcutAction)) {
            g_pendingAction = shortcutAction;
            CloseMenus();
            g_wantsKeyboardCapture = true;
        }

        // Capture input while the menu is active
        g_wantsMouseCapture = menuBarRect.Contains(mousePosition) || menuWasOpen || g_openMenuIndex >= 0 || mousePressConsumed;
        g_wantsKeyboardCapture = g_wantsKeyboardCapture || g_openMenuIndex >= 0;

        if (g_wantsMouseCapture) {
            Hell::BackEnd::SetCursor(HELL_CURSOR_ARROW);
        }
    }

    void Render() {
        if (g_menus.empty()) return;
        const EditorStyle& style = GetStyle();

        for (size_t i = 0; i < g_menus.size(); i++) {
            const Menu& menu = g_menus[i];
            const bool highlighted = static_cast<int32_t>(i) == g_hoveredMenuIndex || static_cast<int32_t>(i) == g_openMenuIndex;
            if (highlighted) {
                UI::DrawSolidRect(menu.buttonRect, style.colors.hover);
            }

            UIBackEnd::BlitText(UICanvas::NATIVE, WithColor(style.font.textColorTag, menu.label), style.font.name, glm::ivec2(menu.buttonRect.x + style.menu.buttonHorizontalPadding, menu.buttonRect.y + menu.buttonRect.height / 2), Alignment::CENTERED_VERTICAL, style.font.scale, TextureFilter::NEAREST);
        }

        if (g_openMenuIndex < 0 || g_openMenuIndex >= static_cast<int32_t>(g_menus.size())) return;

        const Menu& menu = g_menus[g_openMenuIndex];
        RenderPopup(menu.items, menu.popupRect, 0);
    }

    void Close() {
        CloseMenus();
        g_hoveredMenuIndex = -1;
        g_wantsMouseCapture = false;
        g_wantsKeyboardCapture = false;
        g_pendingAction = EditorMenuAction::NONE;
        g_pendingPlacementTool = PlacementTool::NONE;
    }

    bool WantsMouseCapture() {
        return g_wantsMouseCapture;
    }

    bool WantsKeyboardCapture() {
        return g_wantsKeyboardCapture;
    }

    EditorMenuAction ConsumeAction() {
        const EditorMenuAction action = g_pendingAction;
        g_pendingAction = EditorMenuAction::NONE;
        return action;
    }

    PlacementTool ConsumePlacementTool() {
        const PlacementTool placementTool = g_pendingPlacementTool;
        g_pendingPlacementTool = PlacementTool::NONE;
        return placementTool;
    }
}
