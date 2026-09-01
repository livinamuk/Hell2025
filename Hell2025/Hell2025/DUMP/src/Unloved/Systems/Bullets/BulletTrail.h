#pragma once
#include "Unloved/Systems/Bullets/BulletTypes.h"

#include "Unloved/Common/Types.h"

struct BulletTrailParticle {
    glm::vec3 position;
    glm::vec3 velocity;
    float rotation;
    float rotationalVelocity;
    float lifeTime = 0.0f;
};

struct BulletTrail {
    BulletTrail() = default;
    BulletTrail(uint64_t id, BulletCreateInfo& createInfo);
    BulletTrail(const BulletTrail&) = delete;
    BulletTrail& operator=(const BulletTrail&) = delete;
    BulletTrail(BulletTrail&&) noexcept = default;
    BulletTrail& operator=(BulletTrail&&) noexcept = default;
    ~BulletTrail() = default;

    void Update(float deltaTime);
    void CleanUp();

    BulletCreateInfo m_createInfo;
    uint64_t m_objectId = 0;
    glm::vec3 m_position = glm::vec3(0.0f);
    glm::vec3 m_forward = glm::vec3(0.0f);
    float m_speed = 0.0f;
    float m_distanceTraveled = 0.0f;
    float m_maxDistance = 0.0f;
    float m_phaseAccumulator = 0.0f;
    float m_randomPhaseOffset = 0.0f;

private:
};
