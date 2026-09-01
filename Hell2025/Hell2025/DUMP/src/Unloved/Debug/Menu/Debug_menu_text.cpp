#include "Debug_menu.h"

#include "Unloved/Debug/Debug.h"

#include "Hell/UI/UIBackEnd.h"

#include <cstdint>
#include <string>

namespace Debug::Menu::Text {

    enum struct Setting : uint32_t {
        PLAYER_INFO,
        PLAYER_LADDER_INFO,
        PER_PLAYER_WEAPON_INFO,
        GLOBAL_TEXT,
    };

    PageId g_homePage = ROOT_PAGE_ID;
    PageId g_playerInfoPage = ROOT_PAGE_ID;
    PageId g_playerLadderInfoPage = ROOT_PAGE_ID;
    PageId g_playerWeaponInfoPage = ROOT_PAGE_ID;
    PageId g_globalTextPage = ROOT_PAGE_ID;

    void BuildMainMenu();
    void DisplayPlayerInfo();
    void DisplayPlayerLadderInfo();
    void DisplayPlayerWeaponInfo();
    void DisplayGlobalText();

    void RegisterMenu() {
        g_homePage = RegisterRootPage("Debug text", "DEBUG TEXT", BuildMainMenu, nullptr);
        g_playerInfoPage = RegisterDisplayPage(g_homePage, DisplayPlayerInfo);
        g_playerLadderInfoPage = RegisterDisplayPage(g_homePage, DisplayPlayerLadderInfo);
        g_playerWeaponInfoPage = RegisterDisplayPage(g_homePage, DisplayPlayerWeaponInfo);
        g_globalTextPage = RegisterDisplayPage(g_homePage, DisplayGlobalText);
    }

    Registrar g_registrar(RegisterMenu);

    void BuildMainMenu() {
        AddSubMenu(static_cast<uint32_t>(Setting::PLAYER_INFO), "Player misc info", g_playerInfoPage);
        AddSubMenu(static_cast<uint32_t>(Setting::PLAYER_LADDER_INFO), "Player ladder info", g_playerLadderInfoPage);
        AddSubMenu(static_cast<uint32_t>(Setting::PER_PLAYER_WEAPON_INFO), "Player weapon info", g_playerWeaponInfoPage);
        AddSubMenu(static_cast<uint32_t>(Setting::GLOBAL_TEXT), "Global text", g_globalTextPage);
    }

    void DisplayPlayerInfo() {
        Debug::SetDebugTextMode(DebugTextMode::PER_PLAYER);
    }

    void DisplayPlayerLadderInfo() {
        Debug::SetDebugTextMode(DebugTextMode::PER_PLAYER_LADDER_INFO);
    }

    void DisplayPlayerWeaponInfo() {
        Debug::SetDebugTextMode(DebugTextMode::PER_PLAYER_WEAPON_INFO);
    }

    void DisplayGlobalText() {
        const std::string& text = Debug::GetText();
        if (text.empty()) return;
        UIBackEnd::BlitText(text, "StandardFont", 0, 0, Alignment::TOP_LEFT, 2.0f);
    }
}
