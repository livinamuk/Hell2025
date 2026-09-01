#include "Session.h"

#include "Hell/Containers/SlotMap.h"
#include "Hell/Logging.h"

#include "Unloved/ObjectId.h"

#include <algorithm>

namespace Unloved::Session {
    Hell::SlotMap<Unloved::Player> g_players(8);
    std::vector<uint64_t> g_playerIds;
    std::vector<uint64_t> g_localPlayerIds;
    std::vector<uint64_t> g_remotePlayerIds;

    namespace {
        constexpr int32_t MAX_LOCAL_PLAYER_COUNT = 4;

        glm::vec3 GetDefaultPlayerPosition(int32_t playerIndex) {
            const float column = static_cast<float>(playerIndex % 2);
            const float row = static_cast<float>(playerIndex / 2);
            return glm::vec3(12.82f + (2.39f * column), 0.5f, 18.27f + (1.3f * column) + (2.0f * row));
        }

        glm::vec3 GetDefaultPlayerRotation(int32_t playerIndex) {
            return playerIndex == 0 ? glm::vec3(-0.13f, -1.46f, 0.0f) : glm::vec3(-0.49f, -0.74f, 0.0f);
        }

        void ErasePlayerId(std::vector<uint64_t>& playerIds, uint64_t playerId) {
            playerIds.erase(std::remove(playerIds.begin(), playerIds.end(), playerId), playerIds.end());
        }

        void RemovePlayerById(uint64_t playerId) {
            if (Unloved::Player* player = g_players.get(playerId)) {
                player->CleanUp();
            }

            g_players.erase(playerId);
            ErasePlayerId(g_playerIds, playerId);
            ErasePlayerId(g_localPlayerIds, playerId);
            ErasePlayerId(g_remotePlayerIds, playerId);
        }

        void RemoveAllPlayers() {
            const std::vector<uint64_t> playerIds = g_playerIds;
            for (uint64_t playerId : playerIds) {
                RemovePlayerById(playerId);
            }
        }

        void ConfigureLocalPlayers() {
            for (int32_t playerIndex = 0; playerIndex < GetLocalPlayerCount(); playerIndex++) {
                const int32_t inputIndex = playerIndex == 0 ? 0 : 1;
                SetPlayerKeyboardAndMouseIndex(playerIndex, inputIndex, inputIndex);
            }

            switch (GetLocalPlayerCount()) {
                case 0:
                case 1:  SetSplitscreenMode(SplitscreenMode::FULLSCREEN); break;
                case 2:  SetSplitscreenMode(SplitscreenMode::TWO_PLAYER); break;
                default: SetSplitscreenMode(SplitscreenMode::FOUR_PLAYER); break;
            }
        }
    }

    void AddLocalPlayer(const glm::vec3& position, const glm::vec3& rotation) {
        if (g_localPlayerIds.size() >= static_cast<size_t>(MAX_LOCAL_PLAYER_COUNT)) {
            return;
        }

        const uint64_t playerId = Unloved::GetNextObjectId(ObjectType::PLAYER);
        const int32_t viewportIndex = static_cast<int32_t>(g_localPlayerIds.size());
        if (!g_players.emplace_with_id(playerId)) {
            return;
        }

        Unloved::Player* player = g_players.get(playerId);
        if (!player) {
            return;
        }

        player->Init(playerId, position, rotation, viewportIndex);
        g_playerIds.push_back(playerId);
        g_localPlayerIds.push_back(playerId);
    }

    void AddRemotePlayer(const glm::vec3& position, const glm::vec3& rotation) {
        const uint64_t playerId = Unloved::GetNextObjectId(ObjectType::PLAYER);
        if (!g_players.emplace_with_id(playerId)) {
            return;
        }

        Unloved::Player* player = g_players.get(playerId);
        if (!player) {
            return;
        }

        player->Init(playerId, position, rotation, -1);
        g_playerIds.push_back(playerId);
        g_remotePlayerIds.push_back(playerId);
    }

