#include "Session.h"

#include "Unloved/Common/Constants.h"

#include "Hell/Time.h"

namespace Unloved::Session {
    extern float g_sessionTime;

    void Update() {
        const float deltaTime = Hell::Time::DeltaTime();

        // Total time
        g_sessionTime += deltaTime;
        if (g_sessionTime > TIME_WRAP) {
            g_sessionTime -= TIME_WRAP; // Keep it continuous
        }
    }

    void PostWorldUpdate() {
        for (uint64_t playerId : GetLocalPlayerIds()) {
            Player* player = GetPlayerById(playerId);
            if (!player) continue;

            player->PostWorldUpdate();
        }
    }
}
