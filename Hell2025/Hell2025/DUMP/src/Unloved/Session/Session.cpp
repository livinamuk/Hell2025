#include "Session.h"

#include "Hell/Logging.h"
#include "Unloved/World/World.h"

#include <algorithm>
#include <optional>
#include <utility>

namespace {
    struct NewGameRequest {
        GameMode mode;
        std::string mapName;
        int32_t localPlayerCount = 0;
    };

    std::optional<NewGameRequest> g_pendingNewGame;
}

namespace Unloved::Session {

    float g_sessionTime = 0;
    GameMode g_gameMode = GameMode::CAMPAIGN;
    SplitscreenMode g_splitscreenMode = SplitscreenMode::FULLSCREEN;

    void Create() {
        RecreateLocalPlayers(2);
        SetSplitscreenMode(SplitscreenMode::FULLSCREEN);
    }

    float GetSessionTime() {
        return g_sessionTime;
    }

    void StartNewGame(GameMode mode, const std::string& mapName) {
        g_gameMode = mode;
        g_sessionTime = 0.0f;
        World::NewRun(mapName);
    }

    void RequestNewGame(GameMode mode, const std::string& mapName) {
        g_pendingNewGame = NewGameRequest{ mode, mapName };
    }

    void RequestNewGame(GameMode mode, const std::string& mapName, int32_t localPlayerCount) {
        g_pendingNewGame = NewGameRequest{ mode, mapName, localPlayerCount };
    }

    void ProcessPendingNewGame() {
        if (!g_pendingNewGame) return;

        NewGameRequest request = std::move(*g_pendingNewGame);
        g_pendingNewGame.reset();

        if (request.localPlayerCount > 0) {
            RecreateLocalPlayers(request.localPlayerCount);
        }

        StartNewGame(request.mode, request.mapName);
    }

    bool HasPendingNewGame() {
        return g_pendingNewGame.has_value();
    }

    GameMode GetGameMode() {
        return g_gameMode;
    }

    void NextSplitScreenMode() {
        const int32_t localPlayerCount = GetLocalPlayerCount();
        if (localPlayerCount <= 1) return;

        if (localPlayerCount == 2) {
            SetSplitscreenMode(g_splitscreenMode == SplitscreenMode::FULLSCREEN
                ? SplitscreenMode::TWO_PLAYER
                : SplitscreenMode::FULLSCREEN);
            return;
        }

        switch (g_splitscreenMode) {
            case SplitscreenMode::FULLSCREEN:  SetSplitscreenMode(SplitscreenMode::TWO_PLAYER);  break;
            case SplitscreenMode::TWO_PLAYER:  SetSplitscreenMode(SplitscreenMode::FOUR_PLAYER); break;
            case SplitscreenMode::FOUR_PLAYER: SetSplitscreenMode(SplitscreenMode::FULLSCREEN);  break;
            default:                           SetSplitscreenMode(SplitscreenMode::FULLSCREEN);  break;
        }
    }

    void SetSplitscreenMode(SplitscreenMode mode) {
        g_splitscreenMode = mode;
    }

    const SplitscreenMode& GetSplitscreenMode() {
        return g_splitscreenMode;
    }

    int32_t GetActiveViewportCount() {
        switch (g_splitscreenMode) {
            case SplitscreenMode::FULLSCREEN:  return std::min(GetLocalPlayerCount(), 1);
            case SplitscreenMode::TWO_PLAYER:  return std::min(GetLocalPlayerCount(), 2);
            case SplitscreenMode::FOUR_PLAYER: return std::min(GetLocalPlayerCount(), 4);
            default: return 1;
        }
    }
}
