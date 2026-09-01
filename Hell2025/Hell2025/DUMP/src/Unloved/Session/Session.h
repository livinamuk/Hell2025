#pragma once

#include "Unloved/Common/Types.h"
#include "Unloved/Camera/Camera.h"
#include "Unloved/Objects/Spawns/SpawnPoint.h"
#include "Unloved/Player/Player.h"
#include "Unloved/Session/Session_types.h"

#include <string>
#include <vector>

namespace Unloved::Session {
    void BeginFrame();
    void Create();
    void StartNewGame(GameMode mode, const std::string& mapName);
    void RequestNewGame(GameMode mode, const std::string& mapName);
    void RequestNewGame(GameMode mode, const std::string& mapName, int32_t localPlayerCount);
    void ProcessPendingNewGame();
    bool HasPendingNewGame();

    void Update();
    void PostWorldUpdate();

    float GetSessionTime();
    GameMode GetGameMode();

    // Players
    void AddLocalPlayer(const glm::vec3& position, const glm::vec3& rotation);
    void AddRemotePlayer(const glm::vec3& position, const glm::vec3& rotation);
    void RecreateLocalPlayers(int32_t playerCount);
    void KeepOnlyFirstLocalPlayer();
    void RespawnPlayers();
    const std::vector<uint64_t>& GetLocalPlayerIds();
    Unloved::Player* GetPlayerById(uint64_t playerId);
    Unloved::Player* GetLocalPlayerByViewportIndex(uint32_t index);
    void SetPlayerKeyboardAndMouseIndex(int playerIndex, int keyboardIndex, int mouseIndex);
    int32_t GetPlayerCount();
    int32_t GetLocalPlayerCount();
    int32_t GetRemotePlayerCount();
    int32_t GetOnlinePlayerCount();
    Unloved::Camera* GetLocalPlayerCameraByViewportIndex(uint32_t index);
    float GetLocalPlayerFovByViewportIndex(uint32_t index);

    // Spawn Points
    const SpawnPoint& GetRandomCampaignSpawnPoint();
    const SpawnPoint& GetRandomDeathmatchSpawnPoint();

    void NextSplitScreenMode();
    void SetSplitscreenMode(SplitscreenMode mode);
    const SplitscreenMode& GetSplitscreenMode();
    int32_t GetActiveViewportCount();
}
