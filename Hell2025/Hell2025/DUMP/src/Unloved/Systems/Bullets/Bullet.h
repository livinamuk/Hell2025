#pragma once
#include "Unloved/Systems/Bullets/BulletTypes.h"

#include "glm/vec3.hpp"

struct Bullet {
    Bullet() = default;
    Bullet(BulletCreateInfo& createInfo, uint64_t parentBulletTrailId = 0) {
        m_createInfo = createInfo;
        m_parentBulletTrailId = parentBulletTrailId;
    }

    const bool PlaysPiano() const                           { return m_createInfo.playsPiano; }
    const bool CreatesDecalTexturePaintedWounds() const     { return m_createInfo.createsDecalTexturePaintedWounds; }
    const bool CreatesDecals() const                        { return m_createInfo.createsDecals; }
    const bool CreatesFolloWThroughBulletOnGlassHit() const { return m_createInfo.createsFollowThroughBulletOnGlassHit; }
    const float GetRayLength() const                        { return m_createInfo.rayLength; }
    const float GetImpactImpulse() const                    { return m_createInfo.impactImpulse; }
    const glm::vec3 GetOrigin() const                       { return m_createInfo.origin; }
    const glm::vec3 GetDirection() const                    { return m_createInfo.direction; }
    const int32_t GetWeaponIndex() const                    { return m_createInfo.weaponIndex; }
    const uint32_t GetDamage() const                        { return m_createInfo.damage; }
    const uint64_t GetOwnerObjectId() const                 { return m_createInfo.ownerObjectId; }
    const uint64_t GetHitGroupId() const                    { return m_createInfo.hitGroupId; }
    const uint64_t GetParentBulletTrailId() const           { return m_parentBulletTrailId; }
    BulletCreateInfo GetCreateInfo() const                  { return m_createInfo; }

private:
    BulletCreateInfo m_createInfo;
    uint64_t m_parentBulletTrailId = 0; // 0 for no parent, any other value means this belongs to a bullet trail
};