    void RecreateLocalPlayers(int32_t playerCount) {
        if (playerCount < 1 || playerCount > MAX_LOCAL_PLAYER_COUNT) {
            return;
        }

        RemoveAllPlayers();

        for (int32_t playerIndex = 0; playerIndex < playerCount; playerIndex++) {
            AddLocalPlayer(GetDefaultPlayerPosition(playerIndex), GetDefaultPlayerRotation(playerIndex));
        }

        ConfigureLocalPlayers();
    }

    void KeepOnlyFirstLocalPlayer() {
        const uint64_t firstLocalPlayerId = g_localPlayerIds.empty() ? 0 : g_localPlayerIds.front();
        const std::vector<uint64_t> playerIds = g_playerIds;

        for (uint64_t playerId : playerIds) {
            if (playerId != firstLocalPlayerId) {
                RemovePlayerById(playerId);
            }
        }

        if (firstLocalPlayerId == 0) {
            AddLocalPlayer(GetDefaultPlayerPosition(0), GetDefaultPlayerRotation(0));
        }

        ConfigureLocalPlayers();
    }

    void BeginFrame() {
        for (uint64_t playerId : g_localPlayerIds) {
            if (Unloved::Player* player = GetPlayerById(playerId)) {
                player->BeginFrame();
            }
        }
    }

    void RespawnPlayers() {
        for (uint64_t playerId : g_localPlayerIds) {
            if (Unloved::Player* player = GetPlayerById(playerId)) {
                player->Respawn();
            }
        }
    }

    const std::vector<uint64_t>& GetLocalPlayerIds() {
        return g_localPlayerIds;
    }

    Unloved::Player* GetPlayerById(uint64_t playerId) {
        return g_players.get(playerId);
    }

    Unloved::Player* GetLocalPlayerByViewportIndex(uint32_t index) {
        for (uint64_t playerId : g_localPlayerIds) {
            Unloved::Player* player = GetPlayerById(playerId);
            if (player && player->GetViewportIndex() == static_cast<int32_t>(index)) {
                return player;
            }
        }
        return nullptr;
    }

    Unloved::Camera* GetLocalPlayerCameraByViewportIndex(uint32_t index) {
        if (Unloved::Player* player = GetLocalPlayerByViewportIndex(index)) {
            return &player->GetCamera();
        }
        else {
            Logging::Debug() << "Session::GetLocalPlayerCameraByViewportIndex(..) failed. " << index << " out of range of local player count " << g_localPlayerIds.size() << "\n";
            return nullptr;
        }
    }

    float GetLocalPlayerFovByViewportIndex(uint32_t index) {
        if (Unloved::Player* player = GetLocalPlayerByViewportIndex(index)) {
            return player->GetFov();
        }
        else {
            Logging::Debug() << "Session::GetLocalPlayerFovByViewportIndex(..) failed. " << index << " out of range of local player count " << g_localPlayerIds.size() << "\n";
            return 1.0f;
        }
    }

    int32_t GetPlayerCount() {
        return static_cast<int32_t>(g_playerIds.size());
    }

    int32_t GetLocalPlayerCount() {
        return static_cast<int32_t>(g_localPlayerIds.size());
    }

    int32_t GetRemotePlayerCount() {
        return static_cast<int32_t>(g_remotePlayerIds.size());
    }

    int32_t GetOnlinePlayerCount() {
        return GetRemotePlayerCount();
    }

    void SetPlayerKeyboardAndMouseIndex(int playerIndex, int keyboardIndex, int mouseIndex) {
        if (playerIndex < 0) {
            return;
        }

        if (Unloved::Player* player = GetLocalPlayerByViewportIndex(static_cast<uint32_t>(playerIndex))) {
            player->SetKeyboardIndex(keyboardIndex);
            player->SetMouseIndex(mouseIndex);
        }
    }
}
