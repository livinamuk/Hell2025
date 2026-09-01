#pragma once

#include "Hell/Containers/SlotMap.h"

#include "Unloved/Systems/Bullets/Bullet.h"
#include "Unloved/Systems/Bullets/BulletTrail.h"

#include <cstdint>
#include <vector>

namespace Unloved::BulletSystem {
    uint64_t CreateHitGroup();
    void AddBullet(BulletCreateInfo createInfo, uint64_t parentBulletTrailId = 0);
    void AddBulletTrail(BulletCreateInfo createInfo);
    bool RemoveBulletTrail(uint64_t objectId);

    std::vector<Bullet>& GetBullets();
    Hell::SlotMap<BulletTrail>& GetBulletTrails();
    std::vector<BulletTrailParticle>& GetBulletTrailParticles();

    void Update();
    void CleanUp();
}
