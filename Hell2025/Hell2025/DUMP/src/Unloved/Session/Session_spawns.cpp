#include "Session.h"

#include "Hell/Common/Random.h"

#include "Unloved/World/World.h"

#include <glm/geometric.hpp>

namespace Unloved::Session {

    void CreateFallbackCampaignSpawnPoints() {
        World::AddSpawnPointCampaign({ glm::vec3(43.9485, 32.6516, 36.7408), glm::vec2(-0.294f, -5.0020f) });
        World::AddSpawnPointCampaign({ glm::vec3(40.3495, 32.6486, 34.1408), glm::vec2(-0.168f, -9.4820f) });
        World::AddSpawnPointCampaign({ glm::vec3(42.6229, 32.6482, 41.4889), glm::vec2(-0.282f, -11.772f) });
        World::AddSpawnPointCampaign({ glm::vec3(34.7497, 35.4520, 37.4222), glm::vec2(-0.206f, -15.736f) });
        World::AddSpawnPointCampaign({ glm::vec3(34.9035, 32.6505, 39.5006), glm::vec2(-0.146f, -14.242f) });
        World::AddSpawnPointCampaign({ glm::vec3(34.8531, 32.6496, 33.6023), glm::vec2(-0.258f, -15.138f) });
        World::AddSpawnPointCampaign({ glm::vec3(33.3506, 32.6481, 41.1310), glm::vec2(-0.166f, -18.282f) });
        World::AddSpawnPointCampaign({ glm::vec3(57.3242, 33.5911, 48.8959), glm::vec2(-0.134f, -18.100f) });
        World::AddSpawnPointCampaign({ glm::vec3(40.0950, 32.4311, 31.6613), glm::vec2(-0.110f, -14.256f) });
    }

    void CreateFallbackDeathmatchSpawnPoints() {
        World::AddSpawnPointDeathMatch({ glm::vec3(43.9485, 32.6516, 36.7408), glm::vec2(-0.294f, -5.0020f) });
        World::AddSpawnPointDeathMatch({ glm::vec3(40.3495, 32.6486, 34.1408), glm::vec2(-0.168f, -9.4820f) });
        World::AddSpawnPointDeathMatch({ glm::vec3(42.6229, 32.6482, 41.4889), glm::vec2(-0.282f, -11.772f) });
        World::AddSpawnPointDeathMatch({ glm::vec3(34.7497, 35.4520, 37.4222), glm::vec2(-0.206f, -15.736f) });
        World::AddSpawnPointDeathMatch({ glm::vec3(34.9035, 32.6505, 39.5006), glm::vec2(-0.146f, -14.242f) });
        World::AddSpawnPointDeathMatch({ glm::vec3(34.8531, 32.6496, 33.6023), glm::vec2(-0.258f, -15.138f) });
        World::AddSpawnPointDeathMatch({ glm::vec3(33.3506, 32.6481, 41.1310), glm::vec2(-0.166f, -18.282f) });
        World::AddSpawnPointDeathMatch({ glm::vec3(57.3242, 33.5911, 48.8959), glm::vec2(-0.134f, -18.100f) });
        World::AddSpawnPointDeathMatch({ glm::vec3(40.0950, 32.4311, 31.6613), glm::vec2(-0.110f, -14.256f) });
    }

    bool SpawnPointIsSafeDistance(const SpawnPoint& spawnPoint) {
        for (int i = 0; i < GetLocalPlayerCount(); i++) {
            Player* player = GetLocalPlayerByViewportIndex(i);
            if (!player) {
                continue;
            }

            float distanceToOtherPlayer = glm::distance(spawnPoint.GetPosition(), player->GetFootPosition());

            if (distanceToOtherPlayer < 1.0f) {
                return false;
            }
        }

        return true;
    }

    const SpawnPoint& GetRandomSafeSpawnPoint(Hell::SlotMap<SpawnPoint>& spawnPoints) {
        const int32_t spawnPointCount = static_cast<int32_t>(spawnPoints.size());
        const int32_t fallbackIndex = Hell::Random::Int(0, spawnPointCount - 1);

        for (int32_t attempt = 0; attempt < spawnPointCount; attempt++) {
            const int32_t index = Hell::Random::Int(0, spawnPointCount - 1);
            const SpawnPoint& spawnPoint = spawnPoints[index];
            if (SpawnPointIsSafeDistance(spawnPoint)) {
                return spawnPoint;
            }
        }

        return spawnPoints[fallbackIndex];
    }

    const SpawnPoint& GetRandomCampaignSpawnPoint() {
        Hell::SlotMap<SpawnPoint>& spawnPoints = World::GetSpawnPointsCampaign();

        if (spawnPoints.empty()) CreateFallbackCampaignSpawnPoints();

        return GetRandomSafeSpawnPoint(spawnPoints);
    }

    const SpawnPoint& GetRandomDeathmatchSpawnPoint() {
        Hell::SlotMap<SpawnPoint>& spawnPoints = World::GetSpawnPointsDeathMatch();

        if (spawnPoints.empty()) CreateFallbackDeathmatchSpawnPoints();

        return GetRandomSafeSpawnPoint(spawnPoints);
    }
}
