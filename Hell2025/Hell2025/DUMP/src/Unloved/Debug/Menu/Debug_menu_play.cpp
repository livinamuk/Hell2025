#include "Debug_menu.h"

#include "Hell/File/File.h"

#include "Unloved/Debug/Debug.h"
#include "Unloved/Session/Session.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace Debug::Menu::Play {

    enum struct Action : uint32_t {
        CAMPAIGN,
        DEATH_MATCH,
    };

    PageId g_homepageId = ROOT_PAGE_ID;
    PageId g_campaignPageId = ROOT_PAGE_ID;
    PageId g_campaignOnePlayerPageId = ROOT_PAGE_ID;
    PageId g_campaignTwoPlayersPageId = ROOT_PAGE_ID;
    PageId g_campaignThreePlayersPageId = ROOT_PAGE_ID;
    PageId g_campaignFourPlayersPageId = ROOT_PAGE_ID;
    PageId g_deathMatchPageId = ROOT_PAGE_ID;
    PageId g_deathMatchOnePlayerPageId = ROOT_PAGE_ID;
    PageId g_deathMatchTwoPlayersPageId = ROOT_PAGE_ID;
    PageId g_deathMatchThreePlayersPageId = ROOT_PAGE_ID;
    PageId g_deathMatchFourPlayersPageId = ROOT_PAGE_ID;
    std::vector<std::string> g_mapNames;

    void BuildMainMenu();
    void BuildCampaignMenu();
    void BuildDeathmatchMenu();

    void ApplyEdit(uint32_t, const Value&);
    void ApplyOnePlayerCampaignEdit(uint32_t id, const Value&);
    void ApplyTwoPlayerCampaignEdit(uint32_t id, const Value&);
    void ApplyThreePlayerCampaignEdit(uint32_t id, const Value&);
    void ApplyFourPlayerCampaignEdit(uint32_t id, const Value&);
    void ApplyOnePlayerDeathmatchEdit(uint32_t id, const Value&);
    void ApplyTwoPlayerDeathmatchEdit(uint32_t id, const Value&);
    void ApplyThreePlayerDeathmatchEdit(uint32_t id, const Value&);
    void ApplyFourPlayerDeathmatchEdit(uint32_t id, const Value&);

    void BuildMapList() {
        g_mapNames.clear();
        for (const FileInfo& fileInfo : Hell::File::IterateDirectory("res/maps", { "map" })) g_mapNames.push_back(fileInfo.name);
        std::sort(g_mapNames.begin(), g_mapNames.end());
        for (uint32_t i = 0; i < g_mapNames.size(); i++) AddAction(i, g_mapNames[i]);
    }

    void StartGame(uint32_t mapIndex, GameMode mode, int32_t playerCount) {
        if (mapIndex >= g_mapNames.size()) return;
        Unloved::Session::RequestNewGame(mode, g_mapNames[mapIndex], playerCount);
        Debug::HideMenu();
    }

    void StartCampaign(uint32_t mapIndex, int32_t playerCount) {
        if (mapIndex >= g_mapNames.size()) return;
        StartGame(mapIndex, GameMode::CAMPAIGN, playerCount);
    }

    void StartDeathmatch(uint32_t mapIndex, int32_t playerCount) {
        if (mapIndex >= g_mapNames.size()) return;
        StartGame(mapIndex, GameMode::DEATH_MATCH, playerCount);
    }

    void RegisterMenu() {
        g_homepageId = RegisterRootPage("Play", "PLAY", BuildMainMenu, ApplyEdit);
        g_campaignPageId = RegisterPage("CAMPAIGN", g_homepageId, BuildCampaignMenu, ApplyEdit);
        g_campaignOnePlayerPageId = RegisterPage("1 PLAYER CAMPAIGN", g_campaignPageId, BuildMapList, ApplyOnePlayerCampaignEdit);
        g_campaignTwoPlayersPageId = RegisterPage("2 PLAYER CAMPAIGN", g_campaignPageId, BuildMapList, ApplyTwoPlayerCampaignEdit);
        g_campaignThreePlayersPageId = RegisterPage("3 PLAYER CAMPAIGN", g_campaignPageId, BuildMapList, ApplyThreePlayerCampaignEdit);
        g_campaignFourPlayersPageId = RegisterPage("4 PLAYER CAMPAIGN", g_campaignPageId, BuildMapList, ApplyFourPlayerCampaignEdit);
        g_deathMatchPageId = RegisterPage("DEATHMATCH", g_homepageId, BuildDeathmatchMenu, ApplyEdit);
        g_deathMatchOnePlayerPageId = RegisterPage("1 PLAYER DEATHMATCH", g_deathMatchPageId, BuildMapList, ApplyOnePlayerDeathmatchEdit);
        g_deathMatchTwoPlayersPageId = RegisterPage("2 PLAYER DEATHMATCH", g_deathMatchPageId, BuildMapList, ApplyTwoPlayerDeathmatchEdit);
        g_deathMatchThreePlayersPageId = RegisterPage("3 PLAYER DEATHMATCH", g_deathMatchPageId, BuildMapList, ApplyThreePlayerDeathmatchEdit);
        g_deathMatchFourPlayersPageId = RegisterPage("4 PLAYER DEATHMATCH", g_deathMatchPageId, BuildMapList, ApplyFourPlayerDeathmatchEdit);
    }

    Registrar g_registrar(RegisterMenu);

    void BuildMainMenu() {
        AddSubMenu(static_cast<uint32_t>(Action::CAMPAIGN), "Campaign", g_campaignPageId);
        AddSubMenu(static_cast<uint32_t>(Action::DEATH_MATCH), "Deathmatch", g_deathMatchPageId);
    }

    void BuildCampaignMenu() {
        AddSubMenu(1, "1 Player", g_campaignOnePlayerPageId);
        AddSubMenu(2, "2 Players", g_campaignTwoPlayersPageId);
        AddSubMenu(3, "3 Players", g_campaignThreePlayersPageId);
        AddSubMenu(4, "4 Players", g_campaignFourPlayersPageId);
    }

    void BuildDeathmatchMenu() {
        AddSubMenu(1, "1 Player", g_deathMatchOnePlayerPageId);
        AddSubMenu(2, "2 Players", g_deathMatchTwoPlayersPageId);
        AddSubMenu(3, "3 Players", g_deathMatchThreePlayersPageId);
        AddSubMenu(4, "4 Players", g_deathMatchFourPlayersPageId);
    }

    void ApplyEdit(uint32_t, const Value&) {
    }

    void ApplyOnePlayerCampaignEdit(uint32_t id, const Value&) {
        StartCampaign(id, 1);
    }

    void ApplyTwoPlayerCampaignEdit(uint32_t id, const Value&) {
        StartCampaign(id, 2);
    }

    void ApplyThreePlayerCampaignEdit(uint32_t id, const Value&) {
        StartCampaign(id, 3);
    }

    void ApplyFourPlayerCampaignEdit(uint32_t id, const Value&) {
        StartCampaign(id, 4);
    }

    void ApplyOnePlayerDeathmatchEdit(uint32_t id, const Value&) {
        StartDeathmatch(id, 1);
    }

    void ApplyTwoPlayerDeathmatchEdit(uint32_t id, const Value&) {
        StartDeathmatch(id, 2);
    }

    void ApplyThreePlayerDeathmatchEdit(uint32_t id, const Value&) {
        StartDeathmatch(id, 3);
    }

    void ApplyFourPlayerDeathmatchEdit(uint32_t id, const Value&) {
        StartDeathmatch(id, 4);
    }

}
