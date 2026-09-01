#include "Unloved/Bible/Bible.h"

namespace Bible {

    constexpr int playerKill = 100;
    constexpr int playerHeadshotKill = 250;

    int GetPlayerKillCashReward() {
        return playerKill;
    }

    int GetPlayerHeadShotCashReward() {
        return playerHeadshotKill;
    }
}